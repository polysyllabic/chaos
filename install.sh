#!/bin/bash

echo "--------------------------------------------------------"
echo "Installing Twitch Controls Chaos: "

#build dependencies:
declare -a depencencies=(build-essential libncurses5-dev libusb-1.0-0-dev libjsoncpp-dev cmake git libzmq3-dev raspberrypi-kernel-headers python3-dev libatlas-base-dev python3-pip)
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

cd chaos
if [ ! -d "build" ];
then
	mkdir build
fi
cd build
cmake ..
make -j4
make install
cd ..

# add dwc2 to /boot/config.txt (lets us run the raspberry pi as a gadget)
sudo -s eval "grep -qxF 'dtoverlay=dwc2' /boot/config.txt  || echo 'dtoverlay=dwc2' >> /boot/config.txt "

sudo -s eval "pip3 install flexx pyzmq numpy pygame --system"

#TODO: Push this into the make install process above
#if [ ! -d "~/chaosLogs" ];
#then
#	mkdir ~/chaosLogs

# cd scripts/
# sudo bash installservices.sh
# cd ..

echo "Done."
echo "--------------------------------------------------------"
