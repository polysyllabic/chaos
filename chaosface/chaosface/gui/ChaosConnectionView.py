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
from flexx import flx
import chaosface.config.globals as config

class ChaosConnectionView(flx.PyWidget):
  def init(self):
    super().init()

    self.dark_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:black"
    self.bright_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:red"

    text = "Chat Connected" if config.relay.connected == True else "Chat Disconnected"

    self.connected_message = flx.Label(flex=0, style=self.dark_style, text=text)

  @flx.action
  def update_connected(self, connected):
    if connected:
      self.connected_message.set_text("Chat Connected")
      self.connected_message.apply_style(self.dark_style)
    else:
      self.connected_message.set_text("Chat Disconnected")


  @flx.action
  def update_connected_bright(self, is_bright):
    logging.debug(f'connect flash bright = {is_bright}')
    if is_bright:
      self.connected_message.apply_style(self.bright_style)
    else:
      self.connected_message.apply_style(self.dark_style)
