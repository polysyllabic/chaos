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
#import random
import json
import copy
import math

import numpy as np

import threading

import logging
log = logging.getLogger(__name__)

from chaosface.communicator.ChaosCommunicator import ChaosCommunicator

class ChaosModel():
  def __init__(self, chatbot, relay):
    self.chatbot = chatbot
    self.relay = relay
    self.firstTime = True

    #  Socket to talk to server
    log.info("Connecting to chaos engine")
    self.chaosCommunicator = ChaosCommunicator()
    self.chaosCommunicator.attach(self)
    self.chaosCommunicator.start()
    self.modifierData = {"1":{"desc":"","groups":""}, "2":{"desc":"","groups":""}, "3":{"desc":"","groups":""},
                         "4":{"desc":"","groups":""}, "5":{"desc":"","groups":""}, "6":{"desc":"","groups":""}}
    self.validData = False
    
    self.pause = True

    self.tmiChatText = ""

  def start(self):
    self.thread = threading.Thread(target=self.process)
    self.thread.start()

  def process(self):
    try:
      log.info("Starting voting loop")
      self._loop()
    except KeyboardInterrupt:
      log.info("Exiting")
      sys.exit(0)
    return True

  def updateCommand(self, message) -> None:
    log.debug("Recieved message from chaos engine: " + str(message))
    received = json.loads(message)

    if "game" in received:
      log.debug("Received game info!")
      self.initializeGameData(received)

    if "mods" in received:
      log.error("The old chaos engine is running. This won't work!")

    if "pause" in received:
      log.info("Got a pause command of: " + str(received["pause"]))
      self.pause = received["pause"]
        
    
  def applyNewMod(self, mod):
    log.debug("Winning mod: " + mod)
    toSend = {}
    toSend["winner"] = mod
    toSend["timePerModifier"] = self.relay.timePerModifier
    response = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    log.debug("Engine response: " + response)


  def initializeGameData(self, gamedata):
    log.debug("Initializing game data")
    self.game_name = gamedata["game"]
    try:
      self.relay.set_gameName(gamedata["game"])
    except Exception as e:
      log.error(e)

    if "errors" in gamedata:
      errors = int(gamedata["errors"])
    else:
      log.error("Missing error count in gamedata message")
      errors = -1
    try:
      self.relay.set_gameErrors(errors)
    except Exception as e:
      log.error(e)

    if "nmods" in gamedata:
      nmods = int(gamedata["nmods"])
    else:
      log.error("Missing modifier count in gamedata message")
      nmods = 3
    try:
      self.relay.set_totalActiveMods(nmods)
    except Exception as e:
      log.error(e)
    
    if "modtime" in gamedata:
      modtime = float(gamedata["modtime"])
    else:
      log.error("Missing modifier time in gamedata message")
      modtime = 180.0
    try:
      self.relay.set_timePerModifier(modtime)
    except Exception as e:
      log.error(e)

    log.debug("Initializing modifier data")
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
    log.info("Resetting SoftMax!")
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
      data[mod]["contribution"] = math.exp(self.modifierData[mod]["count"] * math.log(float(self.relay.softmaxFactor)/100.0))
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
    log.info("New Voting Round:")
    for mod in self.currentMods:
      if "p" in self.modifierData[mod]:
        log.info(" - %0.2f%% %s" % (self.modifierData[mod]["p"]*100.0, mod))
      else:
        log.info(" - %0.2f%% %s" % (0, mod))
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
      
      log.info("Winning Mod: \"%s\" at %0.2f%% chance" % (self.currentMods[ index ], 100.0 * float(self.votes[index])/totalVotes) )
      return self.currentMods[ index ]
    else:
      return self.currentMods[ self.votes.index(max(self.votes)) ]
      
  def updateChatCredentials(self):
    self.chatbot.setChatRate(self.relay.chat_rate)
    self.chatbot.setChannelName(self.relay.channel_name)
    self.chatbot.setBotCredentials(self.relay.bot_name, self.relay.bot_oauth)
    self.chatbot.setIrcInfo(self.relay.ircHost, self.relay.ircPort)
    log.info(f"Credentials set for {self.relay.channel_name} using bot {self.relay.bot_name}")

  def flashPause(self):
    if self.pausedFlashingTimer > 0.5 and self.relay.pausedBrightBackground == True:
      try:
        self.relay.set_pausedBrightBackground(False)
      except Exception as e:
        log.error(e)
    elif self.pausedFlashingTimer > 1.0 and self.relay.pausedBrightBackground == False:
      try:
        self.relay.set_pausedBrightBackground(True)
        self.pausedFlashingTimer = 0.0
      except Exception as e:
        log.error(e)
        
  def flashDisconnected(self):
    if self.disconnectedFlashingTimer > 0.5 and self.relay.connectedBrightBackground == True:
      try:
        self.relay.set_connectedBrightBackground(False)
      except Exception as e:
        log.error(e)
    elif self.disconnectedFlashingTimer > 1.0 and self.relay.connectedBrightBackground == False:
      try:
        self.relay.set_connectedBrightBackground(True)
        self.disconnectedFlashingTimer = 0.0
      except Exception as e:
        log.error(e)
  
  # Ask the engine to tell us about the game we're playing
  # TODO: Request a list of available games
  # TODO: Send a config file from the computer running this bot
  def requestGameInfo(self):
    log.debug("Asking engine about the game")
    toSend = {}
    toSend["game"] = True
    resp = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    log.debug("Socket response: " + resp)


  def _loop(self):
    beginTime = time.time()
    now = beginTime
    dTime = 1.0/self.relay.ui_rate
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
      time.sleep(1.0/self.relay.ui_rate)
      priorTime = now
      now = time.time()
      dTime = now - priorTime
      self.pausedFlashingTimer += dTime
      self.disconnectedFlashingTimer += dTime
      
      #self.updateChatCredentials()
      
      if not self.relay.paused == self.pause:
        try:
          self.relay.set_paused(self.pause)
        except Exception as e:
          log.error(e)
          
      if self.pause:  # hack implementation of pausing
        beginTime += dTime
        dTime = 0
        self.flashPause()

      if not self.relay.connected == self.chatbot.isConnected():
        try:
          self.relay.set_connected(self.chatbot.isConnected())
        except Exception as e:
          log.error(e)
          
      if not self.chatbot.isConnected():  # hack implementation of pausing
        self.flashDisconnected()
              
      self.voteTime =  now - beginTime
      
      # If time-per-modifier has been changed, update time-per-vote and tell the engine
      if not self.timePerVote == (self.relay.timePerModifier/3.0 - 0.5):
        self.timePerVote = self.relay.timePerModifier/3.0 - 0.5
        newVoteTime={}
        newVoteTime["timePerModifier"] = self.relay.timePerModifier
        self.chaosCommunicator.sendMessage(json.dumps(newVoteTime))
      
      for i in range(len(self.activeModTimes)):
        self.activeModTimes[i] -= dTime/((self.timePerVote+0.25)*self.totalVoteOptions)
        
      if self.firstTime and self.validData:
        self.voteTime = self.timePerVote+1
        self.firstTime = False
      
      if self.relay.resetSoftmax:
        try:
          self.relay.set_resetSoftmax(False)
          self.resetSoftMax()
        except Exception as e:
          log.error(e)
      
      if self.voteTime >= self.timePerVote:
        beginTime = now
        
        if not self.validData:
          if self.timeout % (self.relay.ui_rate * 10) == 0:
            # log a waiting message every 10 seconds
            log.info("Waiting for controller sync...")
          self.timeout += 1
          continue
        
        # Send winning choice:
        newMod = self.selectWinningModifier()
        self.applyNewMod(newMod)

        try:
          self.updateSoftMax(newMod)
        except Exception as e:
          log.error(e)
          continue
        
        logString = ""
        for j in range(self.totalVoteOptions):
          logString += self.currentMods[j] + "," + str(int(self.votes[j])) + ","
        logString += newMod + "\n"
        log.info(logString)
        
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
      self.relay.set_mods( self.currentMods )
    except Exception as e:
      log.error(e)
    try:
      self.relay.set_activeMods( self.activeMods )
    except Exception as e:
      log.error(e)
    try:
      self.relay.set_votes( self.votes )
    except Exception as e:
      log.error(e)
    try:
      self.relay.set_voteTime( self.timeToSend )
    except Exception as e:
      log.error(e)
    try:
      self.relay.set_modTimes( self.activeModTimes )
    except Exception as e:
      log.error(e)
    try:
      self.relay.set_tmiResponse(self.tmiChatText)
    except Exception as e:
      log.error(e)
    
    
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
