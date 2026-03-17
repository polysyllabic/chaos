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
selected_build_branch=""
staged_chaos_config="${REPO_ROOT}/chaosconfig.toml"

save_state() {
  typeset -p "$@" >"${statefile}"
}

# Source the state file to restore state from previous runs.
if [ -f "${statefile}" ]; then
  . "${statefile}" 2>/dev/null || :
fi

# Save state automatically on exit.
trap 'save_state install_stage remote_ui interface_addr is_developer selected_build_branch' EXIT

git_repo_ready_for_branch_selection() {
  command -v git >/dev/null 2>&1 || return 1
  git -C "${REPO_ROOT}" rev-parse --is-inside-work-tree >/dev/null 2>&1 || return 1
}

current_git_branch() {
  if ! git_repo_ready_for_branch_selection; then
    return 1
  fi
  git -C "${REPO_ROOT}" rev-parse --abbrev-ref HEAD 2>/dev/null || return 1
}

switch_to_build_branch() {
  local target_branch="$1"

  if ! git_repo_ready_for_branch_selection; then
    echo "ERROR: Git repository metadata is unavailable; cannot switch branches."
    exit 1
  fi

  if ! git -C "${REPO_ROOT}" checkout --quiet "${target_branch}"; then
    echo "ERROR: Could not switch to branch '${target_branch}'."
    echo "Resolve any local checkout issues, then rerun './install.sh'"
    exit 1
  fi
}

select_experimental_build_branch() {
  local current_branch
  local selected_branch
  local -a experimental_branches=("$@")

  if [ "${#experimental_branches[@]}" -eq 0 ]; then
    echo "ERROR: No non-main branches are available to build."
    exit 1
  fi

  echo "Available experimental branches:"
  PS3="Select the branch to build: "
  select selected_branch in "${experimental_branches[@]}"; do
    if [ -z "${selected_branch}" ]; then
      echo "Please choose one of the listed branches."
      continue
    fi
    selected_build_branch="${selected_branch}"
    current_branch="$(current_git_branch || true)"
    if [ "${current_branch}" != "${selected_branch}" ]; then
      echo "Switching checked-out branch from '${current_branch:-HEAD}' to '${selected_branch}' for this build."
      switch_to_build_branch "${selected_branch}"
    fi
    break
  done
  unset PS3
}

select_build_branch() {
  local branch
  local current_branch
  local branch_count
  local has_main=0
  local selection
  local -a local_branches=()
  local -a experimental_branches=()

  if ! git_repo_ready_for_branch_selection; then
    return
  fi

  mapfile -t local_branches < <(git -C "${REPO_ROOT}" for-each-ref --format='%(refname:short)' --sort=refname refs/heads)
  branch_count="${#local_branches[@]}"
  if (( branch_count <= 1 )); then
    return
  fi

  current_branch="$(current_git_branch || true)"
  for branch in "${local_branches[@]}"; do
    if [ "${branch}" = "main" ]; then
      has_main=1
    else
      experimental_branches+=("${branch}")
    fi
  done

  echo "Multiple local branches were found in this repository."
  echo "Current branch: ${current_branch:-HEAD}"
  if (( has_main == 0 )); then
    echo "Local branch 'main' is not available in this checkout."
    select_experimental_build_branch "${experimental_branches[@]}"
    return
  fi
  echo "Do you want the stable release build from 'main'?"

  PS3="Select build type: "
  select selection in "Stable release (main)" "Experimental branch"; do
    case "${REPLY}" in
      1)
        selected_build_branch="main"
        if [ "${current_branch}" = "main" ]; then
          echo "Stable release selected; building from 'main'."
        else
          echo "Stable release selected."
          echo "The checked-out branch will be switched from '${current_branch:-HEAD}' to 'main'."
          switch_to_build_branch "main"
        fi
        break
        ;;
      2)
        select_experimental_build_branch "${experimental_branches[@]}"
        break
        ;;
      *)
        echo "Please choose 1 or 2."
        ;;
    esac
  done
  unset PS3
}

ensure_selected_build_branch() {
  local current_branch

  if [ -z "${selected_build_branch}" ]; then
    return
  fi
  if ! git_repo_ready_for_branch_selection; then
    return
  fi

  current_branch="$(current_git_branch || true)"
  if [ "${current_branch}" = "${selected_build_branch}" ]; then
    return
  fi

  echo "Continuing installation on previously selected branch '${selected_build_branch}'."
  echo "Switching checked-out branch from '${current_branch:-HEAD}' to '${selected_build_branch}'."
  switch_to_build_branch "${selected_build_branch}"
}

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
  remote_ui=0
  interface_addr="localhost"
  is_developer=0

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

