#!/usr/bin/env python3

import sys
import time
import threading
import asyncio
import json
import copy
from datetime import datetime
import math
import numpy as np


import logging
logging.basicConfig(level=logging.DEBUG)

#from chaosface.chatbot.chatbot import Chatbot
from chaosface.chatbot.ChaosBot import ChaosBot

import config.globals as config

from chaosface.gui.ChaosInterface import MainInterface
from chaosface.gui.ActiveMods import ActiveMods
from chaosface.gui.CurrentVotes import CurrentVotes
from chaosface.gui.VoteTimer import VoteTimer

from chaosface.communicator import EngineObserver
from chaosface.communicator import ChaosCommunicator

from flexx import flx, ui

class ChaosModel(EngineObserver):
  def __init__(self):
    self.firstTime = True

    #  Socket to talk to server
    logging.info("Connecting to chaos server")
    self.chaosCommunicator = ChaosCommunicator()
    self.chaosCommunicator.attach(self)
    self.chaosCommunicator.start()
    
    now = datetime.now()
    currentTime = now.strftime("%Y-%m-%d")
    self.votingLog = open("votes-" + currentTime + ".log","a", buffering=1)
    
    self.modifierData = {"1":{"desc":"","groups":""}, "2":{"desc":"","groups":""}, "3":{"desc":"","groups":""},
                         "4":{"desc":"","groups":""}, "5":{"desc":"","groups":""}, "6":{"desc":"","groups":""}}
    self.validData = False
    
    self.pause = True

    self.tmiChatText = ""

#  def start(self):
#    self.thread = threading.Thread(target=self.process)
#    self.thread.start()

#  def process(self):
#    try:
      # Start loop
#      print("Press CTRL-C to stop sample")
#      self._loop()
#    except KeyboardInterrupt:
#      print("Exiting\n")
#      sys.exit(0)

  # Ask the engine to tell us about the game we're playing
  # TODO: Request a list of available games
  # TODO: Send a config file from the computer running this bot
  def requestGameInfo(self):
    logging.debug("Asking engine about the game")
    toSend = { 'game': True }
    resp = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug(f"Socket response: {resp}")

  def updateCommand(self, message) -> None:
    received = json.loads(message)
    logging.debug(f"Notified of message from chaos engine: {received}")

    if "game" in received:
      logging.debug("Received game info!")
      self.initializeGameData(received)

    if "pause" in received:
      logging.info("Got a pause command of: " + str(received["pause"]))
      self.pause = received["pause"]
        
  def initializeGameData(self, gamedata: dict):
    logging.debug("Initializing game data")
    game_name = gamedata["game"]
    logging.debug(f"Setting game name to '{game_name}'")
    config.relay.set_gameName(gamedata["game"])

    errors = int(gamedata["errors"])
    logging.debug(f"Setting game errors to '{errors}'")
    config.relay.set_gameErrors(errors)

    nmods = int(gamedata["nmods"])
    logging.debug(f"Setting total active mods to '{nmods}'")
    config.relay.set_totalActiveMods(nmods)
    
    modtime = float(gamedata["modtime"])
    logging.error("Missing modifier time in gamedata message")
    config.relay.set_timePerModifier(modtime)

    logging.debug(f"Initializing modifier data:")
    self.modifierData = {}
    for mod in gamedata["mods"][0]:
      print(mod)
      modName = mod["name"]
      self.modifierData[modName] = {}
      self.modifierData[modName]["desc"] = mod["desc"]
      self.modifierData[modName]["groups"] = mod["groups"]

    self.resetSoftMax()
    self.allMods = self.modifierData.keys()
    self.validData = True
    
  def applyNewMod(self, mod):
    logging.debug("Winning mod: " + mod)
    toSend = {
      "winner": mod,
      "timePerModifier": config.relay.timePerModifier
    }
    message = self.chaosCommunicator.sendMessage(json.dumps(toSend))
  
  def resetSoftMax(self):
    logging.info("Resetting SoftMax!")
    for mod in self.modifierData:
      self.modifierData[mod]["count"] = 0
      
  def verifySoftmaxIntegrity(self):
    if len(self.allMods) != len(self.modifierData):
      self.initializeData()
      return
    for mod in self.allMods:
      if not mod in self.modifierData:
        self.initializeData()
        return
