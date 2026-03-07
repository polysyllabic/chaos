#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
import time
from abc import ABC, abstractmethod
from typing import List, Optional
import threading
import queue
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
  listen_poll_interval_ms = 250

  def __init__(self):
    self._observers: List[EngineObserver] = []
    self._relay_callbacks_registered = False
    self.context: Optional[zmq.Context] = None
    self.thread: Optional[threading.Thread] = None
    self._notify_thread: Optional[threading.Thread] = None
    self.keep_running = False
    self.socket_listen: Optional[zmq.Socket] = None
    self.socket_talk: Optional[zmq.Socket] = None
    self._talk_lock = threading.Lock()
    self._listener_lock = threading.Lock()
    self._incoming_messages: queue.Queue[str] = queue.Queue()

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
    self.thread = threading.Thread(target=self._listen_loop, daemon=True, name='chaosface-listener')
    self._notify_thread = threading.Thread(target=self._notify_loop, daemon=True, name='chaosface-notify')
    self.thread.start()
    self._notify_thread.start()

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
    with self._listener_lock:
      if self.socket_listen is not None:
        self.socket_listen.setsockopt(zmq.LINGER, 0)
        self.socket_listen.close()
        self.socket_listen = None
    with self._talk_lock:
      if self.socket_talk is not None:
        self.socket_talk.setsockopt(zmq.LINGER, 0)
        self.socket_talk.close()
        self.socket_talk = None
    self._incoming_messages.put_nowait('')
    if self.thread and self.thread.is_alive():
      self.thread.join(timeout=2.0)
    if self._notify_thread and self._notify_thread.is_alive():
      self._notify_thread.join(timeout=2.0)

  def _notify_loop(self):
    while self.keep_running or not self._incoming_messages.empty():
      try:
        message = self._incoming_messages.get(timeout=0.1)
      except queue.Empty:
        continue
      if not message:
        continue
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
      try:
        poll_result = socket_listen.poll(min(self.request_timeout, self.listen_poll_interval_ms))
      except zmq.error.ZMQError:
        if self.keep_running:
          logging.exception('Listener poll failed; recreating listener socket')
          try:
            self.connect_listener(self.listen_port)
          except Exception:
            logging.exception('Failed to recreate listener socket')
        time.sleep(0.1)
        continue

      if (poll_result & zmq.POLLIN) == 0:
        continue

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
        self._incoming_messages.put(decoded)
  
  def _poll_for_reply_locked(self, timeout_ms: int) -> bool:
    if self.socket_talk is None:
      return False
    remaining = max(0, int(timeout_ms))
    step_ms = 250
    while remaining > 0 and self.keep_running:
      wait_ms = min(step_ms, remaining)
      if (self.socket_talk.poll(wait_ms) & zmq.POLLIN) != 0:
        return True
      remaining -= wait_ms
    return False

  def send_message(self, message):
    with self._talk_lock:
      if self.socket_talk is None or not self.keep_running:
        logging.error('Talk socket is not connected')
        return False
      logging.debug("Sending message: " + message)
      try:
        self.socket_talk.send_string(message)
      except zmq.error.ZMQError:
        logging.exception('Failed to send message on talk socket')
        return False
      # In case the server is dead, we don't wait infinitely for a response. If we get none, we
      # return False to indicate that we couldn't send the message
      retries_left = self.request_retries
      while retries_left != 0 and self.keep_running:
        if self.socket_talk is None:
          logging.error('Talk socket was closed before a reply was received')
          return False
        if self._poll_for_reply_locked(self.request_timeout):
          try:
            self.socket_talk.recv()
          except zmq.error.ZMQError:
            logging.exception('Failed to receive acknowledgment from talk socket')
            return False
          logging.debug('Message acknowledged')
          return True
        else:
          retries_left -= 1
          if not self.keep_running:
            break
          logging.debug('No response: renewing socket and resending')
          try:
            self._reconnect_talker_locked()
          except Exception:
            logging.exception('Failed to reconnect talk socket')
            return False
          assert self.socket_talk is not None
          try:
            self.socket_talk.send_string(message)
          except zmq.error.ZMQError:
            logging.exception('Failed to resend message after reconnect')
            return False
      
      # restart socket for future attempts
      if self.keep_running:
        try:
          self._reconnect_talker_locked()
        except Exception:
          logging.exception('Failed to reset talk socket after retries')
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
