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
  ENGINE_STATUS_PAYLOAD_MAX = 180
  ENGINE_STATUS_UNKNOWN = 'unknown'
  ENGINE_STATUS_NOT_CONNECTED = 'not_connected'
  ENGINE_STATUS_TIMEOUT = 'timeout'
  ENGINE_STATUS_WAITING_FOR_GAME = 'waiting_for_game'
  ENGINE_STATUS_BAD_CONFIG_FILE = 'bad_config_file'
  ENGINE_STATUS_PAUSED = 'paused'
  ENGINE_STATUS_RUNNING = 'running'

  def __init__(self):
    self.thread = None
    self.first_time = True
    self.bot: Optional[ChaosBot] = None
    self._pending_game_startup_setup = False

    #  Socket to talk to server
    logging.info("Connecting to chaos server")
    self.chaos_communicator = ChaosCommunicator()
    self.chaos_communicator.attach(self)
    self.chaos_communicator.start()
    self._engine_handshake_complete = False

  @staticmethod
  def _command_name(payload) -> str:
    if isinstance(payload, dict) and payload:
      keys = [str(key) for key in payload.keys()]
      if len(keys) == 1:
        return keys[0]
      head = '+'.join(keys[:3])
      return f'{head}+...' if len(keys) > 3 else head
    return type(payload).__name__

  def _payload_preview(self, payload) -> str:
    try:
      text = json.dumps(payload, ensure_ascii=False, separators=(',', ':'))
    except TypeError:
      text = str(payload)
    text = str(text).replace('\n', ' ').strip()
    if len(text) <= self.ENGINE_STATUS_PAYLOAD_MAX:
      return text
    return text[: self.ENGINE_STATUS_PAYLOAD_MAX - 3] + '...'

  def _report_engine_message(self, direction: str, payload) -> None:
    command = self._command_name(payload)
    preview = self._payload_preview(payload)
    ui_dispatch.call_soon(config.relay.add_bot_status, f'{direction} [{command}] {preview}')

  def _set_engine_status(self, status: str) -> None:
    ui_dispatch.call_soon(config.relay.set_engine_status, status)

  def _update_engine_status_from_payload(self, payload) -> None:
    status = str(payload.get('engine_status', '')).strip().lower()
    if not status:
      return
    self._set_engine_status(status)

  @staticmethod
  def _status_for_pause(paused: bool) -> str:
    return ChaosModel.ENGINE_STATUS_PAUSED if paused else ChaosModel.ENGINE_STATUS_RUNNING
    
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
    need_eventsub = config.relay.bits_redemptions or config.relay.points_redemptions
    bot_oauth = (config.relay.bot_oauth or '').removeprefix('oauth:').strip()
    eventsub_oauth = (config.relay.eventsub_oauth or '').removeprefix('oauth:').strip()
    return (
      config.relay.channel_name != 'your_channel'
      and config.relay.bot_name != 'your_bot'
      and bool(bot_oauth)
      and (not need_eventsub or bool(eventsub_oauth))
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

  def send_engine_command(self, payload: dict, *, report_status: bool = True) -> bool:
    if report_status:
      self._report_engine_message('Chaosface -> Engine', payload)
    message = json.dumps(payload)
    sent = self.chaos_communicator.send_message(message)
    if sent is False:
      logging.warning('Engine is not responding to command: %s', payload)
      next_status = self.ENGINE_STATUS_TIMEOUT if self._engine_handshake_complete else self.ENGINE_STATUS_NOT_CONNECTED
      self._set_engine_status(next_status)
      if report_status:
        command = self._command_name(payload)
        ui_dispatch.call_soon(config.relay.add_bot_status, f'Engine did not acknowledge command [{command}]')
      return False
    return True

  # Ask the engine to tell us about the game we're playing
  # TODO: Send a config file from the computer running this bot
  def request_game_info(self):
    logging.debug("Asking engine about the game")
    to_send = {'game': True}
    self.send_engine_command(to_send)

  def request_available_games(self):
    logging.debug("Asking engine for available games")
    self.send_engine_command({'available_games': True})

  def request_game_selection(self, game_name: str):
    selected = str(game_name).strip()
    if not selected:
      return
    logging.info("Requesting game selection: %s", selected)
    self.send_engine_command({'select_game': selected})

  def acknowledge_available_games(self, game_count: int):
    payload = {'available_games_ack': True, 'count': max(0, int(game_count))}
    self.send_engine_command(payload)

  def _parse_available_games(self, payload) -> list[str]:
    games = []
    seen = set()
    if not isinstance(payload, list):
      return games
    for item in payload:
      game_name = ''
      if isinstance(item, dict):
        game_name = str(item.get('game', '')).strip()
      else:
        game_name = str(item).strip()
      if game_name and game_name not in seen:
        seen.add(game_name)
        games.append(game_name)
    games.sort(key=lambda name: name.lower())
    return games

  def update_command(self, message) -> None:
    try:
      received = json.loads(message)
    except json.JSONDecodeError:
      preview = self._payload_preview(message)
      ui_dispatch.call_soon(config.relay.add_bot_status, f'Engine -> Chaosface [raw] {preview}')
      logging.warning('Unparseable engine message: %s', preview)
      return

    self._engine_handshake_complete = True
    self._report_engine_message('Engine -> Chaosface', received)
    self._update_engine_status_from_payload(received)

    handled = False

    if "pause" in received:
      handled = True
      paused = bool(received["pause"])
      logging.info("Got a pause command of: {paused}")
      ui_dispatch.call_soon(config.relay.set_paused, paused)
      if 'engine_status' not in received:
        self._set_engine_status(self._status_for_pause(paused))

    if "available_games" in received:
      handled = True
      games = self._parse_available_games(received.get('available_games'))
      ui_dispatch.call_soon(config.relay.set_available_games, games)
      self.acknowledge_available_games(len(games))
      reported_status = str(received.get('engine_status', '')).strip().lower()
      engine_needs_game = reported_status in (self.ENGINE_STATUS_WAITING_FOR_GAME, self.ENGINE_STATUS_BAD_CONFIG_FILE)
      if engine_needs_game:
        config.relay.valid_data = False
        ui_dispatch.call_soon(config.relay.reset_current_mods)
      preferred = config.relay.get_preferred_game(games)
      if preferred and (engine_needs_game or not config.relay.valid_data or config.relay.game_name != preferred):
        self.request_game_selection(preferred)
      elif 'engine_status' not in received and not config.relay.valid_data:
        self._set_engine_status(self.ENGINE_STATUS_WAITING_FOR_GAME)

    if "mods_reset" in received:
      handled = True
      ui_dispatch.call_soon(config.relay.reset_current_mods)

    if "game" in received:
      handled = True
      logging.debug("Received game info!")
      ui_dispatch.call_soon(config.relay.initialize_game, received)
      self._pending_game_startup_setup = True
      if 'engine_status' not in received and 'pause' not in received:
        self._set_engine_status(self._status_for_pause(config.relay.paused))

    if "engine_status" in received:
      handled = True

    if not handled:
      logging.warning(f"Unprocessed message from engine: {received}")

  def apply_new_mod(self, mod_name):
    logging.debug("Winning mod: " + mod_name)
    to_send = {"winner": mod_name,
              "time": config.relay.get_attribute('modifier_time')}
    self.send_engine_command(to_send)

  def replace_mod(self, mod_key, refresh_voting_pool: bool = True):
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
    if self._voting_disabled() or not refresh_voting_pool:
      return
    ui_dispatch.call_soon(config.relay.get_new_voting_pool)


  def remove_mod(self, mod_name):
    logging.debug("Removing mod: " + mod_name)
    to_send = {"remove": mod_name}
    self.send_engine_command(to_send)

  def reset_mods(self):
    logging.debug("Resetting all mods")
    to_send = {"reset": True }
    self.send_engine_command(to_send)

  def _voting_disabled(self) -> bool:
    return (
      config.relay.voting_type == 'DISABLED'
      or config.relay.voting_cycle == 'DISABLED'
    )

  def _configured_vote_length(self) -> float:
    value = config.relay.get_attribute('vote_time')
    if isinstance(value, bool):
      vote_time = float(value)
    elif isinstance(value, (int, float, str)):
      try:
        vote_time = float(value)
      except ValueError:
        vote_time = config.relay.time_per_vote()
    else:
      vote_time = config.relay.time_per_vote()
    return max(1.0, vote_time)

  def _configured_vote_delay(self) -> float:
    value = config.relay.get_attribute('vote_delay')
    if isinstance(value, bool):
      vote_delay = float(value)
    elif isinstance(value, (int, float, str)):
      try:
        vote_delay = float(value)
      except ValueError:
        vote_delay = 0.0
    else:
      vote_delay = 0.0
    return max(0.0, vote_delay)

  def _default_vote_length(self, cycle: str) -> float:
    if cycle == 'Continuous':
      return max(1.0, float(config.relay.time_per_vote()))
    return self._configured_vote_length()

  def _next_vote_start_for_cycle(self, cycle: str, now: float) -> float:
    if cycle == 'Continuous':
      return now
    if cycle == 'Interval':
      return now + self._configured_vote_delay()
    if cycle == 'Random':
      delay_max = self._configured_vote_delay()
      return now + random.uniform(0.0, delay_max)
    return float('inf')

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
    if not config.relay.candidate_keys:
      return ''
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

    if index >= len(config.relay.candidate_keys):
      return ''
    new_mod_key = config.relay.candidate_keys[index]
    logging.info(f'Winning Mod: #{index+1} {new_mod_key}')
    return new_mod_key
  

  def _loop(self):
    now = time.time()
    vote_started_at = now
    next_vote_start_at = now
    vote_open = False
    vote_target = 1.0
    was_voting_disabled = self._voting_disabled()
    delta_time = config.relay.sleep_time()
    prior_time = now - delta_time
    last_engine_request = 0.0
    self.paused_flashing_timer = 0.0
    self.disconnected_flashing_timer = 0.0
    # Ask engine for available games. We'll request game details after a game is selected.
    self.request_available_games()
    remembered_selection = str(config.relay.selected_game or '').strip()
    if remembered_selection and remembered_selection.upper() != 'NONE':
      self.request_game_selection(remembered_selection)

    while config.relay.keep_going:
      time.sleep(config.relay.sleep_time())
      prior_time = now
      now = time.time()
      delta_time = now - prior_time
      self.paused_flashing_timer += delta_time
      self.disconnected_flashing_timer += delta_time

      if config.relay.bot_reboot:
        config.relay.bot_reboot = False
        logging.info('Restarting chatbot after credential/config update')
        self.restart_bot()
      
      # If we haven't yet received game data, keep requesting available games.
      if config.relay.valid_data == False:
        last_engine_request += delta_time
        if last_engine_request > 30.0 and not list(config.relay.available_games):
          self.request_available_games()
          last_engine_request = 0.0

      game_selection = config.relay.consume_game_selection_request()
      if game_selection:
        self.request_game_selection(game_selection)

      if self._pending_game_startup_setup and config.relay.valid_data:
        if config.relay.engine_status == self.ENGINE_STATUS_PAUSED:
          if not self._voting_disabled():
            ui_dispatch.call_soon(config.relay.get_new_voting_pool)
          if bool(config.relay.get_attribute('startup_random_modifier')):
            mod_key = config.relay.get_random_mod()
            if mod_key:
              self.replace_mod(mod_key, refresh_voting_pool=False)
          self._pending_game_startup_setup = False

      if config.relay.paused:
        if vote_open:
          vote_started_at += delta_time
        if next_vote_start_at != float('inf'):
          next_vote_start_at += delta_time
        delta_time = 0
        self.flash_pause()
          
      if config.relay.connected == False:
        self.flash_disconnected()

      # Update remaining time for modifiers if not paused      
      if delta_time > 0:
        ui_dispatch.call_soon(config.relay.decrement_mod_times, delta_time)

      # Check for an immediate mod insertion/removal
      if config.relay.force_mod:
        self.replace_mod(config.relay.force_mod, refresh_voting_pool=vote_open)
        config.relay.force_mod = ''
        if vote_open:
          vote_started_at = now

      remove_request = config.relay.consume_remove_mod_request()
      if remove_request:
        mod_name = remove_request
        if remove_request in config.relay.modifier_data:
          mod_name = config.relay.modifier_data[remove_request]['name']
        self.remove_mod(mod_name)
        ui_dispatch.call_soon(config.relay.remove_active_mod, mod_name)

      if config.relay.reset_mods:
        self.reset_mods()
        ui_dispatch.call_soon(config.relay.reset_current_mods)
        config.relay.reset_mods = False

      cycle = config.relay.voting_cycle
      voting_disabled = self._voting_disabled()
      start_request = config.relay.consume_start_vote_request()
      end_requested = config.relay.consume_end_vote_request()

      if voting_disabled and not was_voting_disabled:
        ui_dispatch.call_soon(config.relay.reset_voting)

      if voting_disabled:
        vote_open = False
        self.vote_time = 0.0
        next_vote_start_at = float('inf')
      else:
        def open_vote(length: float):
          nonlocal vote_open, vote_started_at, vote_target, next_vote_start_at
          if not config.relay.valid_data:
            return
          vote_open = True
          vote_started_at = now
          vote_target = max(1.0, float(length))
          self.vote_time = 0.0
          next_vote_start_at = now
          ui_dispatch.call_soon(config.relay.get_new_voting_pool)

        def close_vote(choose_winner: bool):
          nonlocal vote_open, next_vote_start_at
          mod_key = ''
          if choose_winner and config.relay.valid_data:
            mod_key = self.select_winning_modifier()
            self.replace_mod(mod_key, refresh_voting_pool=False)
            if config.relay.announce_winner and mod_key:
              self.send_chat_message(config.relay.list_winning_mod(mod_key))
          vote_open = False
          self.vote_time = 0.0
          if cycle == 'Continuous':
            open_vote(self._default_vote_length(cycle))
          else:
            ui_dispatch.call_soon(config.relay.reset_voting)
            next_vote_start_at = self._next_vote_start_for_cycle(cycle, now)

        if start_request is not None and config.relay.valid_data:
          if start_request > 0:
            start_length = float(start_request)
          else:
            start_length = self._default_vote_length(cycle)
          open_vote(start_length)

        if end_requested and vote_open:
          close_vote(choose_winner=True)

        if (not vote_open and cycle != 'Triggered' and config.relay.valid_data and now >= next_vote_start_at):
          open_vote(self._default_vote_length(cycle))

        if vote_open:
          self.vote_time = now - vote_started_at
          if self.vote_time >= vote_target:
            close_vote(choose_winner=True)
        else:
          self.vote_time = 0.0

      ui_dispatch.call_soon(config.relay.set_vote_open, vote_open)
      vote_ratio = (self.vote_time / vote_target) if vote_open and vote_target > 0 else 0.0
      ui_dispatch.call_soon(config.relay.set_vote_time, vote_ratio)
      was_voting_disabled = voting_disabled

    # Exitiing loop: clean up
    self.chaos_communicator.stop()
