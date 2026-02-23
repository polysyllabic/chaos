#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
import time
from abc import ABC, abstractmethod
from typing import List, Optional
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
  """
  The ChaosCommunicator handles communications between the interface and the Chaos engine.
  
  Since both the engine and the interface can initiate communication, we maintain listen and talk
  sockets on different ports. The talk port acts as a client, and the listen port acts as a server.
  """
  request_retries = 3
  request_timeout = 30000

  def __init__(self):
    self._observers: List[EngineObserver] = []
    self._relay_callbacks_registered = False
    self.context: Optional[zmq.Context] = None
    self.thread: Optional[threading.Thread] = None
    self.keep_running = False
    self.socket_listen: Optional[zmq.Socket] = None
    self.socket_talk: Optional[zmq.Socket] = None
    self._talk_lock = threading.Lock()
    self._listener_lock = threading.Lock()

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
    self._bind_relay_callbacks()
        
    self.keep_running = True
    self.thread = threading.Thread(target=self._listen_loop)
    self.thread.start()

  def _bind_relay_callbacks(self):
    if self._relay_callbacks_registered:
      return

    config.relay.on('pi_host', lambda _value: self.connect_talker(config.relay.pi_host, config.relay.talk_port))
    config.relay.on('talk_port', lambda _value: self.connect_talker(config.relay.pi_host, config.relay.talk_port))
    config.relay.on('listen_port', lambda _value: self.connect_listener(config.relay.listen_port))
    self._relay_callbacks_registered = True
    
  def connect_listener(self, port):
    self.listen_port = port
    if self.context is None:
      raise RuntimeError('Communicator context has not been initialized')
    with self._listener_lock:
      if self.socket_listen is not None:
        # Close and reconnect socket
        self.socket_listen.setsockopt(zmq.LINGER, 0)
        self.socket_listen.close()
      listener_socket = self.context.socket(zmq.REP)
      logging.debug(f"Bind listener to tcp://*:{self.listen_port}")
      try:
        listener_socket.bind(f"tcp://*:{self.listen_port}")
      except zmq.error.ZMQError as e:
        logging.error(f"Could not bind listener socket: {e}")
        raise e
      self.socket_listen = listener_socket

  def connect_talker(self, host_addr, port):
    self.pi_host = host_addr
    self.talk_port = port
    if self.context is None:
      raise RuntimeError('Communicator context has not been initialized')
    with self._talk_lock:
      if self.socket_talk is not None:
        logging.debug('Closing open socket')
        self.socket_talk.setsockopt(zmq.LINGER, 0)
        self.socket_talk.close()
      talk_socket = self.context.socket(zmq.REQ)
      logging.debug(f"Connect talker to tcp://{host_addr}:{self.talk_port}")
      try:
        talk_socket.connect(f"tcp://{host_addr}:{self.talk_port}")
      except zmq.error.ZMQError as e:
        logging.error(f"Could not connect to {host_addr}:{self.talk_port}")
        raise e
      talk_socket.setsockopt(zmq.RCVTIMEO, self.request_timeout)
      self.socket_talk = talk_socket

  def stop(self):
    self.keep_running = False
    if self.thread and self.thread.is_alive():
      self.thread.join()

  def _notify_safe(self, message: str):
    try:
      self.notify(message)
    except Exception:
      logging.exception('Unhandled exception while processing engine message')
    
  def _listen_loop(self):
    while self.keep_running:
      with self._listener_lock:
        socket_listen = self.socket_listen
      if socket_listen is None:
        time.sleep(0.05)
        continue
      #  Poll for engine message.
      if (socket_listen.poll(self.request_timeout) & zmq.POLLIN) != 0:
        try:
          message = socket_listen.recv()
        except zmq.error.ZMQError:
          if self.keep_running:
            logging.exception('Error receiving message from engine')
          continue
        # Return acknowledgment immediately so the engine can continue sending.
        try:
          socket_listen.send(b"Pong")
        except zmq.error.ZMQError:
          if self.keep_running:
            logging.exception('Error acknowledging message from engine')
          continue
        # Dispatch processing on a worker thread so socket reads are never blocked by observer work.
        if message:
          try:
            decoded = message.decode("utf-8")
          except UnicodeDecodeError:
            logging.warning('Received non-UTF8 message from engine; dropping it')
            continue
          threading.Thread(target=self._notify_safe, args=(decoded,), daemon=True).start()
  
  def send_message(self, message):
    with self._talk_lock:
      if self.socket_talk is None:
        logging.error('Talk socket is not connected')
        return False
      logging.debug("Sending message: " + message)
      self.socket_talk.send_string(message)
      # In case the server is dead, we don't wait infinitely for a response. If we get none, we
      # return False to indicate that we couldn't send the message
      retries_left = self.request_retries
      while retries_left != 0:
        if self.socket_talk is None:
          logging.error('Talk socket was closed before a reply was received')
          return False
        if (self.socket_talk.poll(self.request_timeout) & zmq.POLLIN) != 0:
          self.socket_talk.recv()
          logging.debug('Message acknowledged')
          return True
        else:
          retries_left -= 1
          logging.debug('No response: renewing socket and resending')
          self._reconnect_talker_locked()
          assert self.socket_talk is not None
          self.socket_talk.send_string(message)
      
      # restart socket for future attempts
      self._reconnect_talker_locked()
      return False

  def _reconnect_talker_locked(self):
    if self.context is None:
      raise RuntimeError('Communicator context has not been initialized')
    if self.socket_talk is not None:
      logging.debug('Closing open socket')
      self.socket_talk.setsockopt(zmq.LINGER, 0)
      self.socket_talk.close()
    talk_socket = self.context.socket(zmq.REQ)
    logging.debug(f"Connect talker to tcp://{self.pi_host}:{self.talk_port}")
    try:
      talk_socket.connect(f"tcp://{self.pi_host}:{self.talk_port}")
    except zmq.error.ZMQError as e:
      logging.error(f"Could not connect to {self.pi_host}:{self.talk_port}")
      raise e
    talk_socket.setsockopt(zmq.RCVTIMEO, self.request_timeout)
    self.socket_talk = talk_socket
class TestObserver(EngineObserver):
  def update_command(self, message ) -> None:
    print("Notified about message: " + str(message))

if __name__ == "__main__":
  logging.getLogger().setLevel(logging.DEBUG)
  subject = ChaosCommunicator()  
  testObserver = TestObserver()
  subject.attach(testObserver)
  subject.start()
  subject.send_message("game")
  time.sleep(10)
  subject.stop()
