#!/bin/bash
# Package chaosface source and deploy it into /usr/local/chaos with a runtime venv.
# Developer installs also provision a source-tree venv for local tooling and tests.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CHAOSFACE_SRC_ROOT="${REPO_ROOT}/chaosface"

CHAOS_INSTALL_ROOT="${CHAOS_INSTALL_ROOT:-/usr/local/chaos}"
CHAOSFACE_STAGE_DIR="${CHAOS_INSTALL_ROOT}/chaosface-src"
CHAOSFACE_RUNTIME_VENV_DIR="${CHAOS_INSTALL_ROOT}/venv"
CHAOSFACE_DEV_VENV_DIR="${CHAOSFACE_SRC_ROOT}/.venv"

install_deps=1
restart_service=0
developer_install=0

ensure_venv() {
  local venv_dir="$1"
  local use_sudo="${2:-0}"

  if [ -x "${venv_dir}/bin/python3" ]; then
    return
  fi

  echo "Creating virtualenv at ${venv_dir}"
  if (( use_sudo )); then
    sudo python3 -m venv "${venv_dir}"
  else
    python3 -m venv "${venv_dir}"
  fi
}

upgrade_build_tooling() {
  local python_bin="$1"
  local use_sudo="${2:-0}"

  if (( use_sudo )); then
    sudo "${python_bin}" -m pip install --upgrade pip setuptools wheel
  else
    "${python_bin}" -m pip install --upgrade pip setuptools wheel
  fi
}

install_shared_python_dependencies() {
  local python_bin="$1"
  local requirements_path="$2"
  local use_sudo="${3:-0}"

  if (( use_sudo )); then
    sudo "${python_bin}" -m pip install --upgrade \
      -r "${requirements_path}" \
      pyzmq \
      pygame
  else
    "${python_bin}" -m pip install --upgrade \
      -r "${requirements_path}" \
      pyzmq \
      pygame
  fi
}

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
    --restart)
      restart_service=1
      ;;
    --developer)
      developer_install=1
      ;;
    *)
      echo "Unknown option: ${arg}"
      echo "Usage: $0 [--skip-deps] [--restart] [--developer]"
      exit 2
      ;;
  esac
done

bundle_path="$(mktemp /tmp/chaosface-src.XXXXXX.tar.gz)"
trap 'rm -f "${bundle_path}"' EXIT

echo "--------------------------------------------------------"
echo "Packaging chaosface source"
tar \
  --exclude='venv' \
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
if [ -f "${REPO_ROOT}/VERSION" ]; then
  sudo install -m 0644 "${REPO_ROOT}/VERSION" "${CHAOS_INSTALL_ROOT}/VERSION"
fi

echo "Preparing runtime directories"
sudo mkdir -p "${CHAOS_INSTALL_ROOT}/configs"

ensure_venv "${CHAOSFACE_RUNTIME_VENV_DIR}" 1

ensure_openblas_runtime

echo "Upgrading build tooling in runtime venv"
upgrade_build_tooling "${CHAOSFACE_RUNTIME_VENV_DIR}/bin/python3" 1

if (( install_deps )); then
  echo "Installing runtime dependencies"
  install_shared_python_dependencies "${CHAOSFACE_RUNTIME_VENV_DIR}/bin/python3" "${CHAOSFACE_STAGE_DIR}/requirements.txt" 1
fi

if (( developer_install )); then
  ensure_venv "${CHAOSFACE_DEV_VENV_DIR}"
  echo "Upgrading build tooling in developer venv"
  upgrade_build_tooling "${CHAOSFACE_DEV_VENV_DIR}/bin/python3"

  if (( install_deps )); then
    echo "Installing developer dependencies"
    install_shared_python_dependencies "${CHAOSFACE_DEV_VENV_DIR}/bin/python3" "${CHAOSFACE_SRC_ROOT}/requirements.txt"
    "${CHAOSFACE_DEV_VENV_DIR}/bin/python3" -m pip install --upgrade pytest
  fi

  echo "Installing chaosface package into developer venv"
  "${CHAOSFACE_DEV_VENV_DIR}/bin/python3" -m pip install --upgrade --editable "${CHAOSFACE_SRC_ROOT}"
fi

echo "Installing chaosface package into runtime venv"
sudo "${CHAOSFACE_RUNTIME_VENV_DIR}/bin/python3" -m pip install --upgrade "${CHAOSFACE_STAGE_DIR}"

if (( restart_service )); then
  echo "Restarting chaosface service"
  sudo systemctl restart chaosface
  sudo systemctl status chaosface --no-pager --lines=20 || true
fi

echo "Chaosface deploy complete."
echo "Runtime Python: ${CHAOSFACE_RUNTIME_VENV_DIR}/bin/python3"
if (( developer_install )); then
  echo "Developer Python: ${CHAOSFACE_DEV_VENV_DIR}/bin/python3"
fi
