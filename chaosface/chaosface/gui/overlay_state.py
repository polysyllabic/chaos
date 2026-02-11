# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Shared overlay state payload for OBS endpoints."""

from __future__ import annotations

from typing import Any, Dict

import chaosface.config.globals as config

from .ui_helpers import clamp01


def overlay_state_payload() -> Dict[str, Any]:
  votes = list(config.relay.votes or [])
  candidate_mods = list(config.relay.candidate_mods or [])
  active_mods = list(config.relay.active_mods or [])
  mod_times = list(config.relay.mod_times or [])
  vote_total = sum(votes)
  return {
    'vote_time': clamp01(config.relay.vote_time),
    'vote_total': vote_total,
    'votes': votes,
    'candidate_mods': candidate_mods,
    'active_mods': active_mods,
    'mod_times': [clamp01(v) for v in mod_times],
    'num_active_mods': int(config.relay.num_active_mods),
    'connected': bool(config.relay.connected),
    'connected_bright': bool(config.relay.connected_bright),
    'paused': bool(config.relay.paused),
    'paused_bright': bool(config.relay.paused_bright),
  }
