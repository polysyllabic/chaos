#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHAOS_SRC_DIR="${REPO_ROOT}/chaos"
BUILD_DIR="${CHAOS_BUILD_DIR:-${CHAOS_SRC_DIR}/build}"
BIN_PATH="${BUILD_DIR}/gamepad_test"

ensure_configured() {
  if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "Configuring CMake build tree at ${BUILD_DIR}..." >&2
    cmake -S "${CHAOS_SRC_DIR}" -B "${BUILD_DIR}" >&2
  fi
}

ensure_configured
cmake --build "${BUILD_DIR}" --target gamepad_test -j4 >&2

exec "${BIN_PATH}" "$@"
