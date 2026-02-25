#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${CHAOS_BUILD_DIR:-${REPO_ROOT}/chaos/build}"
BIN_PATH="${BUILD_DIR}/chaos_parse_game_config"

if [[ ! -x "${BIN_PATH}" ]]; then
  echo "Building chaos_parse_game_config..." >&2
  cmake --build "${BUILD_DIR}" --target chaos_parse_game_config -j4 >&2
fi

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
