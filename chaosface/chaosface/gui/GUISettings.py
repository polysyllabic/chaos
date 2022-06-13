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
import chaosface.config.globals as config

class GUISettings(flx.PyWidget):
  def init(self):
    super().init()

    styleLabel = "text-align:right"
    styleField = "background-color:#BBBBBB;text-align:center"

    with flx.VSplit(flex=1):
      flx.Label(style="text-align:center", text="Settings" )
      with flx.HBox():
        with flx.VBox(flex=1):
          flx.Widget(flex=1)
        with flx.VBox():
          flx.Label(style=styleLabel, text="Browser Update Rate (Hz):" )
        with flx.VBox(flex=1):
          self.uiRate = flx.LineEdit(style=styleField, text=str(config.relay.uiRate))
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
