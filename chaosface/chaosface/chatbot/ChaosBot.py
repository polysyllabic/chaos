#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The Twitch Bot to monitor chat for votes and other commands.
"""
import logging
from twitchbot import BaseBot, Message
from flexx import flx
import chaosface.config.globals as config

class ChaosBot(BaseBot):

  def __init__(self):
    super().__init__()

  # All we do here is look for potential votes. Commands are dispatched by the parent class and
  # handled by the commands defined in the commands subdirectory
  async def on_privmsg_received(self, msg: Message):
    if config.relay.connected and config.relay.voting_type != 'DISABLED':
      logging.debug(f'Checking message {msg.content} for a vote')
      # A vote message must be a pure number
      if msg.content.isdigit():
        vote_num = int(msg.content) - 1
        if vote_num >= 0:
          flx.loop.call_soon(config.relay.tally_vote, vote_num, msg.author)

  async def on_connected(self):
    flx.loop.call_soon(config.relay.set_connected, True)



if __name__ == "__main__":
  logging.info("Starting ChaosBot...")
  ChaosBot().run()
  
