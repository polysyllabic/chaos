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

 