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

import logging
import threading
import time
from datetime import datetime
import json
import zmq
import math
import numpy as np
import copy
import random

from communicator import ChaosCommunicator
from config import relay

class ChaosModel():
  def __init__(self, chatbot):
    self.chatbot = chatbot
    
    self.context = zmq.Context()
    self.firstTime = True

    #  Socket to talk to server
    logging.info("Connecting to chaos engine")
    self.chaosCommunicator = ChaosCommunicator()
    self.chaosCommunicator.attach(self)
    self.chaosCommunicator.start()
    
    now = datetime.now()
    currentTime = now.strftime("%Y-%m-%d_%H:%M:%S")
    self.votingLog = open("~/chaosLogs/votes-" + currentTime + ".log","a", buffering=1)
    
    self.openDatabase("~/chaosLogs/chaosModifierData.json")
    
    self.pause = True

    self.tmiChatText = ""

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
    self.resetSoftMax()
  
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
    now = beginTime
    dTime = 1.0/relay.ui_rate
    priorTime = beginTime - dTime
    
    self.timePerVote = 1.0  # This will be set by the chaos engine
        
    self.totalVoteOptions = 3
    self.votes = [0.0] * self.totalVoteOptions
    self.votedUsers = []
    
    self.activeMods = ["", "", ""]
    self.activeModTimes = [0.0, 0.0, 0.0]
    
    self.gotNewMods = False
    
    self.proportionalVoting = True
    
    # allMods will be set by the chaos engine
    self.allMods = list(self.modifierData.keys())
    self.verifySoftmaxIntegrity()
    
    self.currentMods = random.sample(self.allMods, k=self.totalVoteOptions)

    self.pausedFlashingTimer = 0.0
    self.pausedFlashingToggle = True
    
    self.disconnectedFlashingTimer = 0.0
    self.disconnectedFlashingToggle = True

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
          logging.info(e)
          
      if self.pause:  # hack implementation of pausing
        beginTime += dTime
        dTime = 0
        self.flashPause()
          

      if not relay.connected:
        pass
      #  try:
      #    relay.set_connected(self.chatbot.isConnected())
      #  except Exception as e:
      #    logging.info(e)
          
      #if not self.chatbot.isConnected():  # hack implementation of pausing
      #  self.flashDisconnected()
              
      self.voteTime =  now - beginTime
        
      
      if not self.timePerVote == (relay.timePerModifier/3.0 - 0.5):
        self.timePerVote = relay.timePerModifier/3.0 - 0.5
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
          
      
      if self.voteTime >= self.timePerVote:
        beginTime = now
        
        # Send winning choice:
        newMod = self.selectWinningModifier()
        self.applyNewMod( newMod )
        if self.gotNewMods:
          self.gotNewMods = False
          self.modsFromControllerProgram = self.newAllMods
          self.allMods = [x["name"] for x in self.modsFromControllerProgram]
          
          self.verifySoftmaxIntegrity()
          self.validData = True

        if not self.validData:
          logging.info("Waiting for controller sync...")
          continue
        
        try:
          self.updateSoftMax(newMod)
        except Exception as e:
          logging.info(e)
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
    
          
