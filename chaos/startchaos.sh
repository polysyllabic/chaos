#!/bin/bash

echo "--------------------------------------------------------"
echo "Starting Chaos"

cd /home/pi/chaos/raw-gadget-timeout/raw_gadget
./insmod.sh

#while true;
#do
	/usr/local/chaos/chaos tlou2.toml
#done
