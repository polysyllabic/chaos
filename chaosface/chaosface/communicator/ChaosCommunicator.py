"""
  Twitch Controls Chaos (TCC)
  Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
  file at the top-level directory of this distribution for details of the
  contributers.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""
from chaosface.config.globals import relay
import time
from abc import ABC, abstractmethod
from typing import List
import threading
import zmq
import logging
log = logging.getLogger(__name__)

# An observer for listening to events from the chaos engine
class EngineObserver(ABC):
  @abstractmethod
  def updateCommand(self, message ) -> None:
    # Process message from ZMQ
    pass

class ChaosCommunicator():
  _observers: List[EngineObserver] = []
  request_retries = -1
  request_timeout = 2500
  socketListen = None 
  socketTalk = None

  def attach(self, observer: EngineObserver) -> None:
    log.debug("Attached an observer.")
    self._observers.append(observer)

  def detach(self, observer: EngineObserver) -> None:
    self._observers.remove(observer)
    
  def notify(self, message) -> None:
    # Trigger an update in each subscriber.
    for observer in self._observers:
      observer.updateCommand(message)
  
  def start(self):
    log.debug("Opening zmq socket")
    self.context = zmq.Context()
    self.reconnectListener()
    self.reconnectTalker()
        
    self.keepRunning = True
    self.thread = threading.Thread(target=self.listenLoop)
    self.thread.start()
    
  def reconnectListener(self):
    if (self.socketListen):
      # Close and reconnect socket
      self.socketListen.setsockopt(zmq.LINGER, 0)
      self.socketListen.close()
    self.socketListen = self.context.socket(zmq.REP)
    self.socketListen.bind(f"tcp://*:{relay.listenPort}")

  def reconnectTalker(self):
    if (self.socketTalk):
      self.socketTalk.setsockopt(zmq.LINGER, 0)
      self.socketTalk.close()
    self.socketTalk = self.context.socket(zmq.REQ)
    self.socketTalk.connect(f"tcp://{relay.piHost}:{relay.talkPort}")

  def stop(self):
    self.keepRunning = False
    self.thread.join()
    
  def listenLoop(self):
    while self.keepRunning:
      #  Wait for next message from engine
      message = self.socketListen.recv()
      log.debug("Received from engine: " + message.decode("utf-8"))

      self.notify(message.decode("utf-8"))
      self.socketListen.send(b"Pong")
  
  def sendMessage(self, message):
    log.debug("Sending message: " + message)
    self.socketTalk.send_string(message)
    return self.socketTalk.recv()

  @relay.reaction('updatePiHost')
  def _updatePiHost(self, *events):
    self.reconnectTalker()

  @relay.reaction('updateTalkerPort')
  def _updateTalkerPort(self, *events):
    self.reconnectTalker()

  @relay.reaction('updateListenPort')
  def _updatePiHost(self, *events):
    self.reconnectListener()

class TestObserver(EngineObserver):
  def updateCommand(self, message ) -> None:
    print("Notified about message: " + str(message))

if __name__ == "__main__":
  log.setLevel(logging.DEBUG)
  subject = ChaosCommunicator()  
  testObserver = TestObserver()
  subject.attach(testObserver)
  subject.start()
  subject.sendMessage("game")
  time.sleep(10)
  subject.stop()


