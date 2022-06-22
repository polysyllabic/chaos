# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The ChaosRelay class provides a model-view connector. It's the central repository for data that
  different components need to share.

  The relay is initialized once and makes heavy use of flexx properties. It is safe
"""
import logging
import json
import math
import copy
import numpy as np
from pathlib import Path
from flexx import flx
from twitchbot import BaseBot, cfg

# We use these values when we can't read a value from the json file, typically because it hasn't
# been created yet.
chaosDefaults = {
  'active_modifiers': 3,
  'modifier_time': 180.0,
  'softmax_factor': 33,
  'vote_options': 3,
  'voting_type': 'Proportional',
  'chat_rate': 0.67,
  'pi_host': "192.168.1.232",
  'listen_port': 5556,
  'talk_port': 5555,
  'announce_mods': False,
  'ui_rate': 20.0,
  'ui_port': 80,
  'announce_candidates': False,
  'announce_winner': False,
  'bits_redemptions': False,
  'bits_per_credit': 100,
  'points_redemptions': False,
  'points_per_credit': 20000,
  'raffles': False,
  'redemption_cooldown': 600,
  'nick': "your_bot",
  'oauth': "oauth:",
  'owner': "bot_owner",
  'channel': 'your_channel'
}

botKeys = {
  'nick', 'oauth', 'owner'
}

class ChaosRelay(flx.Component):
  keep_going = True
  valid_data = False
  bot_connected = False
  chatbot: BaseBot

  modifier_data = {"1":{"desc":"","groups":""}, "2":{"desc":"","groups":""}, "3":{"desc":"","groups":""},
                  "4":{"desc":"","groups":""}, "5":{"desc":"","groups":""}, "6":{"desc":"","groups":""}}
  all_mods = []
  voted_users = []

  # Model-View bridge:
  game_name = flx.StringProp(settable=True)
  game_errors = flx.IntProp(settable=True)

  vote_time = flx.FloatProp(settable=True)
  mod_times = flx.ListProp(settable=True)
  votes = flx.ListProp(settable=True)

  candidate_mods = flx.ListProp(settable=True)
  active_mods = flx.ListProp(settable=True)
  paused = flx.BoolProp(settable=True)
  paused_bright = flx.BoolProp(settable=True)
  connected = flx.BoolProp(settable=True)
  connected_bright = flx.BoolProp(settable=True)
  
  # Chaos Settings:
  num_active_mods = flx.IntProp(settable=True)
  time_per_modifier = flx.FloatProp(settable=True)
  softmax_factor = flx.IntProp(settable=True)
  vote_options = flx.IntProp(settable=True)
  voting_type = flx.EnumProp(['Proportional', 'Majority', 'DISABLED'], 'Proportional', settable=True)
  bits_redemption = flx.BoolProp(settable=True)
  bits_per_credit = flx.IntProp(settable=True)
  points_redemption = flx.BoolProp(settable=True)
  points_per_credit = flx.IntProp(settable=True)
  raffles = flx.BoolProp(settable=True)
  redemption_cooldown = flx.IntProp(settable=True)

  # Chat Bot Configuration  
  bot_name = flx.StringProp(settable=True)
  bot_oauth = flx.StringProp(settable=True)
  channel_name = flx.StringProp(settable=True)
  chat_rate = flx.FloatProp(settable=True)

  # Engine Settings
  pi_host = flx.StringProp(settable=True)
  listen_port = flx.IntProp(settable=True)
  talk_port = flx.IntProp(settable=True)

  # User Interface Settings      
  announce_candidates = flx.BoolProp(settable=True)
  announce_winner = flx.BoolProp(settable=True)
  ui_rate = flx.FloatProp(settable=True)
  ui_port = flx.IntProp(settable=True)
  
  need_save = flx.BoolProp(settable=True)

  def init(self, configfile: Path):
    self.chaos_config = {}
    self.load_settings(configfile)
    self.botConfig = {}
    self.reset_all()

  def get_attribute(self, key):
    if key in self.chaos_config:
      return self.chaos_config[key]
    # Nothing found, return default setting if there is one
    if key in chaosDefaults:
      self.chaos_config[key] = chaosDefaults[key]
      return chaosDefaults[key]
    logging.error(f"No default for '{key}'")
    return None

  def load_settings(self, configfile: Path):
    self.config_file = configfile.resolve()
    try:
      logging.info(f"Initializing settings from {self.config_file}")
      with open(self.config_file) as json_data_file:
        self.chaos_config = json.load(json_data_file)
    except Exception as e:
      logging.info(f"Could not read settings in {self.config_file}. Using defaults")
      self.chaos_config = {}

  @flx.reaction('need_save')
  def save_settings(self, *events):
    for ev in events:
      if ev.new_value == True:
        logging.info(f"Saving settings to {self.config_file}")
        with open(self.config_file, 'w') as outfile:
          json.dump(self.chaos_config, outfile)
        # save the bot's settings
        logging.info("Saving bot configuration")
        cfg.save()
        self.set_need_save(False)

  @flx.action
  def reset_all(self):
    self.set_num_active_mods(self.get_attribute('active_modifiers'))
    self.set_time_per_modifier(self.get_attribute('modifier_time'))
    self.set_vote_options(self.get_attribute('vote_options'))
    self.set_voting_type(self.get_attribute('voting_type'))
    self.set_softmax_factor(self.get_attribute('softmax_factor'))
    self.set_bot_name(self.get_attribute('nick'))
    self.set_bot_oauth(self.get_attribute('oauth'))
    self.set_channel_name(self.get_attribute('channel'))
    self.set_chat_rate(self.get_attribute('chat_rate'))
    self.set_pi_host(self.get_attribute('pi_host'))
    self.set_listen_port(self.get_attribute('listen_port'))
    self.set_talk_port(self.get_attribute('talk_port'))
    self.set_announce_candidates(self.get_attribute('announce_candidates'))
    self.set_announce_winner(self.get_attribute('announce_winner'))
    self.set_bits_redemption(self.get_attribute('bits_redemptions'))
    self.set_bits_per_credit(self.get_attribute('bits_per_credit'))
    self.set_points_redemption(self.get_attribute('points_redemptions'))
    self.set_points_per_credit(self.get_attribute('points_per_credit'))
    self.set_raffles(self.get_attribute('raffles'))
    self.set_redemption_cooldown(self.get_attribute('redemption_cooldown'))
    self.set_ui_rate(self.get_attribute('ui_rate'))
    self.set_ui_port(self.get_attribute('ui_port'))

    self.set_game_name("NONE")
    self.set_game_errors(0)
    self.set_vote_time(0.5)
    self.set_paused(True)
    self.set_paused_bright(True)
    self.set_connected(False)
    self.set_connected_bright(True)
    self.set_mod_times([0.0]*self.get_attribute('active_modifiers'))
    self.set_active_mods([""]*self.get_attribute('active_modifiers'))
    self.set_votes([0]*self.get_attribute('vote_options'))
    self.set_candidate_mods([""]*self.get_attribute('vote_options'))
    
    
  def time_per_vote(self) -> float:
    return self.get_attribute('modifier_time')/float(self.get_attribute('active_modifiers'))

  @flx.action
  def update_vote_time(self, time):
    self.set_vote_time(time/self.time_per_vote())

  @flx.action
  def initialize_game(self, gamedata: dict):
    logging.info(f"Setting game data for {gamedata['game']}")
    self.set_game_name(gamedata['game'])
    self.set_game_errors(int(gamedata['errors']))
    self.set_num_active_mods(int(gamedata["nmods"]))
    self.set_time_per_modifier(float(gamedata["modtime"]))

    logging.debug(f"Initializing modifier data:")
    self.modifier_data = {}
    for mod in gamedata['mods']:
      mod_name = mod['name']
      self.modifier_data[mod_name] = {}
      self.modifier_data[mod_name]['desc'] = mod['desc']
      self.modifier_data[mod_name]['groups'] = mod['groups']
      self.modifier_data[mod_name]
    self.all_mods = list(self.modifier_data.keys())
    self.reset_softmax()
    self.valid_data = True

  def sleep_time(self):
    return 1.0 / self.get_attribute('ui_rate')

  @flx.reaction('num_active_mods', 'time_per_modifier')
  def reset_current_mods(self, *events):
    self.set_mod_times([0.0]*self.num_active_mods)
    self.set_active_mods([""]*self.num_active_mods)

  @flx.reaction('vote_options')
  def reset_vote_options(self, *events):
    self.set_votes([0]* self.vote_options)
    self.set_candidate_mods([""]*self.vote_options)
    self.voted_users = []

  
  @flx.action
  def tally_vote(self, index: int, user: str):
    if index >= 0 and index < self.get_attribute('vote_options'):
      if not user in self.voted_users:
        self.voted_users.append(user)
        votes_counted = list(self.votes)
        votes_counted[index] += 1
        self.set_votes(votes_counted)
        logging.debug(f'User {user} voted for choice {index+1}, which now has {votes_counted[index]} votes')
      else:
        logging.debug(f'User {user} has already voted')
    else:
      logging.debug(f'Received an out of range vote ({index+1})')

  @flx.action
  def decrement_mod_times(self, dTime):
    time_remaining = list(self.mod_times)
    delta = dTime/self.time_per_modifier
    time_remaining[:] = [t - delta for t in time_remaining]
    self.set_mod_times(time_remaining)


  def reset_softmax(self):
    logging.info("Resetting SoftMax!")
    for mod in self.modifier_data:
      self.modifier_data[mod]["count"] = 0

  def get_softmax_divisor(self, data):
    # determine the sum for the softmax divisor:
    divisor = 0
    for key in data:
      divisor += data[key]["contribution"]
    return divisor

  def update_softmax_probabilities(self, data):
    for mod in data:
      data[mod]["contribution"] = math.exp(self.modifier_data[mod]["count"] *
          math.log(float(self.softmax_factor)/100.0))
    divisor = self.get_softmax_divisor(data)
    for mod in data:
      data[mod]["p"] = data[mod]["contribution"]/divisor
      
  def update_softmax(self, new_mod):
    if new_mod in self.modifier_data:
      self.modifier_data[new_mod]["count"] += 1        
      # update all probabilities:
      self.update_softmax_probabilities(self.modifier_data)
    else:
      logging.error(f"Updating SoftMax: modifier '{new_mod}' not in mod list")

  def get_new_voting_pool(self):
    # Ignore currently active mods:
    inactive_mods = set(np.setdiff1d(self.all_mods, list(self.active_mods)))
        
    candidates = []
    for k in range(self.vote_options):
      # build a list of contributor for this selection:
      votableTracker = {}
      for mod in inactive_mods:
        votableTracker[mod] = copy.deepcopy(self.modifier_data[mod])
              
      # Calculate the softmax probablities (must be done each time):
      self.update_softmax_probabilities(votableTracker)
      # make a decision:
      theChoice = np.random.uniform(0,1)
      selectionTracker = 0
      for mod in votableTracker:
        selectionTracker += votableTracker[mod]["p"]
        if selectionTracker > theChoice:
          candidates.append(mod)
          inactive_mods.remove(mod)  #remove this to prevent a repeat
          break
    
    if logging.getLogger().isEnabledFor(logging.DEBUG):
      logging.debug("New Voting Round:")
      for mod in candidates:
        if "p" in self.modifier_data[mod]:
          logging.debug(f" - {self.modifier_data[mod]['p']*100.0:0.2f}% {mod}")
        else:
          logging.debug(f" - 0.00% {mod}")

    self.set_candidate_mods(candidates)
    # Reset votes since there is a new voting pool
    self.set_votes([0]*self.get_attribute('vote_options'))


  @flx.action
  def replace_mod(self, new_mod):
    self.update_softmax(new_mod)
    mods = list(self.active_mods)
    # Find the oldest mod
    times = list(self.mod_times)
    finished_mod = times.index(min(times))
    mods[finished_mod] = new_mod
    times[finished_mod] = 1.0
    self.set_mod_times(times)
    self.set_active_mods(mods)
    self.get_new_voting_pool()


  @flx.reaction('game_name')
  def on_game_name(self, *events):
    for ev in events:
      self.update_game_name(ev.new_value)

  @flx.reaction('game_errors')
  def on_game_errors(self, *events):
    for ev in events:
      self.update_game_errors(ev.new_value)

  # TODO: If the number of active mods changes, we should reset the list of active and candidate mods
  @flx.reaction('num_active_mods')
  def on_num_active_mods(self, *events):
    for ev in events:
      self.update_num_active_mods(ev.new_value)

  @flx.reaction('vote_time')
  def on_vote_time(self, *events):
    for ev in events:
      self.update_vote_time(ev.new_value)
      
  @flx.reaction('mod_times')
  def on_mod_times(self, *events):
    for ev in events:
      self.update_mod_times(ev.new_value)
      
  @flx.reaction('votes')
  def on_votes(self, *events):
    for ev in events:
      self.update_votes(ev.new_value)
      
  @flx.reaction('candidate_mods')
  def on_candidate_mods(self, *events):
    for ev in events:
      self.update_candidate_mods(ev.new_value)
      
  @flx.reaction('active_mods')
  def on_active_mods(self, *events):
    for ev in events:
      self.update_active_mods(ev.new_value)
      
  @flx.reaction('time_per_modifier')
  def on_time_per_modifier(self, *events):
    for ev in events:
      self.chaos_config["modifier_time"]  = ev.new_value
      
  @flx.reaction('softmax_factor')
  def on_softmax_factor(self, *events):
    for ev in events:
      self.chaos_config["softmax_factor"]  = ev.new_value
      
  @flx.reaction('paused')
  def on_paused(self, *events):
    for ev in events:
      self.update_paused(ev.new_value)
      
  @flx.reaction('paused_bright')
  def on_paused_bright(self, *events):
    for ev in events:
      self.update_paused_bright(ev.new_value)
      
  @flx.reaction('connected')
  def on_connected(self, *events):
    for ev in events:
      self.update_connected(ev.new_value)
      
  @flx.reaction('connected_bright')
  def on_connected_bright(self, *events):
    for ev in events:
      self.update_connected_bright(ev.new_value)
      
  @flx.action
  def change_bot_name(self, new_value):
    logging.debug(f'Setting bot nick to {new_value}')
    self.chaos_config["nick"] = new_value
    cfg['nick'] = new_value
    self.set_bot_name(new_value)
      
  @flx.action
  def change_bot_oauth(self, new_value):
    logging.debug('Changing oauth')
    self.chaos_config["oauth"] = new_value
    cfg['oauth'] = new_value
    self.set_bot_oauth(new_value)
  
  @flx.action
  def change_channel_name(self, new_value):
    logging.debug(f'Changing channel to {new_value}')
    self.chaos_config["channel"] = new_value
    cfg['owner'] = new_value
    cfg['channels'] = [new_value]
    self.set_channel_name(new_value)

  @flx.reaction('announce_winner')
  def on_announce_winner(self, *events):
    for ev in events:
      self.chaos_config["announce_winner"] = ev.new_value
                        
  @flx.reaction('ui_rate')
  def on_ui_rate(self, *events):
    for ev in events:
      self.chaos_config["ui_rate"]  = ev.new_value
      
  @flx.reaction('ui_port')
  def on_ui_port(self, *events):
    for ev in events:
      self.chaos_config["ui_port"]  = ev.new_value

  @flx.action
  def change_pi_host(self, new_value):
    logging.debug(f'Changing pi host address to {new_value}')
    self.chaos_config["pi_host"]  = new_value
    self.set_pi_host(new_value)

  @flx.action
  def change_listen_port(self, new_value):
    logging.debug(f'Changing chaos engine listen port to {new_value}')
    self.chaos_config["listen_port"]  = new_value
    self.set_listen_port(new_value)

  @flx.action
  def change_talk_port(self, new_value):
    logging.debug(f'Changing chaos engine talk port to {new_value}')
    self.chaos_config["talk_port"]  = new_value
    self.set_talk_port(new_value)

  # Emitters to relay events to all participants.
    
  @flx.emitter
  def update_game_name(self, value):
    return dict(value=value)

  @flx.emitter
  def update_game_errors(self, value):
    return dict(value=value)

  @flx.emitter
  def update_num_active_mods(self, value):
    return dict(value=value)

  @flx.emitter
  def update_vote_time(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_mod_times(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_votes(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_candidate_mods(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_active_mods(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_paused(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_paused_bright(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_connected(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_connected_bright(self, value):
    return dict(value=value)
    
  @flx.emitter
  def update_pi_host(self, value):
    return dict(value=value)

  @flx.emitter
  def update_listen_port(self, value):
    return dict(value=value)

  @flx.emitter
  def update_talk_port(self, value):
    return dict(value=value)

