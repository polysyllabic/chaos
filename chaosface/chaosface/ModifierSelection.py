import time
import threading
import logging
import json
import random
import numpy as np
from datetime import datetime
from flexx import flx
import chaosface.config.globals as config
from chaosface.communicator import EngineObserver
from chaosface.communicator import ChaosCommunicator

# Handles the main event loop for modifier selection (voting, etc.)
# Note: This event loop runs in a separate thread from the flexx UI. It's safe to read the data in
# the ChaosRelay class directly, but to change any values of flexx properties, we must schedule the
# requisite action in the UI with a call to flx.loop.call_soon(<function>, *args)

class ModifierSelection(EngineObserver):
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
    
    self.data_received = False
    
    self.tmiChatText = ""

  def start(self):
    self.thread = threading.Thread(target=self.loop)
    self.thread.start()

  # Ask the engine to tell us about the game we're playing
  # TODO: Request a list of available games
  # TODO: Send a config file from the computer running this bot
  def requestGameInfo(self):
    logging.debug("Asking engine about the game")
    toSend = {"game": True}
    resp = self.chaosCommunicator.sendMessage(json.dumps(toSend))
    logging.debug(f"Socket response: {resp}")

  def updateCommand(self, message) -> None:
    received = json.loads(message)
    logging.debug(f"Notified of message from chaos engine")

    if "pause" in received:
      paused = bool(received["pause"])
      logging.info("Got a pause command of: {paused}")
      flx.loop.call_soon(config.relay.set_paused, paused)

    elif "game" in received:
      logging.debug("Received game info!")
      self.data_received = True
      flx.loop.call_soon(config.relay.initializeGame, received)
    else:
      logging.warn(f"Unprocessed message from engine: {received}")
        
  def applyNewMod(self, mod):
    logging.debug("Winning mod: " + mod)
    toSend = {"winner": mod,
              "timePerModifier": config.relay.get_attribute('modifier_time')}
    message = self.chaosCommunicator.sendMessage(json.dumps(toSend))

  def flashPause(self):
    if self.pausedFlashingTimer > 0.5 and config.relay.pausedBrightBackground == True:
      flx.loop.call_soon(config.relay.set_pausedBrightBackground, False)
    elif self.pausedFlashingTimer > 1.0 and config.relay.pausedBrightBackground == False:
      flx.loop.call_soon(config.relay.set_pausedBrightBackground, True)
      self.pausedFlashingTimer = 0.0
        
  def flashDisconnected(self):
    if self.disconnectedFlashingTimer > 0.5 and config.relay.connectedBrightBackground == True:
      flx.loop.call_soon(config.relay.set_connectedBrightBackground, False)
    elif self.disconnectedFlashingTimer > 1.0 and config.relay.connectedBrightBackground == False:
      flx.loop.call_soon(config.relay.set_connectedBrightBackground, True)
      self.disconnectedFlashingTimer = 0.0
  
  def selectWinningModifier(self):
    tally = list(config.relay.votes)
    if config.relay.votingType == 'Proportional':
      totalVotes = sum(tally)
      if totalVotes < 1:
        for i in range(len(tally)):
          tally[i] += 1
        totalVotes = sum(tally)
      
      theChoice = np.random.uniform(0, totalVotes)
      index = 0
      accumulator = 0
      for i in range(len(tally)):
        index = i
        accumulator += tally[i]
        if accumulator >= theChoice:
          break
      
      logging.info(f"Winning Mod: '{config.relay.candidateMods[index]}' at {100.0 * float(tally[index])/float(totalVotes)}")

    else:
      # get indices of all mods with the max number of votes, then randomly pick one
      maxval = None
      indices = []
      for ind, val in enumerate(tally):
        if maxval is None or val > maxval:
          indices = [ind]
          maxval = val
        elif val == maxval:
          indices.append(ind)
      index = random.choice(indices)

    newMod = config.relay.candidateMods[index]
    return newMod
  
  def loop(self):
    beginTime = time.time() #0.0
    now = beginTime
    dTime = 1.0/config.relay.get_attribute('ui_rate')
    priorTime = beginTime - dTime
    
    # Ping the engine for game information
    self.requestGameInfo()

    self.pausedFlashingTimer = 0.0
    self.pausedFlashingToggle = True
    
    self.disconnectedFlashingTimer = 0.0
    self.disconnectedFlashingToggle = True

    while config.relay.keepGoing:
      time.sleep(config.relay.sleepTime())
      priorTime = now
      now = time.time()
      dTime = now - priorTime
      self.pausedFlashingTimer += dTime
      self.disconnectedFlashingTimer += dTime
      
#      self.updateChatCredentials()
      
      if config.relay.paused:
        beginTime += dTime
        dTime = 0
        self.flashPause()
          
      if not config.relay.connected:
        self.flashDisconnected()
              
      self.voteTime =  now - beginTime

      if dTime > 0:
        flx.loop.call_soon(config.relay.decrementVoteTimes, dTime)
        
      if self.firstTime and config.relay.validData:
        self.voteTime = config.relay.timePerVote() + 1
        self.firstTime = False
          
      if self.voteTime >= config.relay.timePerVote():
        # Vote time expired. Pick a winner
        beginTime = now
        
        if not config.relay.validData or config.relay.votingType == 'DISABLE':
          continue
        
        # Pick the winner
        newMod = self.selectWinningModifier()
        # Tell the engine
        self.applyNewMod(newMod)

        flx.loop.call_soon(config.relay.replaceMod, newMod)
          
        #self.announceMods()
        #self.announceVoting()
        
      flx.loop.call_soon(config.relay.setTimePerVote, self.voteTime)

    # Exitiing loop: clean up
    self.chaosCommunicator.stop()

    
