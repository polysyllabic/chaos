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
