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
from twitchbot import Command, SubCommand, Message

from config import relay

# General description of TCC and how it works
@Command('chaos',
         help='How Twitch Controls Chaos works',
         cooldown=relay.get_value('info_cmd_cooldown'))
async def cmd_chaos(msg: Message, *args):
  if not args:
    await msg.reply(relay.getAbout())

# Explain the currently active voting method
@SubCommand(cmd_chaos, 'voting',
            help='How voting works',
            cooldown=relay.get_value('info_cmd_cooldown'))
async def cmd_chaos_vote(msg: Message, *args):
  await msg.reply(relay.explainVoting())

@SubCommand(cmd_chaos, 'redeem',
            help='How to redeem modifier credits',
            cooldown=relay.get_value('info_cmd_cooldown'))
async def cmd_chaos_redeem(msg: Message, *args):
  await msg.reply(relay.explainRedemption())

@SubCommand(cmd_chaos, 'credits',
            help='How to earn modifier credits',
            cooldown=relay.get_value('info_cmd_cooldown'))
async def cmd_chaos_credits(msg: Message, *args):
  await msg.reply(relay.explainCredits())

# Describe one mod
@Command('mod')
async def cmd_mod(msg: Message, *args):
  if not args:
    await msg.reply("Usage: !mod <mod name>")
  else:
    await msg.reply(relay.getModDescription(*args))

# TODO: Give a link to all mods
# This is currently disabled because a fixed list of all mods won't be particularly useful when
# we are chaging around what mods are available per game. Ideally, we will generate the active mods
# list on the fly and put it somehwere that is publicly accessible
@Command('mods')
async def cmd_mods(msg: Message, *args):
  await msg.reply(' '.join(args))

# List currently active mods
@SubCommand(cmd_mods, 'active')
async def cmd_mods_active(msg: Message, *args):
  await msg.reply(relay.getActiveMods())

# List the mods currently being voted on in chat
@SubCommand(cmd_mods, 'voting')
async def cmd_mods_voting(msg: Message, *args):
  await msg.reply(relay.getVotableMods())
