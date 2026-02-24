# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Shared version parsing for chaosface, mirroring engine VERSION formatting."""

from __future__ import annotations

import os
import re
from functools import lru_cache
from pathlib import Path
from typing import Optional

_INT_PATTERNS = {
  'major': re.compile(r'^VERSION_MAJOR[ \t]+([0-9]+)\s*$', re.MULTILINE),
  'minor': re.compile(r'^VERSION_MINOR[ \t]+([0-9]+)\s*$', re.MULTILINE),
  'patch': re.compile(r'^VERSION_PATCH[ \t]+([0-9]+)\s*$', re.MULTILINE),
  'prerelease_version': re.compile(r'^PRERELEASE_VERSION[ \t]+([0-9]+)\s*$', re.MULTILINE),
}
_PRERELEASE_PATTERN = re.compile(r'^PRERELEASE[ \t]+([^\n]+)\s*$', re.MULTILINE)
_FALLBACK_VERSION = '0.0.0'


def _parse_int_field(name: str, text: str) -> Optional[int]:
  pattern = _INT_PATTERNS[name]
  match = pattern.search(text)
  if not match:
    return None
  try:
    return int(match.group(1))
  except (TypeError, ValueError):
    return None


def _parse_version_text(text: str) -> Optional[str]:
  major = _parse_int_field('major', text)
  minor = _parse_int_field('minor', text)
  patch = _parse_int_field('patch', text)
  if major is None or minor is None or patch is None:
    return None

  version = f'{major}.{minor}.{patch}'
  prerelease_match = _PRERELEASE_PATTERN.search(text)
  if not prerelease_match:
    return version

  prerelease = prerelease_match.group(1).strip()
  if not prerelease:
    return version

  prerelease_version = _parse_int_field('prerelease_version', text)
  if prerelease_version is None:
    return f'{version}-{prerelease}'
  return f'{version}-{prerelease}.{prerelease_version}'


def _candidate_version_paths() -> list[Path]:
  candidates: list[Path] = []

  env_path = str(os.getenv('CHAOS_VERSION_FILE', '') or '').strip()
  if env_path:
    candidates.append(Path(env_path).expanduser())

  cwd = Path.cwd()
  candidates.extend([
    cwd / 'VERSION',
    cwd.parent / 'VERSION',
    Path('/usr/local/chaos/VERSION'),
  ])

  this_file = Path(__file__).resolve()
  candidates.extend(parent / 'VERSION' for parent in this_file.parents[:6])

  unique: list[Path] = []
  seen = set()
  for path in candidates:
    key = str(path)
    if key in seen:
      continue
    seen.add(key)
    unique.append(path)
  return unique


@lru_cache(maxsize=1)
def get_version() -> str:
  for path in _candidate_version_paths():
    try:
      text = path.read_text(encoding='utf-8')
    except Exception:
      continue
    parsed = _parse_version_text(text)
    if parsed:
      return parsed
  return _FALLBACK_VERSION

