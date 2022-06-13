
from flexx import flx
import chaosface.config.globals as config

class ChaosConnectionView(flx.PyWidget):
  def init(self):
    super().init()

    self.styleDark = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:black"
    self.styleBright = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:25px; min-width:250px; line-height: 1.1; background-color:red"

    if config.relay.connected:
      text="Chat Connected"
    else:
      text="Chat Disconnected"

    self.connectedText = flx.Label(flex=0, style=self.styleDark, text=text)

  @flx.action
  def updateConnected(self, connected):
    if connected:
      self.connectedText.set_text("Chat Connected")
      self.connectedText.apply_style(self.styleDark)
    else:
      self.connectedText.set_text("Chat Disconnected")


  @flx.action
  def updateConnectedBrightBackground(self,  connectedBright):
    if connectedBright:
      self.connectedText.apply_style(self.styleBright)
    else:
      self.connectedText.apply_style(self.styleDark)