running_kernel_is_64_bit() {
  case "$(uname -m)" in
    aarch64|arm64)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

current_device_model() {
  if [ -r /proc/device-tree/model ]; then
    tr -d '\0' </proc/device-tree/model
  fi
}

device_only_supports_64bit_kernel() {
  case "$(current_device_model)" in
    *Raspberry\ Pi\ 5*|*Raspberry\ Pi\ 500*|*Compute\ Module\ 5*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

expected_32bit_kernel_image_name() {
  case "$(current_device_model)" in
    *Raspberry\ Pi\ 4*|*Raspberry\ Pi\ 400*|*Compute\ Module\ 4*)
      printf '%s\n' kernel7l.img
      ;;
    *Raspberry\ Pi\ 3*|*Raspberry\ Pi\ 2*|*Zero\ 2\ W*|*Compute\ Module\ 3*)
      printf '%s\n' kernel7.img
      ;;
    *Raspberry\ Pi\ Zero*|*Raspberry\ Pi\ 1*|*Compute\ Module\ 1*)
      printf '%s\n' kernel.img
      ;;
    *)
      return 1
      ;;
  esac
}

recommended_32bit_kernel_image_package() {
  case "$(current_device_model)" in
    *Raspberry\ Pi\ 4*|*Raspberry\ Pi\ 400*|*Compute\ Module\ 4*)
      first_available_package linux-image-rpi-v7l
      ;;
    *Raspberry\ Pi\ 3*|*Raspberry\ Pi\ 2*|*Zero\ 2\ W*|*Compute\ Module\ 3*)
      first_available_package linux-image-rpi-v7 linux-image-rpi-v7l || printf '%s\n' linux-image-rpi-v7
      ;;
    *Raspberry\ Pi\ Zero*|*Raspberry\ Pi\ 1*|*Compute\ Module\ 1*)
      first_available_package linux-image-rpi-v6 || printf '%s\n' linux-image-rpi-v6
      ;;
    *)
      first_available_package linux-image-rpi-v7 linux-image-rpi-v7l linux-image-rpi-v6
      ;;
  esac
}

recommended_32bit_kernel_headers_package() {
  case "$(current_device_model)" in
    *Raspberry\ Pi\ 4*|*Raspberry\ Pi\ 400*|*Compute\ Module\ 4*)
      first_available_package linux-headers-rpi-v7l
      ;;
    *Raspberry\ Pi\ 3*|*Raspberry\ Pi\ 2*|*Zero\ 2\ W*|*Compute\ Module\ 3*)
      first_available_package linux-headers-rpi-v7 linux-headers-rpi-v7l || printf '%s\n' linux-headers-rpi-v7
      ;;
    *Raspberry\ Pi\ Zero*|*Raspberry\ Pi\ 1*|*Compute\ Module\ 1*)
      first_available_package linux-headers-rpi-v6 || printf '%s\n' linux-headers-rpi-v6
      ;;
    *)
      first_available_package linux-headers-rpi-v7 linux-headers-rpi-v7l linux-headers-rpi-v6
      ;;
  esac
}

ensure_32bit_kernel_available_for_reboot() {
  local config_path="$1"
  local expected_image
  local image_pkg
  local headers_pkg

  if ! expected_image="$(expected_32bit_kernel_image_name)"; then
    echo "ERROR: Could not determine which 32-bit kernel image is required"
    echo "for $(current_device_model)."
    return 1
  fi

  if [ -f "${config_path}/${expected_image}" ]; then
    return 0
  fi

  image_pkg=""
  headers_pkg=""
  if image_pkg="$(recommended_32bit_kernel_image_package)"; then
    if headers_pkg="$(recommended_32bit_kernel_headers_package)"; then
      echo "Installing 32-bit kernel packages needed for reboot: ${image_pkg} ${headers_pkg}"
      sudo apt-get update
      sudo apt-get install -y "${image_pkg}" "${headers_pkg}"
    else
      echo "Installing 32-bit kernel package needed for reboot: ${image_pkg}"
      sudo apt-get update
      sudo apt-get install -y "${image_pkg}"
    fi
  fi

  if [ -f "${config_path}/${expected_image}" ]; then
    return 0
  fi

  echo "ERROR: TCC requires the 32-bit kernel image ${expected_image}, but it"
  echo "is not present in ${config_path}."
  case "$(current_device_model)" in
    *Raspberry\ Pi\ 4*|*Raspberry\ Pi\ 400*|*Compute\ Module\ 4*)
      echo "Current Raspberry Pi OS Trixie images for Pi 4-class devices do not"
      echo "ship the packaged 32-bit Pi 4 kernel (${expected_image})."
      echo "Please use Raspberry Pi OS Bookworm 32-bit, or install a custom"
      echo "${expected_image} before rerunning './install.sh'"
      ;;
    *)
      echo "Please install a matching 32-bit Raspberry Pi kernel image before"
      echo "rerunning './install.sh'"
      ;;
  esac
  return 1
}

