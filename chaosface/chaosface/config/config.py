from pathlib import Path
from gui import ChaosRelay
from model import ChaosModel

CHAOS_PATH = Path()
CHAOS_LOGS_PATH = "~/chaosLogs"
CHAOS_JSON_CONFIG_PATH = CHAOS_PATH + "/chatbot/configs"
CHAOS_CONFIG_FILE = CHAOS_JSON_CONFIG_PATH + "chaos_config.json"
CHATBOT_CONFIG_FILE = CHAOS_JSON_CONFIG_PATH + "config.json"

model = ChaosModel()
relay = ChaosRelay()
