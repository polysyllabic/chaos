# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Shared helper functions for NiceGUI views."""

from __future__ import annotations

from typing import Any, Optional

import chaosface.config.globals as config


def clamp01(value: Any) -> float:
  try:
    out = float(value)
  except (TypeError, ValueError):
    return 0.0
  return max(0.0, min(1.0, out))


def safe_int(value: Any, fallback: int, minval: Optional[int] = None, maxval: Optional[int] = None) -> int:
  try:
    out = int(float(value))
  except (TypeError, ValueError):
    out = fallback
  if minval is not None and out < minval:
    out = minval
  if maxval is not None and out > maxval:
    out = maxval
  return out


def safe_float(value: Any, fallback: float, minval: Optional[float] = None, maxval: Optional[float] = None) -> float:
  try:
    out = float(value)
  except (TypeError, ValueError):
    out = fallback
  if minval is not None and out < minval:
    out = minval
  if maxval is not None and out > maxval:
    out = maxval
  return out


def relay_config_float(key: str, fallback: float, minval: Optional[float] = None, maxval: Optional[float] = None) -> float:
  return safe_float(config.relay.get_attribute(key), fallback, minval, maxval)


def sync_enabled_mods() -> None:
  config.relay.enabled_mods = [
    key for key, data in config.relay.modifier_data.items() if bool(data.get('active', True))
  ]
