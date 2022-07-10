# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View to configure the appearance of the UI and the broswer sources
"""
from flexx import flx
import chaosface.config.globals as config

class GUISettings(flx.PyWidget):
  def init(self):
    super().init()

    styleLabel = "text-align:right"
    styleField = "background-color:#BBBBBB;text-align:center"

    with flx.VSplit(flex=1):
      flx.Label(style="text-align:center", text="Settings" )
      with flx.HBox():
        with flx.VBox():
          flx.Label(style=styleLabel, text="Font:")
          flx.Label(style=styleLabel, text="Font Color:")
          flx.Label(style=styleLabel, text="Font Size:")
          flx.Label(style=styleLabel, text="Progress Bar Color:")
          flx.Label(style=styleLabel, text="Browser Update Rate (Hz):" )
        with flx.VBox(flex=1):
          self.font = flx.LineEdit(style=styleField, text=config.relay.overlay_font)
          self.font_color = flx.LineEdit(style=styleField, text=config.relay.overlay_font_color)
          self.font_size = flx.LineEdit(style=styleField, text=str(config.relay.overlay_font_size))
          self.progress_color = flx.LineEdit(style=styleField, text=config.relay.progress_bar_color)
          self.uiRate = flx.LineEdit(style=styleField, text=str(config.relay.ui_rate))
      with flx.HBox():
        flx.Widget(flex=1)
        self.saveButton = flx.Button(flex=0,text="Save")
        flx.Widget(flex=1)
      with flx.HBox():
        flx.Widget(flex=1)
        self.successLabel = flx.Label(style="text-align:center", text="" )
        flx.Widget(flex=1)

  @flx.reaction('saveButton.pointer_click')
  def _save_button_clicked(self, *events):
    ev = events[-1]
    config.relay.set_ui_rate(float(self.uiRate.text))
    config.relay.set_shouldSave(True)
    self.successLabel.set_text('Saved!')
