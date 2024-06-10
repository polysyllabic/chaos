# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Chatbot commands that control voting
"""
from twitchbot import Command, Message

import chaosface.config.globals as config


@Command('startvote')
async def cmd_startvote(msg: Message, vote_time: int):
  """
  Open a vote now if one is not already open
  """
  link = config.relay.get_attribute('mod_list_link')
  if link:
    await msg.reply(config.relay.get_attribute('msg_mod_list').format(link))
