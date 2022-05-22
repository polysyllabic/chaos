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

def chat(sock, msg, channel):
	"""
	Send a chat message to the server.
	Keyword arguments:
	sock -- the socket over which to send the message
	msg  -- the message to be sent
	"""
	return sock.sendall(("PRIVMSG {} :{}\r\n".format(channel, msg)).encode("UTF-8"))

def ban(sock, user):
	"""
	Ban a user from the current channel.
	Keyword arguments:
	sock -- the socket over which to send the ban command
	user -- the user to be banned
	"""
	chat(sock, ".ban {}".format(user))

def timeout(sock, user, secs=600):
	"""
	Time out a user for a set period of time.
	Keyword arguments:
	sock -- the socket over which to send the timeout command
	user -- the user to be timed out
	secs -- the length of the timeout in seconds (default 600)
	"""
	chat(sock, ".timeout {}".format(user, secs))
