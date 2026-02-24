#!/usr/bin/env bash
# Rebuild and reinstall the chaos engine runtime on a Pi/Linux host.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CHAOS_SRC_DIR="${REPO_ROOT}/chaos"
CHAOS_BUILD_DIR="${CHAOS_SRC_DIR}/build"
CHAOS_INSTALL_ROOT="${CHAOS_INSTALL_ROOT:-/usr/local/chaos}"

restart_service=0

for arg in "$@"; do
  case "${arg}" in
    --restart-service)
      restart_service=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--restart-service]"
      exit 2
      ;;
  esac
done

echo "--------------------------------------------------------"
echo "Updating chaos engine"
echo "Source: ${CHAOS_SRC_DIR}"
echo "Install root: ${CHAOS_INSTALL_ROOT}"

mkdir -p "${CHAOS_BUILD_DIR}"
cd "${CHAOS_BUILD_DIR}"

echo "Configuring build"
cmake ..

echo "Compiling engine"
make -j"$(nproc)"

echo "Installing engine"
sudo make install
sudo install -m 0644 "${REPO_ROOT}/VERSION" "${CHAOS_INSTALL_ROOT}/VERSION"

echo "Synchronizing startup script and service"
sudo install -m 0755 "${SCRIPT_DIR}/startchaos.sh" "${CHAOS_INSTALL_ROOT}/startchaos.sh"
sudo install -m 0644 "${SCRIPT_DIR}/chaos.service" /etc/systemd/system/chaos.service

sudo systemctl daemon-reload
sudo systemctl enable chaos

if (( restart_service )); then
  echo "Restarting chaos service"
  sudo systemctl reset-failed chaos || true
  sudo systemctl restart chaos
  sudo systemctl status chaos --no-pager --lines=20 || true
fi

echo "Chaos engine update complete."
