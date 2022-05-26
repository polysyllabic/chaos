"""
  Twitch Controls Chaos (TCC)
  Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
  file at the top-level directory of this distribution for details of the
  contributers.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""
import logging
from twitchbot import BaseBot, Message
from config import model
from model import Voting

class ChaosBot(BaseBot):

  def __init__(self):
    super().__init__()

  # All we do here is look for potential votes
  async def on_privmsg_received(self, msg: Message):
    if model.engineConnected and model.get_value('voting_enabled'):
      # A vote message must be a pure number
      if msg.content.isdigit():
        messageAsInt = int(msg.content) - 1
        if messageAsInt >= 0:
          Voting.tallyVote(messageAsInt, msg.author)

  async def on_connected(self):
    model.botConnected = True

  # TODO: handle bot being banned or timed out
  async def on_bot_shutdown(self):
    model.keepGoing = False


if __name__ == "__main__":
  logging.info("Starting ChaosBot...")
  ChaosBot().run()
  
