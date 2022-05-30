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
import logging
import collections
import re

class irc():
  CHAT_MSG = re.compile(r"^:\w+!\w+@\w+\.tmi\.twitch\.tv PRIVMSG #\w+ :")

  def tagEmotesParse(tag):
    emoteTags = tag[1].split("/")
    #logging.info("emoteTags: " + str(emoteTags))
    emotesDic = {}
    for emote in emoteTags:
      emoteDef = emote.split(":")
      if len(emoteDef) <= 1:
        continue
      emoteInstances = emoteDef[1].split(",")
      for emoteInstance in emoteInstances:
        startIndex = emoteInstance.split("-",1)
        #logging.info("startIndex = " + str(startIndex[0]))
        emotesDic[int(startIndex[0])] = emoteDef[0]
    return collections.OrderedDict(sorted(emotesDic.items()))
    #logging.info("emotes: " + str(newTags["emotes"]))

  def tagBadgesParse(tag):
    badges = tag[1].split(",")
    result = {}
    for tag in badges:
      splitTag = tag.split("/")
      if len(splitTag) > 1:
        result[splitTag[0]] = splitTag[1]
    return result

  def tagDisplayNameParse(tag):
    return tag[1]

  def ircEscapeReplace(input):
    output = input.replace("\\:", ":")
    output = output.replace("\\s"," ")
    output = output.replace("\\\\","\\")
    output = output.replace("\\r","\r")
    output = output.replace("\\n","\n")
    return output

  def tagSystemMessageParse(self, tag):
    return self.ircEscapeReplace(tag[1])

  def tagDefault(tag):
    return tag[1]

  def responseToTags(self, response):
    # A Handy guide for parsing: https://discuss.dev.twitch.tv/t/how-to-grab-messages-when-using-tags-python/11513
    newTags = {}
    if response[0] == '@':
      tags = response.split(" ", 1)[0]
      #logging.info("Tags: " + tags)
      tags = tags.split(";")
      #logging.info("New Tags: " + str(tags))
      #newTags = {}
      switcher = {
        "@badge-info": self.tagBadgesParse,
        "badges": self.tagBadgesParse,
        "emotes": self.tagEmotesParse,
        "display-name": self.tagDisplayNameParse,
        "system-msg": self.tagSystemMessageParse,
        "msg-param-sub-plan-name": self.tagSystemMessageParse,
        "msg-param-origin-id": self.tagSystemMessageParse
      }
      for i,tag in enumerate(tags):
        tag = tag.split("=", 1)
        #logging.info("New Tag: " + str(tag))
        func = switcher.get(tag[0], self.tagDefault)
        newTags[tag[0]] = func(tag)
    #logging.info("newTags: " + str(newTags))
    return newTags

  def responseToDictionary(self, response):
    notice = {}
    notice["tags"] = {}

    if response[0] == '@':
      splitFirstSpace = response.split(" ", 1)
      if len(splitFirstSpace) < 2:
        return None
      notice["tags"] = self.responseToTags(splitFirstSpace[0])
      response = splitFirstSpace[1]

    prefix = ""
    command = response
    arguments = ''
  
    if command[0] == ':':
      splitResponse = command.split(" ", 2)
      if len(splitResponse) < 3:
        return None
      prefix = splitResponse[0]
      command = splitResponse[1]
      arguments = splitResponse[2]

    commandArguments = []
    i = 0
    while True:
      if len(arguments) == 0:
        break
      
      if arguments[0] == ':':
        notice["message"] = arguments.split(':', 1)[1]
        break

      splitargs = arguments.split(' ', 1)
      commandArguments.append( splitargs[0] )
      if len(splitargs) < 2:
        break

      arguments = splitargs[1]
      i = i + 1

    try:
      notice["command"] = command
      notice["args"] = commandArguments
      if len(prefix) > 0:
        notice["user"] = re.search(r"\w+", prefix).group(0)
      else:
        notice["user"] = ""
    except Exception as e:
      logging.info("Parsing error for response:" + str(response))
    #message = CHAT_MSG.sub("", response)
    #logging.info("Chat " + notice["user"] + ":" + notice["message"])
    #if username == "tmi" or username == "see_bot":
    #  continue

    return notice

  def getUser(notice):
    if "display-name" in notice["tags"] and notice["tags"]["display-name"]:
      return notice["tags"]["display-name"]
    else:
      return notice["user"]

  def getSubMessage(notice):
    return notice["tags"]["system-msg"]

  def getSystemMessage(notice):
    if "system-msg" in notice["tags"].keys():
      return notice["tags"]["system-msg"]
    return None

  def getNewChatterMessage(self,notice):
    return "Welcome " + self.getUser(notice) + "!"

  def getSubGiftMessage(self,notice):
    result = self.getUser(notice) + " gifted a sub!"
    #if int(notice["tags"]['msg-param-mass-gift-count']) < int(notice["tags"]['msg-param-sender-count']):
    #  result += "  " + getUser(notice) + " has gifted " + notice["tags"]['msg-param-sender-count'] + " total subs!"
    return result

  def getAnonSubGiftMessage(self,notice):
    result = "Someone gifted a sub to " + notice["tags"]['msg-param-recipient-display-name'] + "!"
    #if int(notice["tags"]['msg-param-mass-gift-count']) < int(notice["tags"]['msg-param-sender-count']):
    #  result += "  " + getUser(notice) + " has gifted " + notice["tags"]['msg-param-sender-count'] + " total subs!"
    return result

  def getMysteryGiftMessage(self,notice):
    result = self.getUser(notice) + " gifted " + notice["tags"]['msg-param-mass-gift-count'] + " subs!  WTF!?"
    #if int(notice["tags"]['msg-param-mass-gift-count']) < int(notice["tags"]['msg-param-sender-count']):
    #  result += "  " + getUser(notice) + " has gifted " + notice["tags"]['msg-param-sender-count'] + " total subs!"
    return result

  def getHighlightMessage(self,notice):
    result = self.getUser(notice) + ": " + notice["message"]
    #if int(notice["tags"]['msg-param-mass-gift-count']) < int(notice["tags"]['msg-param-sender-count']):
    #  result += "  " + getUser(notice) + " has gifted " + notice["tags"]['msg-param-sender-count'] + " total subs!"
    return result

  def messageIdParse(self,notice):
    switcher = {
      "sub": self.getSubMessage,
      "resub": self.getSystemMessage,
      "subgift": self.getSubGiftMessage,
      "anonsubgift": self.getAnonSubGiftMessage,
      "submysterygift": self.getMysteryGiftMessage,
      "giftpaidupgrade": self.getSystemMessage,
      "rewardgift": self.getSystemMessage,
      "anongiftpaidupgrade": self.getSystemMessage,
      "raid": self.getSystemMessage,
      "unraid": self.getSystemMessage,
      "bitsbadgetier": self.getSystemMessage,
      "ritual": self.getNewChatterMessage,
      "highlighted-message": self.getHighlightMessage
    }

    if "msg-id" in notice["tags"].keys():
      func = switcher.get(notice["tags"]["msg-id"], self.getSystemMessage)
      return func(notice)
    return None

  def getBitsMessage(self,notice):
    if "bits" in notice["tags"].keys():
      return self.getUser(notice) + " gave " + str(notice["tags"]["bits"]) +  " bits!"
    return None

  def getRewardMessage(self,notice):
    giftMessage = self.messageIdParse(notice)
    if giftMessage:
      return giftMessage
    return self.getBitsMessage(notice)
