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
    self.connected = True
    self.voting_type = 'Proportional'
    self.voting_cycle = 'Continuous'
    self.vote_open = True
    self.vote_round = 0
    self.attributes = {
      'chaoscmd_link': '',
      'info_cmd_cooldown': 10.0,
      'msg_mod_list': 'A list of the available mods for this game can be found here: {}',
      'mod_list_link': 'https://example.com/mods',
    }
    self.start_vote_requests = []
    self.need_save = False
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

  def request_start_vote(self, duration=None) -> None:
    self.start_vote_requests.append(duration)

  def get_attribute(self, key: str):
    return self.attributes.get(key, '')

  def set_voting_type(self, value: str) -> None:
    self.voting_type = str(value)

  def set_voting_cycle(self, value: str) -> None:
    self.voting_cycle = str(value)

  def set_vote_duration(self, value: float) -> None:
    self.attributes['vote_time'] = float(value)

  def set_vote_delay(self, value: float) -> None:
    self.attributes['vote_delay'] = float(value)

  def set_raffle_time(self, value: float) -> None:
    self.attributes['raffle_time'] = float(value)

  def set_redemption_cooldown(self, value: int) -> None:
    self.attributes['redemption_cooldown'] = int(value)

  def set_need_save(self, value: bool) -> None:
    self.need_save = bool(value)

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

  def get_balance_message(self, user: str) -> str:
    return f'@{user} currently has 3 modifier credits.'

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
    return float(self.attributes.get('redemption_cooldown', 0.0))

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


def test_mod_command_resolves_current_candidate_number_with_real_relay():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Candidate Description Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Sir Robin', 'desc': 'Runs backwards.', 'groups': ['Movement']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
    ],
  })
  relay.candidate_keys = ['mod a', 'sir robin', 'mod c']
  relay.set_candidate_mods(['Mod A', 'Sir Robin', 'Mod C'])

  ctx = RelayBotContext(relay)
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('viewer', '!mod 2'))

  assert messages == ['Runs backwards.']


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


def test_chat_vote_emits_current_vote_round():
  class _User:
    name = 'Alice'

  class _Message:
    text = '2'
    user = _User()

  ctx = _StubContext()
  ctx.vote_round = 17
  votes = []
  bot = ChaosBot(ctx, on_vote=lambda index, user, vote_round: votes.append((index, user, vote_round)))

  asyncio.run(bot._on_chat_message(_Message()))

  assert votes == [(1, 'alice', 17)]


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


def test_applycooldown_requires_manage_modifiers():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('viewer', '!applycooldown 90'))

  assert messages == ["You need 'manage_modifiers' permission to use this command."]


def test_applycooldown_updates_apply_cooldown_value():
  ctx = _StubContext()
  ctx.user_permissions['manager'] = {'manage_modifiers'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('manager', '!applycooldown 90'))

  assert ctx.attributes['redemption_cooldown'] == 90
  assert ctx.need_save is True
  assert messages == ['Apply cooldown set to 90 seconds.']


def test_admin_apply_does_not_trigger_global_apply_cooldown():
  ctx = _ApplyContext()
  ctx.attributes['redemption_cooldown'] = 600
  ctx.user_permissions['adminmod'] = {'admin'}
  bot = ChaosBot(ctx)
  bot._channel_name = 'streamer'
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._cmd_apply('adminmod', ['Mod A'], is_streamer=False))
  assert ctx.requested_force_mod == ('mod a', 'adminmod', False)
  assert bot._last_apply_time == 0.0

  ctx.requested_force_mod = None
  asyncio.run(bot._cmd_apply('viewer', ['Mod A'], is_streamer=False))
  assert ctx.requested_force_mod == ('mod a', 'viewer', True)
  assert bot._last_apply_time > 0.0
  assert messages == []


def test_votemethod_requires_manage_voting():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('viewer', '!votemethod majority'))

  assert messages == ["You need 'manage_voting' permission to use this command."]


def test_votemethod_updates_and_resets_open_vote():
  ctx = _StubContext()
  ctx.user_permissions['manager'] = {'manage_voting'}
  ctx.vote_open = True
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('manager', '!votemethod majority'))

  assert ctx.voting_type == 'Majority'
  assert ctx.start_vote_requests == [None]
  assert ctx.need_save is True
  assert messages == ['Vote method set to Majority. Current vote reset.']


def test_votetime_updates_and_resets_open_vote():
  ctx = _StubContext()
  ctx.user_permissions['manager'] = {'manage_voting'}
  ctx.vote_open = True
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('manager', '!votetime 45'))

  assert ctx.attributes['vote_time'] == 45.0
  assert ctx.start_vote_requests == [45]
  assert ctx.need_save is True
  assert messages == ['Vote time set to 45 seconds. Current vote reset.']


def test_votedelay_updates_and_resets_open_vote():
  ctx = _StubContext()
  ctx.user_permissions['manager'] = {'manage_voting'}
  ctx.vote_open = True
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('manager', '!votedelay 15'))

  assert ctx.attributes['vote_delay'] == 15.0
  assert ctx.start_vote_requests == [None]
  assert ctx.need_save is True
  assert messages == ['Vote delay set to 15 seconds. Current vote reset.']


def test_votecycle_updates_cycle():
  ctx = _StubContext()
  ctx.user_permissions['manager'] = {'manage_voting'}
  ctx.vote_open = True
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('manager', '!votecycle random'))

  assert ctx.voting_cycle == 'Random'
  assert ctx.start_vote_requests == [None]
  assert ctx.need_save is True
  assert messages == ['Vote cycle set to random. Current vote reset.']


def test_raffletime_updates_default_raffle_duration():
  ctx = _StubContext()
  ctx.user_permissions['rafflemgr'] = {'manage_raffles'}
  bot = ChaosBot(ctx)
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('rafflemgr', '!raffletime 120'))

  assert ctx.attributes['raffle_time'] == 120.0
  assert ctx.need_save is True
  assert messages == ['Default raffle time set to 120 seconds.']


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


def test_credits_self_for_streamer_reports_unlimited():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  bot._channel_name = 'streamer'
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('streamer', '!credits'))

  assert messages == ['@streamer has unlimited modifier credits.']


def test_credits_with_argument_uses_balance_lookup_for_streamer():
  ctx = _StubContext()
  bot = ChaosBot(ctx)
  bot._channel_name = 'streamer'
  messages = []

  async def capture(msg: str):
    messages.append(msg)

  bot.send_message = capture  # type: ignore[assignment]
  asyncio.run(bot._handle_command('streamer', '!credits viewer'))

  assert messages == ['@viewer currently has 3 modifier credits.']
