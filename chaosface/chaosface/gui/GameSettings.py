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

class GameSettings(flx.PyWidget):
  def init(self):
    super().init()
    
    styleLabel = "text-align:right"
    styleField = "background-color:#BBBBBB;text-align:left"
    
    with flx.VBox():
      self.gameName = flx.Label(style="text-align:center", text=f'Game: {config.relay.gameName}', wrap=1)
      with flx.HSplit():
        with flx.VBox(flex=1):
          with flx.GroupWidget(title='Modifier Selection'):
            with flx.HBox():
              with flx.VBox():
                flx.Label(style=styleLabel, text="Active modifiers:")
                flx.Label(style=styleLabel, text="Time per modifier (s):")
                flx.Label(style=styleLabel, text="Chance of repeat modifier:")
                flx.Label(style=styleLabel, text="Clear modifier use history:")
              with flx.VBox():
                self.activeModifiers = flx.LineEdit(style=styleField, text=str(config.relay.totalActiveMods))
                self.timePerModifier = flx.LineEdit(style=styleField, text=str(config.relay.timePerModifier))
                self.softmaxFactor = flx.Slider(min=1, max=100, step=1, value=config.relay.softmaxFactor)
                self.resetButton = flx.Button(text="Reset")
              flx.Widget(flex=1)
        
          with flx.GroupWidget(flex=0, title='Modifier Voting'):
            with flx.VBox():
              with flx.HBox():
                with flx.VBox():
                  flx.Label(style=styleLabel, text="Vote Options:")
                  flx.Label(style=styleLabel, text="Voting Method:")
                with flx.VBox():
                  self.voteOptions = flx.LineEdit(style=styleField, text=str(config.relay.voteOptions))
                  self.votingType = flx.ComboBox(options=['Proportional','Majority','DISABLE'], selected_key='Proportional')
                flx.Widget(flex=1)
              self.announceCandidates = flx.CheckBox(text="Announce candidates in chat", checked=config.relay.announceCandidates, style="text-align:left")
              self.announceWinner = flx.CheckBox(text="Announce winner in chat", checked=config.relay.announceWinner, style="text-align:left")
              flx.Widget(flex=1)
          with flx.GroupWidget(flex=0, title='Modifier Redemption'):
            with flx.HBox():
              with flx.VBox():
                self.bitsRedemption = flx.CheckBox(text='Allow bits redemptions', checked=config.relay.bitsRedemption, style="text-align:left")
                self.pointsRedemption = flx.CheckBox(text='Channel points redemption', checked=config.relay.pointsRedemption, style="text-align:left")
                self.raffles = flx.CheckBox(text='Conduct raffles', checked=config.relay.raffles, style="text-align:left")
              with flx.VBox():
                flx.Label(style=styleLabel, text='Bits per mod credit:')
                flx.Label(style=styleLabel, text='Points per mod credit:')
                flx.Label(style=styleLabel, text='Redemption Cooldown (s):')              
              with flx.VBox():
                flx.LineEdit(style=styleField, text=str(config.relay.bitsPerCredit))
                flx.LineEdit(style=styleField, text=str(config.relay.pointsPerCredit))
                flx.LineEdit(style=styleField, text=str(config.relay.redemptionCooldown))
              flx.Widget(flex=1)
        with flx.VBox(flex=1):
          flx.Label(style="text-align:left", text='Available Modifiers')
          self.availableModifiers = flx.TreeWidget(flex=1, max_selected=1, minsize=(300,300))
          with flx.HBox():
            with flx.VBox():
              flx.Label(text='Description:')
              flx.Label(text='Groups')
            with flx.VBox():
              self.modDescription = flx.LineEdit(flex=2)
              self.modGroups = flx.LineEdit(flex=2)
            flx.Widget(flex=1)
        # End of HSplit
      with flx.HBox():
        self.saveButton = flx.Button(text="Save")
        self.resetButton = flx.Button(text="Defaults")
        self.successLabel = flx.Label(flex=1, style="text-align:center", text='')
        flx.Widget(flex=1)
      flx.Widget(flex=1)

  @flx.reaction('saveButton.pointer_click')
  def _save_button_clicked(self, *events):
    ev = events[-1]
    config.relay.set_totalActiveMods(float(self.activeModifiers.text))
    config.relay.set_timePerModifier(float(self.timePerModifier.text))
    config.relay.set_softmaxFactor(self.softmaxFactor.value)
    config.relay.set_announceCandidates(self.announceCandidates.checked)
    config.relay.set_announceWinner(self.announceWinner.checked)
    config.relay.set_shouldSave(True)
    self.successLabel.set_text('Saved!')

  @flx.reaction('resetButton.pointer_click')
  def _reset_button_clicked(self, *events):
    ev = events[-1]
    config.relay.resetSoftmax()
    self.successLabel.set_text('Reset!')
