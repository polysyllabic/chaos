# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Chatbot commands that control modifier choice outside the voting mechanism
"""
from twitchbot import Command, Message
import chaosface.config.globals as config
import logging


# TODO: add permission='apply_modifiers' 
@Command('apply', cooldown=config.relay.redemption_cooldown, cooldown_bypass='admin')
async def cmd_apply_mod(msg: Message, *args):
  """
  Manually insert a mod into the list immediately. Requires a credit unless the command comds from
  the streamer. Credits are only deducted if the command is successfully applied. Admins bypass the
  cooldown.
  """
  # Check credit balance (bypass for streamer)
  logging.debug(f'Sent by {msg.author} channel = {msg.channel_name}')
  if msg.author != msg.channel_name:
    balance = config.relay.get_balance(msg.author)
    if balance == 0:
      await msg.reply(config.relay.get_attribute('msg_no_credits'))
      return

  request = ' '.join(args)
  status = config.relay.mod_enabled(request)
  if status is None:
    ack: str = config.relay.get_attribute('msg_mod_not_found')
    await msg.reply(ack.format(request))
    return
  if status:
    mod_key = request.lower()
    config.relay.force_mod = mod_key
    # TODO: check for error reply from engine
    # Deduct from balance
    balance = config.relay.step_balance(msg.author, -1)
    await msg.reply(config.relay.get_attribute('msg_mod_applied'))
    return
  ack: str = config.relay.get_attribute('msg_mod_not_active')
  await msg.reply(ack.format(config.relay.modifier_data[mod_key]['name']))

@Command('disable', permission='admin')
async def cmd_disable_mod(msg: Message, *args):
  request = ' '.join(args)
  await msg.reply(config.relay.set_mod_enabled(request, False))

@Command('enable', permission='admin')
async def cmd_disable_mod(msg: Message, *args):
  request = ' '.join(args)
  await msg.reply(config.relay.set_mod_enabled(request, True))

@Command('resetmods', permission='admin')
async def cmd_reset_mods(msg: Message, *args):
  config.relay.reset_mods = True

# 
#@Command('queue', permission='apply_credits')
#async def cmd_queue_mod(msg: Message, *args):
#  """
#  Queue a mod for manual insertion into the list whenever the cooldown for the previous insertion
#  expires.
#  """
#  pass

