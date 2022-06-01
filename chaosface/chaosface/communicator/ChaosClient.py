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

from abc import ABC, abstractmethod
from typing import List
import zmq
import logging

class EngineObserver(ABC):
  # An observer for listening to events from the chaos engine
  
  @abstractmethod
  def updateCommand(self, message) -> None:
    # Process message from ZMQ
    pass

class ChaosCommunicator():
  _observers: List[EngineObserver] = []
  server_addr = "192.168.1.8"
  server_port = "5555"
  request_retries = -1
  request_timeout = 2500

  def __init__(self):
    logging.debug("Opening zmq socket as a client")
    self.context = zmq.Context()
    self.client = self.context.socket(zmq.REQ)
    self.makeConnection()

  def attach(self, observer: EngineObserver) -> None:
    logging.debug("Attached an observer.")
    self._observers.append(observer)

  def detach(self, observer: EngineObserver) -> None:
    self._observers.remove(observer)
    
  def notify(self, message) -> None:
    #Trigger an update in each subscriber.
    for observer in self._observers:
      observer.updateCommand(message)
  
  def makeConnection(self) -> None:
    self.client.connect(f"tcp://{self.server_addr}:{self.server_port}")

  # This implements the "lazy pirate" pattern from the 0MQ guide
  def sendMessage(self, message) -> bool:
    print("Sending message: " + message)
    self.client.send_string(message)
    retries = self.request_retries
    while True:
      # Poll with timeout rather than just blocking
      if (self.client.poll(self.request_timeout) & zmq.POLLIN):
        message = self.client.recv()
        logging.debug("Received message from engine: " + message.decode("utf-8"))
        retries = self.request_retries
        # Inform observers about the response
        self.notify(message.decode("utf-8"))
        return True
      # No response received (setting to -1 retries means retry indefinitely)
      if retries > 0:
        retries -= 1
      # Close and reconnect socket
      self.client.setsockopt(zmq.LINGER, 0)
      self.client.close()
      if retries == 0:
        logging.error("Server offline. Discarding message")
        return False
      logging.debug("Reconnecting to server")
      self.makeConnection()


class TestObserver(EngineObserver):
  def updateCommand(self, message ) -> None:
    print("Notified about message: " + str(message))

if __name__ == "__main__":
  logging.basicConfig(level=logging.DEBUG)
  subject = ChaosCommunicator()  
  testObserver = TestObserver()
  subject.attach(testObserver)
  subject.retries = 10
  subject.sendMessage("game")
  


