"""OAuth redirect and TLS self-signed cert helper regression tests."""

from pathlib import Path

from chaosface.config import security_utils, token_utils


def test_generate_auth_url_uses_redirect_override():
  redirect_url = 'https://example.invalid/api/oauth/callback'
  auth_url = token_utils.generate_auth_url(
    'client123',
    token_utils.Scopes.CHAT_READ,
    redirect_url=redirect_url,
  )
  assert 'redirect_uri=https%3A%2F%2Fexample.invalid%2Fapi%2Foauth%2Fcallback' in auth_url


def test_ensure_self_signed_cert_overwrite(tmp_path: Path):
  cert_file = tmp_path / 'tls' / 'cert.pem'
  key_file = tmp_path / 'tls' / 'key.pem'

  security_utils.ensure_self_signed_cert(cert_file, key_file, 'old-host.invalid')
  first_cert = cert_file.read_bytes()

  security_utils.ensure_self_signed_cert(
    cert_file,
    key_file,
    'new-host.invalid',
    overwrite=True,
  )
  second_cert = cert_file.read_bytes()

  assert first_cert != second_cert

