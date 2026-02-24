"""Command-level permission checks for ChaosBot."""

import asyncio

from chaosface.chatbot.ChaosBot import ChaosBot


class _StubContext:
  def __init__(self):
    self.user_permissions = {}
    self.removed_mod = ''
    self.attributes = {'chaoscmd_link': ''}
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
