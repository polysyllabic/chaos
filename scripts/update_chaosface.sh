#!/bin/bash
# Package chaosface source and deploy it into /usr/local/chaos with a dedicated venv.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CHAOSFACE_SRC_ROOT="${REPO_ROOT}/chaosface"

CHAOS_INSTALL_ROOT="${CHAOS_INSTALL_ROOT:-/usr/local/chaos}"
CHAOSFACE_STAGE_DIR="${CHAOS_INSTALL_ROOT}/chaosface-src"
CHAOSFACE_VENV_DIR="${CHAOS_INSTALL_ROOT}/venv"

install_deps=1
restart_service=0

ensure_openblas_runtime() {
  local -a openblas_candidates=(
    libopenblas0-pthread
    libopenblas0
    libopenblas-base
  )
  local pkg

  for pkg in "${openblas_candidates[@]}"; do
    if dpkg-query -W -f='${Status}' "${pkg}" 2>/dev/null | grep -q "ok installed"; then
      return
    fi
  done

  for pkg in "${openblas_candidates[@]}"; do
    if apt-cache show "${pkg}" >/dev/null 2>&1; then
      echo "Installing runtime dependency: ${pkg}"
      sudo apt-get update
      sudo apt-get install -y "${pkg}"
      return
    fi
  done

  echo "WARNING: Could not locate an OpenBLAS runtime package. NumPy may fail to import."
}

for arg in "$@"; do
  case "${arg}" in
    --skip-deps)
      install_deps=0
      ;;
    --restart-service)
      restart_service=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--skip-deps] [--restart-service]"
      exit 2
      ;;
  esac
done

bundle_path="$(mktemp /tmp/chaosface-src.XXXXXX.tar.gz)"
trap 'rm -f "${bundle_path}"' EXIT

echo "--------------------------------------------------------"
echo "Packaging chaosface source"
tar \
  --exclude='.venv' \
  --exclude='__pycache__' \
  --exclude='*.pyc' \
  --exclude='.pytest_cache' \
  --exclude='.mypy_cache' \
  -C "${CHAOSFACE_SRC_ROOT}" \
  -czf "${bundle_path}" \
  chaosface \
  pyproject.toml \
  setup.py \
  requirements.txt \
  README.rst \
  LICENSE

echo "Deploying chaosface source to ${CHAOSFACE_STAGE_DIR}"
tmp_stage_dir="${CHAOS_INSTALL_ROOT}/.chaosface-src.new"
sudo rm -rf "${tmp_stage_dir}"
sudo mkdir -p "${tmp_stage_dir}"
sudo tar -xzf "${bundle_path}" -C "${tmp_stage_dir}"
sudo rm -rf "${CHAOSFACE_STAGE_DIR}"
sudo mv "${tmp_stage_dir}" "${CHAOSFACE_STAGE_DIR}"

echo "Preparing runtime directories"
sudo mkdir -p "${CHAOS_INSTALL_ROOT}/configs"

if [ ! -x "${CHAOSFACE_VENV_DIR}/bin/python3" ]; then
  echo "Creating virtualenv at ${CHAOSFACE_VENV_DIR}"
  sudo python3 -m venv "${CHAOSFACE_VENV_DIR}"
fi

ensure_openblas_runtime

echo "Upgrading build tooling in venv"
sudo "${CHAOSFACE_VENV_DIR}/bin/python3" -m pip install --upgrade pip setuptools wheel

if (( install_deps )); then
  echo "Installing runtime dependencies"
  sudo "${CHAOSFACE_VENV_DIR}/bin/pip" install --upgrade \
    -r "${CHAOSFACE_STAGE_DIR}/requirements.txt" \
    pyzmq \
    pygame
fi

echo "Installing chaosface package into venv"
sudo "${CHAOSFACE_VENV_DIR}/bin/pip" install --upgrade "${CHAOSFACE_STAGE_DIR}"

if (( restart_service )); then
  echo "Restarting chaosface service"
  sudo systemctl restart chaosface
  sudo systemctl status chaosface --no-pager --lines=20 || true
fi

echo "Chaosface deploy complete."
echo "Runtime Python: ${CHAOSFACE_VENV_DIR}/bin/python3"
