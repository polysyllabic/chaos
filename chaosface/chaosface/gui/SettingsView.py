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
from chaosface.config.globals import relay

class SettingsView(flx.PyWidget):

  def init(self):
    super().init()
    styleLabel = "text-align:right"
    styleField = "background-color:#BBBBBB;text-align:center"

    with flx.VBox(flex=1):
      flx.Label(style="text-align:center", text="Settings" )
      flx.Widget(flex=1)
      self.gameLabel = flx.Label(style="font-weight:bold;font-size:large", text=str(relay.gameName))
      self.gameErrors = flx.Label(text=f"Loaded with {str(relay.gameErrors)} errors")
      flx.Widget(flex=1)
      with flx.HBox():
        with flx.VBox():
          flx.Label(style=styleLabel, text="Active Modifiers:")
          flx.Label(style=styleLabel, text="Time Per Modifier (s):")
          flx.Label(style=styleLabel, text="Repeat Mod Probability (%):")
          flx.Label(style=styleLabel, text="Browser Update Rate (Hz):" )
          flx.Label(style=styleLabel, text="Reset Repeat Mod Memory:")
        with flx.VBox(flex=1):
          self.totalActiveMods = flx.LineEdit(style=styleField, text=str(relay.totalActiveMods))
          self.timePerModifier = flx.LineEdit(style=styleField, text=str(relay.timePerModifier))
          self.uiRate = flx.LineEdit(style=styleField, text=str(relay.ui_rate))
          self.softmaxFactor = flx.Slider(min=1, max=100, step=1, value=relay.softmaxFactor)
          self.resetButton = flx.Button(flex=0,text="Reset")
      with flx.VBox(flex=1):
        self.announceMods = flx.CheckBox(flex=0, text="Announce Mods in Chat", value=relay.announceMods)
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
    relay.set_totalActiveMods(int(self.totalActiveMods.text))
    relay.set_timePerModifier(float(self.timePerModifier.text))
    relay.set_ui_rate(float(self.uiRate.text))
    relay.set_softmaxFactor(self.softmaxFactor.value)
    relay.set_announce_mods(self.announceMods.checked)
    relay.set_shouldSave(True)
    self.successLabel.set_text('Saved!')

  @flx.reaction('resetButton.pointer_click')
  def _reset_button_clicked(self, *events):
    ev = events[-1]
    relay.set_resetSoftmax(True)
    self.successLabel.set_text('Reset!')
