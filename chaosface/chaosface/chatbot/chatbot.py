#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# Twitch Controls Chaos (TCC)
# Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
# file at the top-level directory of this distribution for details of the
# contributers.
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
#-----------------------------------------------------------------------------

import socket
import threading
import queue
import time

from .utility import chat
from .irctwitch import irc

import logging
log = logging.getLogger(__name__)

class Chatbot():

  def __init__(self):
    self.connected = False
    self.fullyConnected = False
    self.heartbeatPingPong = 0
    self.pingPongRate = 1.0/8.0
    self.lastPingTime = 0
    self.socketTimeout = 0.1
    self.pingWaitCount = 0
    self.reconfigured = True
    
    self.verbose = False
    
    self.chat_rate = 10
    self.host = "irc.twitch.tv"
    self.port = 6667
    self.bot_oauth = "oauth:"
    self.bot_name = "bot-name"
    self.channel_name = "your-channel"
    
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
    
  def isConnected(self):
    return self.fullyConnected
  
  def _chatLoop(self):
    
    thread = threading.currentThread()

    self.connected = False
#    heartbeatPingPong = 0
#    heartbeatTimeout = 5
    self.reconnectTimeFalloff = 0
    self.reconfigured = False
    
    self.heartbeatPingPong = time.time()
    
    while not getattr(thread, "kill", False):
      self.fullyConnected = False
      while not self.connected and not getattr(thread, "kill", False):
        try:
          log.info("Chatbot: Waiting " + str(self.reconnectTimeFalloff) + " seconds before reconnection attempt")
          time.sleep(self.reconnectTimeFalloff)
          if self.reconnectTimeFalloff == 0:
            self.reconnectTimeFalloff = 1
          elif self.reconnectTimeFalloff < 5:
            self.reconnectTimeFalloff *= 2
          
          self.s = socket.socket()
          self.s.settimeout(self.socketTimeout)
#          self.s.connect((config.HOST, config.PORT))
          self.s.connect((self.host, self.port))
          
          log.info("Attempting to connect to " + self.channel_name + " as " + self.bot_name )

          self.s.sendall("PASS {}\r\n".format( self.bot_oauth ).encode("utf-8"))
          self.s.sendall("NICK {}\r\n".format( self.bot_name ).encode("utf-8"))
          self.s.sendall("JOIN {}\r\n".format( self.channel_name ).encode("utf-8"))

          self.s.sendall("CAP REQ :twitch.tv/tags\r\n".encode("utf-8"))
          self.s.sendall("CAP REQ :twitch.tv/commands\r\n".encode("utf-8"))
          self.s.sendall("CAP REQ :twitch.tv/membership\r\n".encode("utf-8"))

          self.connected = True #Socket successfully connected
        except Exception as e:
          log.error("Error connecting to Twitch, will attempt again...")
          log.error(str(e))
          self.connected = False #Socket failed to connect

      self.heartbeatPingPong = time.time()
      self.waitingForWelcome = True
      
      incoming = b''
      response = ""
      while self.connected and not getattr(thread, "kill", False):
        time.sleep(1 / self.chat_rate)
        if self.reconfigured:
          log.info("New chat information, re-logging in to IRC")
          self.reconfigured = False
          self.connected = False
          self.reconnectTimeFalloff = 0
          break
        
        if (time.time() - self.heartbeatPingPong) >= 30:
          log.info(f"Pingpong failure, attempting to reconnect. Message timeout = {self.heartbeatPingPong}")
          self.connected = False
          continue
        
        if self.s._closed:
          log.info("Socket is closed :(")
          
        try:
          if self.pingWaitCount < 3 and (time.time() - self.lastPingTime) > (1.0/self.pingPongRate):
            if self.s.sendall("PING :tmi.twitch.tv\r\n".encode("utf-8")):
              log.error("Error sending a PING")
            self.lastPingTime = time.time()
            self.pingWaitCount += 1
            log.debug("Sent PING")
        except Exception as e:
          log.error(f"Error sending PING: {e}")
          self.connected = False
          continue
          
        try:
          incoming = b''
          incoming = self.s.recv(1024)
        except Exception as e:
          err = e.args[0]
          if err == 'timed out':
            if len(incoming) > 0:
              log.debug(f"recv timed out, retry later; incoming = {incoming}")
            else:
              continue
          else:
            log.error("Error when calling s.recv(): {e}")
            self.connected = False
            continue
            
        response += incoming.decode("utf-8")
        log.info(f"Raw Running Response: {response}")
          
        if len(response) <= 0:
          continue
          
        try:
          responses = response.split("\r\n")
          response = responses[ len(responses) - 1]            
          for i in range(len(responses)-1):
            self.handleResponseLine(responses[i])
        except Exception as e:
          log.error(f"Error handling response(s) for {response}: {e}")
          response = responses[ len(responses) - 1]
          log.error(f" - Setting response to {response}")
          continue
            
  def handleResponseLine(self, line):
    #giftMessage = irc.getRewardMessage(notice)
    if len(line) == 0:
      log.warning("Length of response line is 0")
      return
    notice = irc.responseToDictionary(line)
    if not notice:
      log.debug("This appears to be an invalid IRC response")
      return True

    if "message" in notice.keys():
      notice["message"] = notice["message"].split("\r\n",1)[0]
      if notice["command"] == "PRIVMSG":
        log.debug(f'Chat {notice["user"]}: {notice["message"]}')
      
      if notice["command"] == "PONG":
        log.debug("Recieved PONG after pinging server!")
        self.heartbeatPingPong = time.time()
        self.pingWaitCount = 0
        return
        
      if notice["command"] == "PING":
        log.debug("Recieved PING, sending PONG...")
        self.s.sendall("PONG :tmi.twitch.tv\r\n".encode("utf-8"))
        self.heartbeatPingPong = time.time()
        self.pingWaitCount = 0
        return
          
      # The following is definitely not good to check for a valid conenction:
      if self.waitingForWelcome and notice["user"] == "tmi":
        if notice["message"] == "Welcome, GLHF!":
          log.debug("Connection successful!")
          self.fullyConnected = True
          self.waitingForWelcome = False
          self.reconnectTimeFalloff = 0
        else:
          log.info("Need to attempt a reconnect")
          self.connected = False
                
      self.messageQueue.put( notice )
  
  def chatResponseLoop(self):
    thread = threading.currentThread()
    cooldownInSeconds = 2
    timeSinceLastResponse = cooldownInSeconds

    while not getattr(thread, "kill", False):
      time.sleep(1 / self.chat_rate)
      if not self.connected:
        continue
      timeSinceLastResponse += (1.0 / self.chat_rate)
      if timeSinceLastResponse < cooldownInSeconds:
        if self.qResponse.qsize() > 0:
          while self.qResponse.qsize() > 0:
            self.qResponse.get()
        continue
        
      else:
        while self.qResponse.qsize() > 0:
          # q.empty(), q.get(), q.qsize()
          notice = self.qResponse.get()
        
          print("Sending response message: " + notice)
          try:
            result = chat(self.s, notice, self.channel_name)
            if result:
              log.info(f"chatResponseLoop() send result: {result}")
          except Exception as e:
            err = e.args[0]
            if err == 'timed out':
              log.info('utility.chat() timed out, retry later')
            else:
              log.error(str(e))
              self.connected = False
          time.sleep(1 / self.chat_rate)
        
          timeSinceLastResponse = 0
