# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Chatbot commands that ask for information about TCC
"""
import logging
from twitchbot import Command, SubCommand, Message

import chaosface.config.globals as config

@Command('chaos',
         help='How Twitch Controls Chaos works',
         cooldown=config.relay.get_attribute('info_cmd_cooldown'))
async def cmd_chaos(msg: Message, *args):
  """
  Provide a general description of TCC and how it works
  """
  if not args:
    await msg.reply(config.relay.about_tcc())

# Explain the currently active voting method
@SubCommand(cmd_chaos, 'voting',
            help='How voting works',
            cooldown=config.relay.get_attribute('info_cmd_cooldown'))
async def cmd_chaos_vote(msg: Message, *args):
  await msg.reply(config.relay.explain_voting())

@SubCommand(cmd_chaos, 'redeem',
            help='How to redeem modifier credits',
            cooldown=config.relay.get_attribute('info_cmd_cooldown'))
async def cmd_chaos_redeem(msg: Message, *args):
  await msg.reply(config.relay.explain_redemption())

@SubCommand(cmd_chaos, 'credits',
            help='How to earn modifier credits',
            cooldown=config.relay.get_attribute('info_cmd_cooldown'))
async def cmd_chaos_credits(msg: Message, *args):
  await msg.reply(config.relay.explain_credits())

# Describe one mod
@Command('mod')
async def cmd_mod(msg: Message, *args):
  if not args:
    await msg.reply("Usage: !mod <mod name>")
  else:
    mod = ' '.join(args)
    await msg.reply(config.relay.get_mod_description(mod))

@Command('mods')
async def cmd_mods(msg: Message, *args):
  link = config.relay.get_attribute('mod_list_link')
  if link:
    await msg.reply(config.relay.get_attribute('msg_mod_list').format(link))

# List currently active mods
@SubCommand(cmd_mods, 'active')
async def cmd_mods_active(msg: Message, *args):
  await msg.reply(config.relay.list_active_mods())

# List the mods currently being voted on in chat
@SubCommand(cmd_mods, 'voting')
async def cmd_mods_voting(msg: Message, *args):
  await msg.reply(config.relay.list_candidate_mods())
