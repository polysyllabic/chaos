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
import time
#import random
import json
import copy
import math

import numpy as np
import logging
import threading

from communicator import ChaosCommunicator
from config import relay

class ChaosModel():
  def __init__(self, chatbot):
    self.chatbot = chatbot
    
    self.firstTime = True

    #  Socket to talk to server
    logging.info("Connecting to chaos engine")
    self.chaosCommunicator = ChaosCommunicator()
    self.chaosCommunicator.attach(self)
    self.chaosCommunicator.start()
    self.modifierData = {"1":{"desc":"","groups":""}, "2":{"desc":"","groups":""}, "3":{"desc":"","groups":""},
                         "4":{"desc":"","groups":""}, "5":{"desc":"","groups":""}, "6":{"desc":"","groups":""}}
    self.validData = False
    
    self.pause = True

    self.tmiChatText = ""

  def start(self):
    self.thread = threading.Thread(target=self._loop)
    self.thread.start()


  def updateCommand(self, message) -> None:
    logging.debug("Recieved message from chaos engine: " + str(message))
    received = json.loads(message)

    if "game" in received:
      logging.debug("Received game info!")
      self.initializeGameData(received)

    if "mods" in received:
      logging.error("The old chaos engine is running. This won't work!")

    if "pause" in received:
      logging.info("Got a pause command of: " + str(received["pause"]))
      self.pause = received["pause"]
        
    
  def applyNewMod(self, mod):
    logging.debug("Winning mod: " + mod)
    toSend = {}
    toSend["winner"] = mod
    toSend["timePerModifier"] = relay.timePerModifier
    response = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug("Engine response: " + response)


  def initializeGameData(self, gamedata):
    logging.debug("Initializing game data")
    self.game_name = gamedata["game"]
    try:
      relay.set_gameName(gamedata["game"])
    except Exception as e:
      logging.error(e)

    if "errors" in gamedata:
      errors = int(gamedata["errors"])
    else:
      logging.error("Missing error count in gamedata message")
      errors = -1
    try:
      relay.set_gameErrors(errors)
    except Exception as e:
      logging.error(e)

    if "nmods" in gamedata:
      nmods = int(gamedata["nmods"])
    else:
      logging.error("Missing modifier count in gamedata message")
      nmods = 3
    try:
      relay.set_totalActiveMods(nmods)
    except Exception as e:
      logging.error(e)
    
    if "modtime" in gamedata:
      modtime = float(gamedata["modtime"])
    else:
      logging.error("Missing modifier time in gamedata message")
      modtime = 180.0
    try:
      relay.set_timePerModifier(modtime)
    except Exception as e:
      logging.error(e)

    logging.debug("Initializing modifier data")
    self.modifierData = {}
    for mod in gamedata:
      modName = mod["name"]
      self.modifierData[modName] = {}
      self.modifierData[modName]["desc"] = mod["desc"]
      self.modifierData[modName]["groups"] = mod["groups"]
    self.resetSoftMax()
    self.allMods = self.modifierData.keys()
    self.validData = True
  
  def resetSoftMax(self):
    logging.info("Resetting SoftMax!")
    for mod in self.modifierData:
      self.modifierData[mod]["count"] = 0
    
  def getSoftmaxDivisor(self, data):
    # determine the sum for the softmax divisor:
    softMaxDivisor = 0
    for key in data:
      softMaxDivisor += data[key]["contribution"]
    return softMaxDivisor
    
  def updateSoftmaxProbabilities(self, data):
    for mod in data:
      data[mod]["contribution"] = math.exp(self.modifierData[mod]["count"] * math.log(float(relay.softmaxFactor)/100.0))
    softMaxDivisor = self.getSoftmaxDivisor(data)
    for mod in data:
      data[mod]["p"] = data[mod]["contribution"]/softMaxDivisor
      
  def updateSoftMax(self, newMod):
    if newMod in self.modifierData:
      self.modifierData[newMod]["count"] += 1
        
      # update all probabilities:
      self.updateSoftmaxProbabilities(self.modifierData)
      self.saveDatabase()
    
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
      
  def updateChatCredentials(self):
    self.chatbot.setChatRate(relay.chat_rate)
    self.chatbot.setChannelName(relay.channel_name)
    self.chatbot.setBotCredentials(relay.bot_name, relay.bot_oauth)
    self.chatbot.setIrcInfo(relay.ircHost, relay.ircPort)
