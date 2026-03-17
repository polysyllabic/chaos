#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"

UNIT_TARGETS=(
  test_remapping
  test_modifier_types
  test_tlou2_modifier_coverage
  test_engine_lifecycle
)

echo "Building unit test targets in '${BUILD_DIR}'..."
cmake --build "${BUILD_DIR}" --target "${UNIT_TARGETS[@]}"

echo "Running unit tests..."
for test_name in "${UNIT_TARGETS[@]}"; do
  test_path="${BUILD_DIR}/tests/${test_name}"
  echo "==> ${test_path}"
  "${test_path}"
done

echo "PASS: all unit tests"
