#!/usr/bin/env bash
# Sync game configuration files into the installed games directory.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SOURCE_GAMES_DIR="${REPO_ROOT}/chaos/examples"

CHAOS_INSTALL_ROOT="${CHAOS_INSTALL_ROOT:-/usr/local/chaos}"
CHAOS_GAMES_DIR="${CHAOS_GAMES_DIR:-${CHAOS_INSTALL_ROOT}/games}"

dry_run=0
restart_service=0

for arg in "$@"; do
  case "${arg}" in
    --dry-run)
      dry_run=1
      ;;
    --restart-service)
      restart_service=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--dry-run] [--restart-service]"
      exit 2
      ;;
  esac
done

if [ ! -d "${SOURCE_GAMES_DIR}" ]; then
  echo "Source games directory not found: ${SOURCE_GAMES_DIR}"
  exit 1
fi

echo "--------------------------------------------------------"
echo "Updating game configurations"
echo "Source: ${SOURCE_GAMES_DIR}"
echo "Destination: ${CHAOS_GAMES_DIR}"

sudo mkdir -p "${CHAOS_GAMES_DIR}"

rsync_args=(
  -a
  --checksum
  --itemize-changes
)

if (( dry_run )); then
  rsync_args+=(--dry-run)
fi

sync_output="$(
  sudo rsync \
    "${rsync_args[@]}" \
    "${SOURCE_GAMES_DIR}/" \
    "${CHAOS_GAMES_DIR}/"
)"

if [ -n "${sync_output}" ]; then
  echo "${sync_output}"
else
  if (( dry_run )); then
    echo "Dry run complete. No changes detected."
  else
    echo "No game configuration changes detected."
  fi
fi

if (( restart_service )) && (( ! dry_run )); then
  echo "Restarting chaos service"
  sudo systemctl reset-failed chaos || true
  sudo systemctl restart chaos
  sudo systemctl status chaos --no-pager --lines=20 || true
fi

echo "Game configuration update complete."
