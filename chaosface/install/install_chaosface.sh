#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_BIN="${PYTHON_BIN:-python3}"

if ! command -v "${PYTHON_BIN}" >/dev/null 2>&1; then
  echo "Could not find '${PYTHON_BIN}'. Install Python 3.10+ and try again."
  exit 1
fi

exec "${PYTHON_BIN}" "${SCRIPT_DIR}/install_chaosface.py" "$@"