recommended_kernel_headers_package() {
  local kernel_release

  kernel_release="$(uname -r)"
  case "${kernel_release}" in
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

require_running_kernel_build_tree() {
  ensure_kernel_headers_available
  if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    echo "ERROR: raw-gadget must be built against the headers for the running"
    echo "kernel ($(uname -r)), but /lib/modules/$(uname -r)/build is missing."
    echo "Please boot the intended 32-bit kernel, install its matching headers,"
    echo "then rerun './install.sh'"
    exit 1
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

  # TCC currently requires a 32-bit kernel. Recent Raspberry Pi OS images may
  # still boot a 64-bit kernel by default even when the user selected a
  # "32-bit" image, so switch that first and resume after reboot.
  if running_kernel_is_64_bit; then
    if device_only_supports_64bit_kernel; then
      echo "ERROR: $(current_device_model) only supports a 64-bit kernel."
      echo "raw-gadget currently requires a 32-bit kernel, so this device"
      echo "is not supported yet."
      echo "Please use a Raspberry Pi 4-class device for now, then rerun"
      echo "'./install.sh'"
      exit 1
    fi

    if ! ensure_32bit_kernel_available_for_reboot "${config_path}"; then
      exit 1
    fi

    sudo sed -Ei \
      -e 's|^[[:space:]]*arm_64bit[[:space:]]*=.*$|arm_64bit=0|' \
      "${config_file}"
    if ! sudo grep -Eq '^[[:space:]]*arm_64bit[[:space:]]*=[[:space:]]*0([[:space:]]*#.*)?$' "${config_file}"; then
      echo 'arm_64bit=0' | sudo tee -a "${config_file}" >/dev/null
    fi

    echo "Before continuing, TCC must reboot into a 32-bit kernel."
    echo "After rebooting, please rerun './install.sh'"
    confirm_reboot
  fi

  # Replace XHCI USB controller with DWC2 peripheral mode. Recent Raspberry Pi OS
  # images may ship CM4/CM5 defaults that explicitly force host mode.
  sudo sed -Ei \
    -e '/^[[:space:]]*otg_mode[[:space:]]*=[[:space:]]*1([[:space:]]*#.*)?$/ s/^/#/' \
    -e "s|^([[:space:]]*)dtoverlay[[:space:]]*=[[:space:]]*dwc2(,dr_mode=[^[:space:]#]+)?([[:space:]]*(#.*)?)$|\\1${gadget_overlay}\\3|" \
    "${config_file}"
  if ! sudo grep -Eq '^[[:space:]]*dtoverlay[[:space:]]*=[[:space:]]*dwc2,dr_mode=peripheral([[:space:]]*#.*)?$' "${config_file}"; then
    echo "${gadget_overlay}" | sudo tee -a "${config_file}" >/dev/null
  fi

  # raw-gadget currently requires a 32-bit Raspberry Pi kernel build tree.
  require_running_kernel_build_tree
}

build_raw_gadget() {
  # Build customized raw-gadget kernel module.
  require_running_kernel_build_tree
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
        selected_build_branch=""
        break
        ;;
      No)
        exit
        ;;
    esac
  done
fi

if (( install_stage > 0 && install_stage < 6 )); then
  ensure_selected_build_branch
fi

# Skip earlier stages if we're resuming an installation.
if (( install_stage < 1 )); then
  if [ -z "${selected_build_branch}" ]; then
    select_build_branch
  else
    ensure_selected_build_branch
  fi
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
