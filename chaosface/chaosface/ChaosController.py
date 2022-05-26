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
import asyncio
import time
from datetime import datetime
import json
import numpy as np
import copy

from config import model
from model import ModelObserver, SoftMax, Voting
from communicator import ChaosCommunicator, CommunicatorObserver

class ChaosController(CommunicatorObserver, ModelObserver):
  def __init__(self):
    
    #  Socket to talk to server
    logging.info("Connecting to chaos engine")
    self.chaosCommunicator = ChaosCommunicator()
    self.chaosCommunicator.attach(self)
    self.chaosCommunicator.start()
    
    now = datetime.now()

  # Process any messages we receive from the engine
  def communicatorUpdate(self, message) -> None:
    model.engineConnected = True
    msg = json.loads(message)

    # Received information about the game just loaded
    if "game" in msg:
      logging.debug("Received game info")
      model.initializeGameData(msg)
      
    if "pause" in msg:
      logging.debug("Got a pause command of: " + str(msg["pause"]))
      self.pause = msg["pause"]

  # Process data changes from the model
  def modelUpdate(self, message) -> None:
    if message == 'active_modifiers':
      self.updateNumModifiers()

  def applyNewMod(self, mod):
    toSend = {}
    toSend["winner"] = mod
    toSend["timePerModifier"] = model.get_value('modifier_time')
    response = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug("Socket response: " + response)
    
  def getNewVotingPool(self):
    # Ignore currently active mods
    inactiveMods = set(np.setdiff1d(model.getModNames(), self.activeMods))
        
    self.currentMods = []
    for k in range(model.get_value('vote_options')):
      # Build a list of contributor for this selection.
      votableTracker = {}
      for mod in inactiveMods:
        votableTracker[mod] = copy.deepcopy(model.getModInfo(mod))
              
      # We must recalculate the softmax probablities each time because the subset from which we're
      # choosing changes each iteration.
      SoftMax.updateProbabilities(votableTracker)
      # make a decision:
      theChoice = np.random.uniform(0,1)
      selectionTracker = 0
      for mod in votableTracker:
        selectionTracker += votableTracker[mod]["p"]
        if selectionTracker > theChoice:
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
    Voting.clearVotes()
    
      
  def flashPause(self):
    if self.pausedFlashingTimer > 0.5 and model.pausedBrightBackground == True:
      try:
        model.set_pausedBrightBackground(False)
      except Exception as e:
        logging.info(e)
    elif self.pausedFlashingTimer > 1.0 and model.pausedBrightBackground == False:
      try:
        model.set_pausedBrightBackground(True)
        self.pausedFlashingTimer = 0.0
      except Exception as e:
        logging.info(e)
        
  def flashDisconnected(self):
    if self.disconnectedFlashingTimer > 0.5 and model.connectedBrightBackground == True:
      try:
        model.set_connectedBrightBackground(False)
      except Exception as e:
        logging.info(e)
    elif self.disconnectedFlashingTimer > 1.0 and model.connectedBrightBackground == False:
      try:
        model.set_connectedBrightBackground(True)
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
    message = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug("Socket response: " + message)

    
  def updateNumModifiers(self):
    self.numMods = model.get_value('active_modifiers')
    self.activeMods = [""] * self.numMods
    self.activeModTimes = [0.0] * self.numMods

  async def loop(self):
    beginTime = time.time() #0.0
    now = beginTime
    dTime = 1.0/model.ui_rate
    priorTime = beginTime - dTime

    # Send a message to the engine asking for info about the game
    self.requestGameInfo()
    self.updateNumModifiers()    
    
    self.pausedFlashingTimer = 0.0
    self.pausedFlashingToggle = True
    
    self.disconnectedFlashingTimer = 0.0
    self.disconnectedFlashingToggle = True

    while model.keepGoing:
      asyncio.sleep(1.0/model.get_value('ui_rate'))
      priorTime = now
      now = time.time()
      dTime = now - priorTime
      self.pausedFlashingTimer += dTime
      self.disconnectedFlashingTimer += dTime
      
      if model.paused:
        beginTime += dTime
        dTime = 0
        self.flashPause()

      if not model.botConnected:
        self.flashDisconnected()
              
      self.voteTime =  now - beginTime
      
      for i in range(len(self.activeModTimes)):
        self.activeModTimes[i] -= dTime/Voting.adjustedModTime()
      
      if self.voteTime >= Voting.timePerVote():
        beginTime = now
        
        # Send winning choice:
        newMod = Voting.selectWinningModifier()
        if newMod:
          self.applyNewMod(newMod)
          SoftMax.addCount(newMod)
        
        logString = ""
        for j in range(model.get_value('vote_options')):
          logString += self.currentMods[j] + ": " + str(Voting.getVotes(j)) + ", "
        logString += newMod + "\n"
        logging.debug(logString)
        
        # Update view:
        if not (newMod.isdigit() and 0 < int(newMod) and int(newMod) < 7):
          finishedModIndex = self.activeModTimes.index(min(self.activeModTimes))
          self.activeMods[finishedModIndex] = newMod
          self.activeModTimes[finishedModIndex] = 1.0
        
        self.getNewVotingPool()

        if model.get_value('announce_mods'):
          model.announceMods()
          model.announceVoting()
        


    
          
