#-----------------------------------------------------------------------------
# Twitch Controls Chaos (TCC)
# Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
# file at the top-level directory of this distribution for details of the
# contributers.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#-----------------------------------------------------------------------------
from twitchbot import BaseBot
from twitchbot import Message
from twitchbot import Command, SubCommand

import logging
import json 

from ChaosRelay import ChaosRelay

class ChaosBot(BaseBot):

  def __init__(self):
    self.connected = False
    self.ignoreVotes = False
    super().__init__()
    

# All we do here is look for potential votes
async def on_privmsg_received(self, msg: Message):
  if self.ignoreVotes:
    return
  # A vote message must be a single digit only
  if msg.content.isdigit():
    messageAsInt = int(msg.content) - 1
    if messageAsInt >= 0 and messageAsInt < self.totalVoteOptions and not msg.author in self.votedUsers:
      self.votedUsers.append(msg.author)
      self.votes[messageAsInt] += 1
      needToUpdateVotes = True

async def on_connected(self):
  self.connected = True


# Command definitions
# Give a link to all mods
@Command('mods')
async def cmd_mods(msg: Message, *args):
  await msg.reply(ChaosRelay.getModList() + " @{msg.author}")

# List currently active mods
@SubCommand(cmd_mods, 'active')
async def cmd_mods_active(msg: Message, *args):
  await msg.reply(ChaosRelay.getActiveMods())

# List the mods currently being voted on in chat
@SubCommand(cmd_mods, 'voting')
async def cmd_mods_voting(msg: Message, *args):
  await msg.reply(ChaosRelay.getVotableMods())

# Manually add a mod credit to a user's account
@Command('modcredit', permission='admin')
async def cmd_add_mod_credit(msg: Message, *args):

  await msg.reply(f"@{msg.author} now has " )

# Describe one mod
@Command('mod')
async def cmd_mod(msg: Message, *args):
  if not args:
    await msg.reply("Usage: !mod <mod name> - Try !mods for a link to a complete list")
  else:
    await msg.reply(f"{ChaosBot.getModDescription(*args)} @{msg.author}")


# Manually insert a mod into the list. This will displace the oldest mod with the specified one
@Command('apply', permission='admin')
async def cmd_add_mod(msg: Message, *args):
  pass

# General description of TCC and how it works
@Command('chaos')
async def cmd_chaos(msg: Message, *args):
  if not args:
    await msg.reply(ChaosRelay.getAbout())

@SubCommand(cmd_chaos, 'vote')
async def cmd_chaos_vote(msg: Message, *args):
  pass

if __name__ == "__main__":
  # Start the chatbot
  logging.info("Starting ChaosBot...")
  ChaosBot().run_threaded()

  
