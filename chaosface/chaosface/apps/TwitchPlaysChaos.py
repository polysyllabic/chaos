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

import sys
import time

import threading
import asyncio
import logging
import random
import json
from datetime import datetime

import zmq

from chatbot import chatbot
from config import ChaosRelay

from gui import BotConfigurationView, SettingsView
from communicator import ChaosCommunicator

from flexx import flx, ui
logging.basicConfig(level="INFO")

class PlaysModel():
	def __init__(self, chatbot):
		self.chatbot = chatbot
		
		self.context = zmq.Context()
		self.firstTime = True

		#  Socket to talk to server
		logging.info("Connecting to chaos server")
		self.chaosCommunicator = ChaosCommunicator()
		self.chaosCommunicator.attach(self)
		self.chaosCommunicator.start()
		
		self.now = datetime.now()
		currentTime = self.now.strftime("%Y-%m-%d_%H:%M:%S")
		self.votingLog = open("/home/pi/chaosLogs/votes-" + currentTime + ".log","a", buffering=1)
		
		self.openDatabase("/home/pi/chaosLogs/playsModifierData.json")
		
		self.pause = True

		self.tmiChatText = ""

		self.runningVotes = []
		self.voteHistogram = []
		self.needToBuildHistogram = True
		
		self.currentActions = []

	def openDatabase(self, modifierDataFile):
		self.modifierDataFile = modifierDataFile
		try:
			with open(modifierDataFile) as json_data_file:
				self.modifierData = json.load(json_data_file)
			logging.info("Successfully loaded modifier data from " + modifierDataFile)
			self.validData = True
		except Exception as e:
			self.modifierData = { "1":{"desc":""}, "2":{"desc":""}, "3":{"desc":""}, "4":{"desc":""}, "5":{"desc":""}, "6":{"desc":""}}
			self.validData = False
			logging.info("Generating modifier data, " + modifierDataFile + " load error")
		

	def saveDatabase(self):
		with open(self.modifierDataFile, 'w') as outfile:
			json.dump(self.modifierData, outfile)

	def start(self):
		self.thread = threading.Thread(target=self.process)
		self.thread.start()

	def process(self):
		#self.args = self.parser.parse_args()
		
		try:
			# Start loop
			print("Press CTRL-C to stop sample")
			self._loop()
		except KeyboardInterrupt:
			print("Exiting\n")
			sys.exit(0)

		return True
		
	def updateCommand(self, message ) -> None:
		y = json.loads(message)

		if "mods" in y:
			self.newAllMods = y["mods"]
			self.gotNewMods = True
			
		if "pause" in y:
			logging.info("Got a pause command of: " + str(y["pause"]))
			self.pause = y["pause"]
				
		
	def applyNewMod(self, mod):
		toSend = {}
		toSend["winner"] = mod
		toSend["timePerModifier"] = relay.timePerModifier
		message = self.chaosCommunicator.sendMessage(json.dumps(toSend))
	
	def initializeData(self):
		logging.info("Initializing modifierData")
		self.modifierData = {}
		for mod in self.modsFromControllerProgram:
			modName = mod["name"]
			self.modifierData[modName] = {}
			self.modifierData[modName]["desc"] = mod["desc"]
			lifespan = mod["lifespan"]
			if lifespan < 0:
				lifespan = 3.0
			self.modifierData[modName]["lifespan"] = lifespan
		self.resetSoftMax()
	
	def resetSoftMax(self):
		logging.info("Resetting SoftMax!")
		for mod in self.modifierData:
			self.modifierData[mod]["count"] = 0
			
	def verifyDatabaseIntegrity(self):
		if len(self.allMods) != len(self.modifierData):
			self.initializeData()
			return
		for mod in self.allMods:
			if not mod in self.modifierData:
				self.initializeData()
				return
			
	def updateChatCredentials(self):
		self.chatbot.setChatRate(relay.chat_rate)
		self.chatbot.setChannelName(relay.channel_name)
		self.chatbot.setBotCredentials(relay.bot_name, relay.bot_oauth)
		self.chatbot.setIrcInfo(relay.ircHost, relay.ircPort)
	
	def flashPause(self):
		if self.pausedFlashingTimer > 0.5 and relay.pausedBrightBackground == True:
			try:
				relay.set_pausedBrightBackground(False)
			except Exception as e:
				logging.info(e)
		elif self.pausedFlashingTimer > 1.0 and relay.pausedBrightBackground == False:
			try:
				relay.set_pausedBrightBackground(True)
				self.pausedFlashingTimer = 0.0
			except Exception as e:
				logging.info(e)
				
	def flashDisconnected(self):
		if self.disconnectedFlashingTimer > 0.5 and relay.connectedBrightBackground == True:
			try:
				relay.set_connectedBrightBackground(False)
			except Exception as e:
				logging.info(e)
		elif self.disconnectedFlashingTimer > 1.0 and relay.connectedBrightBackground == False:
			try:
				relay.set_connectedBrightBackground(True)
				self.disconnectedFlashingTimer = 0.0
			except Exception as e:
				logging.info(e)
	
	def _loop(self):
		beginTime = time.time() #0.0
		self.now = beginTime
