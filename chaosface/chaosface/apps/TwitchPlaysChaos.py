#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Legacy Twitch Plays Chaos entrypoint.

The original legacy widget-based app has been retired. This module now forwards to the
NiceGUI-based Twitch Controls Chaos interface.
"""

import argparse

from chaosface.apps.TwitchControlsChaos import twitch_controls_chaos


def twitch_plays_chaos(args):
  """Backward-compatible alias for legacy callers."""
  twitch_controls_chaos(args)


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('--reauthorize', help='Delete previous OAuth tokens and wait for new ones to attempt to connect')
  twitch_plays_chaos(parser.parse_args())
