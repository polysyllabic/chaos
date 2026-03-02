"""Command-level permission checks for ChaosBot."""

import asyncio
from unittest.mock import patch

from chaosface.chatbot.ChaosBot import ChaosBot
from chaosface.chatbot.context import RelayBotContext
from chaosface.gui.ChaosRelay import ChaosRelay


class _StubContext:
  def __init__(self):
    self.user_permissions = {}
    self.removed_mod = ''
    self.attributes = {
      'chaoscmd_link': '',
      'info_cmd_cooldown': 10.0,
      'msg_mod_list': 'A list of the available mods for this game can be found here: {}',
      'mod_list_link': 'https://example.com/mods',
    }
    self.permission_groups = {
      'admin': {'permissions': ['admin'], 'members': ['streamer']},
      'mods': {'permissions': ['manage_modifiers'], 'members': ['alice', 'bob']},
    }

  def has_permission(self, user: str, permission: str) -> bool:
    return permission in self.user_permissions.get(user, set())

  def mod_enabled(self, mod: str):
    if mod.lower() == 'mod a':
      return True
    return None

  def request_remove_mod(self, mod_key: str) -> None:
    self.removed_mod = mod_key

  def get_attribute(self, key: str):
    return self.attributes.get(key, '')

  def list_active_mods(self) -> str:
    return ''

  def list_candidate_mods(self) -> str:
    return 'Voting mods: mod a, mod b'

  def get_mod_description(self, mod: str) -> str:
    return f'Mod description for {mod}'

  def about_tcc(self) -> str:
    return 'Chaos is chat-controlled modifier voting.'

  def explain_voting(self) -> str:
    return 'Voting explanation.'

  def explain_redemption(self) -> str:
    return 'Apply explanation.'

  def explain_credits(self) -> str:
    return 'Credits explanation.'

  def list_permission_groups(self) -> str:
    return 'Permission groups: admin, mods'

  def describe_permission_group(self, group: str) -> str:
    key = str(group).strip().lower()
    if key not in self.permission_groups:
      return f"The group '{key}' does not exist."
    data = self.permission_groups[key]
    permissions = ', '.join(data['permissions']) if data['permissions'] else '(none)'
    members = ', '.join(data['members']) if data['members'] else '(none)'
    return f"Group '{key}' | Permissions: {permissions} | Users: {members}"

  def remove_permission_group(self, group: str) -> str:
    key = str(group).strip().lower()
    if not key:
      return 'Usage: !delgroup <group>'
    if key not in self.permission_groups:
      return f"The group '{key}' does not exist."
    if key == 'admin':
      return "The group 'admin' cannot be deleted."
    self.permission_groups.pop(key, None)
    return f"Deleted permission group '{key}'."


class _ApplyContext(_StubContext):
  def __init__(self):
    super().__init__()
    self.requested_force_mod = None
    self.balance_steps = []

  @property
  def redemption_cooldown(self) -> float:
    return 0.0

  def get_balance(self, user: str) -> int:
    return 1

  def request_force_mod(self, mod_key: str, requested_by: str = '', consume_credit: bool = False) -> bool:
    self.requested_force_mod = (mod_key, requested_by, consume_credit)
    return True

  def step_balance(self, user: str, delta: int) -> int:
    self.balance_steps.append((user, delta))
    return 1


def test_remove_requires_admin_permission():
  ctx = _StubContext()
  ctx.user_permissions['moduser'] = {'manage_modifiers'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_remove('moduser', ['Mod A']))

  assert ctx.removed_mod == ''
  assert messages == ["You need 'admin' permission to use this command."]


def test_remove_succeeds_for_admin():
  ctx = _StubContext()
  ctx.user_permissions['streamer'] = {'admin'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_remove('streamer', ['Mod A']))

  assert ctx.removed_mod == 'mod a'
  assert messages == ['Modifier removed']


def test_mods_active_reports_no_active_modifiers():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_mods(['active']))

  assert messages == ['No active modifiers']


def test_chaoscmd_reports_configured_link():
  ctx = _StubContext()
  ctx.attributes['chaoscmd_link'] = 'https://example.com/commands'
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_chaoscmd())

  assert messages == ['A list of commands can be found here: https://example.com/commands']


def test_permgroups_requires_manage_permissions():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_perm_groups('viewer'))

  assert messages == ["You need 'manage_permissions' permission to use this command."]


def test_permgroups_lists_groups_when_allowed():
  ctx = _StubContext()
  ctx.user_permissions['moduser'] = {'manage_permissions'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_perm_groups('moduser'))

  assert messages == ['Permission groups: admin, mods']


def test_permgroup_lists_group_details_when_allowed():
  ctx = _StubContext()
  ctx.user_permissions['moduser'] = {'manage_permissions'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_perm_group('moduser', ['mods']))

  assert messages == ["Group 'mods' | Permissions: manage_modifiers | Users: alice, bob"]


def test_delgroup_requires_manage_permissions():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_del_group('viewer', ['mods']))

  assert messages == ["You need 'manage_permissions' permission to use this command."]


def test_delgroup_deletes_group_when_allowed():
  ctx = _StubContext()
  ctx.user_permissions['moduser'] = {'manage_permissions'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_del_group('moduser', ['mods']))

  assert messages == ["Deleted permission group 'mods'."]
  assert 'mods' not in ctx.permission_groups


def test_delgroup_works_with_real_relay_context():
  relay = ChaosRelay()
  relay.add_permission_group('mods')
  relay.add_group_permission('mods', 'manage_permissions')
  relay.add_group_member('mods', 'manager')
  ctx = RelayBotContext(relay)
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_del_group('manager', ['mods']))

  assert messages == ["Deleted permission group 'mods'."]
  assert 'mods' not in relay.permission_groups


def test_apply_queues_without_immediate_credit_debit():
  ctx = _ApplyContext()
  bot = ChaosBot(ctx)
  bot._channel_name = 'streamer'
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_apply('viewer', ['Mod A'], is_streamer=False))

  assert ctx.requested_force_mod == ('mod a', 'viewer', True)
  assert ctx.balance_steps == []
  assert messages == []


def test_apply_still_reports_refusal_messages():
  ctx = _ApplyContext()
  ctx.attributes['msg_no_credits'] = 'You need a modifier credit to do that.'
  bot = ChaosBot(ctx)
  bot._channel_name = 'streamer'
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  ctx.get_balance = lambda _user: 0  # type: ignore[assignment]
  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_apply('viewer', ['Mod A'], is_streamer=False))

  assert ctx.requested_force_mod is None
  assert messages == ['You need a modifier credit to do that.']


def test_info_commands_are_silently_rate_limited():
  ctx = _StubContext()
  ctx.attributes['chaoscmd_link'] = 'https://example.com/commands'
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  with patch.object(bot, '_info_command_on_cooldown', side_effect=[False, True, False]):
    asyncio.run(bot._handle_command('viewer', '!chaos'))
    asyncio.run(bot._handle_command('viewer', '!mod stealth'))
    asyncio.run(bot._handle_command('viewer', '!chaoscmd'))

  assert messages == [
    'Chaos is chat-controlled modifier voting.',
    'A list of commands can be found here: https://example.com/commands',
  ]


def test_info_command_cooldown_applies_across_mods_variants():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  with patch.object(bot, '_info_command_on_cooldown', side_effect=[False, True]):
    asyncio.run(bot._handle_command('viewer', '!mods'))
    asyncio.run(bot._handle_command('viewer', '!mods active'))

  assert messages == ['A list of the available mods for this game can be found here: https://example.com/mods']
