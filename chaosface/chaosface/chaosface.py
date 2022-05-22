#!/usr/bin/env python3
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
# This is a basline version derived from the original interface program. It uses flexx and the old
# custom twitchbot.

import threading
import asyncio
import logging

from flexx import flx

from config import relay
import chatbot.chatbot

from gui import ActiveMods, VoteTimer, Votes, Interface

import chaosface.ChaosModel as ChaosModel

logging.basicConfig(level="INFO")

    
def startFlexx():
  flx.App(ActiveMods, relay).serve()
  flx.App(VoteTimer, relay).serve()
  flx.App(Votes, relay).serve()
  flx.App(Interface, relay).serve()
  
  flx.create_server(host='0.0.0.0', port=relay.uiPort, loop=asyncio.new_event_loop())
  flx.start()

if __name__ == "__main__":
  # Start the chatbot
  chatbot = chatbot.Chatbot()
  chatbot.setIrcInfo(relay.ircHost, relay.ircPort)
  chatbot.start()
  
  # Start the GUI
  flexxThread = threading.Thread(target=startFlexx)
  flexxThread.start()

  # Start the voting loop
  chaosModel = ChaosModel(chatbot)
  if (not chaosModel.process()):
    chaosModel.print_help()
      
  logging.info("Stopping threads...")
  
  flx.stop()
  flexxThread.join()

