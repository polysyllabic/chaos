#!/usr/bin/env python3

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
	# for chat
	chatbot = chatbot.Chatbot()
	chatbot.setIrcInfo(relay.ircHost, relay.ircPort)
	chatbot.start()
	
	#startFlexx()
	flexxThread = threading.Thread(target=startFlexx)
	flexxThread.start()

	# Voting model:
	chaosModel = ChaosModel(chatbot)
	if (not chaosModel.process()):
		chaosModel.print_help()
			
	logging.info("Stopping threads...")
	
	flx.stop()
	flexxThread.join()

