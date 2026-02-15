from __future__ import annotations

import json
import logging
import os
import secrets
import threading
import time
from pathlib import Path
from typing import Dict, Optional, Sequence
from urllib.parse import urlencode

import requests
from cryptography.fernet import Fernet, InvalidToken


class Scopes:
  BITS_READ = 'bits:read'
  CHANNEL_READ_REDEMPTIONS = 'channel:read:redemptions'
  CHAT_READ = 'chat:read'
  CHAT_EDIT = 'chat:edit'
  CHANNEL_MODERATE = 'channel:moderate'
  WHISPERS_READ = 'whispers:read'
  WHISPERS_EDIT = 'whispers:edit'
  CHANNEL_EDITOR = 'channel_editor'
  MODERATOR_ANNOUNCEMENT_MANAGE = 'moderator:manage:announcements'
  MODERATOR_SHOUTOUT_MANAGE = 'moderator:manage:shoutouts'
  MODERATOR_BAN_MANAGE = 'moderator:manage:banned_users'
  MODERATOR_READ_CHATTERS = 'moderator:read:chatters'


VALIDATE_URL = 'https://id.twitch.tv/oauth2/validate'
AUTHORIZE_URL = 'https://id.twitch.tv/oauth2/authorize'
REDIRECT_PATH = '/api/oauth/callback'
DEFAULT_REDIRECT_URL = f'http://localhost{REDIRECT_PATH}'
SUPPORTED_OAUTH_TARGETS = {'bot', 'eventsub'}
OAUTH_STATE_TTL_SECONDS = 600

OAUTH_STORAGE_DIR = Path.cwd() / 'configs'
OAUTH_STORAGE_KEY_FILE = OAUTH_STORAGE_DIR / '.oauth_key'
OAUTH_STORAGE_FILE = OAUTH_STORAGE_DIR / '.oauth_tokens.json'

BOT_OAUTH_SCOPES = (
  Scopes.CHAT_READ,
  Scopes.CHAT_EDIT,
  Scopes.CHANNEL_MODERATE,
  Scopes.WHISPERS_READ,
  Scopes.WHISPERS_EDIT,
  Scopes.CHANNEL_EDITOR,
)
EVENTSUB_OAUTH_SCOPES = (
  Scopes.CHANNEL_READ_REDEMPTIONS,
  Scopes.BITS_READ,
)

_oauth_lock = threading.Lock()
_oauth_state_cache: Dict[str, tuple[str, float]] = {}
_generated_oauth_cache: Dict[str, str] = {}
_oauth_fernet: Optional[Fernet] = None


def _now() -> float:
  return time.time()


def _purge_expired_states() -> None:
  now = _now()
  expired = [key for key, (_, expires_at) in _oauth_state_cache.items() if expires_at <= now]
  for key in expired:
    del _oauth_state_cache[key]


def _normalize_token(token: str) -> str:
  clean = (token or '').strip()
  if not clean:
    return ''
  if clean.startswith('oauth:'):
    return clean
  return f'oauth:{clean}'


def join_scopes(*scopes: str) -> str:
  return ' '.join(scope for scope in scopes if scope)


def _ensure_oauth_storage_dir() -> None:
  OAUTH_STORAGE_DIR.mkdir(parents=True, exist_ok=True)


def _write_private_bytes(path: Path, data: bytes) -> None:
  tmp_path = path.with_suffix(f'{path.suffix}.tmp')
  tmp_path.write_bytes(data)
  try:
    os.chmod(tmp_path, 0o600)
  except OSError:
    pass
  tmp_path.replace(path)
  try:
    os.chmod(path, 0o600)
  except OSError:
    pass


def _get_fernet_locked() -> Optional[Fernet]:
  global _oauth_fernet
  if _oauth_fernet is not None:
    return _oauth_fernet

  try:
    _ensure_oauth_storage_dir()
    if OAUTH_STORAGE_KEY_FILE.exists():
      key = OAUTH_STORAGE_KEY_FILE.read_bytes().strip()
    else:
      key = Fernet.generate_key()
      _write_private_bytes(OAUTH_STORAGE_KEY_FILE, key + b'\n')
    _oauth_fernet = Fernet(key)
  except Exception:
    logging.exception('Failed to initialize OAuth encryption key; regenerating')
    try:
      key = Fernet.generate_key()
      _write_private_bytes(OAUTH_STORAGE_KEY_FILE, key + b'\n')
      _oauth_fernet = Fernet(key)
    except Exception:
      logging.exception('Failed to regenerate OAuth encryption key')
      _oauth_fernet = None
  return _oauth_fernet


