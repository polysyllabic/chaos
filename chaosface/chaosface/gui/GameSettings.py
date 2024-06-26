# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View for chaos settings that control the gameplay
"""
from flexx import flx

import chaosface.config.globals as config


class GameSettings(flx.PyWidget):
  def init(self):
    super().init()
    self.mod_items = []
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
                  flx.Label(style=label_style, text="Voting cycle:")
                  flx.Label(style=label_style, text="Time to vote (s):")
                  flx.Label(style=label_style, text="Time between votes (s):")
                with flx.VBox():
                  self.vote_options = flx.LineEdit(style=field_style, text=str(config.relay.vote_options))
                  self.voting_type = flx.ComboBox(options=['Proportional','Majority','Authoritarian'],
                                                  selected_key='Proportional')
                  self.voting_cycle = flx.ComboBox(options=['Continuous','Interval', 'Random', 'Triggered', 'DISABLED'],
                                                  selected_key='Continuous')
                  self.vote_time = flx.LineEdit(style=field_style, text=str(config.relay.vote_time))
                  self.vote_delay = flx.LineEdit(style=field_style, text=str(config.relay.vote_delay))
                                                
                flx.Widget(flex=1)
              self.announce_candidates = flx.CheckBox(text="Announce candidates in chat", checked=config.relay.announce_candidates, style="text-align:left")
              self.announce_winner = flx.CheckBox(text="Announce winner in chat", checked=config.relay.announce_winner, style="text-align:left")
              self.announce_active = flx.CheckBox(text="Announce active mods in chat", checked=config.relay.announce_active, style="text-align:left")
              flx.Widget(flex=1)
          with flx.GroupWidget(flex=0, title='Modifier Redemption'):
            with flx.HBox():
              with flx.VBox():
                flx.Label(style=label_style, text='Redemption Cooldown (s):')              
                self.bits_redemptions = flx.CheckBox(text='Allow bits redemptions', checked=config.relay.bits_redemptions, style="text-align:left")
                self.points_redemptions = flx.CheckBox(text='Channel points redemption', checked=config.relay.points_redemptions, style="text-align:left")
                self.raffles = flx.CheckBox(text='Conduct raffles', checked=config.relay.raffles, style="text-align:left")
              with flx.VBox():
                self.redemption_cooldown = flx.LineEdit(style=field_style, text=str(config.relay.redemption_cooldown))
                flx.Label(style=label_style, text='Bits per mod credit:')
                flx.Label(style=label_style, text='Points Reward Title:')
                flx.Label(style=label_style, text='Default Raffle Time:')
              with flx.VBox():
                flx.Label(style=label_style, text=' ')
                self.bits_per_credit = flx.LineEdit(style=field_style, text=str(config.relay.bits_per_credit))
                self.points_reward_title = flx.LineEdit(style=field_style, text=str(config.relay.points_reward_title))
                self.raffle_time = flx.LineEdit(style=field_style, text=str(config.relay.raffle_time))
              with flx.VBox():
                flx.Label(style=label_style, text=' ')
                self.multiple_credits = flx.CheckBox(text='Allow multiple credits per cheer', checked=config.relay.multiple_credits, style="text-align:left")
                flx.Label(style=label_style, text=' ')
                flx.Label(style=label_style, text=' ')
        with flx.VBox(flex=1):
          self.available_mods = ModifierView()
        # End of HSplit
      with flx.HBox():
        self.save_button = flx.Button(text="Save")
        self.restore_button = flx.Button(text="Restore")
        self.status_label = flx.Label(flex=1, style="text-align:center", text='')
        flx.Widget(flex=1)
      flx.Widget(flex=1)

    self.available_mods.set_data(config.relay.modifier_data)

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
      elif modtime > 172800.0:
        modtime = 172800.0
        msg = "Maximum time for mods is 48 hours. Get some sleep!"
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

  @flx.reaction('vote_time.text')
  def _votetime_changed(self, *events):
    msg = ''
    votetime = config.relay.get_attribute('vote_time')
    try:
      votetime = float(self.vote_time.text)
      if votetime <= 0:
        votetime = 60.0
        msg = "Vote time must be greater than 0."
      elif votetime > 3600.0:
        votetime = 3600.0
        msg = 'Maximum vote time is 1 hour'
    except ValueError:
      msg = 'Value must be a number'
    if msg:
      self.vote_time.set_text(str(votetime))
    # Report any errors or clear the field
    self.status_label.set_text(msg)

  @flx.reaction('vote_delay.text')
  def _votedelay_changed(self, *events):
    msg = ''
    votedelay = config.relay.get_attribute('vote_delay')
    try:
      votedelay = float(self.vote_delay.text)
      if votedelay < 0.0:
        votedelay = 0.0
        msg = "Vote delay cannot be negative."
      elif votedelay > 3600.0:
        votedelay = 3600.0
        msg = 'Maximum vote delay cannot be more than 1 hour'
    except ValueError:
      msg = 'Value must be a number'
    if msg:
      self.vote_time.set_text(str(votedelay))
    # Report any errors or clear the field
    self.status_label.set_text(msg)

  @flx.reaction('bits_per_credit.text')
  def _bitspercredit_changed(self, *events):
    bits = self.validate_int(self.bits_per_credit.text, minval=1, fallback='bits_per_credit')
    self.bits_per_credit.set_text(str(bits))

  @flx.reaction('raffle_time.text')
  def _raffletime_changed(self, *events):
    time = self.validate_int(self.raffle_time.text, minval=10, fallback='raffle_time')
    self.raffle_time.set_text(str(time))

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
    if (config.relay.num_active_mods != int(self.num_active_mods.text)):
      config.relay.set_num_active_mods(int(self.num_active_mods.text))
      need_save = True
    if (config.relay.time_per_modifier != float(self.time_per_modifier.text)):
      config.relay.set_time_per_modifier(float(self.time_per_modifier.text))
      need_save = True
    if (config.relay.softmax_factor != self.softmax_factor.value):
      config.relay.set_softmax_factor(self.softmax_factor.value)
      need_save = True
    if (config.relay.vote_options != int(self.vote_options.text)):
      temp = int(self.vote_options.text)
      config.relay.set_vote_options(temp)
      need_save = True
    if (config.relay.voting_type != self.voting_type.text):
      config.relay.set_voting_type(self.voting_type.text)
      need_save = True
    if (config.relay.announce_candidates != self.announce_candidates.checked):
      config.relay.set_announce_candidates(self.announce_candidates.checked)
      need_save = True
    if (config.relay.announce_winner != self.announce_winner.checked):
      config.relay.set_announce_winner(self.announce_winner.checked)
      need_save = True
    if (config.relay.bits_redemptions != self.bits_redemptions.checked):
      config.relay.set_bits_redemptions(self.bits_redemptions.checked)
      need_save = True
    if (config.relay.multiple_credits != self.multiple_credits.checked):
      config.relay.set_multiple_credits(self.multiple_credits.checked)
      need_save = True
    if (config.relay.bits_per_credit != int(self.bits_per_credit.text)):
      config.relay.set_bits_per_credit(int(self.bits_per_credit.text))
      need_save = True
    if (config.relay.points_redemptions != self.points_redemptions.checked):
      config.relay.set_points_redemptions(self.points_redemptions.checked)
      need_save = True
    if (config.relay.points_reward_title != self.points_reward_title.text):
      config.relay.set_points_reward_title(self.points_reward_title.text)
      need_save = True
    if (config.relay.raffles != self.raffles.checked):
      config.relay.set_raffles(self.raffles.checked)
      need_save = True
    if (config.relay.raffle_time != float(self.raffle_time.text)):
      config.relay.set_raffle_time(float(self.time_per_modifier.text))
      need_save = True
    if (config.relay.redemption_cooldown != float(self.redemption_cooldown.text)):
      config.relay.set_redemption_cooldown(float(self.redemption_cooldown.text))
      need_save = True
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
    config.relay.reset_softmax()
    self.status_label.set_text('Modifier history reset')


class ModifierView(flx.VBox):

  def init(self):
    with flx.HBox():
      flx.Label(style="text-align:left", text='Available Modifiers')
      self.enable_all = flx.Button(text='Enable All')
      self.disable_all = flx.Button(text='Disable All')
      flx.Label(flex=1, text=' ')
    with flx.TreeWidget(flex=1, max_selected=1, minsize=(300,300)) as self.tree:
      if len(config.relay.modifier_data) > 0:
        for mod, data in config.relay.modifier_data.items():
          self.mod_items.append(flx.TreeItem(text=data['name'], checked=data['active']))
        else:
          for i in range(4):
            self.mod_items.append(flx.TreeItem(text='Modifier ' + str(i+1), checked=False))
    self.debug_line = flx.Label(text='')

  @flx.action
  def set_data(self, data):
    # This is a brute-force way to do it. Update only what's necessary?
    # Remove old tree items
    for item in self.tree.children:
      item.set_parent(None)
      item.dispose()
    for mod, mod_data in data:
      with self.tree:
        flx.TreeItem(text=mod_data['name'], checked=mod_data['active'])

  @flx.reaction('enable_all.pointer_click')
  def _enable_all_button_clicked(self, *events):
    pass

  @flx.reaction('disable_all.pointer_click')
  def _disable_all_button_clicked(self, *events):
    pass
  
  @flx.reaction('tree.children**.checked')
  def on_tree_checked(self, *events):
    for ev in events:
      key = ev.source.text.lower()
      if key in config.relay.modifier_data:
        config.relay.modifier_data[key]['active'] = ev.new_data
        self.debug_line.set_text('Modifier ' + ev.source.text + ' active = ' + ev.new_data)
      else:
        self.debug_line.set_text('Fake Modifier ' + ev.source.text + ' active = ' + ev.new_data)

  