#    logging.info("Credentions set to: " + relay.channel_name + " using bot: " + relay.bot_name + " " + relay.bot_oauth)

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
  
  # Ask the engine to tell us about the game we're playing
  # TODO: Request a list of available games
  # TODO: Send a config file from the computer running this bot
  def requestGameInfo(self):
    logging.debug("Asking engine about the game")
    toSend = {}
    toSend["game"] = True
    resp = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug("Socket response: " + resp)


  def _loop(self):
    beginTime = time.time()
    now = beginTime
    dTime = 1.0/relay.ui_rate
    priorTime = beginTime - dTime
    
    self.timePerVote = 1.0  # This will be set by the engine
        
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

    self.timeout = 0

    while True:
      time.sleep(1.0/relay.ui_rate)
      priorTime = now
      now = time.time()
      dTime = now - priorTime
      self.pausedFlashingTimer += dTime
      self.disconnectedFlashingTimer += dTime
      
      self.updateChatCredentials()
      
      if not relay.paused == self.pause:
        try:
          relay.set_paused(self.pause)
        except Exception as e:
          logging.error(e)
          
      if self.pause:  # hack implementation of pausing
        beginTime += dTime
        dTime = 0
        self.flashPause()

      if not relay.connected == self.chatbot.isConnected():
        try:
          relay.set_connected(self.chatbot.isConnected())
        except Exception as e:
          logging.error(e)
          
      if not self.chatbot.isConnected():  # hack implementation of pausing
        self.flashDisconnected()
              
      self.voteTime =  now - beginTime
      
      # If time-per-modifier has been changed, update time-per-vote and tell the engine
      if not self.timePerVote == (relay.timePerModifier/3.0 - 0.5):
        self.timePerVote = relay.timePerModifier/3.0 - 0.5
        newVoteTime={}
        newVoteTime["timePerModifier"] = relay.timePerModifier
        self.chaosCommunicator.sendMessage(json.dumps(newVoteTime))
      
      for i in range(len(self.activeModTimes)):
        self.activeModTimes[i] -= dTime/((self.timePerVote+0.25)*self.totalVoteOptions)
        
      if self.firstTime and self.validData:
        self.voteTime = self.timePerVote+1
        self.firstTime = False
      
      if relay.resetSoftmax:
        try:
          relay.set_resetSoftmax(False)
          self.resetSoftMax()
        except Exception as e:
          logging.error(e)
      
      if self.voteTime >= self.timePerVote:
        beginTime = now
        
        if not self.validData:
          if self.timeout % (relay.ui_rate * 10) == 0:
            # log a waiting message every 10 seconds
            logging.info("Waiting for controller sync...")
          self.timeout += 1
          continue
        
        # Send winning choice:
        newMod = self.selectWinningModifier()
        self.applyNewMod(newMod)

        try:
          self.updateSoftMax(newMod)
        except Exception as e:
          logging.error(e)
          continue
        
        logString = ""
        for j in range(self.totalVoteOptions):
          logString += self.currentMods[j] + "," + str(int(self.votes[j])) + ","
        logString += newMod + "\n"
        logging.info(logString)
        
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
  
  def updateUI(self):
    try:
      relay.set_mods( self.currentMods )
    except Exception as e:
      logging.error(e)
    try:
      relay.set_activeMods( self.activeMods )
    except Exception as e:
      logging.error(e)
    try:
      relay.set_votes( self.votes )
    except Exception as e:
      logging.error(e)
    try:
      relay.set_voteTime( self.timeToSend )
    except Exception as e:
      logging.error(e)
    try:
      relay.set_modTimes( self.activeModTimes )
    except Exception as e:
      logging.error(e)
    try:
      relay.set_tmiResponse(self.tmiChatText)
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
