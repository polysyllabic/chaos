# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Shared chatbot command metadata and alias resolution helpers."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


@dataclass(frozen=True)
class ChatbotCommandSpec:
  key: str
  label: str
  default_parts: Tuple[str, ...]
  default_alias: str = ''

  @property
  def default_command(self) -> str:
    return ' '.join(self.default_parts)

  @property
  def default_chat_syntax(self) -> str:
    return '!' + self.default_command


CHATBOT_COMMANDS: List[ChatbotCommandSpec] = [
  ChatbotCommandSpec('chaos', 'About Chaos', ('chaos',)),
  ChatbotCommandSpec('chaoscmd', 'Command List Link', ('chaoscmd',)),
  ChatbotCommandSpec('mod', 'Modifier Description', ('mod',)),
  ChatbotCommandSpec('mods', 'Modifier List Link', ('mods',)),
  ChatbotCommandSpec('mods_voting', 'List Candidate Mods', ('mods', 'voting'), default_alias='candidates'),
  ChatbotCommandSpec('mods_active', 'List Active Mods', ('mods', 'active'), default_alias='active'),
  ChatbotCommandSpec('credits', 'Show Credits', ('credits',)),
  ChatbotCommandSpec('addcredits', 'Add Credits', ('addcredits',)),
  ChatbotCommandSpec('setcredits', 'Set Credits', ('setcredits',)),
  ChatbotCommandSpec('givecredits', 'Give Credits', ('givecredits',), default_alias='givecredit'),
  ChatbotCommandSpec('apply', 'Apply Modifier', ('apply',)),
  ChatbotCommandSpec('enable', 'Enable Modifier', ('enable',)),
  ChatbotCommandSpec('disable', 'Disable Modifier', ('disable',)),
  ChatbotCommandSpec('resetmods', 'Reset Modifiers', ('resetmods',)),
  ChatbotCommandSpec('raffle', 'Start Raffle', ('raffle',), default_alias='chaosraffle'),
  ChatbotCommandSpec('raffletime', 'Set Raffle Time', ('raffletime',)),
  ChatbotCommandSpec('join', 'Join Raffle', ('join',), default_alias='joinchaos'),
  ChatbotCommandSpec('startvote', 'Start Vote', ('startvote',), default_alias='newvote'),
  ChatbotCommandSpec('endvote', 'End Vote', ('endvote',)),
  ChatbotCommandSpec('votemethod', 'Set Vote Method', ('votemethod',)),
  ChatbotCommandSpec('votetime', 'Set Vote Time', ('votetime',)),
  ChatbotCommandSpec('votecycle', 'Set Vote Cycle', ('votecycle',)),
  ChatbotCommandSpec('remove', 'Remove Modifier', ('remove',)),
  ChatbotCommandSpec('addgroup', 'Add Permission Group', ('addgroup',)),
  ChatbotCommandSpec('addmember', 'Add Group Member', ('addmember',)),
  ChatbotCommandSpec('addperm', 'Add Group Permission', ('addperm',)),
  ChatbotCommandSpec('delgroup', 'Delete Permission Group', ('delgroup',)),
  ChatbotCommandSpec('delmember', 'Remove Group Member', ('delmember',)),
  ChatbotCommandSpec('delperm', 'Remove Group Permission', ('delperm',)),
  ChatbotCommandSpec('permgroups', 'List Permission Groups', ('permgroups',)),
  ChatbotCommandSpec('permgroup', 'Show Permission Group', ('permgroup',)),
]

CHATBOT_COMMANDS_BY_KEY: Dict[str, ChatbotCommandSpec] = {spec.key: spec for spec in CHATBOT_COMMANDS}

DEFAULT_COMMAND_ALIASES: Dict[str, str] = {spec.key: spec.default_alias for spec in CHATBOT_COMMANDS}
DEFAULT_ALIAS_ONLY: Dict[str, bool] = {spec.key: False for spec in CHATBOT_COMMANDS}

_COMMANDS_SORTED_BY_DEFAULT_LENGTH = sorted(
  CHATBOT_COMMANDS,
  key=lambda spec: len(spec.default_parts),
  reverse=True,
)


def normalize_alias(value: str) -> str:
  alias = str(value or '').strip().lower().lstrip('!')
  if not alias or any(ch.isspace() for ch in alias):
    return ''
  return alias


def sanitize_alias_map(value) -> Dict[str, str]:
  source = value if isinstance(value, dict) else {}
  cleaned: Dict[str, str] = {}
  for spec in CHATBOT_COMMANDS:
    raw_alias = source.get(spec.key, spec.default_alias)
    cleaned[spec.key] = normalize_alias(raw_alias)
  return cleaned


def sanitize_alias_only_map(value) -> Dict[str, bool]:
  source = value if isinstance(value, dict) else {}
  cleaned: Dict[str, bool] = {}
  for spec in CHATBOT_COMMANDS:
    cleaned[spec.key] = bool(source.get(spec.key, False))
  return cleaned


def resolve_chatbot_command(
  parts: List[str],
  aliases: Dict[str, str],
  alias_only: Dict[str, bool],
) -> Optional[Tuple[str, List[str]]]:
  """Resolve parsed message parts to a command key and remaining args."""
  if not parts:
    return None

  original = [str(part).strip() for part in parts if str(part).strip()]
  if not original:
    return None
  words = [part.lower() for part in original]

  clean_aliases = sanitize_alias_map(aliases)
  clean_alias_only = sanitize_alias_only_map(alias_only)

  first = words[0]
  for spec in CHATBOT_COMMANDS:
    alias = clean_aliases.get(spec.key, '')
    if alias and first == alias:
      return spec.key, original[1:]

  for spec in _COMMANDS_SORTED_BY_DEFAULT_LENGTH:
    expected = list(spec.default_parts)
    size = len(expected)
    if words[:size] != expected:
      continue
    alias = clean_aliases.get(spec.key, '')
    alias_only_mode = clean_alias_only.get(spec.key, False)
    if alias_only_mode and alias:
      return None
    return spec.key, original[size:]

  return None
