#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHAOS_SRC_DIR="${REPO_ROOT}/chaos"
BUILD_DIR="${CHAOS_BUILD_DIR:-${CHAOS_SRC_DIR}/build}"
BIN_PATH="${BUILD_DIR}/chaos_parse_game_config"

ensure_configured() {
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "Configuring CMake build tree at ${BUILD_DIR}..." >&2
    cmake -S "${CHAOS_SRC_DIR}" -B "${BUILD_DIR}" >&2
  fi
}

ensure_configured
cmake --build "${BUILD_DIR}" --target chaos_parse_game_config -j4 >&2

EXTRA_ARGS=()
HAS_CHAOS_CONFIG=0
for arg in "$@"; do
  if [[ "${arg}" == "--chaos-config" ]]; then
    HAS_CHAOS_CONFIG=1
    break
  fi
done

DEFAULT_CHAOS_CONFIG="${REPO_ROOT}/chaos/chaosconfig.toml"
if [[ ${HAS_CHAOS_CONFIG} -eq 0 && -f "${DEFAULT_CHAOS_CONFIG}" ]]; then
  EXTRA_ARGS+=(--chaos-config "${DEFAULT_CHAOS_CONFIG}")
fi

exec "${BIN_PATH}" "${EXTRA_ARGS[@]}" "$@"
