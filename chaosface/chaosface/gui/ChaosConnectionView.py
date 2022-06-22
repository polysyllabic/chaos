# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View to show when the chatbot is connected to Twitch
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
