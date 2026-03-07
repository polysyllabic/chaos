#!/usr/bin/env bash
# Sync startup/service files without rebuilding engine or redeploying chaosface packages.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHAOS_INSTALL_ROOT="${CHAOS_INSTALL_ROOT:-/usr/local/chaos}"

restart_services=0

for arg in "$@"; do
  case "${arg}" in
    --restart)
      restart_services=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--restart]"
      exit 2
      ;;
  esac
done

echo "--------------------------------------------------------"
echo "Updating startup scripts and service units"
echo "Install root: ${CHAOS_INSTALL_ROOT}"

echo "Installing startup script"
sudo install -m 0755 "${SCRIPT_DIR}/startchaos.sh" "${CHAOS_INSTALL_ROOT}/startchaos.sh"

echo "Installing systemd unit files"
sudo install -m 0644 "${SCRIPT_DIR}/chaos.service" /etc/systemd/system/chaos.service
sudo install -m 0644 "${SCRIPT_DIR}/chaosface.service" /etc/systemd/system/chaosface.service

echo "Reloading systemd daemon"
sudo systemctl daemon-reload
sudo systemctl enable chaos
sudo systemctl enable chaosface

if (( restart_services )); then
  echo "Restarting services"
  sudo systemctl reset-failed chaos || true
  sudo systemctl reset-failed chaosface || true
  sudo systemctl restart chaos
  sudo systemctl restart chaosface
  sudo systemctl status chaos --no-pager --lines=20 || true
  sudo systemctl status chaosface --no-pager --lines=20 || true
fi

echo "Service update complete."
