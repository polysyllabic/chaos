#!/usr/bin/env bash
# Update both chaos engine and chaosface runtimes on a Pi/Linux host.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

restart_services=0
skip_deps=0

for arg in "$@"; do
  case "${arg}" in
    --restart)
      restart_services=1
      ;;
    --skip-deps)
      skip_deps=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--restart] [--skip-deps]"
      exit 2
      ;;
  esac
done

engine_args=()
chaosface_args=()

if (( restart_services )); then
  engine_args+=(--restart)
  chaosface_args+=(--restart)
fi
if (( skip_deps )); then
  chaosface_args+=(--skip-deps)
fi

echo "--------------------------------------------------------"
echo "Updating full chaos stack"
"${SCRIPT_DIR}/update_engine.sh" "${engine_args[@]}"
"${SCRIPT_DIR}/update_chaosface.sh" "${chaosface_args[@]}"
echo "Full chaos update complete."
