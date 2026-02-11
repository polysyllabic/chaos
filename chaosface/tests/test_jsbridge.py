"""Relay mutation smoke tests."""

from chaosface.gui.ChaosRelay import ChaosRelay


def test_relay_setters_and_getters():
  relay = ChaosRelay()
  relay.set_vote_options(4)
  relay.set_voting_type('Majority')

  assert relay.vote_options == 4
  assert relay.voting_type == 'Majority'
