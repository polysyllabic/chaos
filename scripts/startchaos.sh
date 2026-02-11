#!/bin/bash
set -euo pipefail

echo "--------------------------------------------------------"
echo "Starting Twitch Controls Chaos engine"
cd /usr/local/chaos
/usr/local/modules/insmod.sh
exec /usr/local/chaos/chaos