#		self.rate = 20.0
#		dTime = 1.0/self.rate
		dTime = 1.0/relay.ui_rate
		priorTime = beginTime - dTime
		
		self.timePerVote = 1.0	# This will be set by the C program
				
		self.totalVoteOptions = 3
		self.votes = [0.0] * self.totalVoteOptions
		self.votedUsers = []
		
		self.activeMods = ["", "", ""]
		self.activeModTimes = [0.0, 0.0, 0.0]
		
		self.gotNewMods = False
		
		self.proportionalVoting = True
		
		# allMods will be set by thte C program
		self.allMods = list(self.modifierData.keys())
		self.verifyDatabaseIntegrity()
		
		#self.allMods = [ "No Run/Dodge", "Disable Crouch/Prone", "Drunk Control"]
		self.currentMods = random.sample(self.allMods, k=self.totalVoteOptions)

		self.pausedFlashingTimer = 0.0
		self.pausedFlashingToggle = True

		self.disconnectedFlashingTimer = 0.0
		self.disconnectedFlashingToggle = True
		while True:
#			time.sleep(1.0/self.rate)
			time.sleep(1.0/relay.ui_rate)
			priorTime = self.now
			self.now = time.time()
			dTime = self.now - priorTime
			self.pausedFlashingTimer += dTime
			self.disconnectedFlashingTimer += dTime
			
			self.updateChatCredentials()
			
			if not relay.paused == self.pause:
				try:
					relay.set_paused(self.pause)
				except Exception as e:
					logging.info(e)
					
			if self.pause:	# hack implementation of pausing
				beginTime += dTime
				dTime = 0
				self.flashPause()
					

			if not relay.connected == self.chatbot.isConnected():
				try:
					relay.set_connected(self.chatbot.isConnected())
				except Exception as e:
					logging.info(e)
					
			if not self.chatbot.isConnected():	# hack implementation of pausing
#				beginTime += dTime
#				dTime = 0
				self.flashDisconnected()
							
			self.voteTime = self.now - beginTime
				
			
			if not self.timePerVote == (relay.timePerModifier/3.0 - 0.5):
				self.timePerVote = relay.timePerModifier/3.0 - 0.5
#				relay.saveConfig()
				newVoteTime={}
				newVoteTime["timePerModifier"] = relay.timePerModifier
				self.chaosCommunicator.sendMessage(json.dumps(newVoteTime))
			
			for i in range(len(self.activeModTimes)):
				self.activeModTimes[i] -= dTime/((self.timePerVote+0.25)*self.totalVoteOptions)
				
			if self.firstTime and self.gotNewMods:
				self.voteTime = self.timePerVote+1
				self.firstTime = False
			
			if relay.resetSoftmax:
				try:
					relay.set_resetSoftmax(False)
					self.resetSoftMax()
				except Exception as e:
					logging.info(e)
					
			# If the time is up for the voting round, send the winnder:
			if self.voteTime >= self.timePerVote:
				beginTime = self.now	# reset timer for next round
				
				# Send winning choice:
				#newMod = self.selectWinningModifier()
				
				# Send to chaos controller::
				#self.applyNewMod( newMod )
				if self.gotNewMods:
					self.gotNewMods = False
					self.modsFromControllerProgram = self.newAllMods
					self.allMods = [x["name"] for x in self.modsFromControllerProgram]
					
					self.verifyDatabaseIntegrity()
					self.validData = True

				if not self.validData:
					logging.info("Waiting for controller sync...")
					continue
				
#				try:
#					self.updateSoftMax(newMod)
#				except Exception as e:
#					logging.info(e)
#					continue
#
#				logString = ""
#				for j in range(self.totalVoteOptions):
#					logString += self.currentMods[j] + "," + str(int(self.votes[j])) + ","
#				logString += newMod + "\n"
#				self.votingLog.write(logString)
				
#				# Update view:
#				if not (newMod.isdigit() and 0 < int(newMod) and int(newMod) < 7):
#					finishedModIndex = self.activeModTimes.index(min(self.activeModTimes))
#					self.activeMods[finishedModIndex] = newMod
#					self.activeModTimes[finishedModIndex] = 1.0
				
