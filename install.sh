#!/bin/bash
# Installation script for Twitch Controls Chaos.
# This is used both for first-time installations and to reconfigure an existing installation.

set -euo pipefail

echo "--------------------------------------------------------"
echo "Twitch Controls Chaos Installation"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}"
CHAOS_SRC_DIR="${REPO_ROOT}/chaos"
SCRIPTS_DIR="${REPO_ROOT}/scripts"
CHAOS_INSTALL_ROOT="/usr/local/chaos"

# We keep a state file to remember variables between runs.
statefile="${HOME}/.chaosinstall"
install_stage=0
remote_ui=0
interface_addr="localhost"
is_developer=0
staged_chaos_config="${REPO_ROOT}/chaosconfig.toml"

save_state() {
  typeset -p "$@" >"${statefile}"
}

# Source the state file to restore state from previous runs.
if [ -f "${statefile}" ]; then
  . "${statefile}" 2>/dev/null || :
fi

# Save state automatically on exit.
trap 'save_state install_stage remote_ui interface_addr is_developer' EXIT

confirm_reboot() {
  echo "Reboot now?"
  select yn in "Yes" "No"; do
    case "${yn}" in
      Yes)
        echo "Rebooting..."
        sudo reboot
        ;;
      No)
        echo "Reboot manually with 'sudo reboot' when ready"
        exit
        ;;
    esac
  done
}

install_questions() {
  echo "Install chatbot and user interface on a separate computer?"
  select yn in "Yes" "No"; do
    case "${yn}" in
      Yes)
        remote_ui=1
        if [ -f /etc/systemd/system/chaosface.service ]; then
          sudo systemctl disable chaosface || true
          echo "Disabled local chaosface service"
        fi
        echo "You must manually install chaosface on another computer"
        break
        ;;
      No)
        echo "Chaosface will be installed on this device"
        break
        ;;
    esac
  done

  if (( remote_ui )); then
    interface_addr="192.168.1.2"
    echo "What is the address of the machine that will run chaosface?"
    read -r -p "IP address (default ${interface_addr}): " temp_address
    if [ -n "${temp_address}" ]; then
      interface_addr="${temp_address}"
    fi
  fi

  echo "Do you want to develop TCC on this device?"
  select yn in "Yes" "No"; do
    case "${yn}" in
      Yes)
        is_developer=1
        break
        ;;
      No)
        break
        ;;
    esac
  done

  # Stage a modified chaosconfig.toml that will be installed into /usr/local/chaos.
  cp "${CHAOS_SRC_DIR}/chaosconfig.toml" "${staged_chaos_config}"
  sed -i 's/^interface_addr *= *\"[^\"]+\"/interface_addr = \"'"${interface_addr}"'\"/' "${staged_chaos_config}"
}

build_dependencies() {
  local -a dependencies=(
    build-essential
    libncurses5-dev
    libusb-1.0-0-dev
    libjsoncpp-dev
    cmake
    git
    libzmq3-dev
    raspberrypi-kernel-headers
    python3-dev
    python3-pip
    python3-venv
    libatlas-base-dev
  )
  local -a to_install=()
  local dependency
  local openblas_pkg=""
  local -a openblas_candidates=(
    libopenblas0-pthread
    libopenblas0
    libopenblas-base
  )
  local candidate

  for candidate in "${openblas_candidates[@]}"; do
    if apt-cache show "${candidate}" >/dev/null 2>&1; then
      openblas_pkg="${candidate}"
      break
    fi
  done
  if [ -n "${openblas_pkg}" ]; then
    dependencies+=("${openblas_pkg}")
  fi

  if (( is_developer != 0 )); then
    dependencies+=(doxygen)
  fi

  for dependency in "${dependencies[@]}"; do
    echo "Checking for ${dependency}"
    if ! dpkg-query -W -f='${Status}' "${dependency}" 2>/dev/null | grep -q "ok installed"; then
      echo "Not installed: ${dependency}"
      to_install+=("${dependency}")
    fi
  done

  if [ "${#to_install[@]}" -ne 0 ]; then
    echo "Installing required dependencies"
    sudo apt-get update
    sudo apt-get install -y "${to_install[@]}"
  fi
}

