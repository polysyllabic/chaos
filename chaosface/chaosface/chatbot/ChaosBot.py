#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The Twitch Bot to monitor chat for votes and other commands.
"""
import asyncio
import logging
import random
import threading
import time
from contextlib import suppress
from typing import Any, Callable, Optional, Protocol

from twitchAPI.chat import Chat, ChatEvent, ChatMessage, EventData
from twitchAPI.eventsub.websocket import EventSubWebsocket
from twitchAPI.helper import first
from twitchAPI.twitch import Twitch
from twitchAPI.type import AuthScope


def _clean_oauth(token: str) -> str:
  if not token:
    return ''
  return token.removeprefix('oauth:').strip()


class ChaosBotContext(Protocol):
  """Strict behavior contract expected by ChaosBot."""

  @property
  def channel_name(self) -> str:
    ...

  @property
  def bot_oauth(self) -> str:
    ...

  @property
  def eventsub_oauth(self) -> str:
    ...

  @property
  def points_redemptions(self) -> bool:
    ...

  @property
  def bits_redemptions(self) -> bool:
    ...

  @property
  def connected(self) -> bool:
    ...

  @property
  def voting_type(self) -> str:
    ...

  @property
  def voting_cycle(self) -> str:
    ...

  @property
  def vote_open(self) -> bool:
    ...

  @property
  def redemption_cooldown(self) -> float:
    ...

  @property
  def points_reward_title(self) -> str:
    ...

  @property
  def bits_per_credit(self) -> int:
    ...

  @property
  def multiple_credits(self) -> bool:
    ...

  @property
  def raffle_open(self) -> bool:
    ...

  @raffle_open.setter
  def raffle_open(self, value: bool) -> None:
    ...

  @property
  def raffle_start_time(self):
    ...

  @raffle_start_time.setter
  def raffle_start_time(self, value) -> None:
    ...

  @property
  def reset_mods(self) -> bool:
    ...

  @reset_mods.setter
  def reset_mods(self, value: bool) -> None:
    ...

  @property
  def force_mod(self) -> str:
    ...

  @force_mod.setter
  def force_mod(self, value: str) -> None:
    ...

  def get_attribute(self, key: str) -> Any:
    ...

  def get_mod_description(self, mod: str) -> str:
    ...

  def get_balance_message(self, user: str) -> str:
    ...

  def about_tcc(self) -> str:
    ...

  def explain_voting(self) -> str:
    ...

  def explain_redemption(self) -> str:
    ...

  def explain_credits(self) -> str:
    ...

  def list_active_mods(self) -> str:
    ...

  def list_candidate_mods(self) -> str:
    ...

  def step_balance(self, user: str, delta: int) -> int:
    ...

  def set_balance(self, user: str, balance: int) -> int:
    ...

  def get_balance(self, user: str) -> int:
    ...

  def mod_enabled(self, mod: str):
    ...

  def set_mod_enabled(self, mod: str, enabled: bool) -> str:
    ...

  def request_start_vote(self, duration: Optional[float] = None) -> None:
    ...

  def request_end_vote(self) -> None:
    ...

  def request_remove_mod(self, mod_key: str) -> None:
    ...

  def has_permission(self, user: str, permission: str) -> bool:
    ...

  def add_permission_group(self, group: str) -> str:
    ...

  def add_group_member(self, group: str, user: str) -> str:
    ...

  def add_group_permission(self, group: str, permission: str) -> str:
    ...

  def remove_group_member(self, group: str, user: str) -> str:
    ...

  def remove_group_permission(self, group: str, permission: str) -> str:
    ...


class ChaosBot:
  """
  Twitch bot runtime with no direct dependency on UI globals.

  The `context` object is expected to provide the attributes/methods used throughout this class.
  """
  def __init__(
    self,
    context: ChaosBotContext,
    on_connected: Optional[Callable[[bool], None]] = None,
    on_vote: Optional[Callable[[int, str], None]] = None,
  ):
    self._ctx = context
    self._thread: Optional[threading.Thread] = None
    self._loop: Optional[asyncio.AbstractEventLoop] = None
    self._ready = threading.Event()
    self._shutdown = threading.Event()
    self._channel_name = ''

    self._chat_twitch: Optional[Twitch] = None
    self._event_twitch: Optional[Twitch] = None
    self._chat: Optional[Chat] = None
    self._eventsub: Optional[EventSubWebsocket] = None

    self._raffle_task: Optional[asyncio.Task] = None
    self._raffle_entries = set()
    self._last_apply_time = 0.0
    self._on_connected = on_connected or (lambda value: None)
    self._on_vote = on_vote or (lambda index, user: None)

  def _get_event_loop(self) -> Optional[asyncio.AbstractEventLoop]:
    return self._loop

  def _attr_str(self, key: str, fallback: str = '') -> str:
    value = self._ctx.get_attribute(key)
    if value is None:
      return fallback
    if isinstance(value, str):
      return value
    return str(value)

  def _attr_int(self, key: str, fallback: int = 0) -> int:
    value = self._ctx.get_attribute(key)
    try:
      return int(float(value))
    except (TypeError, ValueError):
      return fallback

  def _attr_float(self, key: str, fallback: float = 0.0) -> float:
    value = self._ctx.get_attribute(key)
    try:
      return float(value)
    except (TypeError, ValueError):
      return fallback

  def _attr_bool(self, key: str, fallback: bool = False) -> bool:
    value = self._ctx.get_attribute(key)
    if value is None:
      return fallback
    return bool(value)

  async def _require_permission(self, user: str, permission: str) -> bool:
    if self._ctx.has_permission(user, permission):
      return True
    await self.send_message(f"You need '{permission}' permission to use this command.")
    return False

  def run_threaded(self):
    if self._thread and self._thread.is_alive():
      return
    self._ready.clear()
    self._shutdown.clear()
    self._thread = threading.Thread(target=self._run_forever, daemon=True)
    self._thread.start()
    self._ready.wait(timeout=10.0)

  def _run_forever(self):
    self._loop = asyncio.new_event_loop()
    asyncio.set_event_loop(self._loop)
    self._ready.set()
    try:
      self._loop.run_until_complete(self._main())
    finally:
      with suppress(Exception):
        pending = asyncio.all_tasks(self._loop)
        for task in pending:
          task.cancel()
        self._loop.run_until_complete(asyncio.gather(*pending, return_exceptions=True))
      self._loop.close()
      self._loop = None

  async def _main(self):
    while not self._shutdown.is_set():
      try:
        await self._connect()
        while not self._shutdown.is_set():
          await asyncio.sleep(0.25)
      except Exception:
        logging.exception('Chatbot connection failed')
      finally:
        await self._disconnect()
        self._emit_connected(False)
      if not self._shutdown.is_set():
        await asyncio.sleep(5.0)

  async def _connect(self):
    client_id = self._attr_str('client_id')
    self._channel_name = self._ctx.channel_name.strip().lstrip('#').lower()
    bot_token = _clean_oauth(self._ctx.bot_oauth)
    event_token = _clean_oauth(self._ctx.eventsub_oauth)

    if not client_id:
      raise ValueError('No Twitch client_id is configured')
    if not self._channel_name:
      raise ValueError('No channel name is configured')
    if not bot_token:
      raise ValueError('No bot OAuth token is configured')

    self._chat_twitch = await Twitch(client_id, authenticate_app=False)
    await self._chat_twitch.set_user_authentication(
      bot_token,
      [AuthScope.CHAT_READ, AuthScope.CHAT_EDIT],
      validate=False,
    )

    self._chat = await Chat(self._chat_twitch)
    self._chat.register_event(ChatEvent.READY, self._on_chat_ready)
    self._chat.register_event(ChatEvent.MESSAGE, self._on_chat_message)
    self._chat.start()

    await self._start_eventsub(client_id, event_token)
    self._emit_connected(True)
    logging.info('Connected to Twitch chat for channel %s', self._channel_name)

  async def _start_eventsub(self, client_id: str, event_token: str):
    need_eventsub = self._ctx.points_redemptions or self._ctx.bits_redemptions
    if not need_eventsub:
      return
    if not event_token:
      logging.warning('Bits/points redemptions are enabled, but EventSub OAuth token is missing')
      return

    self._event_twitch = await Twitch(client_id, authenticate_app=False)
    scopes = [AuthScope.BITS_READ, AuthScope.CHANNEL_READ_REDEMPTIONS]
    await self._event_twitch.set_user_authentication(event_token, scopes, validate=False)

    broadcaster_id = await self._get_broadcaster_id(self._event_twitch, self._channel_name)
    if not broadcaster_id:
      logging.warning('Unable to resolve broadcaster ID for %s', self._channel_name)
      return

    self._eventsub = EventSubWebsocket(self._event_twitch)
    self._eventsub.start()

    if self._ctx.points_redemptions:
      await self._eventsub.listen_channel_points_custom_reward_redemption_add(
        broadcaster_id, self._on_points_redemption
      )
    if self._ctx.bits_redemptions:
      await self._eventsub.listen_channel_cheer(broadcaster_id, self._on_channel_cheer)

  async def _get_broadcaster_id(self, twitch: Twitch, channel_name: str) -> str:
    try:
      user = await first(twitch.get_users(logins=[channel_name]))
      if user:
        return user.id
    except Exception:
      logging.exception('Could not resolve broadcaster id')
      return ''
    return ''

  async def _on_chat_ready(self, ready_event: EventData):
    await ready_event.chat.join_room(self._channel_name)

  async def _on_chat_message(self, msg: ChatMessage):
    message = (msg.text or '').strip()
    user_name = (getattr(getattr(msg, 'user', None), 'name', None) or '').strip().lower()
    if not message or not user_name:
      return

    if message.startswith('!'):
      await self._handle_command(user_name, message)
      return

    if (
      not self._ctx.connected
      or self._ctx.voting_type in ('DISABLED', 'Authoritarian')
      or self._ctx.voting_cycle == 'DISABLED'
      or not self._ctx.vote_open
    ):
      return
    if message.isdigit():
      vote_num = int(message) - 1
      if vote_num < 0:
        return
      self._emit_vote(vote_num, user_name)

  def _emit_connected(self, value: bool):
    try:
      self._on_connected(value)
    except Exception:
      logging.exception('Connected callback failed')

  def _emit_vote(self, index: int, user: str):
    try:
      self._on_vote(index, user)
    except Exception:
      logging.exception('Vote callback failed')

  async def _handle_command(self, author: str, message: str):
    parts = message[1:].split()
    if not parts:
      return
    cmd = parts[0].lower()
    args = parts[1:]
    is_streamer = author == self._channel_name

    if cmd == 'chaos':
      await self._cmd_chaos(args)
      return
    if cmd == 'mod':
      if args:
        await self.send_message(self._ctx.get_mod_description(' '.join(args)))
      else:
        await self.send_message('Usage: !mod <mod name>')
      return
    if cmd == 'mods':
      await self._cmd_mods(args)
      return
    if cmd == 'credits':
      target = args[0].lstrip('@').lower() if args else author
      await self.send_message(self._ctx.get_balance_message(target))
      return
    if cmd == 'addcredits':
      await self._cmd_add_credits(author, args)
      return
    if cmd == 'setcredits':
      await self._cmd_set_credits(author, args)
      return
    if cmd in ('givecredits', 'givecredit'):
      await self._cmd_give_credits(author, args)
      return
    if cmd == 'apply':
      await self._cmd_apply(author, args, is_streamer)
      return
    if cmd == 'enable':
      await self._cmd_enable_disable(author, args, True)
      return
    if cmd == 'disable':
      await self._cmd_enable_disable(author, args, False)
      return
    if cmd == 'resetmods':
      if await self._require_permission(author, 'manage_modifiers'):
        self._ctx.reset_mods = True
      return
    if cmd in ('raffle', 'chaosraffle'):
      await self._cmd_raffle(author, args)
      return
    if cmd in ('join', 'joinchaos'):
      if self._ctx.raffle_open:
        self._raffle_entries.add(author)
      return
    if cmd in ('startvote', 'newvote'):
      await self._cmd_start_vote(author, args)
      return
    if cmd == 'endvote':
      await self._cmd_end_vote(author)
      return
    if cmd == 'remove':
      await self._cmd_remove(author, args)
      return
    if cmd == 'addgroup':
      await self._cmd_add_group(author, args)
      return
    if cmd == 'addmember':
      await self._cmd_add_member(author, args)
      return
    if cmd == 'addperm':
      await self._cmd_add_perm(author, args)
      return
    if cmd == 'delmember':
      await self._cmd_del_member(author, args)
      return
    if cmd == 'delperm':
      await self._cmd_del_perm(author, args)
      return

  async def _cmd_chaos(self, args):
    if not args:
      await self.send_message(self._ctx.about_tcc())
      return
    sub = args[0].lower()
    if sub == 'voting':
      await self.send_message(self._ctx.explain_voting())
    elif sub == 'apply':
      await self.send_message(self._ctx.explain_redemption())
    elif sub == 'credits':
      await self.send_message(self._ctx.explain_credits())

  async def _cmd_mods(self, args):
    if not args:
      link = self._attr_str('mod_list_link')
      if link:
        await self.send_message(self._attr_str('msg_mod_list').format(link))
      return
    sub = args[0].lower()
    if sub == 'active':
      await self.send_message(self._ctx.list_active_mods())
    elif sub == 'voting':
      await self.send_message(self._ctx.list_candidate_mods())

  async def _cmd_add_credits(self, author: str, args):
    if not await self._require_permission(author, 'manage_credits'):
      return
    if not args:
      await self.send_message('Must specify a user to receive credits')
      return
    target = args[0].lstrip('@').lower()
    amount = 1
    if len(args) > 1:
      try:
        amount = int(args[1])
      except ValueError:
        await self.send_message('The amount must be a valid integer')
        return
      if amount <= 0:
        await self.send_message('Amount to add must be positive')
        return
    self._ctx.step_balance(target, amount)
    await self.send_message(self._ctx.get_balance_message(target))

  async def _cmd_start_vote(self, author: str, args):
    if not await self._require_permission(author, 'manage_voting'):
      return
    if self._ctx.voting_type == 'DISABLED' or self._ctx.voting_cycle == 'DISABLED':
      await self.send_message(self._attr_str('msg_no_voting'))
      return
    duration = None
    if args:
      try:
        duration = int(args[0])
      except ValueError:
        await self.send_message('Usage: !startvote (time)')
        return
      if duration < 1:
        await self.send_message('Vote time must be a positive integer')
        return
    self._ctx.request_start_vote(duration)
    if duration is None:
      await self.send_message('Starting a new vote.')
    else:
      await self.send_message(f'Starting a new vote for {duration} seconds.')

  async def _cmd_end_vote(self, author: str):
    if not await self._require_permission(author, 'manage_voting'):
      return
    if self._ctx.voting_type == 'DISABLED' or self._ctx.voting_cycle == 'DISABLED':
      await self.send_message(self._attr_str('msg_no_voting'))
      return
    if not self._ctx.vote_open:
      await self.send_message('No vote is currently open.')
      return
    self._ctx.request_end_vote()
    await self.send_message('Ending the current vote.')

  async def _cmd_set_credits(self, author: str, args):
    if not await self._require_permission(author, 'manage_credits'):
      return
    if len(args) < 2:
      await self.send_message('Usage: !setcredits <user> <amount>')
      return
    target = args[0].lstrip('@').lower()
    try:
      amount = int(args[1])
    except ValueError:
      await self.send_message('The amount must be a valid integer')
      return
    if amount < 0:
      await self.send_message('The amount cannot be negative')
      return
    self._ctx.set_balance(target, amount)
    await self.send_message(self._ctx.get_balance_message(target))

  async def _cmd_give_credits(self, author: str, args):
    if not args:
      await self.send_message('Usage: !givecredits <user> (amount)')
      return
    target = args[0].lstrip('@').lower()
    amount = 1
    if len(args) > 1:
      try:
        amount = int(args[1])
      except ValueError:
        await self.send_message('The amount must be a valid integer')
        return
      if amount < 1:
        await self.send_message('The amount must be positive')
        return
    donor_balance = self._ctx.get_balance(author)
    if donor_balance < amount and author != self._channel_name:
      await self.send_message(self._ctx.get_balance_message(author))
      return
    self._ctx.step_balance(target, amount)
    if author != self._channel_name:
      self._ctx.step_balance(author, -amount)
    plural = '' if amount == 1 else 's'
    await self.send_message(f'@{author} has given @{target} {amount} mod credit{plural}')

  async def _cmd_apply(self, author: str, args, is_streamer: bool):
    if not args:
      await self.send_message('Usage: !apply <mod name>')
      return
    if not is_streamer:
      elapsed = time.monotonic() - self._last_apply_time
      if elapsed < self._ctx.redemption_cooldown:
        await self.send_message(self._attr_str('msg_in_cooldown'))
        return
      if self._ctx.get_balance(author) <= 0:
        await self.send_message(self._attr_str('msg_no_credits'))
        return

    request = ' '.join(args)
    status = self._ctx.mod_enabled(request)
    if status is None:
      await self.send_message(self._attr_str('msg_mod_not_found').format(request))
      return
    if not status:
      await self.send_message(self._attr_str('msg_mod_not_active').format(request))
      return

    self._ctx.force_mod = request.lower()
    self._last_apply_time = time.monotonic()
    if not is_streamer:
      self._ctx.step_balance(author, -1)
    await self.send_message(self._attr_str('msg_mod_applied'))

  async def _cmd_enable_disable(self, author: str, args, enabled: bool):
    if not await self._require_permission(author, 'manage_modifiers'):
      return
    if not args:
      await self.send_message('Usage: !enable <mod name>' if enabled else 'Usage: !disable <mod name>')
      return
    request = ' '.join(args)
    await self.send_message(self._ctx.set_mod_enabled(request, enabled))

  async def _cmd_remove(self, author: str, args):
    if not await self._require_permission(author, 'admin'):
      return
    if not args:
      await self.send_message('Usage: !remove <mod name>')
      return
    request = ' '.join(args)
    if self._ctx.mod_enabled(request) is None:
      await self.send_message(self._attr_str('msg_mod_not_found').format(request))
      return
    self._ctx.request_remove_mod(request.lower())
    await self.send_message('Modifier removed')

  async def _cmd_raffle(self, author: str, args):
    if not await self._require_permission(author, 'manage_raffles'):
      return
    if not self._attr_bool('raffles'):
      await self.send_message('Raffles are currently disabled.')
      return
    if self._ctx.raffle_open:
      await self.send_message('A raffle is already open')
      return
    raffle_length = self._attr_int('raffle_time', 60)
    if args:
      try:
        raffle_length = int(args[0])
      except ValueError:
        await self.send_message('Raffle time must be an integer')
        return
    if raffle_length < 30:
      await self.send_message('Raffle time must be at least 30 seconds')
      return
    self._raffle_task = asyncio.create_task(self._raffle_timer_loop(raffle_length))

  async def _cmd_add_group(self, author: str, args):
    if not await self._require_permission(author, 'manage_permissions'):
      return
    if len(args) < 1:
      await self.send_message('Usage: !addgroup <group>')
      return
    await self.send_message(self._ctx.add_permission_group(args[0]))

  async def _cmd_add_member(self, author: str, args):
    if not await self._require_permission(author, 'manage_permissions'):
      return
    if len(args) < 2:
      await self.send_message('Usage: !addmember <group> <user>')
      return
    await self.send_message(self._ctx.add_group_member(args[0], args[1]))

  async def _cmd_add_perm(self, author: str, args):
    if not await self._require_permission(author, 'manage_permissions'):
      return
    if len(args) < 2:
      await self.send_message('Usage: !addperm <group> <permission>')
      return
    await self.send_message(self._ctx.add_group_permission(args[0], args[1]))

  async def _cmd_del_member(self, author: str, args):
    if not await self._require_permission(author, 'manage_permissions'):
      return
    if len(args) < 2:
      await self.send_message('Usage: !delmember <group> <user>')
      return
    await self.send_message(self._ctx.remove_group_member(args[0], args[1]))

  async def _cmd_del_perm(self, author: str, args):
    if not await self._require_permission(author, 'manage_permissions'):
      return
    if len(args) < 2:
      await self.send_message('Usage: !delperm <group> <permission>')
      return
    await self.send_message(self._ctx.remove_group_permission(args[0], args[1]))

  async def _raffle_timer_loop(self, raffle_length: int):
    self._raffle_entries = set()
    self._ctx.raffle_open = True
    self._ctx.raffle_start_time = time.time()
    interval = self._attr_float('raffle_message_interval', 15.0)
    time_elapsed = 0.0
    while time_elapsed < raffle_length and not self._shutdown.is_set():
      if time_elapsed == 0:
        status = 'has begun'
      else:
        status = f'will close in {int(raffle_length - time_elapsed)} seconds'
      notice = self._attr_str('msg_raffle_open')
      await self.send_message(notice.format(status=status))
      await asyncio.sleep(interval)
      time_elapsed = time.time() - self._ctx.raffle_start_time

    self._ctx.raffle_open = False
    if not self._raffle_entries:
      await self.send_message('No one joined the raffle :(')
    else:
      winner = random.choice(list(self._raffle_entries))
      self._ctx.step_balance(winner, 1)
      await self.send_message(f"Congratulations @{winner}! You've won a modifier credit.")

  async def _on_points_redemption(self, payload):
    if not self._ctx.points_redemptions:
      return
    event = getattr(payload, 'event', payload)
    reward = getattr(event, 'reward', None)
    title = getattr(reward, 'title', '')
    if title != self._ctx.points_reward_title:
      return
    user_name = (getattr(event, 'user_name', '') or '').lower()
    if user_name:
      balance = self._ctx.step_balance(user_name, 1)
      await self.send_message(self._attr_str('msg_user_balance').format(
        user=user_name,
        balance=balance,
        plural='' if balance == 1 else 's',
      ))

  async def _on_channel_cheer(self, payload):
    if not self._ctx.bits_redemptions:
      return
    event = getattr(payload, 'event', payload)
    bits = int(getattr(event, 'bits', 0))
    if bits < self._ctx.bits_per_credit:
      return
    credits = bits // self._ctx.bits_per_credit if self._ctx.multiple_credits else 1
    user_name = (getattr(event, 'user_name', '') or '').lower()
    if user_name and credits > 0:
      balance = self._ctx.step_balance(user_name, credits)
      await self.send_message(self._attr_str('msg_user_balance').format(
        user=user_name,
        balance=balance,
        plural='' if balance == 1 else 's',
      ))

  async def send_message(self, msg: str):
    if self._chat and msg:
      await self._chat.send_message(self._channel_name, msg)

  async def _disconnect(self):
    if self._raffle_task and not self._raffle_task.done():
      self._raffle_task.cancel()
    self._raffle_task = None
    self._ctx.raffle_open = False

    if self._eventsub:
      with suppress(Exception):
        await self._eventsub.stop()
      self._eventsub = None
    if self._chat:
      with suppress(Exception):
        self._chat.stop()
      self._chat = None
    if self._event_twitch:
      with suppress(Exception):
        await self._event_twitch.close()
      self._event_twitch = None
    if self._chat_twitch:
      with suppress(Exception):
        await self._chat_twitch.close()
      self._chat_twitch = None

  def shutdown(self):
    self._shutdown.set()
    if self._loop and self._loop.is_running():
      fut = asyncio.run_coroutine_threadsafe(self._disconnect(), self._loop)
      with suppress(Exception):
        fut.result(timeout=10.0)
    if self._thread and self._thread.is_alive():
      self._thread.join(timeout=10.0)
