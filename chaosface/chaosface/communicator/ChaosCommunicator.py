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

# follows patterns from here: https://refactoring.guru/design-patterns/observer/python/example
from abc import ABC, abstractmethod
from typing import List
import threading
import zmq

class EngineObserver(ABC):
	"""
	An observer for listening to events from the chaos engine
	"""
	
	@abstractmethod
	def updateCommand(self, message ) -> None:
		"""
		Process message from ZMQ
		"""
		pass


class EngineSubject(ABC):
	"""
	The Subject interface declares a set of methods for managing subscribers.
	"""

	@abstractmethod
	def attach(self, observer: EngineObserver) -> None:
		"""
		Attach an observer to the subject.
		"""
		pass

	@abstractmethod
	def detach(self, observer: EngineObserver) -> None:
		"""
		Detach an observer from the subject.
		"""
		pass

	@abstractmethod
	def notify(self, message) -> None:
		"""
		Notify all observers about an event.
		"""
		pass

class ChaosCommunicator(EngineSubject):
	_state: int = None
	_observers: List[EngineObserver] = []
	
	def attach(self, observer: EngineObserver) -> None:
		print("Subject: Attached an observer.")
		self._observers.append(observer)

	def detach(self, observer: EngineObserver) -> None:
		self._observers.remove(observer)
		
	def notify(self, message) -> None:
		"""
		Trigger an update in each subscriber.
		"""
		for observer in self._observers:
			observer.updateCommand(message)
	
	def start(self):
		self.context = zmq.Context()
		self.socketListen = self.context.socket(zmq.REP)
		self.socketListen.bind("tcp://*:5556")
				
		#print("Connecting to chaos engine")
		self.socketTalk = self.context.socket(zmq.REQ)
		self.socketTalk.connect("tcp://localhost:5555")
				
		self.keepRunning = True
		self.thread = threading.Thread(target=self.listenLoop)
		self.thread.start()
		
	def stop(self):
		self.keepRunning = False
		
	def listenLoop(self):
		while self.keepRunning:
			#  Wait for next request from client
			message = self.socketListen.recv()
			#logging.info("ChaosListener() - Received request: " + message.decode("utf-8"))

			#self.q.put(message)
			self.notify(message.decode("utf-8"))

			#  Send reply back to client
			self.socketListen.send(b"Pong")
	
	def sendMessage(self, message):
		self.socketTalk.send_string(message)
		return self.socketTalk.recv()
		


