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
    'engine_status': str(config.relay.engine_status or 'not_connected'),
    'paused': bool(config.relay.paused),
    'paused_bright': bool(config.relay.paused_bright),
    'overlay_current_votes_gap': int(config.relay.overlay_current_votes_gap),
    'overlay_active_mods_gap': int(config.relay.overlay_active_mods_gap),
    'overlay_current_votes_text_color': str(config.relay.overlay_current_votes_text_color or '#ffffff'),
    'overlay_current_votes_bar_color': str(config.relay.overlay_current_votes_bar_color or 'rgba(245, 245, 245, 0.8)'),
    'overlay_current_votes_text_side': str(getattr(config.relay, 'overlay_current_votes_text_side', 'right') or 'right'),
    'overlay_current_votes_text_align': str(getattr(config.relay, 'overlay_current_votes_text_align', 'left') or 'left'),
    'overlay_active_mods_text_color': str(config.relay.overlay_active_mods_text_color or '#ffffff'),
    'overlay_active_mods_bar_color': str(config.relay.overlay_active_mods_bar_color or 'rgba(245, 245, 245, 0.75)'),
    'overlay_active_mods_text_side': str(getattr(config.relay, 'overlay_active_mods_text_side', 'right') or 'right'),
    'overlay_active_mods_text_align': str(getattr(config.relay, 'overlay_active_mods_text_align', 'left') or 'left'),
    'overlay_vote_timer_bar_color': str(config.relay.overlay_vote_timer_bar_color or 'rgba(240, 240, 240, 0.85)'),
  }
