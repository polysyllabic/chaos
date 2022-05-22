#!/usr/bin/env python3
#
# Twitch Controls Chaos (TCC)
# Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS file
# at the top-level directory of this distribution for details of the contributers.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# follows patterns from here: https://refactoring.guru/design-patterns/observer/python/example

from ChaosSubject import ChaosSubject
from ChaosObserver import ChaosObserver

from typing import List

import logging
import threading
import zmq

class ChaosCommunicator(ChaosSubject):
	_state: int = None
	_observers: List[ChaosObserver] = []
	
	def attach(self, observer: ChaosObserver) -> None:
		self._observers.append(observer)

	def detach(self, observer: ChaosObserver) -> None:
		self._observers.remove(observer)
		
	#The subscription management methods.
	
	#Trigger an update in each subscriber.
	def notify(self, message) -> None:

		#logging.debug("Subject: Notifying observers...")
		for observer in self._observers:
			observer.update(message)
	
	def start(self):
		#self.q = queue.Queue()
		self.context = zmq.Context()
		self.socketListen = self.context.socket(zmq.REP)
		self.socketListen.bind("tcp://*:5556")
				
		logging.debug("Connecting to chaos engine")
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
			logging.debug("ChaosListener() - Received request: " + message.decode("utf-8"))

			#self.q.put(message)
			self.notify(message.decode("utf-8"))

			#  Send reply back to client
			self.socketListen.send(b"Pong")
	
	def sendMessage(self, message):
		self.socketTalk.send_string(message)
		return self.socketTalk.recv()
		


