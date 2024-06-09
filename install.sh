#!/bin/bash
echo "--------------------------------------------------------"
echo "Installing Twitch Controls Chaos"

confirm_reboot() {
  local ask_for_rerun=$1
  echo "The system must be rebooted before continuing."
  if [ "$ask_for_rerun" = true ];
  then
    echo "After the reboot, please rerun './install.sh'"
  fi
  read -p "Reboot now? (yes/no): " response
  response_lc=$(echo "$response" | tr '[:upper:]' '[:lower:]')
  if ["$response_lc" = "yes"] || ["$response_lc" = "y"];
  then
    # TODO: Make ourselves a service so we can run automatically after the reboot
    echo "Rebooting..."
    sudo reboot
  else
    echo "You must reboot manually before the installation can continue."
    exit 1
  fi
}

# Build dependencies:
nodeclare -a depencencies=(build-essential libncurses5-dev libusb-1.0-0-dev libjsoncpp-dev cmake git libzmq3-dev raspberrypi-kernel-headers python3-dev libatlas-base-dev python3-pip)
toInstall=()
echo "Dependencies:" ${depencencies[@]}
for dependency in "${depencencies[@]}"
do
	echo "Checking for" $dependency
	if [ $(dpkg-query -W -f='${Status}' $dependency 2>/dev/null | grep -c "ok installed") -eq 0 ];
	then
		echo "not installed:" $dependency
		toInstall+=($dependency)
	fi
done
echo ${toInstall[@]}

if [ ${#toInstall[@]} -ne 0 ];
then
	sudo apt-get update
	sudo apt-get install -y ${toInstall[@]}
fi

# Add dwc2 to /boot/config.txt (lets us run the raspberry pi as a gadget)
sudo -s eval "grep -qxF 'dtoverlay=dwc2' /boot/config.txt  || echo 'dtoverlay=dwc2' >> /boot/config.txt"

# The 64-bit kernel (6.1.21-v8+) lacks build directories for the modules, which will cause the rawgadget
# build to fail. If they're missing, switch to the 32-bit kernel, which should have them.
if [  ! -d "/lib/modules/$(uname -r)/build" ];
then
  sudo -s eval "grep -qxF 'arm_64bit=0' /boot/config.txt  || echo 'arm_64bit=0' >> /boot/config.txt"
  confirm_reboot true
fi

# Build customized raw-gadget kernel module.
if [ ! -d "rawgadget" ];
then
  git clone https://github.com/polysyllabic/raw-gadget.git
fi
cd raw-gadget/raw_gadget
make

# Now compile the chaos engine
cd ~/chaos/chaos
if [ ! -d "build" ];
then
	mkdir build
fi
cd build
cmake ..
make -j4
make install
cd

# Get the python modules we need for chaosface
# TO DO: install chaosface in a more regular way (w/ pep?)
sudo -s eval "pip3 install flexx pyzmq numpy pygame pythontwitchbotframework --system"

# cd scripts/
# sudo bash installservices.sh
# cd ..

echo "Twitch Controls Chaos Installation complete"
confirm_reboot false
