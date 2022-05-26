from pathlib import Path
from model import ChaosModel

CHAOS_PATH = Path()
CHAOS_LOGS_PATH = "~/chaosLogs"
CHAOS_JSON_CONFIG_PATH = CHAOS_PATH + "/configs"
CHAOS_CONFIG_FILE = CHAOS_JSON_CONFIG_PATH + "chaos_config.json"

model = ChaosModel()
