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
    
    label_style = "text-align:right"
    field_style = "background-color:#BBBBBB;text-align:left"
    
    with flx.VBox():
      self.game_name = flx.Label(style="text-align:center", text=f'Game: {config.relay.game_name}', wrap=1)
      with flx.HSplit():
        with flx.VBox(flex=1):
          with flx.GroupWidget(title='Modifier Selection'):
            with flx.HBox():
              with flx.VBox():
                flx.Label(style=label_style, text="Active modifiers:")
                flx.Label(style=label_style, text="Time per modifier (s):")
                flx.Label(style=label_style, text="Chance of repeat modifier:")
                flx.Label(style=label_style, text="Clear modifier-use history:")
              with flx.VBox():
                self.num_active_mods = flx.LineEdit(style=field_style, text=str(config.relay.num_active_mods))
                self.time_per_modifier = flx.LineEdit(style=field_style, text=str(config.relay.time_per_modifier))
                self.softmax_factor = flx.Slider(min=1, max=100, step=1, value=config.relay.softmax_factor)
                self.reset_button = flx.Button(text="Reset")
              flx.Widget(flex=1)
        
          with flx.GroupWidget(flex=0, title='Modifier Voting'):
            with flx.VBox():
              with flx.HBox():
                with flx.VBox():
                  flx.Label(style=label_style, text="Vote options:")
                  flx.Label(style=label_style, text="Voting method:")
                with flx.VBox():
                  self.vote_options = flx.LineEdit(style=field_style, text=str(config.relay.vote_options))
                  self.voting_type = flx.ComboBox(options=['Proportional','Majority','DISABLE'], selected_key='Proportional')
                flx.Widget(flex=1)
              self.announce_candidates = flx.CheckBox(text="Announce candidates in chat", checked=config.relay.announce_candidates, style="text-align:left")
              self.announce_winner = flx.CheckBox(text="Announce winner in chat", checked=config.relay.announce_winner, style="text-align:left")
              flx.Widget(flex=1)
          with flx.GroupWidget(flex=0, title='Modifier Redemption'):
            with flx.HBox():
              with flx.VBox():
                self.bits_redemption = flx.CheckBox(text='Allow bits redemptions', checked=config.relay.bits_redemption, style="text-align:left")
                self.points_redemption = flx.CheckBox(text='Channel points redemption', checked=config.relay.points_redemption, style="text-align:left")
                self.raffles = flx.CheckBox(text='Conduct raffles', checked=config.relay.raffles, style="text-align:left")
              with flx.VBox():
                flx.Label(style=label_style, text='Bits per mod credit:')
                flx.Label(style=label_style, text='Points per mod credit:')
                flx.Label(style=label_style, text='Redemption Cooldown (s):')              
              with flx.VBox():
                self.bits_per_credit = flx.LineEdit(style=field_style, text=str(config.relay.bits_per_credit))
                self.points_per_credit = flx.LineEdit(style=field_style, text=str(config.relay.points_per_credit))
                self.redemption_cooldown = flx.LineEdit(style=field_style, text=str(config.relay.redemption_cooldown))
              flx.Widget(flex=1)
        with flx.VBox(flex=1):
          flx.Label(style="text-align:left", text='Available Modifiers')
          self.available_modifiers = flx.TreeWidget(flex=1, max_selected=1, minsize=(300,300))
          with flx.HBox():
            with flx.VBox():
              flx.Label(text='Description:')
              flx.Label(text='Groups')
            with flx.VBox(flex=2):
              self.mod_description = flx.LineEdit(flex=2)
              self.mod_groups = flx.LineEdit(flex=2)
            flx.Widget(flex=1)
        # End of HSplit
      with flx.HBox():
        self.save_button = flx.Button(text="Save")
        self.restore_button = flx.Button(text="Restore")
        self.status_label = flx.Label(flex=1, style="text-align:center", text='')
        flx.Widget(flex=1)
      flx.Widget(flex=1)

  # Validation when changed
  @flx.reaction('num_active_mods.text')
  def _nummods_changed(self, *events):
    nmods = self.validate_int(self.num_active_mods.text, 1, 10, 'active_modifiers')
    self.num_active_mods.set_text(str(nmods))

  @flx.reaction('time_per_modifier.text')
  def _modtime_changed(self, *events):
    msg = ''
    modtime = config.relay.get_attribute('modifier_time')
    try:
      modtime = float(self.time_per_modifier.text)
      if modtime < 0:
        modtime = 30.0
        msg = "Do you think you're The Doctor? No negative times."
      elif modtime > 3600.0:
        modtime = 3600.0
        msg = "Maximum time for mods is 1 hour."
    except ValueError:
      msg = 'Value must be a number'
    if msg:
      self.time_per_modifier.set_text(str(modtime))
    # Report any errors or clear the field
    self.status_label.set_text(msg)

  @flx.reaction('vote_options.text')
  def _voteoptions_changed(self, *events):
    options = self.validate_int(self.vote_options.text, 2, 5, 'vote_options')
    self.vote_options.set_text(str(options))

  @flx.reaction('bits_per_credit.text')
  def _bitspercredit_changed(self, *events):
    bits = self.validate_int(self.bits_per_credit.text, minval=1, fallback='bits_per_credit')
    self.bits_per_credit.set_text(str(bits))

  @flx.reaction('points_per_credit.text')
  def _pointspercredit_changed(self, *events):
    points = self.validate_int(self.points_per_credit.text, minval=1, fallback='points_per_credit')
    self.points_per_credit.set_text(str(points))

  @flx.reaction('redemption_cooldown.text')
  def _cooldown_changed(self, *events):
    cooldown = self.validate_int(self.redemption_cooldown.text, minval=0, fallback='redemption_cooldown')
    self.redemption_cooldown.set_text(str(cooldown))

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
      self.status_label.set_text('')
    else:
      if minval is None and maxval is None:      
        self.status_label.set_text(f"Enter an integer.")
      elif minval is None:
        self.status_label.set_text(f"Enter an integer less than or equal to {maxval}.")
      elif maxval is None:
        self.status_label.set_text(f"Enter an integer greater than or equal to {minval}.")
      else:
        self.status_label.set_text(f"Enter an integer between {minval} and {maxval}.")
    return value

  @flx.reaction('save_button.pointer_click')
  def _save_button_clicked(self, *events):
    need_save = False
    if (config.relay.time_per_modifier != int(self.time_per_modifier.text)):
      config.relay.set_time_per_modifier(int(self.time_per_modifier.text))
      need_save = True
    if (config.relay.softmax_factor != self.softmax_factor.value):
      config.relay.set_softmax_factor(self.softmax_factor.value)
      need_save = True
    if (config.relay.vote_options != int(self.vote_options.text)):
      config.relay.set_vote_options(int(self.vote_options.text))
      need_save = True
    if (config.relay.voting_type != self.voting_type):
      config.relay.set_voting_type(self.voting_type)
      need_save = True
    if (config.relay.announce_candidates != self.announce_candidates.checked):
      config.relay.set_announce_candidates(self.announce_candidates.checked)
      need_save = True
    if (config.relay.announce_winner != self.announce_winner.checked):
      config.relay.set_announce_winner(self.announce_winner.checked)
      need_save = True
    if (config.relay.bits_redemption != self.bits_redemption.checked):
      config.relay.set_bits_redemption(self.bits_redemption.checked)
      need_save = True
    if (config.relay.points_redemption != self.points_redemption.checked):
      config.relay.set_points_redemption(self.points_redemption.checked)
      need_save = True
    if (config.relay.raffles != self.raffles.checked):
      config.relay.set_raffles(self.raffles.checked)
      need_save = True
    if (config.relay.bits_per_credit != self.bits_per_credit.text):
      config.relay.set_bits_per_credit(int(self.bits_per_credit.text))
      need_save = True
    if (config.relay.points_per_credit != self.points_per_credit.text):
      config.relay.set_points_per_credit(int(self.points_per_credit.text))
      need_save = True
    if (config.relay.redemption_cooldown != self.redemption_cooldown.text):
      config.relay.set_redemption_cooldown(int(self.redemption_cooldown.text))
    if need_save:
      config.relay.set_need_save(True)
      self.status_label.set_text('Settings updated')
    else:
      self.status_label.set_text('No settings were changed')

  @flx.reaction('restore_button.pointer_click')
  def _default_button_clicked(self, *events):
    config.relay.resetAll()
    self.status_label.set_text('Reset to saved values')

  @flx.reaction('reset_button.pointer_click')
  def _reset_button_clicked(self, *events):
    ev = events[-1]
    config.relay.reset_softmax()
    self.status_label.set_text('Modifier history reset')
