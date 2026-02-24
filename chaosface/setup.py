from __future__ import annotations

import re
from pathlib import Path

from setuptools import find_packages, setup


def _parse_int(pattern: str, text: str) -> int | None:
  match = re.search(pattern, text, flags=re.MULTILINE)
  if not match:
    return None
  try:
    return int(match.group(1))
  except (TypeError, ValueError):
    return None


def _load_shared_version() -> str:
  here = Path(__file__).resolve().parent
  candidates = [
    here / 'VERSION',
    here.parent / 'VERSION',
  ]
  for path in candidates:
    try:
      text = path.read_text(encoding='utf-8')
    except Exception:
      continue

    major = _parse_int(r'^VERSION_MAJOR[ \t]+([0-9]+)\s*$', text)
    minor = _parse_int(r'^VERSION_MINOR[ \t]+([0-9]+)\s*$', text)
    patch = _parse_int(r'^VERSION_PATCH[ \t]+([0-9]+)\s*$', text)
    if major is None or minor is None or patch is None:
      continue

    version = f'{major}.{minor}.{patch}'
    prerelease_match = re.search(r'^PRERELEASE[ \t]+([^\n]+)\s*$', text, flags=re.MULTILINE)
    if not prerelease_match:
      return version

    prerelease = prerelease_match.group(1).strip()
    prerelease_num = _parse_int(r'^PRERELEASE_VERSION[ \t]+([0-9]+)\s*$', text)
    if prerelease and prerelease_num is not None:
      return f'{version}-{prerelease}.{prerelease_num}'
    if prerelease:
      return f'{version}-{prerelease}'
    return version

  return '0.0.0'


setup(
  name='chaosface',
  version=_load_shared_version(),
  packages=find_packages(),
)
