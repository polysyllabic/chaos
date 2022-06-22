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
from twitchbot import is_config_valid
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
        
  def apply_new_mod(self, mod):
    logging.debug("Winning mod: " + mod)
    toSend = {"winner": mod,
              "time": config.relay.get_attribute('modifier_time')}
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
    if config.relay.voting_type == 'Proportional':
      total_votes = sum(tally)
      if total_votes < 1:
        for i in range(len(tally)):
          tally[i] += 1
        total_votes = sum(tally)
      
      the_choice = np.random.uniform(0, total_votes)
      index = 0
      accumulator = 0
      for i in range(len(tally)):
        index = i
        accumulator += tally[i]
        if accumulator >= the_choice:
          break
      
      logging.info(f"Winning Mod: '{config.relay.candidate_mods[index]}' at {100.0 * float(tally[index])/float(total_votes)}")

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

    newMod = config.relay.candidate_mods[index]
    return newMod
  
  def _loop(self):
    begin_time = time.time()
    now = begin_time
    dTime = 1.0/config.relay.get_attribute('ui_rate')
    prior_time = begin_time - dTime
    last_engine_request = 0.0
    self.paused_flashing_timer = 0.0
    self.disconnected_flashing_timer = 0.0

    while config.relay.keep_going:
      time.sleep(config.relay.sleep_time())
      prior_time = now
      now = time.time()
      dTime = now - prior_time
      self.paused_flashing_timer += dTime
      self.disconnected_flashing_timer += dTime
      
      # Ping the engine for game information. It's in the loop so that we'll retry periodically in
      # the event that the engine is temporarily down. We retry every 30 seconds in that case.
      if config.relay.valid_data == False:
        if last_engine_request == 0.0:
          self.request_game_info()
        last_engine_request += dTime
        if last_engine_request > 30.0:
          last_engine_request = 0.0

      if config.relay.paused:
        begin_time += dTime
        dTime = 0
        self.flash_pause()
          
      if config.relay.connected == False:
        self.flash_disconnected()
              
      self.vote_time =  now - begin_time

      if dTime > 0:
        flx.loop.call_soon(config.relay.decrement_mod_times, dTime)
        
      if self.first_time and config.relay.valid_data:
        # As soon as the data is ready, start the vote
        self.vote_time = config.relay.time_per_vote() + 1
        self.first_time = False
          
      if self.vote_time >= config.relay.time_per_vote():
        # Reset voting time
        begin_time = now
        # Skip voting if no data yet or voting is disabled
        if not config.relay.valid_data or config.relay.voting_type == 'DISABLE':
          continue
        
        # Pick the winner
        new_mod = self.select_winning_modifier()
        # Tell the engine
        self.apply_new_mod(new_mod)

        flx.loop.call_soon(config.relay.replace_mod, new_mod)
          
        #self.announceMods()
        #self.announceVoting()
        
      flx.loop.call_soon(config.relay.update_vote_time, self.vote_time)

    # Exitiing loop: clean up
    self.chaos_communicator.stop()