configure_as_gadget() {
  local config_path

  # Starting with bookworm, the config.txt file is found in /boot/firmware/.
  if [ -f /boot/firmware/config.txt ]; then
    config_path='/boot/firmware'
  else
    config_path='/boot'
  fi

  # Replace XHCI USB controller with DWC2. This lets us run the Raspberry Pi as a gadget.
  sudo sed -i 's/^otg_mode=1/#&/' "${config_path}/config.txt"
  sudo -s eval "grep -qxF 'dtoverlay=dwc2' '${config_path}/config.txt' || echo 'dtoverlay=dwc2' >> '${config_path}/config.txt'"

  # The 64-bit kernel may not provide headers needed for raw-gadget module build on Pi.
  if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    sudo -s eval "grep -qxF 'arm_64bit=0' '${config_path}/config.txt' || echo 'arm_64bit=0' >> '${config_path}/config.txt'"
    echo "Before continuing, we must switch to a 32-bit kernel. This"
    echo "requires a reboot. After rebooting, please rerun './install.sh'"
    confirm_reboot
  fi
}

build_raw_gadget() {
  # Build customized raw-gadget kernel module.
  if [ ! -d "${HOME}/raw-gadget" ]; then
    git clone https://github.com/polysyllabic/raw-gadget.git "${HOME}/raw-gadget"
  fi
  cd "${HOME}/raw-gadget/raw_gadget"
  make
  sudo make install
}

install_engine() {
  # Compile and install the chaos engine.
  cd "${CHAOS_SRC_DIR}"
  mkdir -p build
  cd build
  cmake ..
  make -j"$(nproc)"
  sudo make install

  # Keep start script and service definition in sync with the local checkout.
  sudo install -m 0755 "${SCRIPTS_DIR}/startchaos.sh" "${CHAOS_INSTALL_ROOT}/startchaos.sh"
  sudo install -m 0644 "${SCRIPTS_DIR}/chaos.service" /etc/systemd/system/chaos.service

  # Install the staged runtime config that may contain a user-selected interface address.
  if [ -f "${staged_chaos_config}" ]; then
    sudo install -m 0644 "${staged_chaos_config}" "${CHAOS_INSTALL_ROOT}/chaosconfig.toml"
  fi

  sudo systemctl daemon-reload
  sudo systemctl enable chaos
  echo "TCC engine installed as a systemd service"
}

install_chaosface() {
  # Install chaosface into /usr/local/chaos via venv + staged source package.
  if (( is_developer > 0 || remote_ui == 0 )); then
    "${SCRIPTS_DIR}/deploy_chaosface_update.sh"
  fi

  if (( remote_ui == 0 )); then
    sudo install -m 0644 "${SCRIPTS_DIR}/chaosface.service" /etc/systemd/system/chaosface.service
    sudo systemctl daemon-reload
    sudo systemctl enable chaosface
    echo "Chaosface installed as a service"
  fi
}

# Main script execution.
if (( install_stage >= 6 )); then
  echo "Twitch Controls Chaos has already been installed. Do you want to reconfigure it?"
  select yn in "Yes" "No"; do
    case "${yn}" in
      Yes)
        install_stage=0
        ;;
      No)
        exit
        ;;
    esac
  done
fi

# Skip earlier stages if we're resuming an installation.
if (( install_stage < 1 )); then
  install_questions
  install_stage=1
fi
if (( install_stage == 1 )); then
  build_dependencies
  install_stage=2
fi
if (( install_stage == 2 )); then
  configure_as_gadget
  install_stage=3
fi
if (( install_stage == 3 )); then
  build_raw_gadget
  install_stage=4
fi
if (( install_stage == 4 )); then
  install_engine
  install_stage=5
fi
if (( install_stage == 5 )); then
  install_chaosface
fi

install_stage=6
echo "Twitch Controls Chaos installation complete."
confirm_reboot