#				self.getNewVotingPool()
					
				self.votedUsers = []
			
			self.timeToSend = self.voteTime/self.timePerVote
			
			# Get new voting data:
			self.pruneOldVotes()
			self.checkMessages()
			
			# build data:
			self.buildHistogram()
			
			# action handling:
			self.removeOldActions()
			newMod = self.selectNewAction()
			
			if newMod:
				self.applyNewMod( newMod )
			
			
			self.updateUI()
	
		
	def pruneOldVotes(self):
		oldLength = len(self.runningVotes)
		self.runningVotes = [x for x in self.runningVotes if (self.now - x[1]) < self.modifierData[x[0]]["lifespan"]]
		if not len(self.runningVotes) == oldLength:
			self.needToBuildHistogram = True
	
	def buildHistogram(self):
		if self.needToBuildHistogram:
			self.needToBuildHistogram = False
			self.voteHistogram = dict.fromkeys(self.modifierData, 0)
			for vote in self.runningVotes:
				self.voteHistogram[vote[0]] += 1
			logging.info("Histogram:" + str(self.voteHistogram))
			
	def removeOldActions(self):
		for action in self.currentActions:
			if not "start-time" in self.modifierData[action]:
				self.modifierData[action]["start-time"] = self.now + self.modifierData[action]["lifespan"]
			if self.now - self.modifierData[action]["start-time"] > self.modifierData[action]["lifespan"]:
				self.currentActions.remove(action)
				logging.info("Action '" + action + "' is finished")
				return
				
	def selectNewAction(self):
		if len(self.currentActions) >= 4:
			return None
		inactiveActions = {key: value for key, value in self.voteHistogram.items() if not key in self.currentActions}
		winningAction = max(inactiveActions, key=inactiveActions.get)
		if inactiveActions[winningAction] > 0:
			logging.info("winningAction:" + winningAction)
			self.currentActions.append(winningAction)
			self.modifierData[winningAction]["start-time"] = self.now
			return winningAction
		return None
	
	
	def updateUI(self):
		try:
			relay.set_mods( self.currentMods )
		except Exception as e:
			logging.info(e)
		try:
			relay.set_activeMods( self.activeMods )
		except Exception as e:
			logging.info(e)
		try:
			relay.set_votes( self.votes )
		except Exception as e:
			logging.info(e)
		try:
			relay.set_voteTime( self.timeToSend )
		except Exception as e:
			logging.info(e)
		try:
			relay.set_modTimes( self.activeModTimes )
		except Exception as e:
			logging.info(e)
		try:
			relay.set_tmiResponse(self.tmiChatText)
		except Exception as e:
			logging.info(e)
	
	def checkVote(self, message):
		if message in self.modifierData:
#			logging.info("A valid action was provided!")
#			logging.info("Current time: " + str(time.time()))
			self.runningVotes.append( (message, time.time()) )
#			logging.info("New runningVotes:" + str(self.runningVotes))
			self.needToBuildHistogram = True
			return True
		return False
		
	
	def checkMessages(self):
		needToUpdateVotes = False
		while self.chatbot.messageQueue.qsize() > 0:
			notice = self.chatbot.messageQueue.get();
			
			if not "message" in notice:
				continue
				
			logging.info(notice["user"] + ": " + notice["message"])
			
			if notice["user"] == "tmi":
				self.tmiChatText += "tmi: " + notice["message"] + "\r\n"
				continue
				
			message = notice["message"]
			command = message.split(" ",1)
			
			if len(command) == 1 and self.checkVote(command[0].lower()):
				continue
			
			firstWord = command[0]
			if firstWord == "!mods":
				self.chatbot.sendReply( "!mods: There are currently " + str(len(self.allMods)) + " modifiers!  See them all with descriptions here: https://github.com/blegas78/chaos/tree/main/docs/TLOU2 @" + notice["user"] );
				continue
				
			if firstWord == "!actions":
				response = "Actions: "
				for key in self.modifierData.keys():
					response += key + ", "
				self.chatbot.sendReply( response );
				continue
							
			if firstWord == "!mod":
				if len(command) == 1:
					message = "Usage: !mod <mod name>"
					self.chatbot.sendReply( message );
					continue
				argument = (''.join(c for c in command[1] if c.isalnum())).lower()
				message = "!mod: Unrecognized mod :("
				
				for key in self.modifierData.keys():
					keyReduced = (''.join(c for c in key if c.isalnum())).lower()
					if keyReduced == argument:
						mod = self.modifierData[key]
						if mod["desc"] == "":
							message = "!mod " + key + ": No Description :("
						else:
							message = "!mod " + key + ": " + mod["desc"]
						break
				message += " @" + notice["user"]
				self.chatbot.sendReply( message );
				
relay = ChaosRelay()
relay.openConfig("./playsConfig.json")

class BotSetup(flx.PyWidget):
	def init(self, relay):
		self.relay = relay
		self.configurationView = BotConfigurationView(self)
			
	@relay.reaction('updateTmiResponse')
	def _updateTmiResponse(self, *events):
		for ev in events:
			self.configurationView.updateTmiResponse(ev.value)
			
class Settings(flx.PyWidget):
	def init(self, relay):
		self.relay = relay
		self.settingsView = SettingsView(self)

class Interface(flx.PyWidget):

	def init(self, relay):
		self.relay = relay
		with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080") as self.t:
			self.settings = Settings(self.relay, title='Settings')
			self.botSetup = BotSetup(self.relay, title='Bot Setup')

	def dispose(self):
		super().dispose()
		self.interface.dispose()
		self.settings.dispose()
		self.botSetup.dispose()

def startFlexx():
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
	playsModel = PlaysModel(chatbot)
#	chaosModel.start()
	if (not playsModel.process()):
			playsModel.print_help()
			
	logging.info("Stopping threads...")
	

