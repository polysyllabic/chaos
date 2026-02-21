from __future__ import annotations

import base64
import datetime as dt
import hashlib
import hmac
import ipaddress
import json
import logging
import os
from pathlib import Path
from typing import Optional, Tuple

from cryptography import x509
from cryptography.fernet import Fernet, InvalidToken
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID

CONFIG_DIR = Path.cwd() / 'configs'
UI_AUTH_KEY_FILE = CONFIG_DIR / '.ui_auth_key'
TLS_DIR = CONFIG_DIR / 'tls'
DEFAULT_SELF_SIGNED_CERT_FILE = TLS_DIR / 'selfsigned-cert.pem'
DEFAULT_SELF_SIGNED_KEY_FILE = TLS_DIR / 'selfsigned-key.pem'

SCRYPT_N = 2 ** 14
SCRYPT_R = 8
SCRYPT_P = 1
SCRYPT_DKLEN = 32


def _ensure_config_dir() -> None:
  CONFIG_DIR.mkdir(parents=True, exist_ok=True)


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


def default_self_signed_cert_paths() -> Tuple[Path, Path]:
  return DEFAULT_SELF_SIGNED_CERT_FILE, DEFAULT_SELF_SIGNED_KEY_FILE


def _load_or_create_fernet() -> Optional[Fernet]:
  try:
    _ensure_config_dir()
    if UI_AUTH_KEY_FILE.exists():
      key = UI_AUTH_KEY_FILE.read_bytes().strip()
    else:
      key = Fernet.generate_key()
      _write_private_bytes(UI_AUTH_KEY_FILE, key + b'\n')
    return Fernet(key)
  except Exception:
    logging.exception('Failed to initialize UI auth encryption key')
    return None


def encrypt_password(password: str) -> str:
  clean = str(password or '')
  if not clean:
    return ''
  fernet = _load_or_create_fernet()
  if fernet is None:
    return ''
  salt = os.urandom(16)
  digest = hashlib.scrypt(
    clean.encode('utf-8'),
    salt=salt,
    n=SCRYPT_N,
    r=SCRYPT_R,
    p=SCRYPT_P,
    dklen=SCRYPT_DKLEN,
  )
  payload = {
    'salt': base64.b64encode(salt).decode('ascii'),
    'digest': base64.b64encode(digest).decode('ascii'),
    'n': SCRYPT_N,
    'r': SCRYPT_R,
    'p': SCRYPT_P,
    'dklen': SCRYPT_DKLEN,
  }
  return fernet.encrypt(json.dumps(payload).encode('utf-8')).decode('ascii')


def verify_encrypted_password(password: str, encrypted_payload: str) -> bool:
  clean = str(password or '')
  token = str(encrypted_payload or '')
  if not clean or not token:
    return False
  fernet = _load_or_create_fernet()
  if fernet is None:
    return False
  try:
    payload_raw = fernet.decrypt(token.encode('ascii')).decode('utf-8')
    payload = json.loads(payload_raw)
  except (InvalidToken, ValueError, UnicodeDecodeError):
    return False
  except Exception:
    logging.exception('Unexpected error while decrypting UI password payload')
    return False

  try:
    salt = base64.b64decode(payload['salt'])
    expected_digest = base64.b64decode(payload['digest'])
    derived = hashlib.scrypt(
      clean.encode('utf-8'),
      salt=salt,
      n=int(payload.get('n', SCRYPT_N)),
      r=int(payload.get('r', SCRYPT_R)),
      p=int(payload.get('p', SCRYPT_P)),
      dklen=int(payload.get('dklen', SCRYPT_DKLEN)),
    )
  except Exception:
    return False
  return hmac.compare_digest(derived, expected_digest)


def ensure_self_signed_cert(
  cert_path: Path,
  key_path: Path,
  hostname: str,
  overwrite: bool = False,
) -> Tuple[str, str]:
  cert_file = Path(cert_path)
  key_file = Path(key_path)
  cert_file.parent.mkdir(parents=True, exist_ok=True)
  key_file.parent.mkdir(parents=True, exist_ok=True)

  if overwrite:
    try:
      cert_file.unlink()
    except FileNotFoundError:
      pass
    try:
      key_file.unlink()
    except FileNotFoundError:
      pass

  if cert_file.exists() and key_file.exists():
    return (str(cert_file), str(key_file))

  host = str(hostname or '').strip() or 'localhost'
  common_name = host
  ip_san = None
  try:
    ip_san = ipaddress.ip_address(host)
  except ValueError:
    ip_san = None

  key = rsa.generate_private_key(public_exponent=65537, key_size=2048)

  name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, common_name)])
  san_names: list[x509.GeneralName] = [x509.DNSName(common_name), x509.DNSName('localhost')]
  if ip_san is not None:
    san_names.append(x509.IPAddress(ip_san))
  else:
    san_names.append(x509.IPAddress(ipaddress.ip_address('127.0.0.1')))

  now = dt.datetime.now(dt.timezone.utc)
  cert = (
    x509.CertificateBuilder()
    .subject_name(name)
    .issuer_name(name)
    .public_key(key.public_key())
    .serial_number(x509.random_serial_number())
    .not_valid_before(now - dt.timedelta(minutes=1))
    .not_valid_after(now + dt.timedelta(days=825))
    .add_extension(x509.SubjectAlternativeName(san_names), critical=False)
    .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
    .sign(key, hashes.SHA256())
  )

  key_bytes = key.private_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PrivateFormat.TraditionalOpenSSL,
    encryption_algorithm=serialization.NoEncryption(),
  )
  cert_bytes = cert.public_bytes(serialization.Encoding.PEM)
  _write_private_bytes(key_file, key_bytes)
  _write_private_bytes(cert_file, cert_bytes)
  return (str(cert_file), str(key_file))
