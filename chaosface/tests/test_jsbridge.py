"""Relay mutation smoke tests."""

from chaosface.gui.ChaosRelay import ChaosRelay


def test_relay_setters_and_getters():
  relay = ChaosRelay()
  relay.set_vote_options(4)
  relay.set_voting_type('Majority')

  assert relay.vote_options == 4
  assert relay.voting_type == 'Majority'


def test_relay_game_config_upload_queue_round_trips():
  relay = ChaosRelay()
  files = [
    {'name': 'alpha.toml', 'content_b64': 'YWJj'},
    {'name': 'beta.toml', 'content_b64': 'ZGVm'},
  ]

  relay.request_game_config_upload(files)

  assert relay.consume_game_config_upload_request() == files
  assert relay.consume_game_config_upload_request() == []
