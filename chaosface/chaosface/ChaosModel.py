#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The ChaosModel contains the central loop for modifier selection and updating the UI.

  Note that this event loop runs in a separate thread from the web UI, so we must schedule any
  routines that change relay properties on the main asyncio loop.
"""
import json
import logging
import random
import threading
import time
import asyncio
from typing import Optional

import numpy as np

import chaosface.config.globals as config
from chaosface.chatbot.ChaosBot import ChaosBot
from chaosface.communicator import ChaosCommunicator, EngineObserver
from chaosface.gui import ui_dispatch


class ChaosModel(EngineObserver):
  def __init__(self):
    self.thread = None
    self.first_time = True
    self.bot: Optional[ChaosBot] = None

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
    """
    Check if the user has entered complete credentials to log in to Twitch. This doesn't check
    that the credentials are valid, just that the user has entered something for all necessary
    fields.
    """
    need_pubsub = config.relay.bits_redemptions or config.relay.points_redemptions
    bot_oauth = (config.relay.bot_oauth or '').removeprefix('oauth:').strip()
    pubsub_oauth = (config.relay.pubsub_oauth or '').removeprefix('oauth:').strip()
    return (
      config.relay.channel_name != 'your_channel'
      and config.relay.bot_name != 'your_bot'
      and bool(bot_oauth)
      and (not need_pubsub or bool(pubsub_oauth))
    )

  def configure_bot(self):
    """
    Start the bot after all necessary parameters have been entered.
    
    The chatbot implementation starts with whatever credentials currently exist, and if those
    credentials are changed, we need to restart the bot to log in again. To handle either a first
    run (no credentials yet) or OAuth re-authorization, we wait until required fields are filled
    before starting the bot. The regular voting loop only starts after a verified chat connection.
    """
    
    # Loop until user has entered the necessary credentials
    while config.relay.keep_going and not self.complete_credentials():
      #logging.info("Waiting for user to enter complete bot credentials")
      time.sleep(config.relay.sleep_time())
    if not config.relay.keep_going:
      return
    # Start up the bot
    bot = config.relay.chatbot
    if bot is None:
      logging.warning("Chatbot is not configured; cannot start bot thread")
      return
    self.bot = bot
    self.bot.run_threaded()

  def restart_bot(self):
    """
    Close and restart the bot. This is necessary if we change credentials, because the bot
    framework assumes that the correct credentials will be entered on startup, so we can only
    force a new authorization by restarting the thread.
    """
    if self.bot:
      self.bot.shutdown()
      # start the bot again
      self.configure_bot()    

  # Ask the engine to tell us about the game we're playing
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
      ui_dispatch.call_soon(config.relay.set_paused, paused)

    elif "game" in received:
      logging.debug("Received game info!")
      ui_dispatch.call_soon(config.relay.initialize_game, received)
    else:
      logging.warn(f"Unprocessed message from engine: {received}")

  def apply_new_mod(self, mod_name):
    logging.debug("Winning mod: " + mod_name)
    to_send = {"winner": mod_name,
              "time": config.relay.get_attribute('modifier_time')}
    config.relay.engine_commands.put(to_send)
    # message = self.chaos_communicator.send_message(json.dumps(toSend))

  def replace_mod(self, mod_key):
    # If this is the first time, the candidate pool will be empty. Skip in this case.
    if mod_key and mod_key in config.relay.modifier_data:
      mod_name = config.relay.modifier_data[mod_key]['name']
      logging.debug(f'Telling engine about new mod {mod_name}')
      # Tell the engine
      self.apply_new_mod(mod_name)
      ui_dispatch.call_soon(config.relay.replace_mod, mod_key)
    else:
      logging.debug(f'Mod key "{mod_key}" not in list (not sent)')
    # Pick new candidates, if we're voting
    if config.relay.voting_type == 'DISABLED' or config.relay.voting_type == 'DISABLED':
      return
    ui_dispatch.call_soon(config.relay.get_new_voting_pool)


  def remove_mod(self, mod_name):
    logging.debug("Removing mod: " + mod_name)
    to_send = {"remove": mod_name}
    config.relay.engine_commands.put(to_send)

  def reset_mods(self):
    logging.debug("Resetting all mods")
    to_send = {"reset": True }
    config.relay.engine_commands.put(to_send)

  def send_chat_message(self, msg: str):
    if config.relay.chatbot and self.bot:
      loop = self.bot._get_event_loop()
      if loop and loop.is_running():
        loop.call_soon_threadsafe(asyncio.create_task, self.bot.send_message(msg))

  def flash_pause(self):
    if self.paused_flashing_timer > 0.5 and config.relay.paused_bright == True:
      ui_dispatch.call_soon(config.relay.set_paused_bright, False)
    elif self.paused_flashing_timer > 1.0 and config.relay.paused_bright == False:
      ui_dispatch.call_soon(config.relay.set_paused_bright, True)
      self.paused_flashing_timer = 0.0
        
  def flash_disconnected(self):
    if self.disconnected_flashing_timer > 0.5 and config.relay.connected_bright == True:
      ui_dispatch.call_soon(config.relay.set_connected_bright, False)
    elif self.disconnected_flashing_timer > 1.0 and config.relay.connected_bright == False:
      ui_dispatch.call_soon(config.relay.set_connected_bright, True)
      self.disconnected_flashing_timer = 0.0
  
  def select_winning_modifier(self):
    tally = list(config.relay.votes)
    index = 0
    if config.relay.voting_type == 'Authoritarian':
      return config.relay.get_random_mod()

    elif config.relay.voting_type == 'Proportional':
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
  

  def _loop(self):
    now = time.time()
    start_voting = now
    delta_time = config.relay.sleep_time()
    prior_time = now - delta_time
    last_engine_request = 0.0
    self.paused_flashing_timer = 0.0
    self.disconnected_flashing_timer = 0.0
    # Ask engine for information about game modifiers
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
        start_voting += delta_time
        delta_time = 0
        self.flash_pause()
          
      if config.relay.connected == False:
        self.flash_disconnected()

      # Update remaining time for modifiers if not paused      
      if delta_time > 0:
        ui_dispatch.call_soon(config.relay.decrement_mod_times, delta_time)
        
      self.vote_time =  now - start_voting

      if self.first_time and config.relay.valid_data:
        # As soon as the data is ready, start the vote
        self.vote_time = config.relay.time_per_vote() + 1
        self.first_time = False

      # Check for queued commands to send

      # Check for an immediate mod insertion/removal
      if config.relay.force_mod:
        self.replace_mod(config.relay.force_mod)
        config.relay.force_mod = ''
        start_voting = now

      if config.relay.reset_mods:
        self.reset_mods()
        ui_dispatch.call_soon(config.relay.reset_current_mods)
        config.relay.reset_mods = False


      if self.vote_time >= config.relay.time_per_vote():
        # Reset voting time
        start_voting = now
        # Skip voting if no data yet or voting is disabled
        if not config.relay.valid_data or config.relay.voting_type == 'DISABLE':
          continue
        
        # Pick the winner        
        mod_key = self.select_winning_modifier()
        self.replace_mod(mod_key)

        if config.relay.announce_winner and mod_key:
          self.send_chat_message(config.relay.list_winning_mod(mod_key))

      # Update elapsed voting time
      ui_dispatch.call_soon(config.relay.set_vote_time, self.vote_time/config.relay.time_per_vote())

    # Exitiing loop: clean up
    self.chaos_communicator.stop()
