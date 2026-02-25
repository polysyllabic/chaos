"""Command-result gating tests for ChaosModel engine actions."""

import threading
import time

from chaosface.ChaosModel import ChaosModel


def _blank_model() -> ChaosModel:
  model = ChaosModel.__new__(ChaosModel)
  model._command_result_lock = threading.Lock()  # type: ignore[attr-defined]
  model._command_result_condition = threading.Condition(model._command_result_lock)  # type: ignore[attr-defined]
  model._command_results = {}  # type: ignore[attr-defined]
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
