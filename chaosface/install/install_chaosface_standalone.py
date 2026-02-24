#!/usr/bin/env python3
"""Install Chaosface on a non-Pi machine with minimal user steps."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

MIN_PYTHON = (3, 10)


def _print(msg: str) -> None:
  print(msg, flush=True)


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
  _print(f"+ {' '.join(cmd)}")
  subprocess.run(cmd, check=True, cwd=str(cwd) if cwd else None)


def _write_text(path: Path, content: str, executable: bool = False) -> None:
  path.write_text(content, encoding="utf-8")
  if executable:
    mode = path.stat().st_mode
    path.chmod(mode | 0o111)


def validate_python() -> None:
  current = sys.version_info
  if current < MIN_PYTHON:
    raise SystemExit(
      f"Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ is required. "
      f"Detected {current.major}.{current.minor}.{current.micro}."
    )


def default_install_dir() -> Path:
  return Path.home() / "chaosface-runtime"


def resolve_install_dir(install_dir_arg: str | None) -> Path:
  is_root = hasattr(os, "geteuid") and os.geteuid() == 0
  if install_dir_arg is None and is_root:
    raise SystemExit(
      "Running as root requires an explicit install directory.\n"
      "Use --install-dir <path> (for example: /usr/local/chaos),\n"
      "or rerun without sudo to install under your user home."
    )
  install_dir_value = install_dir_arg if install_dir_arg is not None else str(default_install_dir())
  return Path(install_dir_value).expanduser().resolve()


def create_launchers(install_dir: Path) -> None:
  sh_launcher = install_dir / "run_chaosface.sh"
  bat_launcher = install_dir / "run_chaosface.bat"

  _write_text(
    sh_launcher,
    """#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"
exec "${SCRIPT_DIR}/venv/bin/python" -m chaosface.apps.TwitchControlsChaos "$@"
""",
    executable=True,
  )

  _write_text(
    bat_launcher,
    """@echo off
setlocal
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"
"%SCRIPT_DIR%venv\\Scripts\\python.exe" -m chaosface.apps.TwitchControlsChaos %*
""",
  )


def install_shared_version_file(script_path: Path, install_dir: Path) -> None:
  repo_root = script_path.parents[2]
  candidates = [
    repo_root / "VERSION",
    script_path.parents[1] / "VERSION",
  ]
  for candidate in candidates:
    if candidate.is_file():
      target = install_dir / "VERSION"
      target.write_text(candidate.read_text(encoding="utf-8"), encoding="utf-8")
      _print(f"Installed shared VERSION file at {target}")
      return
  _print("WARNING: Shared VERSION file not found; chaosface version will fall back to 0.0.0.")


def install(args: argparse.Namespace) -> None:
  script_path = Path(__file__).resolve()
  project_root = script_path.parents[1]
  requirements_path = script_path.parent / "requirements-remote.txt"

  if not requirements_path.exists():
    raise SystemExit(f"Missing requirements file: {requirements_path}")
  if not (project_root / "pyproject.toml").exists():
    raise SystemExit(f"Could not locate Chaosface project root: {project_root}")

  install_dir = resolve_install_dir(args.install_dir)
  venv_dir = install_dir / "venv"
  configs_dir = install_dir / "configs"

  _print("--------------------------------------------------------")
  _print("Chaosface Remote Installer")
  _print(f"Install directory: {install_dir}")
  _print(f"Project source: {project_root}")

  install_dir.mkdir(parents=True, exist_ok=True)
  configs_dir.mkdir(parents=True, exist_ok=True)

  if not (venv_dir / "pyvenv.cfg").exists():
    _print("Creating virtual environment")
    _run([sys.executable, "-m", "venv", str(venv_dir)])
  else:
    _print("Reusing existing virtual environment")

  if os.name == "nt":
    venv_python = venv_dir / "Scripts" / "python.exe"
  else:
    venv_python = venv_dir / "bin" / "python"

  _print("Upgrading pip/setuptools/wheel")
  _run([str(venv_python), "-m", "pip", "install", "--upgrade", "pip", "setuptools", "wheel"])

  _print("Installing runtime dependencies")
  _run([str(venv_python), "-m", "pip", "install", "--upgrade", "-r", str(requirements_path)])

  _print("Installing Chaosface from local source")
  _run([str(venv_python), "-m", "pip", "install", "--upgrade", str(project_root)])

  install_shared_version_file(script_path, install_dir)
  create_launchers(install_dir)

  _print("")
  _print("Install complete.")
  _print("Start Chaosface with one of the following:")
  _print(f"  - Linux/macOS: {install_dir / 'run_chaosface.sh'}")
  _print(f"  - Windows:     {install_dir / 'run_chaosface.bat'}")
  _print("")
  _print("The first run creates local config files under:")
  _print(f"  {configs_dir}")


def parse_args() -> argparse.Namespace:
  default_dir = default_install_dir()
  parser = argparse.ArgumentParser(description="Install Chaosface for remote-host usage.")
  parser.add_argument(
    "--install-dir",
    default=None,
    help=f"Target installation directory (default: {default_dir})",
  )
  return parser.parse_args()


def main() -> None:
  validate_python()
  args = parse_args()
  install(args)


if __name__ == "__main__":
  main()
