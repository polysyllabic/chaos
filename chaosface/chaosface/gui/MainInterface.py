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

from .BotConfiguration import BotConfiguration
from .Settings import Settings
from .StreamerInterface import StreamerInterface

class MainInterface(flx.PyWidget):

  def init(self):
    with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080") as self.t:
      self.interface = StreamerInterface(title='Interface')
      self.settings = Settings(title='Settings')
      self.botSetup = BotConfiguration(title='Connection')

  def dispose(self):
    super().dispose()
    # ... do clean up or notify other parts of the app
    self.interface.dispose()
    self.settings.dispose()
    self.botSetup.dispose()
