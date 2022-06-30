# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The ChaosRelay class provides a model-view connector. It's the central repository for data that
  different components need to share.

  The relay is initialized once and makes heavy use of flexx properties. It is safe to read these
  properties from other threads, but to set a property from another thread, you must schedule the
  routine to be executed by the main flexx loop with the command
  flx.loop.call_soon(<function>, *args). For example:

  ONLY DO THIS FROM THE FLEXX THREAD:  config.relay.set_num_active_mods(3)

  FROM OTHER THREADS: flx.loop.call_soon(config.relay.set_num_active_mods, 3)
"""
import logging
import json
import math
import copy
import numpy as np
import queue
from pathlib import Path
from flexx import flx
from twitchbot import BaseBot, cfg

_chaos_description = ("Twitch Controls Chaos lets chat interfere with a streamer playing a "
  "PlayStation game. It uses a Raspberry Pi to modify controller input based on gameplay "
  "modifiers that chat votes on. For a video explanation of how Chaos works, see here: "
  "https://www.twitch.tv/blegas78/clip/SmellyDepressedPancakeKappaPride-llz6ldXSKjSJLs9s")

_chaos_how_to_vote = ("Each cycle you can vote for one modifier. Type the number of the mod you "
  "prefer in chat. Votes with text other than the number will be ignored. Votes are counted while "
  "the game is paused. ")

_chaos_how_to_redeem = ("With a mod credit, apply a modifier of your choice immediately "
"(subject to a {cooldown}-second cooldown) with '!apply <mod name>' (ignored if in "
"cooldown), or queue the mod to run once the cooldown is over with '!queue <mod name>'")


# We use these values when we can't read a value from the json file, typically because it hasn't
# been created yet.
chaos_defaults = {
  'current_game': 'NONE',
  'active_modifiers': 3,
  'modifier_time': 180.0,
  'softmax_factor': 33,
  'vote_options': 3,
  'voting_type': 'Proportional',
  'announce_candidates': False,
  'announce_winner': False,
  'pi_host': '192.168.1.232',
  'listen_port': 5556,
  'talk_port': 5555,
  'obs_overlays': True,
  'use_gui': True,
  'ui_rate': 20.0,
  'ui_port': 80,
  'bits_redemptions': False,
  'bits_per_credit': 100,
  'multiple_credits': True,
  'points_redemptions': False,
  'points_reward_title': 'Chaos Credit',
  'raffles': False,
  'raffle_time': 60.0,
  'redemption_cooldown': 600.0,
  'client_id': 'uic681f91dtxl3pdfyqxld2yvr82r1',
  'nick': "your_bot",
  'oauth': "oauth:",
  'pubsub_oauth': 'oauth:',
  'owner': "bot_owner",
  'channel': 'your_channel',
  'info_cmd_cooldown': 5.0,
  'msg_chaos_description': _chaos_description,
  'msg_how_to_vote': _chaos_how_to_vote,
  'msg_proportional_voting': 'The chances of a mod winning are proportional to the number of votes it receives.',
  'msg_majority_voting': 'The mod with the most votes wins, and ties are broken by random selection.',
  'msg_no_voting': 'Voting is currently disabled.',
  'msg_how_to_redeem': _chaos_how_to_redeem,
  'msg_credit_methods': 'Get modifier credits in these ways: ',
  'msg_by_gift': 'You can also give another user one of your credits with !givecredit <username>',
  'msg_no_redemptions': 'The ability to earn modifier credits is currently disabled.',
  'msg_with_points': 'redeeming channel points',
  'msg_with_bits': 'donating bits (1 credit for {} bits in a single donation)',
  'msg_multiple_credits': 'every {}',
  'msg_no_multiple_credits': '{} or more',
  'msg_by_raffle': 'winning raffles',
  'msg_mod_not_found': "I can't find the modifier '{}'.",
  'msg_mod_not_active': "The mod '{}' is currently disabled",
  'msg_active_mods': 'The currently active mods are ',
  'msg_candidate_mods': 'You can currently vote for the following mods: ',
  'msg_in_cooldown': 'Cannot apply the mod yet. Use !queue to queue the mod for when the cooldown expires.',
  'msg_user_balance': '@{} You currently have {} modifier credits.',
  'msg_mod_applied': 'Modifier applied',
}

CONFIG_PATH = Path.cwd() / 'configs'
CHAOS_CONFIG_FILE = CONFIG_PATH / 'chaos_config.json'
CHAOS_MOD_FILE = CONFIG_PATH / 'chaos_mods.json'
CHAOS_CREDIT_FILE = CONFIG_PATH / 'chaos_credits.json'

class ChaosRelay(flx.Component):
  keep_going = True
  valid_data = False
  chatbot: BaseBot

  modifier_data = {}
  #all_mods = []
  enabled_mods = []
  voted_users = []
  active_keys = []
  candidate_keys = []
  force_mod:str = ''
  insert_cooldown = 0.0

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
  bits_redemptions = flx.BoolProp(settable=True)
  bits_per_credit = flx.IntProp(settable=True)
  multiple_credits = flx.BoolProp(settable=True)
  points_redemptions = flx.BoolProp(settable=True)
  points_reward_title = flx.StringProp(settable=True)
  raffles = flx.BoolProp(settable=True)
  raffle_time = flx.FloatProp(settable=True)
  redemption_cooldown = flx.IntProp(settable=True)

  # Chat Bot Configuration  
  channel_name = flx.StringProp(settable=True)
  bot_name = flx.StringProp(settable=True)
  bot_oauth = flx.StringProp(settable=True)
  pubsub_oauth = flx.StringProp(settable=True)
  chat_rate = flx.FloatProp(settable=True)

  # Engine Settings
  pi_host = flx.StringProp(settable=True)
  listen_port = flx.IntProp(settable=True)
  talk_port = flx.IntProp(settable=True)

  # User Interface Settings      
  announce_candidates = flx.BoolProp(settable=True)
  announce_winner = flx.BoolProp(settable=True)
  obs_overlays = flx.BoolProp(settable=True)
  ui_rate = flx.FloatProp(settable=True)
  ui_port = flx.IntProp(settable=True)
  
  need_save = flx.BoolProp(settable=True)
  new_game_data = flx.BoolProp(settable=True)


  def init(self):
    self.chaos_config = self.load_settings(CHAOS_CONFIG_FILE)
    # Check that all default keys are present
    need_save = False
    for k, v in chaos_defaults.items():
      if k not in self.chaos_config:
        self.chaos_config[k] = v
        need_save = True
    if need_save:
      self.save_config_info()

    self.old_mod_data = self.load_settings(CHAOS_MOD_FILE)
    self.user_credits = self.load_settings(CHAOS_CREDIT_FILE)
    self.modifier_data = {}
    self.reset_all()

  def get_attribute(self, key):
    if key in self.chaos_config:
      return self.chaos_config[key]
    # Nothing found, return default setting if there is one
    if key in chaos_defaults:
      self.chaos_config[key] = chaos_defaults[key]
      return chaos_defaults[key]
    logging.error(f"No default for '{key}'")
    return None

  def load_settings(self, configfile: Path):
    # Make sure the file and parent directories exist
    configfile.parents[0].mkdir(parents=True, exist_ok=True)
    configfile.touch(exist_ok=True)
    data = {}
    try:
      if (configfile.stat().st_size > 0):
        logging.info(f"Reading settings from {configfile.resolve()}")
        data = json.loads(configfile.read_bytes())
    except Exception as e:
      logging.warning(f"Error reading {configfile.resolve()}: {str(e)}")
    return data

  @flx.reaction('need_save')
  def save_settings(self, *events):
    logging.info(f"Saving settings to {CHAOS_CONFIG_FILE.resolve()}")    
    for ev in events:
      if ev.new_value == True:
        self.save_config_info()
        cfg.save()
        self.set_need_save(False)

  def save_config_info(self):
    logging.debug('Saving config file')
    with CHAOS_CONFIG_FILE.open('w') as outfile:
      json.dump(self.chaos_config, outfile, indent=2, sort_keys=True)

  def save_mod_info(self):
    with CHAOS_MOD_FILE.open('w') as modfile:
      json.dump(self.modifier_data, modfile, indent=2, sort_keys=True)

  def save_credit_info(self):
    with CHAOS_CREDIT_FILE.open('w') as creditfile:
      json.dump(self.user_credits, creditfile, indent=2, sort_keys=True)

  @flx.action
  def reset_all(self):
    self.set_game_name(self.get_attribute('current_game'))
    self.set_num_active_mods(self.get_attribute('active_modifiers'))
    self.set_time_per_modifier(self.get_attribute('modifier_time'))
    self.set_softmax_factor(self.get_attribute('softmax_factor'))
    self.set_vote_options(self.get_attribute('vote_options'))
    self.set_voting_type(self.get_attribute('voting_type'))
    self.set_announce_candidates(self.get_attribute('announce_candidates'))
    self.set_announce_winner(self.get_attribute('announce_winner'))
    self.set_pi_host(self.get_attribute('pi_host'))
    self.set_listen_port(self.get_attribute('listen_port'))
    self.set_talk_port(self.get_attribute('talk_port'))
    self.set_bot_name(self.get_attribute('nick'))
    self.set_bot_oauth(self.get_attribute('oauth'))
    self.set_pubsub_oauth(self.get_attribute('pubsub_oauth'))
    self.set_channel_name(self.get_attribute('channel'))
    self.set_bits_redemptions(self.get_attribute('bits_redemptions'))
    self.set_bits_per_credit(self.get_attribute('bits_per_credit'))
    self.set_multiple_credits(self.get_attribute('multiple_credits'))
    self.set_points_redemptions(self.get_attribute('points_redemptions'))
    self.set_points_reward_title(self.get_attribute('points_reward_title'))
    self.set_raffles(self.get_attribute('raffles'))
    self.set_raffle_time(self.get_attribute('raffle_time'))
    self.set_redemption_cooldown(self.get_attribute('redemption_cooldown'))
    self.set_ui_rate(self.get_attribute('ui_rate'))
    self.set_ui_port(self.get_attribute('ui_port'))

    self.set_game_errors(0)
    self.set_vote_time(0.5)
    self.set_paused(True)
    self.set_paused_bright(True)
    self.set_connected(False)
    self.set_connected_bright(True)
    self.set_mod_times([0.0]*self.get_attribute('active_modifiers'))
    self.set_active_mods(['']*self.get_attribute('active_modifiers'))
    self.set_votes([0]*self.get_attribute('vote_options'))
    self.set_candidate_mods(['']*self.get_attribute('vote_options'))

    self.active_keys = ['']*self.get_attribute('active_modifiers')
    self.candidate_keys = ['']*self.get_attribute('vote_options')

  def time_per_vote(self) -> float:
    return float(self.get_attribute('modifier_time'))/float(self.get_attribute('active_modifiers'))

  @flx.action
  def initialize_game(self, gamedata: dict):
    logging.info(f"Setting game data for {gamedata['game']}")
    self.set_game_name(gamedata['game'])
    self.set_game_errors(int(gamedata['errors']))
    self.set_num_active_mods(int(gamedata['nmods']))
    self.set_time_per_modifier(float(gamedata['modtime']))

    logging.debug(f"Initializing modifier data:")
    self.modifier_data = {}
    self.enabled_mods = []
    for mod in gamedata['mods']:
      mod_key = mod['name'].lower()
      self.modifier_data[mod_key] = {}
      self.modifier_data[mod_key]['name'] = mod['name']
      self.modifier_data[mod_key]['desc'] = mod['desc']
      self.modifier_data[mod_key]['groups'] = mod['groups']
      if mod_key in self.old_mod_data:
        self.modifier_data[mod_key]['active'] = self.old_mod_data[mod_key]['active'] 
      else:
        self.modifier_data[mod_key]['active'] = True
      if self.modifier_data[mod_key]['active']:
        self.enabled_mods.append(mod_key)
    self.reset_softmax()
    self.save_mod_info()
    self.set_new_game_data(True)
    self.valid_data = True

  def sleep_time(self):
    return 1.0/self.get_attribute('ui_rate')

  def mod_status(self, mod_key: str):
    """
    Returns 0 if not found, -1 if disabled, 1 if enabled
    """
    if mod_key in self.modifier_data:
      return 1 if self.modifier_data[mod_key]['active'] else -1
    else:
      return 0

  def get_mod_description(self, mod: str):
    key = mod.lower()
    try:
      return self.modifier_data[key]['desc']
    except:
      return self.get_attribute('msg_mod_not_found').format(mod)
  
  def get_balance(self, user: str):
    if user not in self.user_credits:
      return 0
    else:
      return self.user_credits[user]
  
  def step_balance(self, user: str, delta: int):
    balance = self.get_balance(user) + delta
    if balance < 0:
      balance = 0
    self.user_credits[user] = balance
    self.save_credit_info()
    return balance


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
      self.modifier_data[mod]['count'] = 0

  def get_softmax_divisor(self, data):
    # determine the sum for the softmax divisor:
    divisor = 0
    for key in data:
      divisor += data[key]['contribution']
    return divisor

  def update_softmax_probabilities(self, data):
    for mod in data:
      data[mod]['contribution'] = math.exp(self.modifier_data[mod]['count'] *
          math.log(float(self.softmax_factor)/100.0))
    divisor = self.get_softmax_divisor(data)
    for mod in data:
      data[mod]['p'] = data[mod]['contribution']/divisor
      
  def update_softmax(self, new_mod):
    if new_mod in self.modifier_data:
      self.modifier_data[new_mod]['count'] += 1        
      # update all probabilities:
      self.update_softmax_probabilities(self.modifier_data)
    else:
      logging.error(f"Updating SoftMax: modifier '{new_mod}' not in mod list")

  def get_new_voting_pool(self):
    # Ignore currently active mods:
    inactive_mods = set(np.setdiff1d(self.enabled_mods, list(self.active_keys)))

    self.candidate_keys = []
    candidates = []
    for k in range(self.vote_options):
      # build a list of contributor for this selection:
      votableTracker = {}
      for mod in inactive_mods:
        votableTracker[mod] = copy.deepcopy(self.modifier_data[mod])
              
      # Calculate the softmax probablities (must be done each time):
      self.update_softmax_probabilities(votableTracker)
      # make a decision:
      choice_threshold = np.random.uniform(0,1)
      selectionTracker = 0
      for mod in votableTracker:
        selectionTracker += votableTracker[mod]['p']
        if selectionTracker > choice_threshold:
          self.candidate_keys.append(mod)
          candidates.append(self.modifier_data[mod]['name'])
          inactive_mods.remove(mod)  #remove this to prevent a repeat
          break
    
    if logging.getLogger().isEnabledFor(logging.DEBUG):
      logging.debug("New Voting Round:")
      for mod in self.candidate_keys:
        if 'p' in self.modifier_data[mod]:
          logging.debug(f" - {self.modifier_data[mod]['p']*100.0:0.2f}% {mod}")
        else:
          logging.debug(f" - 0.00% {mod}")

    self.set_candidate_mods(candidates)
    # Reset votes and user-voted list, since there is a new voting pool
    self.set_votes([0]*self.get_attribute('vote_options'))
    self.voted_users = []

  def about_tcc(self):
    return self.get_attribute('msg_chaos_description')

  def explain_voting(self):
    if self.get_attribute('voting_type') == 'DISABLED':
      return self.get_attribute('msg_no_voting')
    if self.get_attribute('voting_type') == 'Proportional':
      msg = self.get_attribute('msg_proportional_voting')
    else:
      msg = self.get_attribute('msg_majority_voting')
    return self.get_attribute('msg_how_to_vote') + msg

  def explain_redemption(self):
    msg = self.get_attribute('msg_how_to_redeem')
    return msg.format(cooldown=self.get_attribute('redemption_cooldown'))

  def explain_credits(self):
    msg = ''
    if self.points_redemptions:
      msg = self.get_attribute('msg_with_points')
    if self.bits_redemptions:
      if msg:
        msg = msg + '; '
      if self.get_attribute('multiple_credits'):
        r = self.get_attribute('msg_multiple_credits')
      else:
        r = self.get_attribute('msg_no_multiple_credits')
      logging.debug(r)
      r = r.format(self.get_attribute('bits_per_credit'))
      msg = msg + self.get_attribute('msg_with_bits').format(r)
    if self.raffles:
      if msg:
        msg = msg + '; '
      msg = msg + self.get_attribute('msg_by_raffle')
    if not msg:
      return self.get_attribute('msg_no_redemptions')
    msg = self.get_attribute('msg_credit_methods') + msg
    return msg

  def list_active_mods(self):
    return self.get_attribute('msg_active_mods') +  ', '.join(filter(None, self.active_mods))

  def list_candidate_mods(self):    
    msg = self.get_attribute('msg_candidate_mods')
    for num, mod in enumerate(self.candidate_mods, start=1):
      msg += ' {}: {}.'.format(num, mod)
    return msg

  @flx.action
  def replace_mod(self, mod_key):
    self.update_softmax(mod_key)
    mods = list(self.active_mods)
    # Find the oldest mod
    times = list(self.mod_times)
    finished_mod = times.index(min(times))
    mods[finished_mod] = self.modifier_data[mod_key]['name']
    times[finished_mod] = 1.0
    self.set_mod_times(times)
    self.set_active_mods(mods)


  @flx.reaction('game_name')
  def on_game_name(self, *events):
    for ev in events:
      self.chaos_config['current_game'] = ev.new_value

  @flx.action
  def reset_current_mods(self):
    self.set_mod_times([0.0]*self.num_active_mods)
    self.set_active_mods(['']*self.num_active_mods)

  @flx.action
  def reset_voting(self):
    self.set_votes([0]* self.vote_options)
    self.set_candidate_mods(['']*self.vote_options)
    self.voted_users = []

  @flx.reaction('num_active_mods')
  def on_num_active_mods(self, *events):
    for ev in events:
      self.chaos_config['active_modifiers'] = ev.new_value
      
  @flx.reaction('time_per_modifier')
  def on_time_per_modifier(self, *events):
    for ev in events:
      self.chaos_config['modifier_time']  = ev.new_value
      
  @flx.reaction('softmax_factor')
  def on_softmax_factor(self, *events):
    for ev in events:
      self.chaos_config['softmax_factor']  = ev.new_value

  @flx.reaction('vote_options')
  def on_vote_options(self, *events):
    for ev in events:
      self.chaos_config['vote_options'] = ev.new_value
      self.reset_voting()

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
  def change_pubsub_oauth(self, new_value):
    logging.debug('Changing pubsub oauth')
    self.chaos_config["pubsub_oauth"] = new_value
    self.set_pubsub_oauth(new_value)

  @flx.action
  def change_channel_name(self, new_value):
    logging.debug(f'Changing channel to {new_value}')
    self.chaos_config["channel"] = new_value
    cfg['owner'] = new_value
    cfg['channels'] = [new_value]
    self.set_channel_name(new_value)

  @flx.reaction('voting_type')
  def on_voting_type(self, *events):
    for ev in events:
      self.chaos_config['voting_type']  = ev.new_value

  @flx.reaction('announce_candidates')
  def on_announce_candidates(self, *events):
    for ev in events:
      self.chaos_config['announce_candidates'] = ev.new_value

  @flx.reaction('announce_winner')
  def on_announce_winner(self, *events):
    for ev in events:
      self.chaos_config['announce_winner'] = ev.new_value

  @flx.reaction('bits_redemptions')
  def on_bits_redemptions(self, *events):
    for ev in events:
      self.chaos_config['bits_redemptions'] = ev.new_value

  @flx.reaction('bits_per_credit')
  def on_bits_per_credit(self, *events):
    for ev in events:
      self.chaos_config['bits_per_credit'] = ev.new_value

  @flx.reaction('multiple_credits')
  def on_multiple_credits(self, *events):
    for ev in events:
      self.chaos_config['multiple_credits'] = ev.new_value

  @flx.reaction('points_redemptions')
  def on_points_redemptions(self, *events):
    for ev in events:
      self.chaos_config['points_redemptions'] = ev.new_value

  @flx.reaction('raffles')
  def on_raffles(self, *events):
    for ev in events:
      self.chaos_config['raffles'] = ev.new_value

  @flx.reaction('raffle_time')
  def on_raffle_time(self, *events):
    for ev in events:
      self.chaos_config['raffle_time'] = ev.new_value

  @flx.reaction('redemption_cooldown')
  def on_redemption_cooldown(self, *events):
    for ev in events:
      self.chaos_config['redemption_cooldown'] = ev.new_value

  @flx.reaction('ui_rate')
  def on_ui_rate(self, *events):
    for ev in events:
      self.chaos_config["ui_rate"]  = ev.new_value
      
  @flx.reaction('ui_port')
  def on_ui_port(self, *events):
    for ev in events:
      self.chaos_config['ui_port']  = ev.new_value

  @flx.action
  def change_pi_host(self, new_value):
    logging.debug(f'Changing pi host address to {new_value}')
    self.chaos_config['pi_host']  = new_value
    self.set_pi_host(new_value)

  @flx.action
  def change_listen_port(self, new_value):
    logging.debug(f'Changing chaos engine listen port to {new_value}')
    self.chaos_config['listen_port']  = new_value
    self.set_listen_port(new_value)

  @flx.action
  def change_talk_port(self, new_value):
    logging.debug(f'Changing chaos engine talk port to {new_value}')
    self.chaos_config['talk_port']  = new_value
    self.set_talk_port(new_value)

  # Emitters to relay events to all participants.

  @flx.emitter
  def update_mod_times(self, value):
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
    

