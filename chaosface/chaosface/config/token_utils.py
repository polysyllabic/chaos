import logging
import requests
import logging

class Scopes:
    PUBSUB_BITS = 'bits:read'
    PUBSUB_BITS_BADGE = 'bits:read'
    PUBSUB_CHANNEL_POINTS = 'channel:read:redemptions'
    PUBSUB_CHANNEL_SUBSCRIPTION = 'channel_subscriptions'
    PUBSUB_MODERATOR_ACTIONS = 'channel:moderate'
    PUBSUB_WHISPERS = 'whispers:read'
    CHAT_READ = 'chat:read'
    CHAT_EDIT = 'chat:edit'
    CHANNEL_MODERATE = 'channel:moderate'
    WHISPERS_READ = 'whispers:read'
    WHISPERS_EDIT = 'whispers:edit'
    CHANNEL_EDITOR = 'channel_editor'

VALIDATE_URL = 'https://id.twitch.tv/oauth2/validate'
REDIRECT_URL = 'https://twitchapps.com/tmi/'

def generate_auth_url(client_id, *scopes):
    scopes = join_scopes(*scopes)
    return f'https://id.twitch.tv/oauth2/authorize?response_type=token&client_id={client_id}&redirect_uri={REDIRECT_URL}&scope={scopes}'


def generate_irc_oauth(client_id, *extra_scopes):
    return generate_auth_url(client_id, Scopes.CHAT_READ, Scopes.CHAT_EDIT, Scopes.CHANNEL_MODERATE, Scopes.WHISPERS_READ,
                             Scopes.WHISPERS_EDIT, Scopes.CHANNEL_EDITOR, *extra_scopes)

def join_scopes(*scopes):
    return '+'.join(scopes)

def validate_oauth(token: str) -> dict:
    return requests.get(VALIDATE_URL, headers={'Authorization': f'OAuth {token}'}).json()


def revoke_oauth_token(client_id: str, oauth: str):
    oauth = oauth.replace('oauth:', '')
    return requests.post(f'https://id.twitch.tv/oauth2/revoke?client_id={client_id}&token={oauth}')

if __name__ == "__main__":
  client_id = 'uic681f91dtxl3pdfyqxld2yvr82r1'
  print(generate_irc_oauth(client_id, REDIRECT_URL))

  