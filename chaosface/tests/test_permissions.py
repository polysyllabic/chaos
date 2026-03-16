"""Permission-group behavior tests for chaosface relay authorization."""

from uuid import uuid4

from chaosface.gui.ChaosRelay import ChaosRelay


def _rand(prefix: str) -> str:
  return f'{prefix}_{uuid4().hex[:10]}'


def test_streamer_has_admin_permissions():
  relay = ChaosRelay()
  streamer = relay.channel_name

  assert relay.has_permission(streamer, 'manage_permissions')
  assert relay.has_permission(streamer, 'manage_credits')
  assert relay.has_permission(streamer, 'manage_modifiers')
  assert relay.has_permission(streamer, 'manage_raffles')


def test_group_permission_grant_and_revoke():
  relay = ChaosRelay()
  group = _rand('pytest_group')
  user = _rand('pytest_user')

  assert "Added permission group" in relay.add_permission_group(group)
  assert "Added permission 'manage_credits'" in relay.add_group_permission(group, 'manage_credits')
  assert f'Added @{user}' in relay.add_group_member(group, user)

  assert relay.has_permission(user, 'manage_credits')
  assert not relay.has_permission(user, 'manage_modifiers')

  assert "Removed permission 'manage_credits'" in relay.remove_group_permission(group, 'manage_credits')
  assert not relay.has_permission(user, 'manage_credits')


def test_unknown_permission_rejected():
  relay = ChaosRelay()
  group = _rand('pytest_group')
  relay.add_permission_group(group)

  msg = relay.add_group_permission(group, 'definitely_not_real')
  assert "Unknown permission 'definitely_not_real'" in msg


def test_vote_request_flags_round_trip():
  relay = ChaosRelay()

  assert relay.consume_start_vote_request() is None
  relay.request_start_vote()
  assert relay.consume_start_vote_request() == 0.0

  relay.request_start_vote(15)
  assert relay.consume_start_vote_request() == 15.0

  assert relay.consume_end_vote_request() is False
  relay.request_end_vote()
  assert relay.consume_end_vote_request() is True
  assert relay.consume_end_vote_request() is False


def test_game_selection_force_reload_flag_round_trip():
  relay = ChaosRelay()

  relay.request_game_selection('The Last of Us 2', force_reload=True)
  assert relay.consume_game_selection_request() == 'The Last of Us 2'
  assert relay.consume_game_selection_force_reload() is True
  assert relay.consume_game_selection_force_reload() is False

  relay.request_game_selection('The Last of Us 2', force_reload=False)
  assert relay.consume_game_selection_request() == 'The Last of Us 2'
  assert relay.consume_game_selection_force_reload() is False


def test_tally_vote_rejects_stale_rounds_and_closed_votes():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Vote Round Guard Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
      {'name': 'Mod D', 'desc': 'D', 'groups': ['Test']},
    ],
  })

  relay.set_vote_open(True)
  relay.get_new_voting_pool()
  first_round = relay.vote_round

  relay.tally_vote(0, 'alice', first_round)
  assert relay.votes[0] == 1

  relay.get_new_voting_pool()
  assert relay.vote_round > first_round
  assert sum(relay.votes) == 0
  assert relay.voted_users == []

  relay.tally_vote(0, 'alice', first_round)
  assert sum(relay.votes) == 0

  relay.set_vote_open(False)
  relay.tally_vote(0, 'bob', relay.vote_round)
  assert sum(relay.votes) == 0


def test_remove_mod_request_and_active_slot_compacts_progress_slots():
  relay = ChaosRelay()
  relay.set_num_active_mods(3)
  relay.set_active_mods(['Mod A', 'Mod B', 'Mod C'])
  relay.set_mod_times([1.0, 0.7, 0.4])

  relay.request_remove_mod('mod a')
  assert relay.consume_remove_mod_request() == 'mod a'
  assert relay.consume_remove_mod_request() == ''

  relay.remove_active_mod('mod a')
  assert relay.active_mods == ['Mod B', 'Mod C', '']
  assert relay.mod_times == [0.7, 0.4, 0.0]


def test_decrement_mod_times_removes_all_expired_mods_from_display_state():
  relay = ChaosRelay()
  relay.set_num_active_mods(3)
  relay.set_time_per_modifier(100.0)
  relay.set_active_mods(['Mod A', 'Mod B', ''])
  relay.set_mod_times([0.35, 0.03, 0.0])

  # Expire the oldest remaining mod after a prior manual removal case.
  relay.decrement_mod_times(5.0)

  assert relay.active_mods == ['Mod A', '', '']
  assert relay.mod_times == [0.30, 0.0, 0.0]

  # Expire the last active mod; display state should fully clear.
  relay.decrement_mod_times(40.0)

  assert relay.active_mods == ['', '', '']
  assert relay.mod_times == [0.0, 0.0, 0.0]


def test_restore_active_mods_hydrates_names_progress_and_keys():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Reconnect Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
    ],
  })

  relay.restore_active_mods(['Mod B', 'Mod C'], [0.6, 0.25])

  assert relay.active_mods == ['Mod B', 'Mod C', '']
  assert relay.mod_times == [0.6, 0.25, 0.0]
  assert relay.active_keys == ['mod b', 'mod c', '']


def test_restore_active_mods_sorts_by_remaining_time_descending():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Restore Sort Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
    ],
  })

  relay.restore_active_mods(['Mod B', 'Mod C', 'Mod A'], [0.1, 0.8, 0.4])

  assert relay.active_mods == ['Mod C', 'Mod A', 'Mod B']
  assert relay.mod_times == [0.8, 0.4, 0.1]
  assert relay.active_keys == ['mod c', 'mod a', 'mod b']