def _load_encrypted_tokens_locked() -> Dict[str, str]:
  if not OAUTH_STORAGE_FILE.exists():
    return {}
  try:
    raw = OAUTH_STORAGE_FILE.read_text(encoding='utf-8')
    data = json.loads(raw)
  except Exception:
    logging.exception('Failed to load encrypted OAuth token store')
    return {}
  if not isinstance(data, dict):
    return {}
  clean: Dict[str, str] = {}
  for key, value in data.items():
    if key in SUPPORTED_OAUTH_TARGETS and isinstance(value, str):
      clean[key] = value
  return clean


def _save_encrypted_tokens_locked(data: Dict[str, str]) -> None:
  try:
    _ensure_oauth_storage_dir()
    serialized = json.dumps(data, sort_keys=True, indent=2)
    _write_private_bytes(OAUTH_STORAGE_FILE, serialized.encode('utf-8'))
  except Exception:
    logging.exception('Failed to persist encrypted OAuth token store')


def _persist_generated_oauth_locked(target: str, normalized_token: str) -> None:
  fernet = _get_fernet_locked()
  if fernet is None:
    return
  token_store = _load_encrypted_tokens_locked()
  token_store[target] = fernet.encrypt(normalized_token.encode('utf-8')).decode('utf-8')
  _save_encrypted_tokens_locked(token_store)


def _consume_persisted_oauth_locked(target: str) -> str:
  fernet = _get_fernet_locked()
  if fernet is None:
    return ''
  token_store = _load_encrypted_tokens_locked()
  encrypted_token = token_store.pop(target, '')
  if encrypted_token:
    _save_encrypted_tokens_locked(token_store)
  if not encrypted_token:
    return ''
  try:
    token = fernet.decrypt(encrypted_token.encode('utf-8')).decode('utf-8')
  except InvalidToken:
    logging.error('Discarding OAuth token for %s: decrypt failed', target)
    return ''
  except Exception:
    logging.exception('Discarding OAuth token for %s: unexpected decrypt error', target)
    return ''
  return _normalize_token(token)


def build_authorize_url(
  client_id: str,
  redirect_url: str,
  scopes: Sequence[str],
  state: str = '',
) -> str:
  params = {
    'response_type': 'token',
    'client_id': client_id,
    'redirect_uri': redirect_url,
    'scope': join_scopes(*tuple(scopes)),
    'force_verify': 'false',
  }
  if state:
    params['state'] = state
  return f'{AUTHORIZE_URL}?{urlencode(params)}'


def generate_auth_url(client_id, *scopes):
  return build_authorize_url(client_id, DEFAULT_REDIRECT_URL, list(scopes))


def generate_irc_oauth(client_id, *extra_scopes):
  return generate_auth_url(
    client_id,
    Scopes.CHAT_READ,
    Scopes.CHAT_EDIT,
    Scopes.CHANNEL_MODERATE,
    Scopes.WHISPERS_READ,
    Scopes.WHISPERS_EDIT,
    Scopes.CHANNEL_EDITOR,
    *extra_scopes,
  )


def get_scopes_for_target(target: str) -> tuple[str, ...]:
  if target == 'bot':
    return BOT_OAUTH_SCOPES
  if target == 'eventsub':
    return EVENTSUB_OAUTH_SCOPES
  return ()


def create_oauth_state(target: str) -> str:
  if target not in SUPPORTED_OAUTH_TARGETS:
    raise ValueError(f'Unsupported OAuth target: {target}')
  state = secrets.token_urlsafe(24)
  expires_at = _now() + OAUTH_STATE_TTL_SECONDS
  with _oauth_lock:
    _purge_expired_states()
    _oauth_state_cache[state] = (target, expires_at)
  return state


def consume_oauth_state(state: str) -> Optional[str]:
  with _oauth_lock:
    _purge_expired_states()
    entry = _oauth_state_cache.pop(state, None)
  if entry is None:
    return None
  return entry[0]


def store_generated_oauth(target: str, token: str) -> None:
  if target not in SUPPORTED_OAUTH_TARGETS:
    return
  normalized = _normalize_token(token)
  if not normalized:
    return
  with _oauth_lock:
    _generated_oauth_cache[target] = normalized
    _persist_generated_oauth_locked(target, normalized)


def consume_generated_oauth(target: str) -> str:
  if target not in SUPPORTED_OAUTH_TARGETS:
    return ''
  with _oauth_lock:
    generated = _generated_oauth_cache.pop(target, '')
    if generated:
      _consume_persisted_oauth_locked(target)
      return generated
    return _consume_persisted_oauth_locked(target)


def validate_oauth(token: str) -> dict:
  return requests.get(VALIDATE_URL, headers={'Authorization': f'OAuth {token}'}).json()


def revoke_oauth_token(client_id: str, oauth: str):
  oauth = oauth.replace('oauth:', '')
  return requests.post(f'https://id.twitch.tv/oauth2/revoke?client_id={client_id}&token={oauth}')

  