#    logging.info("verifySoftmaxIntegrity() Passed")
    
  def getSoftmaxDivisor(self, data):
    # determine the sum for the softmax divisor:
    softMaxDivisor = 0
    for key in data:
      softMaxDivisor += data[key]["contribution"]
    return softMaxDivisor
    
  def updateSoftmaxProbabilities(self, data):
    for mod in data:
      data[mod]["contribution"] = math.exp(self.modifierData[mod]["count"] * math.log(float(config.relay.softmaxFactor)/100.0))
    softMaxDivisor = self.getSoftmaxDivisor(data)
    for mod in data:
      data[mod]["p"] = data[mod]["contribution"]/softMaxDivisor
      
  def updateSoftMax(self, newMod):
    if newMod in self.modifierData:
      self.modifierData[newMod]["count"] += 1        
      # update all probabilities:
      self.updateSoftmaxProbabilities(self.modifierData)
    
  def getNewVotingPool(self):
    # Ignore currently active mods:
    inactiveMods = set(np.setdiff1d(self.allMods, self.activeMods))
        
    self.currentMods = []
    for k in range(self.totalVoteOptions):
      # build a list of contributor for this selection:
      votableTracker = {}
      for mod in inactiveMods:
        votableTracker[mod] = copy.deepcopy(self.modifierData[mod])
              
      # Calculate the softmax probablities (must be done each time):
      self.updateSoftmaxProbabilities(votableTracker)
      #print("Votables:")
      # make a decision:
      theChoice = np.random.uniform(0,1)
      selectionTracker = 0
      #print("Choice: " + str(theChoice))
      for mod in votableTracker:
        selectionTracker += votableTracker[mod]["p"]
        #print("Checking " + str(selectionTracker) + ">" + str(theChoice))
        if selectionTracker > theChoice:
          #print("Using mod: " + mod)
          self.currentMods.append(mod)
          inactiveMods.remove(mod)  #remove this to prevent a repeat
          break
    logging.info("New Voting Round:")
    for mod in self.currentMods:
      if "p" in self.modifierData[mod]:
        logging.info(" - %0.2f%% %s" % (self.modifierData[mod]["p"]*100.0, mod))
      else:
        logging.info(" - %0.2f%% %s" % (0, mod))
    # Reset votes since there is a new voting pool
    self.votes = [0.0] * self.totalVoteOptions
  
  
  
  def selectWinningModifier(self):
    if self.proportionalVoting:
      totalVotes = sum(self.votes)
      if totalVotes < 1:
        for i in range(len(self.votes)):
          self.votes[i] += 1
        totalVotes = sum(self.votes)
      
      theChoice = np.random.uniform(0,totalVotes)
#      print("theChoice = " + str(theChoice))
      index = 0
      accumulator = 0
      for i in range(len(self.votes)):
        index = i
        accumulator += self.votes[i]
        if accumulator >= theChoice:
          break
      
      logging.info("Winning Mod: \"%s\" at %0.2f%% chance" % (self.currentMods[ index ], 100.0 * float(self.votes[index])/totalVotes) )
      return self.currentMods[ index ]
    else:
      return self.currentMods[ self.votes.index(max(self.votes)) ]
      

  def flashPause(self):
    if self.pausedFlashingTimer > 0.5 and config.relay.pausedBrightBackground == True:
      try:
        config.relay.set_pausedBrightBackground(False)
      except Exception as e:
        logging.info(e)
    elif self.pausedFlashingTimer > 1.0 and config.relay.pausedBrightBackground == False:
      try:
        config.relay.set_pausedBrightBackground(True)
        self.pausedFlashingTimer = 0.0
      except Exception as e:
        logging.info(e)
        
  def flashDisconnected(self):
    if self.disconnectedFlashingTimer > 0.5 and config.relay.connectedBrightBackground == True:
      try:
        config.relay.set_connectedBrightBackground(False)
      except Exception as e:
        logging.info(e)
    elif self.disconnectedFlashingTimer > 1.0 and config.relay.connectedBrightBackground == False:
      try:
        config.relay.set_connectedBrightBackground(True)
        self.disconnectedFlashingTimer = 0.0
      except Exception as e:
        logging.info(e)
  
  
  def loop(self):
    beginTime = time.time() #0.0
    now = beginTime
    dTime = 1.0/config.relay.ui_rate
    priorTime = beginTime - dTime
    
    self.timePerVote = 1.0  # This will be set by the C program
        
    self.totalVoteOptions = 3
    self.votes = [0.0] * self.totalVoteOptions
    self.votedUsers = []
    
    self.totalActiveMods = 3
    self.activeMods = [""] * self.totalActiveMods
    self.activeModTimes = [0.0] * self.totalActiveMods
    self.currentMods = [""] * self.totalActiveMods
    
    self.proportionalVoting = True
    
    # Ping the engine for game information
    self.requestGameInfo()

    self.pausedFlashingTimer = 0.0
    self.pausedFlashingToggle = True
    
    self.disconnectedFlashingTimer = 0.0
    self.disconnectedFlashingToggle = True

    while config.relay.keepGoing:
      time.sleep(1.0/config.relay.uiRate)
      priorTime = now
      now = time.time()
      dTime = now - priorTime
      self.pausedFlashingTimer += dTime
      self.disconnectedFlashingTimer += dTime
      
      if config.relay.paused:
        beginTime += dTime
        dTime = 0
        self.flashPause()
          
      if not config.relay.connected:
        self.flashDisconnected()
              
      self.voteTime =  now - beginTime
      self.timePerVote = config.relay.timePerModifier/3.0 - 0.5
      
      for i in range(len(self.activeModTimes)):
        self.activeModTimes[i] -= dTime/((self.timePerVote+0.25)*self.totalVoteOptions)
        
      if self.firstTime and self.validData:
        self.voteTime = self.timePerVote + 1
        self.firstTime = False
      
      if config.relay.resetSoftmax:
        try:
          config.relay.set_resetSoftmax(False)
          self.resetSoftMax()
        except Exception as e:
          logging.info(e)
      
      if self.voteTime >= self.timePerVote:
        beginTime = now
        
        # Send winning choice:
        newMod = self.selectWinningModifier()
        self.applyNewMod(newMod)

        if not self.validData:
