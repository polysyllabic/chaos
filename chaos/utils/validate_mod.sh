#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHAOS_SRC_DIR="${REPO_ROOT}/chaos"
BUILD_DIR="${CHAOS_BUILD_DIR:-${CHAOS_SRC_DIR}/build}"
BIN_PATH="${BUILD_DIR}/validate_mod"
USB_MODE=0

for arg in "$@"; do
  if [[ "${arg}" == "--usb" ]]; then
    USB_MODE=1
    break
  fi
done

ensure_configured() {
  if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "Configuring CMake build tree at ${BUILD_DIR}..." >&2
    cmake -S "${CHAOS_SRC_DIR}" -B "${BUILD_DIR}" >&2
  fi
}

ensure_configured
cmake --build "${BUILD_DIR}" --target validate_mod -j4 >&2

halt_chaos_engine() {
  local stop_cmd
  stop_cmd='
if command -v systemctl >/dev/null 2>&1; then
  systemctl stop chaos.service >/dev/null 2>&1 || true
  systemctl stop chaos >/dev/null 2>&1 || true
fi
pkill -TERM -x chaos >/dev/null 2>&1 || true
'
  if [[ "${EUID}" -eq 0 ]]; then
    bash -c "${stop_cmd}"
  else
    sudo bash -c "${stop_cmd}"
  fi
}

if [[ "${USB_MODE}" -eq 1 ]]; then
  echo "USB mode requested; stopping any running chaos engine instances." >&2
  halt_chaos_engine
  if [[ "${EUID}" -eq 0 ]]; then
    exec "${BIN_PATH}" "$@"
  fi
  exec sudo "${BIN_PATH}" "$@"
fi

exec "${BIN_PATH}" "$@"
