"""Relay listener behavior smoke tests."""

from chaosface.gui.ChaosRelay import ChaosRelay


def test_relay_field_listener_is_called_on_change():
  relay = ChaosRelay()
  seen = []
  relay.on('listen_port', lambda value: seen.append(value))

  relay.set_listen_port(6000)
  assert seen and seen[-1] == 6000
