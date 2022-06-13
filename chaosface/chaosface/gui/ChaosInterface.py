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

from flexx import flx, ui
import chaosface.config.globals as config

from chaosface.gui.StreamerInterface import StreamerInterface
from chaosface.gui.GameSettings import GameSettings
from chaosface.gui.ConnectionSetup import ConnectionSetup

class ChaosInterface(flx.PyWidget):

  def init(self):
    self.model = config
    self.set_title('Twitch Controls Chaos')
    with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080"):
      self.interface = StreamerInterface(title='Streamer Interface')
      self.game_settings = GameSettings(title='Game Settings')
#     self.ui_settings = GUISettings(title='UI Settings')    
      self.botSetup = ConnectionSetup(title='Connection Setup')

  def dispose(self):
    super().dispose()
    # ... do clean up or notify other parts of the app
    self.interface.dispose()
    self.game_settings.dispose()
    self.botSetup.dispose()

  # Actions that are called from a different thread are here, so they can be invoked by a call to the root app

  @flx.action
  def initializeGame(self, gamedata: dict):
    logging.debug("Initializing game data")
    game_name = gamedata["game"]
    logging.debug(f"Setting game name to '{game_name}'")
    config.relay.set_gameName(gamedata["game"])

    errors = int(gamedata["errors"])
    logging.debug(f"Setting game errors to '{errors}'")
    config.relay.set_gameErrors(errors)

    nmods = int(gamedata["nmods"])
    logging.debug(f"Setting total active mods to '{nmods}'")
    config.relay.set_numActiveMods(nmods)
    
    modtime = float(gamedata["modtime"])
    logging.debug(f"Setting modifier time to {modtime}")
    config.relay.set_timePerModifier(modtime)

    logging.debug(f"Initializing modifier data:")
    config.relay.modifierData = {}
    for mod in gamedata["mods"][0]:
      print(mod)
      modName = mod["name"]
      config.relay.modifierData[modName] = {}
      config.relay.modifierData[modName]["desc"] = mod["desc"]
      config.relay.modifierData[modName]["groups"] = mod["groups"]
    config.relay.allMods = config.relay.modifierData.keys()
    config.relay.resetSoftMax()

  @flx.action
  def setTimePerVote(self, time):
    config.relay.set_voteTime(time/config.relay.get_attribute('time_per_vote'))
