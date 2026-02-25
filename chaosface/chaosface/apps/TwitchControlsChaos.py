#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
Main entrypoint for the Twitch Controls Chaos operator UI and OBS overlays.

The operator UI is rendered with NiceGUI. Overlay pages are plain HTML served by FastAPI
endpoints for use as OBS browser sources.
"""

import argparse
import asyncio
import html
import logging
import secrets
import threading
import time
from pathlib import Path
from typing import Any, Dict, Optional
from urllib.parse import parse_qs, quote, urlsplit

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse, RedirectResponse, Response
from nicegui import app, ui
import uvicorn

import chaosface.config.globals as config
import chaosface.config.security_utils as security_utils
import chaosface.config.token_utils as token_utils
from chaosface.ChaosModel import ChaosModel
from chaosface.chatbot.ChaosBot import ChaosBot
from chaosface.chatbot.context import RelayBotContext
from chaosface.gui import ui_dispatch
from chaosface.gui.ActiveMods import active_mods_overlay_html
from chaosface.gui.ChaosInterface import build_chaos_interface
from chaosface.gui.CurrentVotes import current_votes_overlay_html
from chaosface.gui.VoteTimer import vote_timer_overlay_html
from chaosface.gui.overlay_state import overlay_state_payload

logging.basicConfig(level=logging.DEBUG)

_runtime_started = False
_runtime_lock = threading.Lock()
_model: Optional[ChaosModel] = None
_auth_lock = threading.Lock()
_ui_sessions: Dict[str, float] = {}
_uireq_probe_logged = False
_overlay_server: Optional[uvicorn.Server] = None
_overlay_thread: Optional[threading.Thread] = None

UI_SESSION_COOKIE = 'chaosface_session'
UI_SESSION_MAX_AGE_SECONDS = 60 * 60 * 24
UI_RECONNECT_TIMEOUT_SECONDS = 30.0
OVERLAY_PUBLIC_PATHS = {
  '/ActiveMods',
  '/activemods',
  '/VoteTimer',
  '/votetimer',
  '/CurrentVotes',
  '/currentvotes',
  '/api/overlay/state',
}


def _overlay_http_port() -> int:
  try:
    port = int(getattr(config.relay, 'overlay_http_port', 80))
  except Exception:
    return 0
  if port < 0:
    return 0
  if port > 65535:
    return 65535
  return port


def _overlay_http_enabled(ssl_enabled: bool) -> bool:
  if not ssl_enabled:
    return False
  overlay_port = _overlay_http_port()
  if overlay_port <= 0:
    return False
  try:
    ui_port = int(config.relay.get_attribute('ui_port'))
  except Exception:
    ui_port = 0
  if overlay_port == ui_port:
    logging.warning(
      'Overlay HTTP listener disabled because overlay_http_port (%s) equals ui_port (%s)',
      overlay_port,
      ui_port,
    )
    return False
  return True


def _should_trace_request(path: str) -> bool:
  return (
    path == '/'
    or path == '/login'
    or path.startswith('/api/auth/')
    or path.startswith('/_nicegui/')
    or path.startswith('/_nicegui_ws/')
    or 'socket.io' in path
  )


def _socket_query_details(request: Request) -> str:
  query = request.query_params
  fields = []
  for key in ('EIO', 'transport', 'sid', 't'):
    value = query.get(key)
    if value:
      fields.append(f'{key}={value}')
  return ' '.join(fields)


def _security_setup_html() -> str:
  return """<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Chaosface Security Setup</title>
    <style>
      body { font-family: sans-serif; margin: 2rem; line-height: 1.5; max-width: 40rem; }
      input { width: 100%; padding: 0.5rem; margin: 0.5rem 0; }
      button { margin-right: 0.5rem; margin-top: 0.5rem; padding: 0.5rem 0.8rem; }
      .warn { color: #8a3d00; }
    </style>
  </head>
  <body>
    <h2>Chaosface First-Run Security Setup</h2>
    <p>Set an optional UI login password. If set, each new browser session must log in.</p>
    <label for="pwd">UI password</label>
    <input id="pwd" type="password" autocomplete="new-password" />
    <div>
      <button id="set-password" type="button">Save Password</button>
      <button id="no-password" type="button">Continue Without Password</button>
    </div>
    <p id="status" class="warn"></p>
    <script>
      const statusEl = document.getElementById('status');
      const pwdEl = document.getElementById('pwd');

      async function submit(payload) {
        const response = await fetch('/api/security/setup', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
        });
        const data = await response.json().catch(() => ({}));
        if (!response.ok) {
          statusEl.textContent = data.detail || 'Security setup failed.';
          return;
        }
        if (data.mode === 'password') {
          window.location.href = '/login';
        } else {
          window.location.href = '/';
        }
      }

      document.getElementById('set-password').addEventListener('click', async () => {
        const password = (pwdEl.value || '').trim();
        if (!password) {
          statusEl.textContent = 'Enter a password, or choose Continue Without Password.';
          return;
        }
        await submit({ password, confirm_no_password: false });
      });

      document.getElementById('no-password').addEventListener('click', async () => {
        const proceed = window.confirm(
          'Continue with no UI password? Anyone on this network can access and change settings.'
        );
        if (!proceed) {
          return;
        }
        await submit({ password: '', confirm_no_password: true });
      });
    </script>
  </body>
