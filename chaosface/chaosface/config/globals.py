# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
""" Provides a central repository for initializing globals """
from pathlib import Path
from ..gui.ChaosRelay import ChaosRelay

CHAOS_PATH = Path.cwd()
CHAOS_LOG_FILE = Path.home() / 'chaosface.log'
CHAOS_CONFIG_FILE = Path.cwd() / 'chaosConfig.json'

relay = ChaosRelay(CHAOS_CONFIG_FILE)

