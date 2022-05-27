
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
from flexx import flx

class ConnectionView(flx.PyWidget):
  def init(self):
    super().init()

    #self.apply_style("background-color:black")
    self.styleDisconnectedDark = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:black"
    self.styleDisconnectedBright = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:red"
    if self.model.relay.connected:
      text="Chat Connected"
    else:
      text="Chat Disconnected"
    self.connectedText = flx.Label(flex=0, style=self.styleDisconnectedDark, text=text )

  @flx.action
  def updateConnected(self, connected):
    if connected:
      self.connectedText.set_text("Chat Connected")
      self.connectedText.apply_style(self.styleDisconnectedDark)
    else:
      self.connectedText.set_text("Chat Disconnected")


  @flx.action
  def updateConnectedBrightBackground(self,  connectedBright):
    if connectedBright:
      self.connectedText.apply_style(self.styleDisconnectedBright)
    else:
      self.connectedText.apply_style(self.styleDisconnectedDark)
