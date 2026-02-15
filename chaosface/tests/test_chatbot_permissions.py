"""Command-level permission checks for ChaosBot."""

import asyncio

from chaosface.chatbot.ChaosBot import ChaosBot


class _StubContext:
  def __init__(self):
    self.user_permissions = {}
    self.removed_mod = ''

  def has_permission(self, user: str, permission: str) -> bool:
    return permission in self.user_permissions.get(user, set())

  def mod_enabled(self, mod: str):
    if mod.lower() == 'mod a':
      return True
    return None

  def request_remove_mod(self, mod_key: str) -> None:
    self.removed_mod = mod_key

  def get_attribute(self, key: str):
    return ''


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
