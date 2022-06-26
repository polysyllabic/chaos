import sys
import json
import threading
from chaosface.communicator import EngineObserver
from chaosface.communicator import ChaosCommunicator
import time
import logging
logging.basicConfig(level=logging.DEBUG)

class TestObserver(EngineObserver):
  def __init__(self):
    self.communicator = ChaosCommunicator()
    self.communicator.attach(self)
    self.communicator.start()

  def updateCommand(self, message) -> None:
    print(f"Notified of message from chaos engine: {message}")
    received = json.loads(message)

    if "game" in received:
      print("Received game info: " + str(received["game"]))

    if "pause" in received:
      print("Got a pause command of " + str(received["pause"]))

  def start(self):
    self.thread = threading.Thread(target=self.process)
    self.thread.start()

  def process(self):
    try:
      # Start loop
      print("Press CTRL-C to stop sample")
      self._loop()
    except KeyboardInterrupt:
      print("Exiting\n")
      sys.exit(0)

  def _loop(self):
    toSend = {}
    toSend["game"] = True
    print("sending message")
    self.communicator.sendMessage(json.dumps(toSend))
    while True:
      time.sleep(0.05)

if __name__ == "__main__":
  
  testObserver = TestObserver()
  testObserver.process()
  
