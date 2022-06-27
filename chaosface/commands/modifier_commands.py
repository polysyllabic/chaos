# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Chatbot commands that control modifier choice outside the voting mechanism
"""
from twitchbot import Command, Message, is_command_off_cooldown
import chaosface.config.globals as config
from flexx import flx

# Manually add a mod credit to a user's account
@Command('addcredit', permission='add_credits')
async def cmd_add_mod_credit(msg: Message, *args):
  pass

@Command('apply', permission='apply_credits', cooldown=config.relay.redemption_cooldown, cooldown_bypass='admin')
async def cmd_apply_mod(msg: Message, *args):
  """
  Manually insert a mod into the list immediately. Requires a credit unless issued by someone with
  admin privileges. If a cooldown is still active, the command will be ignored and the user will
  keep the credit. Admins bypass the cooldown.
  """
  request = ' '.join(args)
  mod_key = request.lower()
  status = config.relay.mod_status(mod_key)
  if status == 0:
    ack: str = config.relay.get_attribute('msg_mod_not_found')
    await msg.reply(ack.format(request))
    return
  if status == 1:
    config.relay.force_mod = mod_key
    # TODO: check for error reply from engine
    return
  ack: str = config.relay.get_attribute('msg_mod_not_active')
  await msg.reply(ack.format(config.relay.modifier_data[mod_key]['name']))

# 
#@Command('queue', permission='apply_credits')
#async def cmd_queue_mod(msg: Message, *args):
#  """
#  Queue a mod for manual insertion into the list whenever the cooldown for the previous insertion
#  expires.
#  """
#  pass

