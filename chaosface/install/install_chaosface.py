#!/usr/bin/env python3
"""Deprecated wrapper for the standalone Chaosface installer."""

from __future__ import annotations

import runpy
import sys
from pathlib import Path


def main() -> None:
  script_path = Path(__file__).with_name("install_chaosface_standalone.py")
  print(
    f"DEPRECATED: use {script_path} instead.",
    file=sys.stderr,
    flush=True,
  )
  runpy.run_path(str(script_path), run_name="__main__")


if __name__ == "__main__":
  main()
