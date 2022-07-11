#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The ChaosModel contains the central loop for modifier selection and updating the UI.

  Note that this event loop runs in a separate thread from the flexx UI, so we must schedule any
  routines that change flexx parameters with a call to flx.loop.call_soon(<function>, *args)
"""
import time
import threading
import logging
import json
import random
import numpy as np
from flexx import flx
from twitchbot import is_config_valid, cfg
import chaosface.config.globals as config
from chaosface.communicator import EngineObserver
from chaosface.communicator import ChaosCommunicator


class ChaosModel(EngineObserver):
  def __init__(self):
    self.thread = None
    self.first_time = True

    #  Socket to talk to server
    logging.info("Connecting to chaos server")
    self.chaos_communicator = ChaosCommunicator()
    self.chaos_communicator.attach(self)
    self.chaos_communicator.start()
    
  def start(self):
    self.configure_bot()
    self._loop()

  def start_threaded(self):
    self.thread = threading.Thread(target=self.start)
    self.thread.start()

  def complete_credentials(self):
    return is_config_valid() and config.relay.channel_name != 'your_channel'

  def configure_bot(self):
    # If we don't yet have good bot credentials, we enter a special waiting loop until they are
    # entered. The regular voting loop only starts after we have a verified chat connection.
    if cfg['client_id'] == 'CLIENT_ID':
      cfg['client_id'] = config.relay.get_attribute('client_id')
    while not self.complete_credentials():
      time.sleep(config.relay.sleep_time())
    # Start up the bot
    config.relay.chatbot.run_threaded()

  # Ask the engine to tell us about the game we're playing
  # TODO: Request a list of available games
  # TODO: Send a config file from the computer running this bot
  def request_game_info(self):
    logging.debug("Asking engine about the game")
    to_send = {'game': True}
    resp = self.chaos_communicator.send_message(json.dumps(to_send))
    if resp == False:
      logging.warn(f"Engine is not responding. No game data available")

  def update_command(self, message) -> None:
    received = json.loads(message)

    if "pause" in received:
      paused = bool(received["pause"])
      logging.info("Got a pause command of: {paused}")
      flx.loop.call_soon(config.relay.set_paused, paused)

    elif "game" in received:
      logging.debug("Received game info!")
      flx.loop.call_soon(config.relay.initialize_game, received)
    else:
      logging.warn(f"Unprocessed message from engine: {received}")
        
  def apply_new_mod(self, mod_name):
    logging.debug("Winning mod: " + mod_name)
    toSend = {"winner": mod_name,
              "time": config.relay.get_attribute('modifier_time')}
    message = self.chaos_communicator.send_message(json.dumps(toSend))

  def reset_mods(self, mod_name):
    logging.debug("Resetting mods")
    toSend = {"reset": True }
    message = self.chaos_communicator.send_message(json.dumps(toSend))

  def flash_pause(self):
    if self.paused_flashing_timer > 0.5 and config.relay.paused_bright == True:
      flx.loop.call_soon(config.relay.set_paused_bright, False)
    elif self.paused_flashing_timer > 1.0 and config.relay.paused_bright == False:
      flx.loop.call_soon(config.relay.set_paused_bright, True)
      self.paused_flashing_timer = 0.0
        
  def flash_disconnected(self):
    if self.disconnected_flashing_timer > 0.5 and config.relay.connected_bright == True:
      flx.loop.call_soon(config.relay.set_connected_bright, False)
    elif self.disconnected_flashing_timer > 1.0 and config.relay.connected_bright == False:
      flx.loop.call_soon(config.relay.set_connected_bright, True)
      self.disconnected_flashing_timer = 0.0
  
  def select_winning_modifier(self):
    tally = list(config.relay.votes)
    index = 0
    if config.relay.voting_type == 'Proportional':
      total_votes = sum(tally)
      if total_votes < 1:
        for i in range(len(tally)):
          tally[i] += 1
        total_votes = sum(tally)
      
      the_choice = np.random.uniform(0, total_votes)
      accumulator = 0
      for i in range(len(tally)):
        index = i
        accumulator += tally[i]
        if accumulator >= the_choice:
          break
      
      #logging.info(f"Winning Mod: '{config.relay.candidate_mods[index]}' at {100.0 * float(tally[index])/float(total_votes)}")

    else:
      # get indices of all mods with the max number of votes, then randomly pick one
      maxval = None
      indices = []
      for ind, val in enumerate(tally):
        if maxval is None or val > maxval:
          indices = [ind]
          maxval = val
        elif val == maxval:
          indices.append(ind)
      index = random.choice(indices)

    new_mod_key = config.relay.candidate_keys[index]
    logging.info(f'Winning Mod: #{index+1} {new_mod_key}')
    return new_mod_key
  
  def replace_mod(self, mod_key):
    # If this is the first time, the candidate pool will be empty. Skip in this case.
    if mod_key and mod_key in config.relay.modifier_data:
      mod_name = config.relay.modifier_data[mod_key]['name']
      logging.debug(f'Telling engine about new mod {mod_name}')
      # Tell the engine
      self.apply_new_mod(mod_name)
      flx.loop.call_soon(config.relay.replace_mod, mod_key)
    else:
      logging.debug(f'Mod key "{mod_key}" not in list (not sent)')
    # Pick new candidates
    flx.loop.call_soon(config.relay.get_new_voting_pool)

  def _loop(self):
    begin_time = time.time()
    now = begin_time
    delta_time = config.relay.sleep_time()
    prior_time = begin_time - delta_time
    last_engine_request = 0.0
    self.paused_flashing_timer = 0.0
    self.disconnected_flashing_timer = 0.0
    self.request_game_info()

    while config.relay.keep_going:
      time.sleep(config.relay.sleep_time())
      prior_time = now
      now = time.time()
      delta_time = now - prior_time
      self.paused_flashing_timer += delta_time
      self.disconnected_flashing_timer += delta_time
      
      # If we haven't yet received the game info, retry periodically
      if config.relay.valid_data == False:
        last_engine_request += delta_time
        if last_engine_request > 30.0:
          self.request_game_info()
          last_engine_request = 0.0

      if config.relay.paused:
        begin_time += delta_time
        delta_time = 0
        self.flash_pause()
          
      if config.relay.connected == False:
        self.flash_disconnected()

      # Update remaining time for modifiers if not paused      
      if delta_time > 0:
        flx.loop.call_soon(config.relay.decrement_mod_times, delta_time)
        
      self.vote_time =  now - begin_time

      if self.first_time and config.relay.valid_data:
        # As soon as the data is ready, start the vote
        self.vote_time = config.relay.time_per_vote() + 1
        self.first_time = False

      # Check for an immediate mod insertion
      if config.relay.force_mod:
        self.replace_mod(config.relay.force_mod)
        config.relay.force_mod = ''
        begin_time = now

      if config.relay.reset_mods:
        self.reset_mods()
        flx.loop.call_soon(config.relay.reset_current_mods)
        config.relay.reset_mods = False


      if self.vote_time >= config.relay.time_per_vote():
        # Reset voting time
        begin_time = now
        # Skip voting if no data yet or voting is disabled
        if not config.relay.valid_data or config.relay.voting_type == 'DISABLE':
          continue
        
        # Pick the winner
        mod_key = self.select_winning_modifier()
        self.replace_mod(mod_key)

        #if config.relay.announce_winner:
        #  config.relay.send_winning_mod_to_chat(mod_key)

        #if config.relay.announce_active:
        #  config.relay.send_active_mods_to_chat()

        #if config.relay.announce_candidates:
        #  config.relay.send_candidate_mods_to_chat()

      # Update elapsed voting time
      flx.loop.call_soon(config.relay.set_vote_time, self.vote_time/config.relay.time_per_vote())

    # Exitiing loop: clean up
    self.chaos_communicator.stop()
