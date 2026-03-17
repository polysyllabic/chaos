#!/bin/bash
set -euo pipefail

echo "--------------------------------------------------------"
echo "Starting Twitch Controls Chaos engine"
cd /usr/local/chaos

if [ -f /usr/local/chaos/chaos.env ]; then
  # shellcheck disable=SC1091
  set -a
  . /usr/local/chaos/chaos.env
  set +a
fi

# raw_gadget should only be inserted once per boot. On service restarts it may already
# be loaded, and attempting to insmod again causes a hard failure ("File exists").
if [ -d /sys/module/raw_gadget ]; then
  echo "raw_gadget module already loaded; skipping insert."
elif [ -x /usr/local/modules/insmod.sh ]; then
  /usr/local/modules/insmod.sh
else
  echo "WARNING: /usr/local/modules/insmod.sh not found or not executable; skipping module insert."
fi

exec /usr/local/chaos/chaos
