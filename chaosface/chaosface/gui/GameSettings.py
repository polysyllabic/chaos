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
    
    self.need_save = False
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
                flx.Label(style=styleLabel, text="Clear modifier-use history:")
              with flx.VBox():
                self.activeModifiers = flx.LineEdit(style=styleField, text=str(config.relay.numActiveMods))
                self.timePerModifier = flx.LineEdit(style=styleField, text=str(config.relay.timePerModifier))
                self.softmaxFactor = flx.Slider(min=1, max=100, step=1, value=config.relay.softmaxFactor)
                self.resetButton = flx.Button(text="Reset")
              flx.Widget(flex=1)
        
          with flx.GroupWidget(flex=0, title='Modifier Voting'):
            with flx.VBox():
              with flx.HBox():
                with flx.VBox():
                  flx.Label(style=styleLabel, text="Vote options:")
                  flx.Label(style=styleLabel, text="Voting method:")
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
                self.bitsPerCredit = flx.LineEdit(style=styleField, text=str(config.relay.bitsPerCredit))
                self.pointsPerCredit = flx.LineEdit(style=styleField, text=str(config.relay.pointsPerCredit))
                self.redemptionCooldown = flx.LineEdit(style=styleField, text=str(config.relay.redemptionCooldown))
              flx.Widget(flex=1)
        with flx.VBox(flex=1):
          flx.Label(style="text-align:left", text='Available Modifiers')
          self.availableModifiers = flx.TreeWidget(flex=1, max_selected=1, minsize=(300,300))
          with flx.HBox():
            with flx.VBox():
              flx.Label(text='Description:')
              flx.Label(text='Groups')
            with flx.VBox(flex=2):
              self.modDescription = flx.LineEdit(flex=2)
              self.modGroups = flx.LineEdit(flex=2)
            flx.Widget(flex=1)
        # End of HSplit
      with flx.HBox():
        self.saveButton = flx.Button(text="Save")
        self.restoreButton = flx.Button(text="Restore")
        self.successLabel = flx.Label(flex=1, style="text-align:center", text='')
        flx.Widget(flex=1)
      flx.Widget(flex=1)

  # Validation when changed
  @flx.reaction('activeModifiers.text')
  def _nummods_changed(self, *events):
    nmods = self.validate_int(self.activeModifiers.text, 1, 10, 'active_modifiers')
    self.activeModifiers.text = str(nmods)

  @flx.reaction('timePerModifier.text')
  def _modtime_changed(self, *events):
    msg = ''
    modtime = config.relay.get_attribute('modifier_time')
    try:
      modtime = float(self.timePerModifier.text)
      if modtime < 0:
        modtime = 30.0
        msg = "Do you think you're The Doctor? No negative times."
      elif modtime > 3600.0:
        modtime = 3600.0
        msg = "Maximum time for mods is 1 hour."
    except ValueError:
      msg = 'Value must be a number'
    if msg:
      self.timePerModifier.text = str(modtime)
    # Report any errors or clear the field
    self.successLabel.set_text(msg)

  @flx.reaction('voteOptions.text')
  def _voteoptions_changed(self, *events):
    options = self.validate_int(self.voteOptions.text, 2, 5, 'vote_options')
    self.voteOptions.text = str(options)

  @flx.reaction('bitsPerCredit.text')
  def _bitpercredit_changed(self, *events):
    bits = self.validate_int(self.bitsPerCredit.text, minval=1, fallback='bits_per_credit')
    self.bitsPerCredit.text = str(bits)

  @flx.reaction('pointsPerCredit.text')
  def _pointspercredit_changed(self, *events):
    points = self.validate_int(self.pointsPerCredit.text, minval=1, fallback='points_per_credit')
    self.pointsPerCredit.text = str(points)

  @flx.reaction('redemptionCooldown.text')
  def _cooldown_changed(self, *events):
    cooldown = self.validate_int(self.redemptionCooldown.text, minval=0, fallback='redemption_cooldown')
    self.redemptionCooldown.text = str(cooldown)

  def validate_int(self, field: str, minval=None, maxval=None, fallback=None):
    good = True    
    value = config.relay.get_attribute(fallback) if fallback is not None else 0
    if not field.isdigit():
      good = False      
    else:
      value = int(field)
      if minval is not None and value < minval:
        value = minval
        good = False
      elif maxval is not None and value > maxval:
        value = maxval
        good = False
    if good:
      self.successLabel.set_text('')
    else:
      if minval is None and maxval is None:      
        self.successLabel.set_text(f"Enter an integer.")
      elif minval is None:
        self.successLabel.set_text(f"Enter an integer less than or equal to {maxval}.")
      elif maxval is None:
        self.successLabel.set_text(f"Enter an integer greater than or equal to {minval}.")
      else:
        self.successLabel.set_text(f"Enter an integer between {minval} and {maxval}.")
    return value
    


  @flx.reaction('saveButton.pointer_click')
  def _save_button_clicked(self, *events):
    ev = events[-1]
    need_save = False
    if (config.relay.timePerModifier != int(self.timePerModifier.text)):
      config.relay.set_timePerModifier(int(self.timePerModifier.text))
      need_save = True
    if (config.relay.softmaxFactor != self.softmaxFactor.value):
      config.relay.set_softmaxFactor(self.softmaxFactor.value)
      need_save = True
    if (config.relay.voteOptions != int(self.voteOptions.text)):
      config.relay.set_voteOptions(int(self.voteOptions.text))
      need_save = True
    if (config.relay.votingType != self.votingType):
      config.relay.set_votingType(self.votingType)
      need_save = True
    if (config.relay.announceCandidates != self.announceCandidates.checked):
      config.relay.set_announceCandidates(self.announceCandidates.checked)
      need_save = True
    if (config.relay.announceWinner != self.announceWinner.checked):
      config.relay.set_announceWinner(self.announceWinner.checked)
      need_save = True
    if (config.relay.bitsRedemption != self.bitsRedemption.checked):
      config.relay.set_bitsRedemption(self.bitsRedemption.checked)
      need_save = True
    if (config.relay.pointsRedemption != self.pointsRedemption.checked):
      config.relay.set_pointsRedemption(self.pointsRedemption.checked)
      need_save = True
    if (config.relay.raffles != self.raffles.checked):
      config.relay.set_raffles(self.raffles.checked)
      need_save = True
    if (config.relay.bitsPerCredit != self.bitsPerCredit.text):
      config.relay.set_bitsPerCredit(int(self.bitsPerCredit.text))
      need_save = True
    if (config.relay.pointsPerCredit != self.pointsPerCredit.text):
      config.relay.set_pointsPerCredit(int(self.pointsPerCredit.text))
      need_save = True
    if (config.relay.redemptionCooldown != self.redemptionCooldown.text):
      config.relay.set_redemptionCooldown(int(self.redemptionCooldown.text))
    if self.need_save:
      config.relay.saveSettings()
      self.successLabel.set_text('Settings saved')
      self.need_save = False
    else:
      self.successLabel.set_text('No settings were changed')

  @flx.reaction('defaultButton.pointer_click')
  def _default_button_clicked(self, *events):
    config.relay.resetAll()
    self.successLabe.set_text('Reset to saved values')

  @flx.reaction('resetButton.pointer_click')
  def _reset_button_clicked(self, *events):
    ev = events[-1]
    config.relay.resetSoftmax()
    self.successLabel.set_text('Modifier history reset')
