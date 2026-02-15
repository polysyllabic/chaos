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


def install(args: argparse.Namespace) -> None:
  script_path = Path(__file__).resolve()
  project_root = script_path.parents[1]
  requirements_path = script_path.parent / "requirements-remote.txt"

  if not requirements_path.exists():
    raise SystemExit(f"Missing requirements file: {requirements_path}")
  if not (project_root / "pyproject.toml").exists():
    raise SystemExit(f"Could not locate Chaosface project root: {project_root}")

  install_dir = Path(args.install_dir).expanduser().resolve()
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
  parser = argparse.ArgumentParser(description="Install Chaosface for remote-host usage.")
  parser.add_argument(
    "--install-dir",
    default=str(Path.home() / "chaosface-runtime"),
    help="Target installation directory (default: %(default)s)",
  )
  return parser.parse_args()


def main() -> None:
  validate_python()
  args = parse_args()
  install(args)


if __name__ == "__main__":
  main()
