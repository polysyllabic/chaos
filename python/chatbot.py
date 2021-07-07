#!/usr/bin/env python3

import sys # just for argument passing for bare test
import socket
import threading
import logging
import queue
import time

import utility
import irctwitch as irc

#from chaosRelay import chaosRelay as relay

class Chatbot():

	def __init__(self):
		self.connected = False
		self.heartbeatPingPong = 0.0
		self.heartbeatTimeout = 5
		self.reconfigured = True
		
		self.verbose = False
		
		self.chat_rate = 0.67
		self.host = "irc.twitch.tv"
		self.port = 6667
		self.bot_oauth = "oauth:"
		self.bot_name = "See_Bot"
		self.channel_name = "#blegas78"
		
		self.messageQueue = queue.Queue()
		self.qResponse = queue.Queue()
			
	def setIrcInfo(self, host, port):
		if self.host != host:
			self.host = host
			self.reconfigured = True
		if self.port != port:
			self.port = port
			self.reconfigured = True

	def setBotCredentials(self, name, oauth):
		if self.bot_oauth != oauth:
			self.bot_oauth = oauth
			self.reconfigured = True
		if self.bot_name != name:
			self.bot_name = name
			self.reconfigured = True
			
	def setChannelName(self, name):
		if self.channel_name != name:
			self.channel_name = name
			self.reconfigured = True
			
	def setChatRate(self, rate):
		self.chat_rate = rate
		
	def sendReply(self, message):
		self.qResponse.put( message )

	def start(self):
		self.chatThread = threading.Thread(target=self._chatLoop)
		self.chatThread.start()
		self.chatResponseThread = threading.Thread(target=self.chatResponseLoop)
		self.chatResponseThread.start()
			
	def stop(self):
		self.chatResponseThread.kill = True
		self.chatResponseThread.join()
		self.chatThread.kill = True
		self.chatThread.join()
	
	def _chatLoop(self):
		
		thread = threading.currentThread()

		self.connected = False
		heartbeatPingPong = 0.0
#		heartbeatTimeout = 5
		self.reconnectTimeFalloff = 0
		self.reconfigured = False
		
		while not getattr(thread, "kill", False):
#			self.bot_oauth = relay.bot_oauth
#			self.bot_name = relay.bot_name
#			self.channel_name = relay.channel_name.lower()
#			self.reconnectTimeFalloff = 0
			while not self.connected and not getattr(thread, "kill", False):
				try:
					logging.info("Chatbot: Waiting " + str(self.reconnectTimeFalloff) + " seconds before reconnection attempt")
					time.sleep(self.reconnectTimeFalloff)
					if self.reconnectTimeFalloff == 0:
						self.reconnectTimeFalloff = 1
					else:
						self.reconnectTimeFalloff *= 2
					
					self.s = socket.socket()
					self.s.settimeout(self.heartbeatTimeout)
#					self.s.connect((config.HOST, config.PORT))
					self.s.connect((self.host, self.port))
					
					logging.info("Attempting to connect to " + self.channel_name + " as " + self.bot_name )

					self.s.send("PASS {}\r\n".format( self.bot_oauth ).encode("utf-8"))
					self.s.send("NICK {}\r\n".format( self.bot_name ).encode("utf-8"))
					self.s.send("JOIN {}\r\n".format( self.channel_name ).encode("utf-8"))

					self.s.send("CAP REQ :twitch.tv/tags\r\n".encode("utf-8"))
					self.s.send("CAP REQ :twitch.tv/commands\r\n".encode("utf-8"))
					self.s.send("CAP REQ :twitch.tv/membership\r\n".encode("utf-8"))
					
					

					self.connected = True #Socket successfully connected
				except Exception as e:
					logging.info("Error connecting to Twitch, will attempt again...")
					logging.info(str(e))
					#time.sleep(5)
					self.connected = False #Socket failed to connect

			# yeah starting threads here seems fine :(
			#self.chatResponseThread = threading.Thread(target=self.chatResponseLoop)
			#self.chatResponseThread.start()
		
			#lastTime = 0;
			self.heartbeatPingPong = 0.0
			self.waitingForWelcome = True
			while self.connected and not getattr(thread, "kill", False):
				time.sleep(1 / self.chat_rate)
				#if time.time() - lastTime > 15:
				#	lastTime = time.time()
				#	logging.info("Inserting message into reward queue")
				#	rewardQueue.put("Time: " + str(time.time()))
				if self.reconfigured:
					logging.info("New chat information, re-logging in to IRC")
					#saveConfig()
					self.reconfigured = False
					self.connected = False
					self.reconnectTimeFalloff = 0
					break
				
				self.heartbeatPingPong += (1.0 / self.chat_rate) #self.heartbeatTimeout
				if self.heartbeatPingPong > (5.25*60):	# 5 minutes, 15 seconds
					self.connected = False
					logging.info("Pingpong failure, attempting to reconnect, message timeout = " + str(self.heartbeatTimeout))
					continue
				
				if self.s._closed:
					logging.info("Socket is closed :(")
					
				response = ""
				#try:
