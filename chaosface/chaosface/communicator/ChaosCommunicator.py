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
import time
from abc import ABC, abstractmethod
from typing import List
import threading
import zmq
import logging

import chaosface.config.globals as config

# An observer for listening to events from the chaos engine
class EngineObserver(ABC):
  @abstractmethod
  def update_command(self, message ) -> None:
    # Process message from ZMQ
    pass

class ChaosCommunicator():
  _observers: List[EngineObserver] = []
  request_retries = 3
  request_timeout = 2500
  socket_listen = None 
  socket_talk = None

  def attach(self, observer: EngineObserver) -> None:
    logging.debug("Attached an observer.")
    self._observers.append(observer)

  def detach(self, observer: EngineObserver) -> None:
    self._observers.remove(observer)
    
  def notify(self, message) -> None:
    # Trigger an update in each subscriber.
    for observer in self._observers:
      observer.update_command(message)
  
  def start(self):
    logging.debug("Opening zmq socket")
    self.context = zmq.Context()
    self.listen_port = config.relay.get_attribute('listen_port')
    self.connect_listener(self.listen_port)
    self.pi_host = config.relay.get_attribute('pi_host')
    self.talk_port = config.relay.get_attribute('talk_port')
    self.connect_talker(self.pi_host, self.talk_port)
        
    self.keep_running = True
    self.thread = threading.Thread(target=self._listen_loop)
    self.thread.start()
    
  def connect_listener(self, port):
    self.listen_port = port
    if (not self.socket_listen is None):
      # Close and reconnect socket
      self.socket_listen.setsockopt(zmq.LINGER, 0)
      self.socket_listen.close()
    self.socket_listen = self.context.socket(zmq.REP)
    logging.debug(f"Bind listener to tcp://*:{self.listen_port}")
    self.socket_listen.bind(f"tcp://*:{self.listen_port}")

  def connect_talker(self, host_addr, port):
    self.pi_host = host_addr
    self.talk_port = port
    if (not self.socket_talk is None):
      logging.debug('Closing open socket')
      self.socket_talk.setsockopt(zmq.LINGER, 0)
      self.socket_talk.close()
    self.socket_talk = self.context.socket(zmq.REQ)
    logging.debug(f"Connect talker to tcp://{host_addr}:{self.talk_port}")
    self.socket_talk.connect(f"tcp://{host_addr}:{self.talk_port}")

  def stop(self):
    self.keep_running = False
    self.thread.join()
    
  def _listen_loop(self):
    while self.keep_running:
      #  Wait for next message from engine (this is blocking)
      message = self.socket_listen.recv()
      logging.debug("Received from engine: " + message.decode("utf-8"))
      # Return acknowledgment
      self.socket_listen.send(b"Pong")
      # Tell observers what we received
      self.notify(message.decode("utf-8"))
  
  def send_message(self, message):
    logging.debug("Sending message: " + message)
    self.socket_talk.send_string(message)
    # In case the server is dead, we don't wait infinitely for a response. If we get none, we
    # return False to indicate that we couldn't send the message
    retries_left = self.request_retries
    while retries_left != 0:
      if (self.socket_talk.poll(self.request_timeout) & zmq.POLLIN) != 0:
        reply = self.socket_talk.recv()
        logging.debug('Message acknowledged')
        return True
      else:
        retries_left -= 1
        logging.debug('No response: renewing socket and resending')
        self.connect_talker(self.pi_host, self.talk_port)
        self.socket_talk.send_string(message)
    
    # restart socket for future attempts
    self.connect_talker(self.pi_host, self.talk_port)
    return False  


  @config.relay.reaction('update_pi_host', 'update_talker_port')
  def _update_talker(self, *events):
    self.connect_talker(config.relay.pi_host, config.relay.talk_port)

  @config.relay.reaction('update_listen_port')
  def _update_listener(self, *events):
    self.connect_listener(config.relay.listen_port)

class TestObserver(EngineObserver):
  def update_command(self, message ) -> None:
    print("Notified about message: " + str(message))

if __name__ == "__main__":
  logging.setLevel(logging.DEBUG)
  subject = ChaosCommunicator()  
  testObserver = TestObserver()
  subject.attach(testObserver)
  subject.start()
  subject.send_message("game")
  time.sleep(10)
  subject.stop()


