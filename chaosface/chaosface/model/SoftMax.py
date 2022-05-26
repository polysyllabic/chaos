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
import math
from ModelObserver import ModelObserver
from config import model

class SoftMax(ModelObserver):

  def __init__(self):
    self.factors = {}

  def modelUpdate(self, message) -> None:
    if message == 'load_game_file':
      self.reset(model.getModNames())

  def reset(self):
    logging.debug("Resetting SoftMax")
    for key in self.factors.keys():
      self.factors[key]["count"] = 0
    self.updateProbabilities()

  def addCount(self, key):
    if key in self.factors:
      self.factors[key]["count"] += 1
      self.updateProbabilities()

  def getDivisor(self, data):
    softMaxDivisor = 0
    for key in data:
      softMaxDivisor += data[key]["contribution"]
    return softMaxDivisor


  # Update probabilities for a (sub)set of modifiers
  def updateProbabilities(self, data):
    factor = float(model.get_value('softmax_factor'))/100.0
    for mod in data:
      data[mod]["contribution"] = math.exp(data[mod]["count"] * math.log(factor))
    softMaxDivisor = self.getDivisor(data)
    for mod in data:
      data[mod]["p"] = data[mod]["contribution"]/softMaxDivisor