#          logging.info("Waiting for controller sync...")
          continue
        
        try:
          self.updateSoftMax(newMod)
        except Exception as e:
          logging.error(e)
          continue
        
        logString = ""
        for j in range(self.totalVoteOptions):
          logString += self.currentMods[j] + "," + str(int(self.votes[j])) + ","
        logString += newMod + "\n"
        self.votingLog.write(logString)
        
        # Update view:
        if not (newMod.isdigit() and 0 < int(newMod) and int(newMod) < 7):
          finishedModIndex = self.activeModTimes.index(min(self.activeModTimes))
          self.activeMods[finishedModIndex] = newMod
          self.activeModTimes[finishedModIndex] = 1.0
        
        self.getNewVotingPool()
          
        self.votedUsers = []

        #self.announceMods()
        #self.announceVoting()
        
      self.timeToSend = self.voteTime/self.timePerVote

      self.checkMessages()
      self.updateUI()

    # Exitiing loop: clean up
    self.chaosCommunicator.stop()

  def updateUI(self):
    try:
      config.relay.set_mods( self.currentMods )
    except Exception as e:
      logging.error(e)
    try:
      config.relay.set_activeMods( self.activeMods )
    except Exception as e:
      logging.error(e)
    try:
      config.relay.set_votes( self.votes )
    except Exception as e:
      logging.error(e)
    try:
      config.relay.set_voteTime( self.timeToSend )
    except Exception as e:
      logging.error(e)
    try:
      config.relay.set_modTimes( self.activeModTimes )
    except Exception as e:
      logging.error(e)
    try:
      config.relay.set_tmiResponse(self.tmiChatText)
    except Exception as e:
      logging.error(e)
    
    
  def announceMods(self):
    message = "The currently active mods are " + ", ".join(filter(None, self.activeMods))
    self.chatbot.sendReply( message )
    
  def announceVoting(self):
    message = "You can currently vote for the following mods: "
    for num, mod in enumerate(self.currentMods, start=1):
      message += " {}: {}.".format(num, mod)
    self.chatbot.sendReply( message )
    
      
  def checkMessages(self):
    #if q.qsize() > 0:
    needToUpdateVotes = False
    while self.chatbot.messageQueue.qsize() > 0:
      notice = self.chatbot.messageQueue.get()
      
      if notice["user"] == "tmi":
        self.tmiChatText += "tmi: " + notice["message"] + "\r\n"
        continue
          
      message = notice["message"]
      if message.isdigit():
        messageAsInt = int(message) - 1
        if messageAsInt >= 0 and messageAsInt < self.totalVoteOptions and not notice["user"] in self.votedUsers:
          self.votedUsers.append(notice["user"])
          self.votes[messageAsInt] += 1
          needToUpdateVotes = True
          continue
                
      command = message.split(" ",1)
      firstWord = command[0]
      if firstWord == "!mods":
        self.chatbot.sendReply( "!mods: There are currently " + str(len(self.allMods)) + " modifiers!  See them all with descriptions here: https://github.com/blegas78/chaos/tree/main/docs/TLOU2 @" + notice["user"] )
        continue
              
      if firstWord == "!mod":
        if len(command) == 1:
          message = "Usage: !mod <mod name>"
          self.chatbot.sendReply( message )
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
        
      if firstWord == "!activemods":
        self.announceMods()
        
      if firstWord == "!nowvoting":
        self.announceVoting()


    
def startFlexx():
  flx.App(MainInterface).serve()
  flx.App(ActiveMods).serve()
  flx.App(VoteTimer).serve()
  flx.App(CurrentVotes).serve()
  
  flx.create_server(host='0.0.0.0', port=config.relay.get_attribute('ui_port'), loop=asyncio.new_event_loop())
  flx.start()

if __name__ == "__main__":
  # for chat
  #chatbot = Chatbot()
  #chatbot.setIrcInfo(config.relay.get_attribute('irc_host'), config.relay.get_attribute('irc_port'))
  #chatbot.start()
  chatbot = ChaosBot()
  botthread = chatbot.run_threaded()

  #startFlexx()
  flexxThread = threading.Thread(target=startFlexx)
  flexxThread.start()

  # Voting model:
  chaosModel = ChaosModel(chatbot)
  chaosModel.loop()
      
  logging.info("Stopping flexx...")
  flx.stop()
  flexxThread.join()
  
  logging.info("Stopping chatbot...")
  chatbot.shutdown()
  botthread.join()
