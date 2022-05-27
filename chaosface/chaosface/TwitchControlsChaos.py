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

import threading
import asyncio
import logging

from flexx import flx

from config import model
from chatbot import ChaosBot

from gui import ActiveMods, VoteTimer, Votes, BrowserInterface

from ChaosController import ChaosController

logging.basicConfig(level="INFO")

    
def startFlexx():
  if model.get_value('obs_overlays'):
    flx.App(ActiveMods, model).serve()
    flx.App(VoteTimer, model).serve()
    flx.App(Votes, model).serve()
  if model.get_value('use_gui'):
    flx.App(BrowserInterface, model).serve()
  flx.create_server(host='0.0.0.0', port=model.get_value('ui_port'), loop=asyncio.new_event_loop())
  flx.start()

if __name__ == "__main__":
  
  # Start the UI
  if model.get_value('obs_overlays') or model.get_value('use_gui'):
    flexxThread = threading.Thread(target=startFlexx)
    flexxThread.start()

  # The controller mediates communication between the engine and the chatbot and ui
  controller = ChaosController()
  asyncio.get_event_loop().create_task(ChaosController.loop())
      
  # Finally, start the chatbot
  logging.info("Starting chaos chatbot")
  ChaosBot().run()


