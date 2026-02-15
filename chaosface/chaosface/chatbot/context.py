#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Typed chatbot context adapters.
"""

from chaosface.chatbot.ChaosBot import ChaosBotContext


class RelayBotContext(ChaosBotContext):
  """Strictly typed adapter that exposes only bot-required relay behavior."""

  def __init__(self, relay):
    self._relay = relay

  @property
  def channel_name(self) -> str:
    return self._relay.channel_name

  @property
  def bot_oauth(self) -> str:
    return self._relay.bot_oauth

  @property
  def eventsub_oauth(self) -> str:
    return self._relay.eventsub_oauth

  @property
  def points_redemptions(self) -> bool:
    return self._relay.points_redemptions

  @property
  def bits_redemptions(self) -> bool:
    return self._relay.bits_redemptions

  @property
  def connected(self) -> bool:
    return self._relay.connected

  @property
  def voting_type(self) -> str:
    return self._relay.voting_type

  @property
  def voting_cycle(self) -> str:
    return self._relay.voting_cycle

  @property
  def vote_open(self) -> bool:
    return self._relay.vote_open

  @property
  def redemption_cooldown(self) -> float:
    return self._relay.redemption_cooldown

  @property
  def points_reward_title(self) -> str:
    return self._relay.points_reward_title

  @property
  def bits_per_credit(self) -> int:
    return self._relay.bits_per_credit

  @property
  def multiple_credits(self) -> bool:
    return self._relay.multiple_credits

  @property
  def raffle_open(self) -> bool:
    return self._relay.raffle_open

  @raffle_open.setter
  def raffle_open(self, value: bool):
    self._relay.raffle_open = value

  @property
  def raffle_start_time(self):
    return self._relay.raffle_start_time

  @raffle_start_time.setter
  def raffle_start_time(self, value):
    self._relay.raffle_start_time = value

  @property
  def reset_mods(self) -> bool:
    return self._relay.reset_mods

  @reset_mods.setter
  def reset_mods(self, value: bool):
    self._relay.reset_mods = value

  @property
  def force_mod(self) -> str:
    return self._relay.force_mod

  @force_mod.setter
  def force_mod(self, value: str):
    self._relay.force_mod = value

  def get_attribute(self, key):
    return self._relay.get_attribute(key)

  def get_mod_description(self, mod: str) -> str:
    return self._relay.get_mod_description(mod)

  def get_balance_message(self, user: str) -> str:
    return self._relay.get_balance_message(user)

  def about_tcc(self) -> str:
    return self._relay.about_tcc()

  def explain_voting(self) -> str:
    return self._relay.explain_voting()

  def explain_redemption(self) -> str:
    return self._relay.explain_redemption()

  def explain_credits(self) -> str:
    return self._relay.explain_credits()

  def list_active_mods(self) -> str:
    return self._relay.list_active_mods()

  def list_candidate_mods(self) -> str:
    return self._relay.list_candidate_mods()

  def step_balance(self, user: str, delta: int) -> int:
    return self._relay.step_balance(user, delta)

  def set_balance(self, user: str, balance: int) -> int:
    return self._relay.set_balance(user, balance)

  def get_balance(self, user: str) -> int:
    return self._relay.get_balance(user)

  def mod_enabled(self, mod: str):
    return self._relay.mod_enabled(mod)

  def set_mod_enabled(self, mod: str, enabled: bool) -> str:
    return self._relay.set_mod_enabled(mod, enabled)

  def request_start_vote(self, duration=None) -> None:
    self._relay.request_start_vote(duration)

  def request_end_vote(self) -> None:
    self._relay.request_end_vote()

  def request_remove_mod(self, mod_key: str) -> None:
    self._relay.request_remove_mod(mod_key)

  def has_permission(self, user: str, permission: str) -> bool:
    return self._relay.has_permission(user, permission)

  def add_permission_group(self, group: str) -> str:
    return self._relay.add_permission_group(group)

  def add_group_member(self, group: str, user: str) -> str:
    return self._relay.add_group_member(group, user)

  def add_group_permission(self, group: str, permission: str) -> str:
    return self._relay.add_group_permission(group, permission)

  def remove_group_member(self, group: str, user: str) -> str:
    return self._relay.remove_group_member(group, user)

  def remove_group_permission(self, group: str, permission: str) -> str:
    return self._relay.remove_group_permission(group, permission)
