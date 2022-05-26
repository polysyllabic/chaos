import logging
import numpy as np
from typing import List

from config import model 
import ModelObserver
import VotingSubject
from VotingObserver import VotingObserver

class Voting(VotingSubject, ModelObserver):
  _observers: List[VotingObserver] = []

  def __init__(self):
    model.attach(self)
    self.resetVoteOptions()

  def attach(self, observer: VotingObserver) -> None:
    self._observers.append(observer)

  def detach(self, observer: VotingObserver) -> None:
    self._observers.remove(observer)

  # process the parameter changes that matter to us
  def modelUpdate(self, message):
    if message == 'vote_options' or message == 'modifier_time':
      self.resetVoteOptions()

  def clearVotes(self) -> None:
    self.votes = [0] * model.get_value('vote_options')
    self.votedUsers = []

  def resetVoteOptions(self) -> None:
    value = model.get_value('vote_options')
    if value < 2:
      value = 2
      logging.warn('Must be at least 2 voting options')        
      model._set_value('vote_options', value)
    # Reset voting tally
    self.time_per_vote = (model.get_value('modifier_time')/model.get_value('vote_options') - 0.5)
    self.adjusted_mod_time = (self.time_per_vote + 0.25) * value
    self.clearVotes()

  def timePerVote(self):
    return self.time_per_vote

  def adjustedModTime(self):
    return self.adjusted_mod_time

  def tallyVote(self, index: int, user: str) -> None:
    if index >= 0 and index < model.get_value('vote_options') and not user in self.votedUsers:
      self.votedUsers.append(user)
      self.votes[index] += 1
      model.updateVotes()

  def getVotes(self, index: int) -> int:
    return self.votes[index]

  def selectWinningModifier(self):
    if model.get_value('proportional_voting'):
      totalVotes = sum(self.votes)
      if totalVotes < 1:
        for i in range(len(self.votes)):
          self.votes[i] += 1
        totalVotes = sum(self.votes)
      
      theChoice = np.random.uniform(0,totalVotes)
      index = 0
      accumulator = 0
      for i in range(len(self.votes)):
        index = i
        accumulator += self.votes[i]
        if accumulator >= theChoice:
          break
      if (model.get_value('log_votes')):
        logging.info("Winning Mod: \"%s\" at %0.2f%% chance" % (self.currentMods[index], 100.0 * float(self.votes[index])/totalVotes) )
      return self.currentMods[index]
    else:
      return self.currentMods[self.votes.index(max(self.votes))]
