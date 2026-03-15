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

package_is_installed() {
  dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q "ok installed"
}

first_available_package() {
  local package

  for package in "$@"; do
    if package_is_installed "${package}" || apt-cache show "${package}" >/dev/null 2>&1; then
      printf '%s\n' "${package}"
      return 0
    fi
  done

  return 1
}

recommended_kernel_headers_package() {
  local kernel_release

  kernel_release="$(uname -r)"
  case "${kernel_release}" in
    *2712*|*v8*)
      first_available_package linux-headers-rpi-v8 || printf '%s\n' linux-headers-rpi-v8
      ;;
    *v7l*)
      first_available_package linux-headers-rpi-v7l linux-headers-rpi-v7 || printf '%s\n' linux-headers-rpi-v7l
      ;;
    *v7*)
      first_available_package linux-headers-rpi-v7 linux-headers-rpi-v7l || printf '%s\n' linux-headers-rpi-v7
      ;;
    *v6*)
      first_available_package linux-headers-rpi-v6 || printf '%s\n' linux-headers-rpi-v6
      ;;
    *)
      case "$(uname -m)" in
        aarch64|arm64)
          first_available_package linux-headers-rpi-v8 || printf '%s\n' linux-headers-rpi-v8
          ;;
        armv7l)
          first_available_package linux-headers-rpi-v7l linux-headers-rpi-v7 || printf '%s\n' linux-headers-rpi-v7l
          ;;
        armv6l)
          first_available_package linux-headers-rpi-v6 || printf '%s\n' linux-headers-rpi-v6
          ;;
        *)
          return 1
          ;;
      esac
      ;;
  esac
}

ensure_kernel_headers_available() {
  local headers_pkg

  if [ -d "/lib/modules/$(uname -r)/build" ]; then
    return
  fi

  if ! headers_pkg="$(recommended_kernel_headers_package)"; then
    echo "WARNING: Could not determine a Raspberry Pi kernel headers package for $(uname -r)"
    return
  fi

  echo "Kernel headers for $(uname -r) are missing; trying ${headers_pkg}"
  sudo apt-get update
  if ! sudo apt-get install -y "${headers_pkg}"; then
    echo "WARNING: Failed to install ${headers_pkg}; will fall back if headers remain unavailable."
  fi
}

build_dependencies() {
  local -a dependencies=(
    build-essential
    cmake
    git
    libusb-1.0-0-dev
    libzmq3-dev
  )
  local -a to_install=()
  local dependency

  if (( remote_ui == 0 || is_developer != 0 )); then
    dependencies+=(
      python3-dev
      python3-pip
      python3-venv
    )
  fi

  if (( is_developer != 0 )); then
    dependencies+=(doxygen)
  fi

  for dependency in "${dependencies[@]}"; do
    echo "Checking for ${dependency}"
    if ! package_is_installed "${dependency}"; then
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
  local config_file
  local gadget_overlay='dtoverlay=dwc2,dr_mode=peripheral'

  # Starting with bookworm, the config.txt file is found in /boot/firmware/.
  if [ -f /boot/firmware/config.txt ]; then
    config_path='/boot/firmware'
  else
    config_path='/boot'
  fi
  config_file="${config_path}/config.txt"

  # Replace XHCI USB controller with DWC2 peripheral mode. Recent Raspberry Pi OS
  # images may ship CM4/CM5 defaults that explicitly force host mode.
  sudo sed -Ei \
    -e '/^[[:space:]]*otg_mode[[:space:]]*=[[:space:]]*1([[:space:]]*#.*)?$/ s/^/#/' \
    -e "s|^([[:space:]]*)dtoverlay[[:space:]]*=[[:space:]]*dwc2(,dr_mode=[^[:space:]#]+)?([[:space:]]*(#.*)?)$|\\1${gadget_overlay}\\3|" \
    "${config_file}"
  if ! sudo grep -Eq '^[[:space:]]*dtoverlay[[:space:]]*=[[:space:]]*dwc2,dr_mode=peripheral([[:space:]]*#.*)?$' "${config_file}"; then
    echo "${gadget_overlay}" | sudo tee -a "${config_file}" >/dev/null
  fi

  # Recent 64-bit Raspberry Pi OS images use linux-headers-rpi-v8 instead of the
  # old raspberrypi-kernel-headers package. Try the matching headers package
  # first; if that still does not provide /lib/modules/.../build, fall back to a
  # 32-bit kernel that is known to work with the raw-gadget module build.
  ensure_kernel_headers_available
  if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    sudo -s eval "grep -qxF 'arm_64bit=0' '${config_file}' || echo 'arm_64bit=0' >> '${config_file}'"
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
  sudo install -m 0644 "${REPO_ROOT}/VERSION" "${CHAOS_INSTALL_ROOT}/VERSION"

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
    "${SCRIPTS_DIR}/update_chaosface.sh"
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