#					count = self.s.send(b'a')
				#	utility.chat(self.s, "test", self.channel_name);
#					logging.info("Sent " + str(count) + " bytes")
				#except Exception as e:
				#	logging.info(str(e))
					
				try:
					response = self.s.recv(1024)
				except Exception as e:
					err = e.args[0]
					if err == 'timed out':
						if self.verbose:
							logging.info('recv timed out, retry later')
							logging.info("response = " + str(response))
						#time.sleep(heartbeatTimeout)
						continue
					else:
						logging.info(str(e))
						self.connected = False
						continue
						
				response = response.decode("utf-8")
				if self.verbose:
					logging.info("Raw Response:" + response)
					
				if response == "PING :tmi.twitch.tv\r\n":
					self.s.send("PONG :tmi.twitch.tv\r\n".encode("utf-8"))
					logging.info("Pong")
					self.heartbeatPingPong = 0.0
					continue
					
				if len(response) <= 0:
					#logging.info("len(response) <= 0")
					continue
						
				#heartbeatPingPong = 0.0
					
				try:
					responses = response.split("\r\n")
#					logging.info("# of Responses: " + str(len(responses)))
					for response in responses:
						self.handleChatLine(response)

				except Exception as e:
					logging.info(str(e))
					continue
						
	def handleChatLine(self, line):
		#giftMessage = irc.getRewardMessage(notice)
		if len(line) == 0:
			return
		notice = irc.responseToDictionary(line)

		if "message" in notice.keys():
			notice["message"] = notice["message"].split("\r\n",1)[0]
			logging.info("Chat " + notice["user"] + ":" + notice["message"])
			#q.put( (str(emoteId),img) )
			#chatRelay.create_message(notice["user"], notice["message"])
			
			#chatRelay.create_message(notice["user"], notice["message"])
					
			#q.put( notice )
			self.messageQueue.put( notice )
					
			if self.waitingForWelcome and notice["user"] == "tmi":
				if notice["message"] == "Welcome, GLHF!":
					logging.info("Connection successful!")
					self.waitingForWelcome = False
					self.reconnectTimeFalloff = 0
				else:
					logging.info("Need to attempt a reconnect")
					self.connected = False
	
				
				
	def chatResponseLoop(self):
		thread = threading.currentThread()
		cooldownInSeconds = 2
		timeSinceLastResponse = cooldownInSeconds

		while not getattr(thread, "kill", False):
			time.sleep(1 / self.chat_rate)
			timeSinceLastResponse += (1.0 / self.chat_rate)
			if timeSinceLastResponse < cooldownInSeconds:
				if self.qResponse.qsize() > 0:
					while self.qResponse.qsize() > 0:
						self.qResponse.get()
					utility.chat(self.s, "!mods Cooldown at " + str(int(cooldownInSeconds - timeSinceLastResponse)) + " seconds", self.channel_name);
				continue
				
			else:
				while self.qResponse.qsize() > 0:
					# q.empty(), q.get(), q.qsize()
					notice = self.qResponse.get()
				
					print("Sending response message: " + notice)
					try:
						result = utility.chat(self.s, notice, self.channel_name)
						if result:
							logging.info("chatResponseLoop() send result:" + str(result))
					except Exception as e:
						err = e.args[0]
						if err == 'timed out':
							logging.info('utility.chat() timed out, retry later')
							logging.info("response = " + str(result))
							#time.sleep(heartbeatTimeout)
						else:
							logging.info(str(e))
							self.connected = False
					time.sleep(1 / self.chat_rate)
				
					timeSinceLastResponse = 0


if __name__ == "__main__":
	logging.basicConfig(level="INFO")
	
	chatbot = Chatbot()
	chatbot.verbose = True
	
	chatbot.setChatRate(0.2)
	if len(sys.argv) == 3:
		logging.info("Setting custom credentials nick: " + sys.argv[1] + " - auth: " + sys.argv[2])
		chatbot.setBotCredentials(sys.argv[1], sys.argv[2])
	
	chatbot.start()
	count = 0
	while True:
		time.sleep(10)
		
		while chatbot.messageQueue.qsize() > 0:
			logging.info("Read: " + str(chatbot.messageQueue.get()))
#		chatbot.sendReply("Counter: " + str(count))
		chatbot.sendReply(str(count))
		count += 1
		if count > 4:
			count = 0
		
