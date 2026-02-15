# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
Shared application relay used by the NiceGUI UI, model loop, and chatbot.

This module intentionally avoids deprecated widget dependencies and exposes plain Python setters/getters plus
field change listeners for components that need to react to updates.
"""

from __future__ import annotations

import asyncio
import copy
import json
import logging
import math
import queue
import threading
from copy import deepcopy
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Set

import numpy as np

from chaosface.chatbot.ChaosBot import ChaosBot

_chaos_description = (
  "Twitch Controls Chaos lets chat interfere with a streamer playing a "
  "PlayStation game. It uses a Raspberry Pi to modify controller input based on gameplay "
  "modifiers that chat votes on. For a video explanation of how Chaos works, see here: "
  "https://www.twitch.tv/blegas78/clip/SmellyDepressedPancakeKappaPride-llz6ldXSKjSJLs9s"
)

_chaos_how_to_vote = (
  "Each cycle you can vote for one modifier. Type the number of the mod you "
  "prefer in chat. Votes with text other than the number will be ignored. Votes are counted while "
  "the game is paused. "
)

_chaos_how_to_redeem = (
  "With a mod credit, apply a modifier of your choice immediately "
  "(subject to a {cooldown}-second cooldown) with '!apply <mod name>' (ignored if in cooldown)"
)


# We use these values when we can't read a value from the json file, typically because it hasn't
# been created yet.
chaos_defaults = {
  'current_game': 'NONE',
  'selected_game': 'NONE',
  'selected_game_history': [],
  'available_games': [],
  'active_modifiers': 3,
  'modifier_time': 180.0,
  'softmax_factor': 33,
  'vote_options': 3,
  'voting_type': 'Proportional',
  'voting_cycle': 'Continuous',
  'vote_time': 60.0,
  'vote_delay': 0.0,
  'announce_candidates': False,
  'announce_winner': False,
  'announce_active': False,
  'pi_host': '192.168.1.232',
  'listen_port': 5556,
  'talk_port': 5555,
  'obs_overlays': True,
  'use_gui': True,
  'ui_rate': 20.0,
  'ui_port': 80,
  'bits_redemptions': False,
  'bits_per_credit': 100,
  'multiple_credits': True,
  'points_redemptions': False,
  'points_reward_title': 'Chaos Credit',
  'raffles': False,
  'raffle_time': 60.0,
  'raffle_message_interval': 15.0,
  'raffle_cooldown': 600.0,
  'redemption_cooldown': 600.0,
  'client_id': 'uic681f91dtxl3pdfyqxld2yvr82r1',
  'nick': 'your_bot',
  'oauth': 'oauth:',
  'eventsub_oauth': 'oauth:',
  'owner': 'bot_owner',
  'channel': 'your_channel',
  'info_cmd_cooldown': 5.0,
  'credit_name': 'credits',
  'msg_chaos_description': _chaos_description,
  'msg_how_to_vote': _chaos_how_to_vote,
  'msg_proportional_voting': 'The chances of a mod winning are proportional to the number of votes it receives.',
  'msg_majority_voting': 'The mod with the most votes wins, and ties are broken by random selection.',
  'msg_authoritarian_voting': "Authoritarian voting mode is enabled. Chat votes are ignored and chaos picks a random modifier each cycle.",
  'msg_no_voting': 'Voting is currently disabled.',
  'msg_how_to_redeem': _chaos_how_to_redeem,
  'msg_credit_methods': 'Get modifier credits in these ways: ',
  'msg_by_gift': 'You can also give another user one of your credits with !givecredit <username>',
  'msg_no_redemptions': 'The ability to earn modifier credits is currently disabled.',
  'msg_with_points': 'redeeming channel points',
  'msg_with_bits': 'donating bits (1 credit for {} bits in a single donation)',
  'msg_multiple_credits': 'every {}',
  'msg_no_multiple_credits': '{} or more',
  'msg_by_raffle': 'winning raffles',
  'msg_mod_not_found': "I can't find the modifier '{}'.",
  'msg_mod_not_active': "The mod '{}' is currently disabled",
  'msg_active_mods': 'The currently active modifiers are ',
  'msg_candidate_mods': 'You can currently vote for the following modifiers: ',
  'msg_winning_mod': "The modifier '{}' has won the vote.",
  'msg_in_cooldown': 'Cannot apply the mod yet.',
  'msg_no_credits': 'You need a modifier credit to do that.',
  'msg_user_balance': '@{user} currently has {balance} modifier credit{plural}.',
  'msg_mod_applied': 'Modifier applied',
  'msg_mod_status': "The modifier '{}' is now {}",
  'msg_user_not_found': "The user '{}' doesn't appear to be in chat. If you're sure you've spelled the name right, you can give a credit to this user anyway with the command '!givecredit #{}'",
  'msg_credit_transfer': '@{} has given @{} 1 mod credit',
  'msg_mod_list': 'A list of the available mods for this game can be found here: {}',
  'mod_list_link': 'https://github.com/polysyllabic/chaos/blob/main/chaos/examples/tlou_mods.txt',
  'msg_raffle_open': 'A raffle for a modifier credit {status}. Type !join to enter.',
}

KNOWN_PERMISSIONS = {
  'admin',
  'manage_raffles',
  'manage_credits',
  'manage_modifiers',
  'manage_permissions',
  'manage_voting',
}

CONFIG_PATH = Path.cwd() / 'configs'
CHAOS_CONFIG_FILE = CONFIG_PATH / 'chaos_config.json'
CHAOS_MOD_FILE = CONFIG_PATH / 'chaos_mods.json'
CHAOS_CREDIT_FILE = CONFIG_PATH / 'chaos_credits.json'
CHAOS_PERMISSION_FILE = CONFIG_PATH / 'chaos_permissions.json'


class ChaosRelay:
  """Thread-safe state bridge shared by NiceGUI, model loop, and chatbot."""

  def __init__(self):
    self._lock = threading.RLock()
    self._listeners: Dict[str, List[Callable[[Any], None]]] = {}

    self.keep_going = True
    self.valid_data = False
    self.chatbot: Optional[ChaosBot] = None
    self.bot_reboot = False

    self.modifier_data: Dict[str, Dict[str, Any]] = {}
    self.enabled_mods: List[str] = []
    self.voted_users: List[str] = []
    self.active_keys: List[str] = []
    self.candidate_keys: List[str] = []
    self.force_mod: str = ''
    self.remove_mod_request: str = ''
    self.game_selection_request: str = ''
    self.reset_mods = False
    self.engine_commands = queue.Queue()
    self.insert_cooldown = 0.0
    self.vote_open = False
    self.start_vote_requested = False
    self.start_vote_duration = 0.0
    self.end_vote_requested = False
    self.raffle_open = False
    self.raffle_start_time = None
    self.user_credits: Dict[str, int] = {}
    self.permission_groups: Dict[str, Dict[str, List[str]]] = {}

    # UI/model state
    self.game_name = ''
    self.selected_game = ''
    self.selected_game_history: List[str] = []
    self.available_games: List[str] = []
    self.game_errors = 0
    self.vote_time = 0.0
    self.mod_times: List[float] = []
    self.votes: List[int] = []
    self.candidate_mods: List[str] = []
    self.active_mods: List[str] = []
    self.paused = True
    self.paused_bright = True
    self.connected = False
    self.connected_bright = True

    # Config-backed fields
    self.num_active_mods = 0
    self.time_per_modifier = 0.0
    self.softmax_factor = 0
    self.vote_options = 0
    self.voting_type = 'Proportional'
    self.voting_cycle = 'Continuous'
    self.vote_delay = 0.0
    self.bits_redemptions = False
    self.bits_per_credit = 0
    self.multiple_credits = True
    self.points_redemptions = False
    self.points_reward_title = ''
    self.raffles = False
    self.raffle_time = 0.0
    self.raffle_cooldown = 0
    self.raffle_message = ''
    self.redemption_cooldown = 0
    self.credit_name = ''

    self.channel_name = ''
    self.bot_name = ''
    self.bot_oauth = ''
    self.eventsub_oauth = ''

    self.pi_host = ''
    self.listen_port = 0
    self.talk_port = 0

    self.announce_candidates = False
    self.announce_active = False
    self.announce_winner = False
    self.obs_overlays = True
    self.overlay_font = ''
    self.overlay_font_size = 24.0
    self.overlay_font_color = '#ffffff'
    self.progress_bar_color = '#ffffff'

    self.ui_rate = 20.0
    self.ui_port = 80
    self.need_save = False
    self.new_game_data = False

    self.chaos_config = self.load_settings(CHAOS_CONFIG_FILE)
    migrated_legacy_oauth = self._migrate_legacy_eventsub_oauth_key()
    # Check that all default keys are present
    need_save = migrated_legacy_oauth
    for key, value in chaos_defaults.items():
      if key not in self.chaos_config:
        self.chaos_config[key] = value
        need_save = True
    if need_save:
      self.save_config_info()

    self.modifier_data = self.load_settings(CHAOS_MOD_FILE)
    self.user_credits = self.load_settings(CHAOS_CREDIT_FILE)
    self.permission_groups = self.load_settings(CHAOS_PERMISSION_FILE)
    if self._sanitize_permission_groups():
      self.save_permission_info()
    self.old_mod_data = deepcopy(self.modifier_data)
    self.reset_all()

  def on(self, field: str, callback: Callable[[Any], None]) -> None:
    with self._lock:
      self._listeners.setdefault(field, []).append(callback)

  def off(self, field: str, callback: Callable[[Any], None]) -> None:
    with self._lock:
      listeners = self._listeners.get(field, [])
      if callback in listeners:
        listeners.remove(callback)

  def _notify(self, field: str, value: Any) -> None:
    with self._lock:
      listeners = list(self._listeners.get(field, []))
    for callback in listeners:
      try:
        callback(value)
      except Exception:
        logging.exception('Relay listener failed for field %s', field)

  def _set_value(self, field: str, value: Any, config_key: Optional[str] = None) -> bool:
    with self._lock:
      old_value = getattr(self, field)
      changed = old_value != value
      setattr(self, field, value)
      if config_key is not None:
        self.chaos_config[config_key] = value
    if changed:
      self._notify(field, value)
    return changed

  def get_attribute(self, key):
    if key in self.chaos_config:
      return self.chaos_config[key]
    # Nothing found, return default setting if there is one
    if key in chaos_defaults:
      self.chaos_config[key] = chaos_defaults[key]
      return chaos_defaults[key]
    logging.error("No default for '%s'", key)
    return None

  def _attr_str(self, key: str, fallback: str = '') -> str:
    value = self.get_attribute(key)
    if value is None:
      return fallback
    if isinstance(value, str):
      return value
    return str(value)

  def _attr_float(self, key: str, fallback: float = 0.0) -> float:
    value = self.get_attribute(key)
    if isinstance(value, bool):
      return float(value)
    if isinstance(value, (int, float)):
      return float(value)
    if isinstance(value, str):
      try:
        return float(value)
      except ValueError:
        return fallback
    return fallback

  def _attr_int(self, key: str, fallback: int = 0) -> int:
    value = self.get_attribute(key)
    if isinstance(value, bool):
      return int(value)
    if isinstance(value, int):
      return value
    if isinstance(value, float):
      return int(value)
    if isinstance(value, str):
      try:
        return int(float(value))
      except ValueError:
        return fallback
    return fallback

  def _attr_bool(self, key: str, fallback: bool = False) -> bool:
    value = self.get_attribute(key)
    if value is None:
      return fallback
    return bool(value)

  def load_settings(self, configfile: Path):
    # Make sure the file and parent directories exist
    configfile.parents[0].mkdir(parents=True, exist_ok=True)
    configfile.touch(exist_ok=True)
    data = {}
    try:
      if configfile.stat().st_size > 0:
        logging.info('Reading settings from %s', configfile.resolve())
        data = json.loads(configfile.read_bytes())
    except Exception as exc:
      logging.warning('Error reading %s: %s', configfile.resolve(), str(exc))
    return data

  def _migrate_legacy_eventsub_oauth_key(self) -> bool:
    if not isinstance(self.chaos_config, dict):
      return False
    legacy_key = 'pub' + 'sub_oauth'
    if 'eventsub_oauth' in self.chaos_config:
      return self.chaos_config.pop(legacy_key, None) is not None
    if legacy_key in self.chaos_config:
      self.chaos_config['eventsub_oauth'] = str(self.chaos_config.get(legacy_key, 'oauth:'))
      self.chaos_config.pop(legacy_key, None)
      return True
    return False

  def save_config_info(self):
    logging.debug('Saving config file')
    with CHAOS_CONFIG_FILE.open('w') as outfile:
      json.dump(self.chaos_config, outfile, indent=2, sort_keys=True)

  def save_mod_info(self):
    with CHAOS_MOD_FILE.open('w') as modfile:
      json.dump(self.modifier_data, modfile, indent=2, sort_keys=True)

  def save_credit_info(self):
    with CHAOS_CREDIT_FILE.open('w') as creditfile:
      json.dump(self.user_credits, creditfile, indent=2, sort_keys=True)

  def save_permission_info(self):
    with CHAOS_PERMISSION_FILE.open('w') as permfile:
      json.dump(self.permission_groups, permfile, indent=2, sort_keys=True)

  @staticmethod
  def _normalize_user(user: str) -> str:
    return str(user).strip().lstrip('@').lower()

  @staticmethod
  def _normalize_group(group: str) -> str:
    return str(group).strip().lower()

  @staticmethod
  def _normalize_permission(permission: str) -> str:
    return str(permission).strip().lower()

  def _sanitize_permission_groups(self) -> bool:
    clean_groups: Dict[str, Dict[str, List[str]]] = {}
    source_groups = self.permission_groups if isinstance(self.permission_groups, dict) else {}
    for raw_group, raw_data in source_groups.items():
      group = self._normalize_group(raw_group)
      if not group:
        continue
      members_raw = []
      permissions_raw = []
      if isinstance(raw_data, dict):
        members_raw = raw_data.get('members', [])
        permissions_raw = raw_data.get('permissions', [])
      if not isinstance(members_raw, list):
        members_raw = []
      if not isinstance(permissions_raw, list):
        permissions_raw = []

      members = []
      for member in members_raw:
        user = self._normalize_user(member)
        if user:
          members.append(user)

      permissions = []
      for permission in permissions_raw:
        perm = self._normalize_permission(permission)
        if perm:
          permissions.append(perm)

      clean_groups[group] = {
        'members': sorted(set(members)),
        'permissions': sorted(set(permissions)),
      }

    changed = clean_groups != source_groups or not isinstance(self.permission_groups, dict)
    self.permission_groups = clean_groups
    return changed

  def _ensure_streamer_admin(self):
    streamer = self._normalize_user(self.channel_name)
    admin_group = self.permission_groups.setdefault('admin', {'members': [], 'permissions': []})
    members = admin_group.setdefault('members', [])
    permissions = admin_group.setdefault('permissions', [])
    changed = False

    if 'admin' not in permissions:
      permissions.append('admin')
      changed = True

    if streamer and streamer not in members:
      members.append(streamer)
      changed = True

    if changed:
      admin_group['members'] = sorted(set(members))
      admin_group['permissions'] = sorted(set(permissions))
      self.save_permission_info()

  def reset_all(self):
    self.set_game_name(self.get_attribute('current_game'))
    selected_game = self.get_attribute('selected_game') or self.get_attribute('current_game')
    self.set_selected_game(selected_game)
    self.set_selected_game_history(self.get_attribute('selected_game_history'))
    self.set_available_games(self.get_attribute('available_games'))
    self.set_num_active_mods(self.get_attribute('active_modifiers'))
    self.set_time_per_modifier(self.get_attribute('modifier_time'))
    self.set_softmax_factor(self.get_attribute('softmax_factor'))
    self.set_vote_options(self.get_attribute('vote_options'))
    self.set_voting_type(self.get_attribute('voting_type'))
    self.set_voting_cycle(self.get_attribute('voting_cycle'))
    self.set_announce_candidates(self.get_attribute('announce_candidates'))
    self.set_announce_active(self.get_attribute('announce_active'))
    self.set_announce_winner(self.get_attribute('announce_winner'))
    self.set_pi_host(self.get_attribute('pi_host'))
    self.set_listen_port(self.get_attribute('listen_port'))
    self.set_talk_port(self.get_attribute('talk_port'))
    self.set_bot_name(self.get_attribute('nick'))
    self.set_bot_oauth(self.get_attribute('oauth'))
    self.set_eventsub_oauth(self.get_attribute('eventsub_oauth'))
    self.set_channel_name(self.get_attribute('channel'))
    self.set_bits_redemptions(self.get_attribute('bits_redemptions'))
    self.set_bits_per_credit(self.get_attribute('bits_per_credit'))
    self.set_multiple_credits(self.get_attribute('multiple_credits'))
    self.set_points_redemptions(self.get_attribute('points_redemptions'))
    self.set_points_reward_title(self.get_attribute('points_reward_title'))
    self.set_raffles(self.get_attribute('raffles'))
    self.set_raffle_time(self.get_attribute('raffle_time'))
    self.set_redemption_cooldown(self.get_attribute('redemption_cooldown'))
    self.set_ui_rate(self.get_attribute('ui_rate'))
    self.set_ui_port(self.get_attribute('ui_port'))

    self.set_game_errors(0)
    self.set_vote_time(0.5)
    self.set_vote_open(False)
    self.set_paused(True)
    self.set_paused_bright(True)
    self.set_connected(False)
    self.set_connected_bright(True)
    self.set_mod_times([0.0] * self.num_active_mods)
    self.set_active_mods([''] * self.num_active_mods)
    self.set_votes([0] * self.vote_options)
    self.set_candidate_mods([''] * self.vote_options)

    self.active_keys = [''] * self.num_active_mods
    self.candidate_keys = [''] * self.vote_options
    self.start_vote_requested = False
    self.start_vote_duration = 0.0
    self.end_vote_requested = False
    self.remove_mod_request = ''
    self.game_selection_request = ''

  def time_per_vote(self) -> float:
    active_modifiers = self._attr_float('active_modifiers', 1.0)
    if active_modifiers <= 0:
      return 1.0
    return self._attr_float('modifier_time', 180.0) / active_modifiers

  def initialize_game(self, gamedata: dict):
    logging.info("Setting game data for %s", gamedata['game'])
    self.set_game_name(gamedata['game'])
    self.record_selected_game(gamedata['game'])
    self.set_game_errors(int(gamedata['errors']))
    self.set_num_active_mods(int(gamedata['nmods']))
    self.set_time_per_modifier(float(gamedata['modtime']))

    logging.debug('Initializing modifier data:')
    self.modifier_data = {}
    self.enabled_mods = []
    for mod in gamedata['mods']:
      mod_key = mod['name'].lower()
      self.modifier_data[mod_key] = {}
      self.modifier_data[mod_key]['name'] = mod['name']
      self.modifier_data[mod_key]['desc'] = mod['desc']
      self.modifier_data[mod_key]['groups'] = mod['groups']
      if mod_key in self.old_mod_data:
        self.modifier_data[mod_key]['active'] = self.old_mod_data[mod_key]['active']
      else:
        self.modifier_data[mod_key]['active'] = True
      if self.modifier_data[mod_key]['active']:
        self.enabled_mods.append(mod_key)

    self.reset_softmax()
    self.save_mod_info()
    self.set_new_game_data(True)
    self.valid_data = True

  def sleep_time(self):
    rate = self._attr_float('ui_rate', 20.0)
    if rate <= 0.0:
      rate = 20.0
    return 1.0 / rate

  # Explicit setters used by UI/model/chatbot
  def set_game_name(self, value):
    self._set_value('game_name', str(value), 'current_game')

  def set_selected_game(self, value):
    game_name = str(value).strip() if value is not None else ''
    if not game_name:
      game_name = 'NONE'
    self._set_value('selected_game', game_name, 'selected_game')

  def set_selected_game_history(self, value):
    games: List[str] = []
    if isinstance(value, list):
      for game in value:
        game_name = str(game).strip()
        if game_name:
          games.append(game_name)
    self._set_value('selected_game_history', games, 'selected_game_history')

  # Backward-compatible alias for legacy call sites.
  def set_selected_games(self, value):
    self.set_selected_game_history(value)

  def set_available_games(self, value):
    unique_games = set()
    games: List[str] = []
    if isinstance(value, list):
      for game in value:
        game_name = str(game).strip()
        if game_name and game_name not in unique_games:
          unique_games.add(game_name)
          games.append(game_name)
    games.sort(key=lambda name: name.lower())
    self._set_value('available_games', games, 'available_games')

  def record_selected_game(self, game_name: str):
    name = str(game_name).strip()
    if not name:
      return
    self.set_selected_game(name)
    history = list(self.selected_game_history)
    if not history or history[-1] != name:
      history.append(name)
      self.set_selected_game_history(history)
    self.save_config_info()

  def get_preferred_game(self, available_games: Optional[List[str]] = None) -> str:
    games = list(available_games) if available_games is not None else list(self.available_games)
    preferred = str(self.selected_game).strip()
    if preferred.upper() == 'NONE':
      preferred = ''
    if preferred and preferred in games:
      return preferred
    return ''

  def set_game_errors(self, value):
    self._set_value('game_errors', int(value))

  def set_vote_time(self, value):
    self._set_value('vote_time', float(value))

  def set_vote_open(self, value):
    self._set_value('vote_open', bool(value))

  def set_mod_times(self, value):
    self._set_value('mod_times', [float(v) for v in value])

  def set_votes(self, value):
    self._set_value('votes', [int(v) for v in value])

  def set_candidate_mods(self, value):
    self._set_value('candidate_mods', [str(v) for v in value])

  def set_active_mods(self, value):
    self._set_value('active_mods', [str(v) for v in value])

  def set_paused(self, value):
    self._set_value('paused', bool(value))

  def set_paused_bright(self, value):
    self._set_value('paused_bright', bool(value))

  def set_connected(self, value):
    self._set_value('connected', bool(value))

  def set_connected_bright(self, value):
    self._set_value('connected_bright', bool(value))

  def set_num_active_mods(self, value):
    value = max(1, int(value))
    self._set_value('num_active_mods', value, 'active_modifiers')

  def set_time_per_modifier(self, value):
    value = max(1.0, float(value))
    self._set_value('time_per_modifier', value, 'modifier_time')

  def set_softmax_factor(self, value):
    value = max(1, int(value))
    self._set_value('softmax_factor', value, 'softmax_factor')

  def set_vote_options(self, value):
    value = max(2, int(value))
    changed = self._set_value('vote_options', value, 'vote_options')
    if changed:
      self.reset_voting()

  def set_voting_type(self, value):
    value = str(value)
    if value not in ('Proportional', 'Majority', 'Authoritarian', 'DISABLED'):
      value = 'Proportional'
    self._set_value('voting_type', value, 'voting_type')

  def set_voting_cycle(self, value):
    value = str(value)
    if value not in ('Continuous', 'Interval', 'Random', 'Triggered', 'DISABLED'):
      value = 'Continuous'
    self._set_value('voting_cycle', value, 'voting_cycle')

  def set_bits_redemptions(self, value):
    self._set_value('bits_redemptions', bool(value), 'bits_redemptions')

  def set_bits_per_credit(self, value):
    self._set_value('bits_per_credit', max(1, int(value)), 'bits_per_credit')

  def set_multiple_credits(self, value):
    self._set_value('multiple_credits', bool(value), 'multiple_credits')

  def set_points_redemptions(self, value):
    self._set_value('points_redemptions', bool(value), 'points_redemptions')

  def set_points_reward_title(self, value):
    self._set_value('points_reward_title', str(value), 'points_reward_title')

  def set_raffles(self, value):
    self._set_value('raffles', bool(value), 'raffles')

  def set_raffle_time(self, value):
    self._set_value('raffle_time', max(1.0, float(value)), 'raffle_time')

  def set_redemption_cooldown(self, value):
    self._set_value('redemption_cooldown', max(0, int(value)), 'redemption_cooldown')

  def set_channel_name(self, value):
    self._set_value('channel_name', str(value), 'channel')
    self._ensure_streamer_admin()

  def set_bot_name(self, value):
    self._set_value('bot_name', str(value), 'nick')

  def set_bot_oauth(self, value):
    self._set_value('bot_oauth', str(value), 'oauth')

  def set_eventsub_oauth(self, value):
    self._set_value('eventsub_oauth', str(value), 'eventsub_oauth')

  def set_pi_host(self, value):
    self._set_value('pi_host', str(value), 'pi_host')

  def set_listen_port(self, value):
    self._set_value('listen_port', int(value), 'listen_port')

  def set_talk_port(self, value):
    self._set_value('talk_port', int(value), 'talk_port')

  def set_announce_candidates(self, value):
    self._set_value('announce_candidates', bool(value), 'announce_candidates')

  def set_announce_active(self, value):
    self._set_value('announce_active', bool(value), 'announce_active')

  def set_announce_winner(self, value):
    self._set_value('announce_winner', bool(value), 'announce_winner')

  def set_obs_overlays(self, value):
    self._set_value('obs_overlays', bool(value), 'obs_overlays')

  def set_ui_rate(self, value):
    self._set_value('ui_rate', max(1.0, float(value)), 'ui_rate')

  def set_ui_port(self, value):
    self._set_value('ui_port', int(value), 'ui_port')

  def set_need_save(self, value):
    value = bool(value)
    self._set_value('need_save', value)
    if value:
      logging.info('Saving settings to %s', CHAOS_CONFIG_FILE.resolve())
      self.save_config_info()
      self._set_value('need_save', False)

  def set_new_game_data(self, value):
    self._set_value('new_game_data', bool(value))

  # Legacy aliases retained for compatibility with older call sites
  def on_channel_name(self, value):
    self.set_channel_name(value)

  def on_bot_name(self, value):
    self.set_bot_name(value)

  def on_bot_oauth(self, value):
    self.set_bot_oauth(value)

  def on_eventsub_oauth(self, value):
    self.set_eventsub_oauth(value)

  def on_pi_host(self, value):
    self.set_pi_host(value)

  def change_listen_port(self, value):
    self.set_listen_port(value)

  def change_talk_port(self, value):
    self.set_talk_port(value)

  def set_shouldSave(self, value):
    self.set_need_save(value)

  def mod_enabled(self, mod: str):
    """Returns a modifier's enabled status: (True/False) or None if not found."""
    key = mod.lower()
    if key in self.modifier_data:
      return self.modifier_data[key]['active']
    return None

  def set_mod_enabled(self, mod: str, enabled: bool) -> str:
    """
    Sets the modifier's enabled status (True/False). Returns a message intended to be sent to chat
    explaining the result.
    """
    key = mod.lower()
    status = self.mod_enabled(mod)
    if status is None:
      ack: str = self._attr_str('msg_mod_not_found').format(mod)
    else:
      self.modifier_data[key]['active'] = bool(enabled)
      self.enabled_mods = [
        mod_key for mod_key, mod_data in self.modifier_data.items() if bool(mod_data.get('active', True))
      ]
      self.save_mod_info()
      status = 'active' if enabled else 'inactive'
      ack = self._attr_str('msg_mod_status').format(mod, status)
    return ack

  def get_mod_description(self, mod: str):
    """Get a description of the modifier."""
    key = mod.lower()
    try:
      return self.modifier_data[key]['desc']
    except Exception:
      return self._attr_str('msg_mod_not_found').format(mod)

  def get_balance_message(self, user: str):
    balance = self.get_balance(user)
    plural = '' if balance == 1 else 's'
    msg: str = self._attr_str('msg_user_balance')
    return msg.format(user=user, balance=balance, plural=plural)

  def get_balance(self, user: str):
    return int(self.user_credits.get(user, 0))

  def get_user_permissions(self, user: str) -> Set[str]:
    user_name = self._normalize_user(user)
    if not user_name:
      return set()

    perms: Set[str] = set()
    for group_data in self.permission_groups.values():
      members = group_data.get('members', [])
      permissions = group_data.get('permissions', [])
      if user_name in members:
        for permission in permissions:
          perm = self._normalize_permission(permission)
          if perm:
            perms.add(perm)

    if user_name == self._normalize_user(self.channel_name):
      perms.add('admin')

    return perms

  def has_permission(self, user: str, permission: str) -> bool:
    required = self._normalize_permission(permission)
    if not required:
      return True
    perms = self.get_user_permissions(user)
    return 'admin' in perms or required in perms

  def request_start_vote(self, duration: Optional[float] = None):
    with self._lock:
      self.start_vote_duration = 0.0 if duration is None else max(1.0, float(duration))
      self.start_vote_requested = True

  def consume_start_vote_request(self) -> Optional[float]:
    with self._lock:
      if not self.start_vote_requested:
        return None
      duration = float(self.start_vote_duration)
      self.start_vote_requested = False
      self.start_vote_duration = 0.0
    return duration

  def request_end_vote(self):
    with self._lock:
      self.end_vote_requested = True

  def consume_end_vote_request(self) -> bool:
    with self._lock:
      requested = bool(self.end_vote_requested)
      self.end_vote_requested = False
    return requested

  def request_game_selection(self, game_name: str):
    selected = str(game_name).strip()
    if not selected:
      return
    with self._lock:
      self.game_selection_request = selected
    self.record_selected_game(selected)

  def consume_game_selection_request(self) -> str:
    with self._lock:
      request = self.game_selection_request
      self.game_selection_request = ''
    return request

  def request_remove_mod(self, mod_key: str):
    with self._lock:
      self.remove_mod_request = str(mod_key).strip().lower()

  def consume_remove_mod_request(self) -> str:
    with self._lock:
      request = self.remove_mod_request
      self.remove_mod_request = ''
    return request

  def add_permission_group(self, group: str) -> str:
    group_name = self._normalize_group(group)
    if not group_name:
      return 'Usage: !addgroup <group>'
    if group_name in self.permission_groups:
      return f"The group '{group_name}' already exists."

    self.permission_groups[group_name] = {'members': [], 'permissions': []}
    self.save_permission_info()
    return f"Added permission group '{group_name}'."

  def add_group_member(self, group: str, user: str) -> str:
    group_name = self._normalize_group(group)
    user_name = self._normalize_user(user)
    if not group_name or not user_name:
      return 'Usage: !addmember <group> <user>'
    if group_name not in self.permission_groups:
      return f"The group '{group_name}' does not exist."

    members = self.permission_groups[group_name].setdefault('members', [])
    if user_name in members:
      return f"@{user_name} is already in group '{group_name}'."

    members.append(user_name)
    self.permission_groups[group_name]['members'] = sorted(set(members))
    self.save_permission_info()
    return f"Added @{user_name} to group '{group_name}'."

  def add_group_permission(self, group: str, permission: str) -> str:
    group_name = self._normalize_group(group)
    permission_name = self._normalize_permission(permission)
    if not group_name or not permission_name:
      return 'Usage: !addperm <group> <permission>'
    if group_name not in self.permission_groups:
      return f"The group '{group_name}' does not exist."
    if permission_name not in KNOWN_PERMISSIONS:
      valid_perms = ', '.join(sorted(KNOWN_PERMISSIONS))
      return f"Unknown permission '{permission_name}'. Valid permissions: {valid_perms}"

    permissions = self.permission_groups[group_name].setdefault('permissions', [])
    if permission_name in permissions:
      return f"The group '{group_name}' already has '{permission_name}'."

    permissions.append(permission_name)
    self.permission_groups[group_name]['permissions'] = sorted(set(permissions))
    self.save_permission_info()
    return f"Added permission '{permission_name}' to group '{group_name}'."

  def remove_group_member(self, group: str, user: str) -> str:
    group_name = self._normalize_group(group)
    user_name = self._normalize_user(user)
    if not group_name or not user_name:
      return 'Usage: !delmember <group> <user>'
    if group_name not in self.permission_groups:
      return f"The group '{group_name}' does not exist."

    members = self.permission_groups[group_name].setdefault('members', [])
    if user_name not in members:
      return f"@{user_name} is not in group '{group_name}'."

    members.remove(user_name)
    self.permission_groups[group_name]['members'] = sorted(set(members))
    self.save_permission_info()
    return f"Removed @{user_name} from group '{group_name}'."

  def remove_group_permission(self, group: str, permission: str) -> str:
    group_name = self._normalize_group(group)
    permission_name = self._normalize_permission(permission)
    if not group_name or not permission_name:
      return 'Usage: !delperm <group> <permission>'
    if group_name not in self.permission_groups:
      return f"The group '{group_name}' does not exist."

    permissions = self.permission_groups[group_name].setdefault('permissions', [])
    if permission_name not in permissions:
      return f"The group '{group_name}' does not have '{permission_name}'."

    permissions.remove(permission_name)
    self.permission_groups[group_name]['permissions'] = sorted(set(permissions))
    self.save_permission_info()
    return f"Removed permission '{permission_name}' from group '{group_name}'."

  def set_balance(self, user: str, balance: int):
    if balance < 0:
      balance = 0
    self.user_credits[user] = int(balance)
    self.save_credit_info()
    return int(balance)

  def step_balance(self, user: str, delta: int):
    balance = self.get_balance(user) + int(delta)
    if balance < 0:
      balance = 0
    self.user_credits[user] = int(balance)
    self.save_credit_info()
    return int(balance)

  def tally_vote(self, index: int, user: str):
    if 0 <= index < self.vote_options:
      if user not in self.voted_users:
        self.voted_users.append(user)
        votes_counted = list(self.votes)
        while len(votes_counted) < self.vote_options:
          votes_counted.append(0)
        votes_counted[index] += 1
        self.set_votes(votes_counted)
        logging.debug('User %s voted for choice %s, which now has %s votes', user, index + 1, votes_counted[index])
      else:
        logging.debug('User %s has already voted', user)
    else:
      logging.debug('Received an out of range vote (%s)', index + 1)

  def decrement_mod_times(self, d_time):
    time_remaining = list(self.mod_times)
    divisor = self.time_per_modifier if self.time_per_modifier > 0 else 1.0
    delta = float(d_time) / divisor
    time_remaining[:] = [t - delta for t in time_remaining]
    self.set_mod_times(time_remaining)

  def reset_softmax(self):
    logging.info('Resetting SoftMax!')
    for mod in self.modifier_data:
      self.modifier_data[mod]['count'] = 0

  def get_softmax_divisor(self, data):
    # determine the sum for the softmax divisor
    divisor = 0
    for key in data:
      divisor += data[key]['contribution']
    return divisor

  def update_softmax_probabilities(self, data):
    for mod in data:
      data[mod]['contribution'] = math.exp(
        self.modifier_data[mod]['count'] * math.log(float(self.softmax_factor) / 100.0)
      )
    divisor = self.get_softmax_divisor(data)
    if divisor == 0:
      return
    for mod in data:
      data[mod]['p'] = data[mod]['contribution'] / divisor

  def update_softmax(self, new_mod):
    if new_mod in self.modifier_data:
      self.modifier_data[new_mod]['count'] += 1
      # update all probabilities
      self.update_softmax_probabilities(self.modifier_data)
    else:
      logging.error("Updating SoftMax: modifier '%s' not in mod list", new_mod)

  def pick_from_list(self, mod_list):
    """Randomly pick a mod from the list provided."""
    mods = list(mod_list)
    if not mods:
      return None

    votable_tracker = {}
    for mod in mods:
      votable_tracker[mod] = copy.deepcopy(self.modifier_data[mod])

    # Calculate softmax probabilities each time
    self.update_softmax_probabilities(votable_tracker)

    # make a decision
    choice_threshold = np.random.uniform(0, 1)
    selection_tracker = 0
    for mod in votable_tracker:
      selection_tracker += votable_tracker[mod]['p']
      if selection_tracker > choice_threshold:
        return mod

    return mods[-1]

  def get_random_mod(self):
    """Randomly pick a mod from the enabled but currently unused mods."""
    inactive_mods = set(np.setdiff1d(self.enabled_mods, list(self.active_keys)))
    mod = self.pick_from_list(inactive_mods)
    if mod is not None:
      return mod
    return self.pick_from_list(self.enabled_mods)

  def get_new_voting_pool(self):
    """Refill the voting pool with random selections."""
    inactive_mods = set(np.setdiff1d(self.enabled_mods, list(self.active_keys)))

    self.candidate_keys = []
    candidates = []
    for _ in range(self.vote_options):
      mod = self.pick_from_list(inactive_mods)
      if mod is None:
        break
      self.candidate_keys.append(mod)
      candidates.append(self.modifier_data[mod]['name'])
      inactive_mods.remove(mod)  # prevent repeats

    if logging.getLogger().isEnabledFor(logging.DEBUG):
      logging.debug('New Voting Round:')
      for mod in self.candidate_keys:
        if 'p' in self.modifier_data[mod]:
          logging.debug(' - %0.2f%% %s', self.modifier_data[mod]['p'] * 100.0, mod)
        else:
          logging.debug(' - 0.00%% %s', mod)

    self.set_candidate_mods(candidates)
    # Reset votes and user-voted list, since there is a new voting pool
    self.set_votes([0] * self.vote_options)
    self.voted_users = []

    # Announce the new voting pool in chat
    if self.announce_candidates:
      msg = self._attr_str('msg_candidate_mods')
      for num, mod in enumerate(candidates, start=1):
        msg += f' {num}: {mod}.'
      if msg:
        self.send_chat_message(msg)

  def send_chat_message(self, msg: str):
    """Schedule async bot send_message inside bot event loop."""
    if self.chatbot:
      loop = self.chatbot._get_event_loop()
      if loop and loop.is_running():
        loop.call_soon_threadsafe(asyncio.create_task, self.chatbot.send_message(msg))

  def about_tcc(self):
    return self._attr_str('msg_chaos_description')

  def explain_voting(self):
    if self._attr_str('voting_type') == 'DISABLED' or self._attr_str('voting_cycle') == 'DISABLED':
      return self._attr_str('msg_no_voting')
    voting_type = self._attr_str('voting_type')
    if voting_type == 'Proportional':
      msg = self._attr_str('msg_proportional_voting')
      return self._attr_str('msg_how_to_vote') + msg
    if voting_type == 'Majority':
      msg = self._attr_str('msg_majority_voting')
      return self._attr_str('msg_how_to_vote') + msg
    if voting_type == 'Authoritarian':
      return self._attr_str('msg_authoritarian_voting')
    else:
      msg = self._attr_str('msg_majority_voting')
      return self._attr_str('msg_how_to_vote') + msg

  def explain_redemption(self):
    msg = self._attr_str('msg_how_to_redeem')
    return msg.format(cooldown=self._attr_int('redemption_cooldown', 600))

  def explain_credits(self):
    msg = ''
    if self.points_redemptions:
      msg = self._attr_str('msg_with_points')
    if self.bits_redemptions:
      if msg:
        msg = msg + '; '
      if self._attr_bool('multiple_credits', True):
        ratio = self._attr_str('msg_multiple_credits')
      else:
        ratio = self._attr_str('msg_no_multiple_credits')
      ratio = ratio.format(self._attr_int('bits_per_credit', 100))
      msg = msg + self._attr_str('msg_with_bits').format(ratio)
    if self.raffles:
      if msg:
        msg = msg + '; '
      msg = msg + self._attr_str('msg_by_raffle')
    if not msg:
      return self._attr_str('msg_no_redemptions')
    return self._attr_str('msg_credit_methods') + msg

  def list_winning_mod(self, winner):
    if winner:
      mod_name = self.modifier_data[winner]['name']
      return self._attr_str('msg_winning_mod').format(mod_name)
    return ''

  def list_active_mods(self):
    if not self.active_mods or not self.active_mods[0]:
      return ''
    return self._attr_str('msg_active_mods') + ', '.join(filter(None, self.active_mods))

  def list_candidate_mods(self):
    msg = self._attr_str('msg_candidate_mods')
    for num, mod in enumerate(self.candidate_mods, start=1):
      msg += f' {num}: {mod}.'
    return msg

  def enable_mod(self, mod_key, value):
    if mod_key in self.modifier_data:
      self.modifier_data[mod_key]['active'] = bool(value)
      self.enabled_mods = [
        key for key, data in self.modifier_data.items() if bool(data.get('active', True))
      ]

  def replace_mod(self, mod_key):
    if not mod_key or mod_key not in self.modifier_data:
      return

    self.update_softmax(mod_key)
    mods = list(self.active_mods)
    times = list(self.mod_times)
    if not times:
      times = [0.0] * self.num_active_mods
    if len(mods) < len(times):
      mods.extend([''] * (len(times) - len(mods)))

    # Find the oldest mod
    finished_mod = times.index(min(times))
    mods[finished_mod] = self.modifier_data[mod_key]['name']
    times[finished_mod] = 1.0
    self.set_mod_times(times)
    self.set_active_mods(mods)

    # Announce what's now active after replacement
    if self.announce_active:
      msg = self.list_active_mods()
      if msg:
        self.send_chat_message(msg)

  def remove_active_mod(self, mod_name: str):
    target = str(mod_name).strip().lower()
    if not target:
      return
    mods = list(self.active_mods)
    times = list(self.mod_times)
    removed = False
    for idx, name in enumerate(mods):
      if str(name).strip().lower() == target:
        mods[idx] = ''
        if idx < len(times):
          times[idx] = 0.0
        removed = True
        break
    if removed:
      self.set_active_mods(mods)
      if times:
        self.set_mod_times(times)

  def reset_current_mods(self):
    self.set_mod_times([0.0] * self.num_active_mods)
    self.set_active_mods([''] * self.num_active_mods)

  def reset_voting(self):
    self.set_votes([0] * self.vote_options)
    self.set_candidate_mods([''] * self.vote_options)
    self.voted_users = []
