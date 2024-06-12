#!/bin/bash
# Installation script for Twitch Controls Chaos
# This is used both for first-time installations and to reconfigure
# an existing installation.
echo "--------------------------------------------------------"
echo "Twitch Controls Chaos Installation"

# We keep a state file to remember variables between runs
statefile='/var/chaosinstall'
install_stage=0
remote_ui=0
interface_addr="localhost"
is_developer=0

save_state () {
  typeset -p "$@" >"$statefile"
}

# source the statefile to restore the state
if [ -f "$statefile" ]; then
  . "$statefile" 2>/dev/null || :;
fi

# save our state file automatically on exit
trap 'save_state install_stage remote_ui interface_addr is_developer' EXIT

confirm_reboot() {
  echo "Reboot now?"
  select yn in "Yes" "No"; do
    case $yn in
        Yes ) echo "Rebooting..."; sudo reboot;;
        No ) echo "Reboot manually with 'sudo reboot' when ready"; exit;;
    esac
  done
}

install_questions() {
  # Gather information about the installation
  echo "Install chatbot and user interface on a separate computer?"
  select yn in "Yes" "No"; do
    case $yn in
      Yes) remote_ui=1; echo "You must manually install chaosface on another computer"; break;;
      No)  echo "Chaosface will be installed on this Raspberry Pi"; break;;
    esac
  done

  if (( $remote_ui )); then
    interface_addr="192.168.1.2"
    echo "What is the address of the machine that will run chaosface?"
    read -p "IP address (default ${interface_addr}): " temp_address;
    if [ ! -z "$temp_address" ]; then
      interface_addr="$temp_address";
  fi

  echo "Do you want to develop TCC on this device?"
  select yn in "Yes" "No"; do
    case $yn in
      Yes) is_developer=1; break;;
      No)  break;;
    esac
  done

  # Update the configuration file based on user settings
  # We make a copy so as to preserve the original and modify that before copying into /usr/local
  # This way we don't need 'sudo -s eval...' and can avoid quotation hell.
  cd ~/chaos
  cp chaos/chaosconfig.toml .
  sed -i 's/^interface_addr *= *\"[^\"]+\"/interface_addr = \"'"$interface_addr"'\"/' ./chaosconfig.toml
}

build_dependencies() {
  declare -a depencencies=('build-essential' 'libncurses5-dev' 'libusb-1.0-0-dev' 'libjsoncpp-dev' 'cmake' 'git' 'libzmq3-dev' 'raspberrypi-kernel-headers' 'python3-dev' 'libatlas-base-dev' 'python3-pip')
  if [ $is_developer -ne 0 ]; then
    dependencies+=('doxygen')
  toInstall=()
  for dependency in "${depencencies[@]}"
  do
	  echo "Checking for" $dependency
	  if [ $(dpkg-query -W -f='${Status}' $dependency 2>/dev/null | grep -c "ok installed") -eq 0 ]; then
		  echo "Not installed: " $dependency
		  toInstall+=($dependency);
	  fi
  done

  if [ ${#toInstall[@]} -ne 0 ]; then
    echo "Installing required dependencies"
	  sudo apt-get update
	  sudo apt-get install -y ${toInstall[@]};
  fi

}

configure_as_gadget() {
  # Starting with bookworm, the config.txt file is found in /boot/firmware/
  if [ -f /boot/firmware/config.txt ]; then
    configPath='/boot/firmware';
  else
    configPath='/boot';
  fi

  # Replace XHCI USB controller with DWC2. This lets us run the Raspberry Pi as a gadget
  sudo sed -i 's/^otg_mode=1/#&/' ${configPath}/config.txt
  sudo -s eval "grep -qxF 'dtoverlay=dwc2' ${configPath}/config.txt  || echo 'dtoverlay=dwc2' >> ${configPath}/config.txt"

  # The 64-bit kernel, which starts by default on a standard intallation, lacks build directories
  # for the modules, which will cause the rawgadget build to fail. If they're missing, switch to
  # the 32-bit kernel, which should have them.
  if [  ! -d "/lib/modules/$(uname -r)/build" ]; then
    sudo -s eval "grep -qxF 'arm_64bit=0' ${configPath}/config.txt  || echo 'arm_64bit=0' >> ${configPath}/config.txt"
    echo "Before continuing, we must switch to a 32-bit kernel. This"
    echo "requires a reboot. After rebooting, please rerun './install.sh'"
    confirm_reboot
  fi
}

build_raw_gadget() {
  # Build customized raw-gadget kernel module.
  if [ ! -d "raw-gadget" ]; then
    git clone https://github.com/polysyllabic/raw-gadget.git
  fi
  cd ~/raw-gadget/raw_gadget
  make
  make install
  cd
}

install_engine() {
  # Now compile and install the chaos engine
  cd ~/chaos/chaos
  if [ ! -d "build" ]; then
	  mkdir build
  fi
  cd build
  cmake ..
  make -j4
  make install  
  sudo cp ~/scripts/chaos.service /etc/systemd/system/
  sudo systemctl daemon-reload
  sudo systemctl enable chaos
  echo "TCC engine installed as a systemd service"
  cd
}

install_chaosface() {
  # Get the python modules we need for chaosface
  # TO DO: package chaosface with pyinstaller
  if (( $is_developer > 0 || $remote_ui == 0 )); then
    sudo -s eval "pip3 install flexx pyzmq numpy pygame pythontwitchbotframework --system"
  fi
  if (( $remote_ui == 0 )); then
    sudo cp ~/scripts/chaosface.service /etc/systemd/system/
    sudo systemctl enable chaosface
    echo "Chaosface installed as a service"
  fi
}

# Main script exectution
if (( "$install_stage" >= 6 )); then
  echo "Twitch Controls Chaos has already been installed. Do you want to reconfigure it?"
  select yn in "Yes" "No"; do
    case $yn in
        Yes) install_stage=0;;
        No) exit;;
    esac
  done
fi

# skip earlier stages if we're resuming an installation
if (( $install_stage < 1 )); then
  install_questions
  install_stage=1;
fi
if (( $install_stage == 1 )); then
  build_dependencies
  install_stage=2
fi
if (( $install_stage == 2 )); then
  configure_as_gadget
  install_stage=3
fi
if (( $install_stage == 3 )); then
  build_raw_gadget
  install_stage=4
fi
if (( $install_stage == 4 )); then
  install_engine
  install_stage=5
fi
if (( $install_stage == 5 )); then
  install_chaosface()
fi
install_stage=6
echo "Twitch Controls Chaos Installation complete."
confirm_reboot
