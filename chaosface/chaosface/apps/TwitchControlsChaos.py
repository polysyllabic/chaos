#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  This is the main program to run the Chaos interface for Twitch Controls Chaos (TCC). It sets up
  a chatbot that monitors votes and other commands in the streamer's channel, runs the voting and
  other mod-selection processes, and communicates with the Chaos engine to tell it what mods to
  apply.
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

from chaosface.ChaosModel import ChaosModel

def start_gui():
  # Set up GUI
  flx.App(ChaosInterface).serve('Chaos')
  flx.App(ActiveMods).serve()
  flx.App(VoteTimer).serve()
  flx.App(CurrentVotes).serve()
  flx.create_server(host='0.0.0.0', port=config.relay.get_attribute('ui_port'), loop=asyncio.new_event_loop())
  flx.start()

def twitch_controls_chaos():
  # Load the chatbot, but don't start it running yet. This gives us a chance to set the bot's
  # credentials on an initial run.
  config.relay.chatbot = ChaosBot()

  # Event loop to select the modifiers and communicate with the chaos engine
  model = ChaosModel()
  model.start_threaded()

  start_gui()

  # Clean up on exit
  config.relay.chatbot.shutdown()
  flx.stop()


if __name__ == "__main__":
  twitch_controls_chaos()


