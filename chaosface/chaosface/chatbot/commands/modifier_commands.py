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

# Manually add a mod credit to a user's account
@Command('addcredit', permission='admin')
async def cmd_add_mod_credit(msg: Message, *args):

  await msg.reply(f"@{msg.author} now has " )



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