def test_replace_mod_places_newest_modifier_at_top():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Replace Sort Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
      {'name': 'Mod D', 'desc': 'D', 'groups': ['Test']},
    ],
  })
  relay.restore_active_mods(['Mod A', 'Mod B', 'Mod C'], [0.9, 0.5, 0.1])

  relay.replace_mod('mod d')

  assert relay.active_mods == ['Mod D', 'Mod A', 'Mod B']
  assert relay.mod_times == [1.0, 0.9, 0.5]
  assert relay.active_keys == ['mod d', 'mod a', 'mod b']


def test_reset_all_uses_current_game_when_selected_game_is_none():
  relay = ChaosRelay()
  relay.chaos_config['current_game'] = 'The Last of Us 2'
  relay.chaos_config['selected_game'] = 'NONE'
  relay.set_vote_time(0.73)

  relay.reset_all()

  assert relay.game_name == 'The Last of Us 2'
  assert relay.selected_game == 'The Last of Us 2'
  assert relay.vote_time == 0.0


def test_initialize_game_resets_vote_timer_and_closes_vote():
  relay = ChaosRelay()
  relay.set_vote_open(True)
  relay.set_vote_time(0.61)

  relay.initialize_game({
    'game': 'The Last of Us 2',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
    ],
  })

  assert relay.vote_open is False
  assert relay.vote_time == 0.0


def test_initialize_game_tolerates_null_mods_payload():
  relay = ChaosRelay()
  relay.set_vote_open(True)
  relay.set_vote_time(0.61)

  relay.initialize_game({
    'game': 'The Last of Us 2',
    'errors': 1,
    'nmods': 3,
    'modtime': 180.0,
    'mods': None,
  })

  assert relay.modifier_data == {}
  assert relay.enabled_mods == []
  assert relay.vote_open is False
  assert relay.vote_time == 0.0


def test_set_mod_enabled_updates_enabled_mod_list():
  relay = ChaosRelay()
  relay.modifier_data = {
    'mod a': {'name': 'Mod A', 'active': True},
    'mod b': {'name': 'Mod B', 'active': True},
  }
  relay.enabled_mods = ['mod a', 'mod b']

  relay.set_mod_enabled('mod a', False)
  assert 'mod a' not in relay.enabled_mods
  assert 'mod b' in relay.enabled_mods

  relay.set_mod_enabled('mod a', True)
  assert 'mod a' in relay.enabled_mods


def test_get_mod_description_resolves_current_candidate_number():
  relay = ChaosRelay()
  relay.initialize_game({
    'game': 'Candidate Description Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Sir Robin', 'desc': 'Runs backwards.', 'groups': ['Movement']},
      {'name': 'Mod C', 'desc': 'C', 'groups': ['Test']},
    ],
  })
  relay.candidate_keys = ['mod a', 'sir robin', 'mod c']
  relay.set_candidate_mods(['Mod A', 'Sir Robin', 'Mod C'])

  assert relay.get_mod_description('2') == 'Runs backwards.'


def test_set_mod_enabled_persists_across_initialize_game_reload():
  relay = ChaosRelay()
  relay.old_mod_data = {
    'mod a': {'name': 'Mod A', 'active': False},
  }
  relay.modifier_data = {
    'mod a': {'name': 'Mod A', 'desc': 'A', 'groups': ['Test'], 'active': False},
  }
  relay.enabled_mods = []

  relay.set_mod_enabled('mod a', True)
  assert relay.old_mod_data['mod a']['active'] is True

  relay.initialize_game({
    'game': 'Reload Persist Test',
    'errors': 0,
    'nmods': 3,
    'modtime': 180.0,
    'mods': [
      {'name': 'Mod A', 'desc': 'A', 'groups': ['Test']},
      {'name': 'Mod B', 'desc': 'B', 'groups': ['Test']},
    ],
  })

  assert relay.modifier_data['mod a']['active'] is True
  assert 'mod a' in relay.enabled_mods


def test_explain_voting_handles_authoritarian_and_disabled_cycle():
  relay = ChaosRelay()
  relay.set_voting_type('Authoritarian')
  relay.set_voting_cycle('Continuous')
  authoritarian_msg = relay.explain_voting()
  assert 'Authoritarian' in authoritarian_msg

  relay.set_voting_cycle('DISABLED')
  assert relay.explain_voting() == relay.get_attribute('msg_no_voting')


def test_balance_message_pluralization():
  relay = ChaosRelay()
  relay.set_balance('someuser', 1)
  assert '1 modifier credit.' in relay.get_balance_message('someuser')

  relay.set_balance('someuser', 2)
  assert '2 modifier credits.' in relay.get_balance_message('someuser')


def test_raffle_time_has_30_second_minimum():
  relay = ChaosRelay()
  relay.set_raffle_time(5)
  assert relay.raffle_time == 30.0


def test_set_vote_duration_updates_vote_time_config_used_by_ui():
  relay = ChaosRelay()
  relay.chaos_config['vote_time'] = 60.0
  relay.set_vote_time(0.33)

  relay.set_vote_duration(42)

  assert float(relay.get_attribute('vote_time')) == 42.0
  assert relay.vote_time == 0.33


def test_set_vote_delay_updates_vote_delay_config_used_by_ui():
  relay = ChaosRelay()
  relay.chaos_config['vote_delay'] = 0.0
  relay.set_vote_delay(17.5)
  assert float(relay.get_attribute('vote_delay')) == 17.5
