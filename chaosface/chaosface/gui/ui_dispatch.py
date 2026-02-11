#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Dispatch relay updates onto the main asyncio loop."""

from __future__ import annotations

import asyncio
from typing import Callable, Optional

_ui_loop: Optional[asyncio.AbstractEventLoop] = None


def set_ui_loop(loop: asyncio.AbstractEventLoop) -> None:
  """Set the loop used for thread-safe relay mutations."""
  global _ui_loop
  _ui_loop = loop


def call_soon(func: Callable, *args) -> None:
  """
  Schedule a function on the configured UI loop.

  If no loop has been configured yet, this falls back to direct execution.
  """
  if _ui_loop and _ui_loop.is_running():
    _ui_loop.call_soon_threadsafe(func, *args)
  else:
    func(*args)
