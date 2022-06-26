# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Chatbot commands that control modifier choice outside the voting mechanism
"""
from twitchbot import Command, Message
import chaosface.config.globals as config

# Manually add a mod credit to a user's account
@Command('addcredit', permission='admin')
async def cmd_add_mod_credit(msg: Message, *args):
  pass

# Manually insert a mod into the list immediately. If a cooldown is still active, the command will
# be ignored and the user will keep the credit
@Command('apply', permission='admin')
async def cmd_add_mod(msg: Message, *args):
  pass

# Queue a mod for manually insertion into the list whenever the cooldown for the previous insertion
# expires
@Command('queue', permission='admin')
async def cmd_add_mod(msg: Message, *args):
  pass

