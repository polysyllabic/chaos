#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "DEPRECATED: use ${SCRIPT_DIR}/install_chaosface_standalone.sh instead." >&2
exec "${SCRIPT_DIR}/install_chaosface_standalone.sh" "$@"
