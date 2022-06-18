#!/usr/bin/env python3
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

import asyncio
import logging
logging.basicConfig(level=logging.DEBUG)

from flexx import flx

import chaosface.config.globals as config

from chaosface.chatbot.ChaosBot import ChaosBot
from chaosface.gui.ActiveMods import ActiveMods
from chaosface.gui.CurrentVotes import CurrentVotes
from chaosface.gui.VoteTimer import VoteTimer
from chaosface.gui.ChaosInterface import ChaosInterface


from chaosface.ModifierSelection import ModifierSelection

if __name__ == "__main__":
  # Start Twitch chatbot
  chatbot = ChaosBot()
  chatbot.run_threaded()
  
  # Set up GUI
  interface = flx.App(ChaosInterface)
  interface.serve('Chaos')
  flx.App(ActiveMods).serve()
  flx.App(VoteTimer).serve()
  flx.App(CurrentVotes).serve()
  flx.create_server(host='0.0.0.0', port=config.relay.get_attribute('ui_port'), loop=asyncio.new_event_loop())

  # Event loop to select the modifiers and communicate with the chaos engine
  selector = ModifierSelection()
  selector.start()

  # Start the browser-based GUI
  flx.start()

