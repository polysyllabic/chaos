#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"

PI_USB_TARGETS=(
  test_usb_sniffify_environment
  test_usb_sniffify_raw_gadget_session
  test_usb_sniffify_passthrough_smoke
)

chaos_was_active=0

restore_chaos_service() {
  if [[ ${chaos_was_active} -eq 1 ]]; then
    echo "Restarting chaos.service..."
    systemctl start chaos.service
  fi
}

echo "Building Pi usb_sniffify hardware test targets in '${BUILD_DIR}'..."
cmake --build "${BUILD_DIR}" --target "${PI_USB_TARGETS[@]}"

if [[ ${EUID} -eq 0 ]]; then
  trap restore_chaos_service EXIT

  if systemctl is-active --quiet chaos.service; then
    chaos_was_active=1
    echo "Stopping chaos.service for exclusive usb_sniffify hardware tests..."
    systemctl stop chaos.service
  fi

  if [[ ! -c /dev/raw-gadget ]] && [[ -x /usr/local/modules/insmod.sh ]]; then
    echo "Loading raw_gadget via /usr/local/modules/insmod.sh..."
    /usr/local/modules/insmod.sh
  fi
fi

echo "Running Pi usb_sniffify hardware tests..."
skipped=0
for test_name in "${PI_USB_TARGETS[@]}"; do
  test_path="${BUILD_DIR}/tests/${test_name}"
  echo "==> ${test_path}"
  set +e
  "${test_path}"
  status=$?
  set -e

  if [[ ${status} -eq 0 ]]; then
    continue
  fi

  if [[ ${status} -eq 77 ]]; then
    skipped=$((skipped + 1))
    continue
  fi

  echo "FAIL: ${test_name} exited with status ${status}"
  exit "${status}"
done

echo "PASS: Pi usb_sniffify hardware tests completed (${skipped} skipped)"
