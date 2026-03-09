"""Tests for configurable chatbot command aliases."""

from chaosface.chatbot.command_aliases import (
  resolve_chatbot_command,
  sanitize_alias_map,
  sanitize_alias_only_map,
)


def test_current_alias_defaults_are_present():
  aliases = sanitize_alias_map({})
  assert aliases['mods_voting'] == 'candidates'
  assert aliases['mods_active'] == 'active'
  assert aliases['givecredits'] == 'givecredit'
  assert aliases['raffle'] == 'chaosraffle'
  assert aliases['join'] == 'joinchaos'
  assert aliases['startvote'] == 'newvote'


def test_resolves_existing_default_aliases():
  aliases = sanitize_alias_map({})
  alias_only = sanitize_alias_only_map({})
  assert resolve_chatbot_command(['chaoscmd'], aliases, alias_only) == ('chaoscmd', [])
  assert resolve_chatbot_command(['votemethod', 'majority'], aliases, alias_only) == ('votemethod', ['majority'])
  assert resolve_chatbot_command(['votetime', '60'], aliases, alias_only) == ('votetime', ['60'])
  assert resolve_chatbot_command(['votecycle', 'interval'], aliases, alias_only) == ('votecycle', ['interval'])
  assert resolve_chatbot_command(['raffletime', '120'], aliases, alias_only) == ('raffletime', ['120'])
  assert resolve_chatbot_command(['permgroups'], aliases, alias_only) == ('permgroups', [])
  assert resolve_chatbot_command(['delgroup', 'mods'], aliases, alias_only) == ('delgroup', ['mods'])
  assert resolve_chatbot_command(['permgroup', 'mods'], aliases, alias_only) == ('permgroup', ['mods'])
  assert resolve_chatbot_command(['candidates'], aliases, alias_only) == ('mods_voting', [])
  assert resolve_chatbot_command(['active'], aliases, alias_only) == ('mods_active', [])
  assert resolve_chatbot_command(['givecredit'], aliases, alias_only) == ('givecredits', [])


def test_alias_only_blocks_default_when_alias_exists():
  aliases = sanitize_alias_map({'mods_voting': 'choices'})
  alias_only = sanitize_alias_only_map({'mods_voting': True})
  assert resolve_chatbot_command(['mods', 'voting'], aliases, alias_only) is None
  assert resolve_chatbot_command(['choices'], aliases, alias_only) == ('mods_voting', [])


def test_alias_only_ignored_when_alias_is_blank():
  aliases = sanitize_alias_map({'mods_voting': ''})
  alias_only = sanitize_alias_only_map({'mods_voting': True})
  assert resolve_chatbot_command(['mods', 'voting'], aliases, alias_only) == ('mods_voting', [])