</html>
"""


def _login_page_html(next_path: str, error_message: str = '') -> str:
  safe_next = quote(next_path or '/', safe='/?=&')
  safe_error = html.escape(str(error_message or '').strip())
  return f"""<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Chaosface Login</title>
    <style>
      body {{ font-family: sans-serif; margin: 2rem; line-height: 1.5; max-width: 28rem; }}
      input {{ width: 100%; padding: 0.5rem; margin: 0.5rem 0; }}
      button {{ margin-top: 0.5rem; padding: 0.5rem 0.8rem; }}
      .err {{ color: #8a0000; }}
    </style>
  </head>
  <body>
    <h2>Chaosface Login</h2>
    <form method="post" action="/api/auth/login-form">
      <input type="hidden" name="next" value="{safe_next}" />
      <label for="pwd">Password</label>
      <input id="pwd" name="password" type="password" autocomplete="current-password" />
      <button id="login" type="submit">Login</button>
    </form>
    <p id="status" class="err">{safe_error}</p>
  </body>
</html>
"""


def _session_cleanup_locked() -> None:
  now = time.time()
  expired = [token for token, exp in _ui_sessions.items() if exp <= now]
  for token in expired:
    del _ui_sessions[token]


def _create_ui_session() -> str:
  token = secrets.token_urlsafe(32)
  with _auth_lock:
    _session_cleanup_locked()
    _ui_sessions[token] = time.time() + UI_SESSION_MAX_AGE_SECONDS
  return token


def _has_ui_session(token: str) -> bool:
  if not token:
    return False
  with _auth_lock:
    _session_cleanup_locked()
    if token not in _ui_sessions:
      return False
    _ui_sessions[token] = time.time() + UI_SESSION_MAX_AGE_SECONDS
  return True


def _clear_ui_session(token: str) -> None:
  if not token:
    return
  with _auth_lock:
    _ui_sessions.pop(token, None)


def _auth_mode() -> str:
  mode = str(getattr(config.relay, 'ui_auth_mode', 'none')).strip().lower()
  if mode not in ('unset', 'password', 'none'):
    return 'none'
  return mode


def _is_public_request_path(path: str, mode: str) -> bool:
  # NiceGUI transport/static endpoints must stay reachable so the client can establish
  # the realtime channel that powers tab interactions and live status updates.
  if path.startswith('/_nicegui/'):
    return True
  if path.startswith('/_nicegui_ws/'):
    return True
  if path.startswith('/overlays/'):
    return True
  if path in OVERLAY_PUBLIC_PATHS:
    return True
  if mode == 'unset':
    return path in {'/setup-security', '/api/security/setup'}
  if mode == 'password':
    if path in {'/login', '/api/auth/login', '/api/auth/login-form', '/api/auth/logout'}:
      return True
  return False


def _normalize_realtime_path(path: str) -> str:
  # Canonical endpoint used by NiceGUI's mounted Socket.IO app.
  canonical = '/_nicegui_ws/socket.io'

  if path.startswith('/_nicegui_ws/socket.io/'):
    return path.replace('/_nicegui_ws/socket.io/', canonical, 1)
  if path.startswith('/_nicegui/ws/socket.io'):
    return path.replace('/_nicegui/ws/socket.io', canonical, 1)
  if path.startswith('/_nicegui/socket.io'):
    return path.replace('/_nicegui/socket.io', canonical, 1)
  return path


def _resolve_tls_files() -> tuple[str, str]:
  mode = str(getattr(config.relay, 'ui_tls_mode', 'off')).strip().lower()
  if mode == 'self-signed':
    cert_path, key_path = security_utils.default_self_signed_cert_paths()
    hostname = _default_ui_hostname()
    try:
      cert_file, key_file = security_utils.ensure_self_signed_cert(cert_path, key_path, hostname)
      return cert_file, key_file
    except Exception:
      logging.exception('Failed to initialize self-signed TLS certificate; starting without TLS')
      return '', ''

  if mode == 'custom':
    cert_file = Path(str(getattr(config.relay, 'ui_tls_cert_file', '') or '')).expanduser()
    key_file = Path(str(getattr(config.relay, 'ui_tls_key_file', '') or '')).expanduser()
    if cert_file.is_file() and key_file.is_file():
      return str(cert_file), str(key_file)
    logging.error('Custom TLS mode configured, but cert/key files are invalid: %s / %s', cert_file, key_file)
    return '', ''

  return '', ''


def _sanitize_hostname(value: str) -> str:
  raw = str(value or '').strip()
  if not raw:
    return ''
  if '://' in raw:
    parsed = urlsplit(raw)
    return parsed.hostname or ''
  parsed = urlsplit(f'//{raw}')
  return parsed.hostname or ''


def _default_ui_hostname() -> str:
  configured = _sanitize_hostname(getattr(config.relay, 'ui_tls_selfsigned_hostname', ''))
  if configured and configured.lower() != 'localhost':
    return configured
  pi_host = _sanitize_hostname(getattr(config.relay, 'pi_host', ''))
  if pi_host and pi_host.lower() != 'localhost':
    return pi_host
  return 'raspberrypi.local'


def _resolve_oauth_redirect_uri(request: Request) -> str:
  del request
  return token_utils.DEFAULT_REDIRECT_URL


def _oauth_callback_html() -> str:
  return """<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Twitch OAuth Complete</title>
    <style>
      body { font-family: sans-serif; margin: 2rem; line-height: 1.5; }
      code { background: #f3f3f3; padding: 0.1rem 0.3rem; border-radius: 0.2rem; }
    </style>
  </head>
  <body>
    <h2>Twitch OAuth</h2>
    <p id="status">Finalizing token capture...</p>
    <p><a href="/">Return to Chaosface connection setup</a></p>
    <script>
      (async () => {
        const status = document.getElementById('status');
        const hashParams = new URLSearchParams(window.location.hash.slice(1));
        const queryParams = new URLSearchParams(window.location.search);
        const token = hashParams.get('access_token');
        const state = hashParams.get('state');
        const error = hashParams.get('error') || queryParams.get('error');
        const errorDescription = hashParams.get('error_description') || queryParams.get('error_description') || '';

        if (error) {
          status.textContent = `OAuth failed: ${error}${errorDescription ? ` (${errorDescription})` : ''}`;
          return;
        }
        if (!token || !state) {
          status.textContent = 'Missing OAuth token or state in callback URL.';
          return;
        }

        try {
          const response = await fetch('/api/oauth/complete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ access_token: token, state }),
          });
          const payload = await response.json();
          if (!response.ok) {
            status.textContent = payload.detail || 'Failed to store OAuth token.';
            return;
          }
          status.textContent = `Token saved for ${payload.target}. Return to Connection Setup and click "Load generated tokens".`;
        } catch (err) {
          status.textContent = `Failed to store token: ${String(err)}`;
        }
      })();
    </script>
  </body>
</html>
"""


def _on_bot_connected(connected: bool):
  ui_dispatch.call_soon(config.relay.set_connected, connected)


def _on_bot_vote(vote_num: int, user: str):
  ui_dispatch.call_soon(config.relay.tally_vote, vote_num, user)


def _on_bot_status(message: str):
  ui_dispatch.call_soon(config.relay.add_bot_status, message)


def ensure_runtime_started(*, bind_ui_loop: bool = True) -> None:
  """
  Bind relay updates to the running asyncio loop and start model thread once.
  """
  global _runtime_started, _model
  loop: Optional[asyncio.AbstractEventLoop] = None
  try:
    loop = asyncio.get_running_loop()
  except RuntimeError:
    loop = None

  with _runtime_lock:
    if bind_ui_loop and loop is not None:
      ui_dispatch.set_ui_loop(loop)
    if not _runtime_started:
      if _model is None:
        _model = ChaosModel()
      if _model.thread is None or not _model.thread.is_alive():
        _model.start_threaded()
      _runtime_started = True


def shutdown_runtime() -> None:
  config.relay.keep_going = False
  if config.relay.chatbot:
    config.relay.chatbot.shutdown()


@app.on_event('startup')
async def _start_runtime_on_app_startup() -> None:
  """
  Start model/engine communication as soon as the service starts, independent of UI login.
  """
  ensure_runtime_started(bind_ui_loop=True)


def _overlay_state_response() -> Dict[str, Any]:
  ensure_runtime_started(bind_ui_loop=False)
  return overlay_state_payload()


def _active_mods_overlay_response() -> HTMLResponse:
  ensure_runtime_started(bind_ui_loop=False)
  return HTMLResponse(active_mods_overlay_html())


def _vote_timer_overlay_response() -> HTMLResponse:
  ensure_runtime_started(bind_ui_loop=False)
  return HTMLResponse(vote_timer_overlay_html())


def _current_votes_overlay_response() -> HTMLResponse:
  ensure_runtime_started(bind_ui_loop=False)
  return HTMLResponse(current_votes_overlay_html())


overlay_http_app = FastAPI()


@overlay_http_app.get('/api/overlay/state')
def overlay_http_state() -> Dict[str, Any]:
  return _overlay_state_response()


@overlay_http_app.get('/ActiveMods', response_class=HTMLResponse)
@overlay_http_app.get('/activemods', response_class=HTMLResponse)
@overlay_http_app.get('/overlays/active-mods', response_class=HTMLResponse)
def overlay_http_active_mods() -> HTMLResponse:
  return _active_mods_overlay_response()


@overlay_http_app.get('/VoteTimer', response_class=HTMLResponse)
@overlay_http_app.get('/votetimer', response_class=HTMLResponse)
@overlay_http_app.get('/overlays/vote-timer', response_class=HTMLResponse)
def overlay_http_vote_timer() -> HTMLResponse:
  return _vote_timer_overlay_response()


@overlay_http_app.get('/CurrentVotes', response_class=HTMLResponse)
@overlay_http_app.get('/currentvotes', response_class=HTMLResponse)
@overlay_http_app.get('/overlays/current-votes', response_class=HTMLResponse)
def overlay_http_current_votes() -> HTMLResponse:
  return _current_votes_overlay_response()


def _run_overlay_http_server(port: int) -> None:
  global _overlay_server
  try:
    server = uvicorn.Server(
      uvicorn.Config(
        overlay_http_app,
        host='0.0.0.0',
        port=port,
        log_level='warning',
        access_log=False,
        lifespan='off',
      )
    )
    _overlay_server = server
    logging.warning('Starting overlay HTTP listener on http://0.0.0.0:%s', port)
    server.run()
  except Exception:
    logging.exception('Overlay HTTP listener failed')
  finally:
    _overlay_server = None


def _start_overlay_http_server(ssl_enabled: bool) -> None:
  global _overlay_thread
  if not _overlay_http_enabled(ssl_enabled):
    return
  if _overlay_thread is not None and _overlay_thread.is_alive():
    return
  port = _overlay_http_port()
  _overlay_thread = threading.Thread(
    target=_run_overlay_http_server,
    args=(port,),
    daemon=True,
    name='chaosface-overlay-http',
  )
  _overlay_thread.start()
  time.sleep(0.2)
  if _overlay_thread is not None and not _overlay_thread.is_alive():
    logging.error('Overlay HTTP listener stopped during startup (port=%s)', port)


def _stop_overlay_http_server() -> None:
  global _overlay_server, _overlay_thread
  server = _overlay_server
  if server is not None:
    server.should_exit = True
  thread = _overlay_thread
  if thread is not None and thread.is_alive():
    thread.join(timeout=2.0)
  _overlay_server = None
  _overlay_thread = None


@app.middleware('http')
async def ui_auth_middleware(request: Request, call_next):
  global _uireq_probe_logged
  if not _uireq_probe_logged:
    logging.warning('UIREQ middleware instrumentation is active')
    _uireq_probe_logged = True

  started = time.monotonic()
  original_path = request.url.path
  path = _normalize_realtime_path(original_path)
  normalized = (path != original_path)
  if normalized:
    request.scope['path'] = path
    request.scope['raw_path'] = path.encode('utf-8')
  mode = _auth_mode()
  response: Optional[Response] = None

  if not _is_public_request_path(path, mode):
    if mode == 'unset':
      response = RedirectResponse('/setup-security')
    if mode == 'password':
      session_token = request.cookies.get(UI_SESSION_COOKIE, '')
      if not _has_ui_session(session_token):
        if (
          path.startswith('/api/')
          or path.startswith('/_nicegui/ws/')
          or path.startswith('/_nicegui_ws/')
        ):
          response = JSONResponse({'detail': 'Authentication required'}, status_code=401)
        else:
          response = RedirectResponse(f"/login?next={quote(path, safe='/?=&')}")

  if response is None:
    response = await call_next(request)

  if _should_trace_request(path):
    elapsed_ms = (time.monotonic() - started) * 1000.0
    socket_details = _socket_query_details(request)
    normalized_note = f' normalized_from={original_path}' if normalized else ''
    socket_note = f' {socket_details}' if socket_details else ''
    logging.warning(
      'UIREQ %s %s%s mode=%s -> %s in %.1fms%s',
      request.method,
      path,
      socket_note,
      mode,
      response.status_code,
      elapsed_ms,
      normalized_note,
    )

  return response


@app.get('/setup-security', response_class=HTMLResponse)
async def setup_security_page() -> Response:
  if _auth_mode() != 'unset':
    return RedirectResponse('/')
  return HTMLResponse(_security_setup_html())


@app.post('/api/security/setup')
async def api_security_setup(payload: Dict[str, Any]) -> Dict[str, str]:
  if _auth_mode() != 'unset':
    raise HTTPException(status_code=400, detail='Security has already been configured')

  password = str(payload.get('password') or '')
  confirm_no_password = bool(payload.get('confirm_no_password', False))

  if password:
    encrypted = security_utils.encrypt_password(password)
    if not encrypted:
      raise HTTPException(status_code=500, detail='Could not store encrypted UI password')
    config.relay.set_ui_password_encrypted(encrypted)
    config.relay.set_ui_auth_mode('password')
    config.relay.set_need_save(True)
    return {'mode': 'password'}

  if not confirm_no_password:
    raise HTTPException(status_code=400, detail='Confirm no-password mode before continuing')

  config.relay.set_ui_password_encrypted('')
  config.relay.set_ui_auth_mode('none')
  config.relay.set_need_save(True)
  return {'mode': 'none'}


@app.get('/login', response_class=HTMLResponse)
async def login_page(request: Request) -> Response:
  mode = _auth_mode()
  if mode == 'unset':
    return RedirectResponse('/setup-security')
  if mode != 'password':
    return RedirectResponse('/')

  session_token = request.cookies.get(UI_SESSION_COOKIE, '')
  if _has_ui_session(session_token):
    return RedirectResponse('/')

  next_path = str(request.query_params.get('next') or '/')
  error_message = str(request.query_params.get('error') or '').strip()
  return HTMLResponse(_login_page_html(next_path, error_message))


@app.post('/api/auth/login')
async def api_auth_login(request: Request, payload: Dict[str, Any]) -> JSONResponse:
  if _auth_mode() != 'password':
    return JSONResponse({'ok': True, 'mode': _auth_mode()})

  password = str(payload.get('password') or '')
  encrypted = str(getattr(config.relay, 'ui_password_encrypted', '') or '')
  if not security_utils.verify_encrypted_password(password, encrypted):
    raise HTTPException(status_code=401, detail='Invalid password')

  token = _create_ui_session()
  response = JSONResponse({'ok': True})
  response.set_cookie(
    key=UI_SESSION_COOKIE,
    value=token,
    # Deliberately use a session cookie so each new browser session requires login.
    httponly=True,
    secure=(request.url.scheme == 'https'),
    samesite='lax',
    path='/',
  )
  return response


@app.post('/api/auth/login-form')
async def api_auth_login_form(request: Request) -> Response:
  if _auth_mode() != 'password':
    return RedirectResponse('/', status_code=303)

  body = (await request.body()).decode('utf-8', errors='ignore')
  form = parse_qs(body, keep_blank_values=True)
  password = str((form.get('password') or [''])[0])
  next_path = str((form.get('next') or ['/'])[0]) or '/'
  next_path = quote(next_path, safe='/?=&')

  encrypted = str(getattr(config.relay, 'ui_password_encrypted', '') or '')
  if not security_utils.verify_encrypted_password(password, encrypted):
    return RedirectResponse(f'/login?next={next_path}&error={quote("Invalid password", safe="")}', status_code=303)

  token = _create_ui_session()
  response = RedirectResponse(next_path or '/', status_code=303)
  response.set_cookie(
    key=UI_SESSION_COOKIE,
    value=token,
    httponly=True,
    secure=(request.url.scheme == 'https'),
    samesite='lax',
    path='/',
  )
  return response


@app.post('/api/auth/logout')
async def api_auth_logout(request: Request) -> JSONResponse:
  session_token = request.cookies.get(UI_SESSION_COOKIE, '')
  _clear_ui_session(session_token)
  response = JSONResponse({'ok': True})
  response.delete_cookie(UI_SESSION_COOKIE, path='/')
  return response


@ui.page('/')
async def chaos_interface() -> None:
  build_chaos_interface(
    ensure_runtime_started=ensure_runtime_started,
    shutdown_runtime=shutdown_runtime,
  )


@app.get('/api/overlay/state')
async def api_overlay_state() -> Dict[str, Any]:
  return _overlay_state_response()


@app.get('/api/oauth/start')
async def api_oauth_start(request: Request, target: str) -> RedirectResponse:
  oauth_target = (target or '').strip().lower()
  if oauth_target not in token_utils.SUPPORTED_OAUTH_TARGETS:
    raise HTTPException(status_code=400, detail='Invalid OAuth target')

  client_id = str(config.relay.get_attribute('client_id') or '').strip()
  if not client_id:
    raise HTTPException(status_code=400, detail='Set Twitch client_id before generating OAuth tokens')

  state = token_utils.create_oauth_state(oauth_target)
  redirect_uri = _resolve_oauth_redirect_uri(request)
  scopes = token_utils.get_scopes_for_target(oauth_target)
  auth_url = token_utils.build_authorize_url(client_id, redirect_uri, list(scopes), state)
  return RedirectResponse(auth_url)


@app.get('/api/oauth/callback', name='api_oauth_callback', response_class=HTMLResponse)
async def api_oauth_callback() -> HTMLResponse:
  return HTMLResponse(_oauth_callback_html())


@app.post('/api/oauth/complete')
async def api_oauth_complete(payload: Dict[str, str]) -> Dict[str, str]:
  state = str(payload.get('state') or '').strip()
  access_token = str(payload.get('access_token') or '').strip()
  if not state or not access_token:
    raise HTTPException(status_code=400, detail='Missing OAuth callback state or token')

  target = token_utils.consume_oauth_state(state)
  if target is None:
    raise HTTPException(status_code=400, detail='OAuth state is invalid or expired. Try generating a new token.')

  token_utils.store_generated_oauth(target, access_token)
  return {'target': target}


@app.get('/ActiveMods', response_class=HTMLResponse)
@app.get('/activemods', response_class=HTMLResponse)
@app.get('/overlays/active-mods', response_class=HTMLResponse)
async def active_mods_overlay() -> HTMLResponse:
  return _active_mods_overlay_response()


@app.get('/VoteTimer', response_class=HTMLResponse)
@app.get('/votetimer', response_class=HTMLResponse)
@app.get('/overlays/vote-timer', response_class=HTMLResponse)
async def vote_timer_overlay() -> HTMLResponse:
  return _vote_timer_overlay_response()


@app.get('/CurrentVotes', response_class=HTMLResponse)
@app.get('/currentvotes', response_class=HTMLResponse)
@app.get('/overlays/current-votes', response_class=HTMLResponse)
async def current_votes_overlay() -> HTMLResponse:
  return _current_votes_overlay_response()


def twitch_controls_chaos(args):
  del args

  bot_context = RelayBotContext(config.relay)
  config.relay.chatbot = ChaosBot(
    context=bot_context,
    on_connected=_on_bot_connected,
    on_vote=_on_bot_vote,
    on_status=_on_bot_status,
  )
  ensure_runtime_started(bind_ui_loop=False)
  logging.warning('Configured Socket.IO transports: %s', app.config.socket_io_js_transports)
  logging.warning('Configured NiceGUI reconnect_timeout: %.1fs', UI_RECONNECT_TIMEOUT_SECONDS)

  ssl_certfile, ssl_keyfile = _resolve_tls_files()
  ssl_enabled = bool(ssl_certfile and ssl_keyfile)
  _start_overlay_http_server(ssl_enabled)
  try:
    if ssl_enabled:
      ui.run(
        host='0.0.0.0',
        port=config.relay.get_attribute('ui_port'),
        title='Twitch Controls Chaos',
        show=False,
        reload=False,
        reconnect_timeout=UI_RECONNECT_TIMEOUT_SECONDS,
        ssl_certfile=ssl_certfile,
        ssl_keyfile=ssl_keyfile,
      )
    else:
      ui.run(
        host='0.0.0.0',
        port=config.relay.get_attribute('ui_port'),
        title='Twitch Controls Chaos',
        show=False,
        reload=False,
        reconnect_timeout=UI_RECONNECT_TIMEOUT_SECONDS,
      )
  finally:
    _stop_overlay_http_server()
    shutdown_runtime()


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--reauthorize", help="Delete previous OAuth tokens and wait for new ones to attempt to connect")
  args = parser.parse_args()
  twitch_controls_chaos(args)
