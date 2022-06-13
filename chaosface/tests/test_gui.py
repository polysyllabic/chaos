# Test the gui on its own
import logging
import asyncio
from flexx import flx

import chaosface.config.globals as config
from chaosface.gui.ChaosInterface import Chaos
from chaosface.gui.ActiveMods import ActiveMods
from chaosface.gui.CurrentVotes import CurrentVotes
from chaosface.gui.VoteTimer import VoteTimer

if __name__ == "__main__":
  logging.basicConfig(filename="test.log", level=logging.DEBUG)

  flx.App(Chaos).serve()
  flx.App(ActiveMods).serve()
  flx.App(VoteTimer).serve()
  flx.App(CurrentVotes).serve()
  
  flx.create_server(host='0.0.0.0', port=config.relay.get_attribute('ui_port'), loop=asyncio.new_event_loop())
  flx.start()
