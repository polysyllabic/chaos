"""Command-result gating tests for ChaosModel engine actions."""

import json
import threading
import time

from chaosface.ChaosModel import ChaosModel


def _blank_model() -> ChaosModel:
  model = ChaosModel.__new__(ChaosModel)
  model._command_result_lock = threading.Lock()  # type: ignore[attr-defined]
  model._command_result_condition = threading.Condition(model._command_result_lock)  # type: ignore[attr-defined]
  model._command_results = {}  # type: ignore[attr-defined]
  model._startup_sync_requested = False  # type: ignore[attr-defined]
  model._startup_pause_requested = False  # type: ignore[attr-defined]
  return model


def _blank_model_for_update_command() -> ChaosModel:
  model = _blank_model()
  model._select_game_lock = threading.Lock()  # type: ignore[attr-defined]
  model._pending_selected_game = ''  # type: ignore[attr-defined]
  model._pending_selected_inflight = False  # type: ignore[attr-defined]
  model._pending_selected_sent_at = 0.0  # type: ignore[attr-defined]
  model._pending_selected_retry_after = 0.0  # type: ignore[attr-defined]
  model._last_reported_game = ''  # type: ignore[attr-defined]
  model._last_failed_selected_game = ''  # type: ignore[attr-defined]
  model._pending_game_startup_setup = False  # type: ignore[attr-defined]
  model._manual_sync_requested = threading.Event()  # type: ignore[attr-defined]
  model._engine_handshake_complete = False  # type: ignore[attr-defined]
  model.request_game_info = lambda: None  # type: ignore[assignment]
  model.send_engine_command = lambda payload, report_status=True: True  # type: ignore[assignment]
  return model


def _blank_model_for_loop() -> ChaosModel:
  class _NoopCommunicator:
    def stop(self):
      return None

  model = _blank_model_for_update_command()
  model.request_available_games = lambda: None  # type: ignore[assignment]
  model.request_game_selection = lambda *_args, **_kwargs: None  # type: ignore[assignment]
  model._drive_pending_game_selection = lambda: None  # type: ignore[assignment]
  model.restart_bot = lambda: None  # type: ignore[assignment]
  model.chaos_communicator = _NoopCommunicator()  # type: ignore[attr-defined]
  model.bot = None  # type: ignore[attr-defined]
  return model


def test_send_engine_command_checked_reports_failed_result():
  model = _blank_model()

  def fake_send(payload, report_status=True):
    del report_status
    model._record_command_result({
      'command_id': payload['command_id'],
      'kind': 'winner',
      'ok': False,
      'message': 'modifier_not_found',
      'target': 'mod a',
    })
    return True

  model.send_engine_command = fake_send  # type: ignore[assignment]
  ok, message = model.send_engine_command_checked(
    {'winner': 'mod a'},
    expected_kind='winner',
    expected_target='mod a',
    timeout=0.2,
  )

  assert ok is False
  assert message == 'modifier_not_found'


def test_send_engine_command_checked_waits_for_async_result():
  model = _blank_model()

  def fake_send(payload, report_status=True):
    del report_status

    def emit_later():
      time.sleep(0.02)
      model._record_command_result({
        'command_id': payload['command_id'],
        'kind': 'reset',
        'ok': True,
        'message': 'modifiers_reset_requested',
      })

    threading.Thread(target=emit_later, daemon=True).start()
    return True

  model.send_engine_command = fake_send  # type: ignore[assignment]
  ok, message = model.send_engine_command_checked(
    {'reset': True},
    expected_kind='reset',
    timeout=0.5,
  )

  assert ok is True
  assert message == 'modifiers_reset_requested'


def test_sync_after_handshake_requests_game_and_pauses_when_running():
  model = _blank_model()
  sent_payloads = []
  requested_game_info = []

  model.request_game_info = lambda: requested_game_info.append(True)  # type: ignore[assignment]
  model.send_engine_command = lambda payload, report_status=True: (sent_payloads.append((payload, report_status)) or True)  # type: ignore[assignment]

  model._sync_after_handshake({'engine_status': 'running'})
  model._sync_after_handshake({'engine_status': 'running'})

  assert requested_game_info == [True]
  assert sent_payloads == [({'pause': True}, False)]


def test_sync_after_handshake_skips_pause_when_already_paused():
  model = _blank_model()
  sent_payloads = []
  requested_game_info = []

  model.request_game_info = lambda: requested_game_info.append(True)  # type: ignore[assignment]
  model.send_engine_command = lambda payload, report_status=True: (sent_payloads.append((payload, report_status)) or True)  # type: ignore[assignment]

  model._sync_after_handshake({'engine_status': 'paused'})

  assert requested_game_info == [True]
  assert sent_payloads == []


def test_parse_active_mod_snapshot_accepts_string_and_object_entries():
  payload = {
    'active_mods': [
      'Mod A',
      {'name': 'Mod B', 'remaining': 0.42},
      {'name': 'Mod C', 'progress': 0.25},
      {'name': ''},
    ],
  }

  names, progress = ChaosModel._parse_active_mod_snapshot(payload)

  assert names == ['Mod A', 'Mod B', 'Mod C']
  assert progress == [1.0, 0.42, 0.25]


def test_missing_remembered_game_waits_for_manual_selection():
  import chaosface.config.globals as config

  model = _blank_model_for_update_command()

  config.relay.reset_all()
  config.relay.set_selected_game('Missing Game')
  config.relay.valid_data = True

  model.update_command(json.dumps({
    'game': 'Missing Game',
    'errors': 1,
    'nmods': 3,
    'modtime': 180.0,
    'mods': None,
    'engine_status': 'bad_config_file',
  }))
  assert config.relay.valid_data is False
  model.update_command(json.dumps({
    'available_games': [
      {'game': 'Valid Game', 'file': '/usr/local/chaos/games/valid.toml'},
    ],
    'engine_status': 'waiting_for_game',
  }))

  assert config.relay.available_games == ['Valid Game']
  assert config.relay.get_preferred_game(config.relay.available_games) == ''
  assert model._pending_selected_game == ''
  assert config.relay.engine_status == ChaosModel.ENGINE_STATUS_WAITING_FOR_GAME
  assert config.relay.valid_data is False

  config.relay.request_game_selection('Valid Game')
  selection = config.relay.consume_game_selection_request()
  model.request_game_selection(selection, force=True)
  assert model._pending_selected_game == 'Valid Game'


def test_reenable_voting_from_disabled_starts_vote_and_candidates():
  import chaosface.config.globals as config
  from chaosface.gui import ui_dispatch

  model = _blank_model_for_loop()

  config.relay.reset_all()
  config.relay.initialize_game({
    'game': 'Voting Transition Test',
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
  config.relay.set_connected(True)
  config.relay.set_paused(False)
  config.relay.set_ui_rate(50.0)
  config.relay.set_voting_type('Proportional')
  config.relay.set_voting_cycle('DISABLED')
  config.relay.keep_going = True
  ui_dispatch._ui_loop = None

  loop_thread = threading.Thread(target=model._loop, daemon=True)
  loop_thread.start()
  try:
    time.sleep(0.12)
    config.relay.set_voting_cycle('Continuous')
    time.sleep(0.30)
  finally:
    config.relay.keep_going = False
    loop_thread.join(timeout=2.0)

  assert not loop_thread.is_alive()
  assert config.relay.vote_open is True
  assert config.relay.vote_time > 0.0
  assert any(str(name).strip() for name in config.relay.candidate_mods)
