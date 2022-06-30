***************
Chaos Interface
***************

Overview
========

The Chaos interface, *chaosface*, is one of two main parts of the Twitch Controls Chaos (TCC)
system for allowing Twitch chat to alter various aspects of gameplay in a Playstation game.
Chaosface is a Python application that provides the connection between viewers in Twitch chat
and the chaos engine that directly controls gameplay modifications (mods). This file provides
reference documentation for the chaos interface. For a general overview and quick guide,
see the general README file for Twitch Controls Chaos.

Chaosface consists of three major parts:

- A voting algorithm to select mods for voting and inform the engine of winning mods
- A chatbot to monitor Twitch chat for votes and other commands
- A user interface for the streamer to alter TCC settings and monitor TCC's status

Chaosface can run along with the chaos engine on your Raspberry Pi, or you can host it on
another computer.

Installation and Setup
======================

The default installation of chaosface is performed will run it from the Raspberry Pi, along with
the engine. See the main TCC documentation for those steps. This section explains the steps
you need to take if you plan to run chaosface on a separate computer.

Installing Chaosface on Another Computer
----------------------------------------

Configure the Pi:

1. From a terminal window, log in to the Pi.
2. Stop the Pi from starting chaosface automatically with the following commands:
        `sudo systemctl stop chaosface`
        `sudo systemctl disable chaosface`
3. Next, we must edit a configuration file to tell the engine where to find chaosface. Open the
    file in an editor, for example, with the command `nano chaosconfig.toml` and edit the key
    `interface_addr` to contain the IP address of the computer running chaosface. **UPDATE WHEN WE 
    HAVE THE FINAL DIRECTORY FOR THE CONFIG FILE**
4. Make a note of the Pi's IP address. You can get it with the command `hostname -I`.

Setup the Computer for Chaosface:

1. Next, make sure Python 3 is installed on the computer you will use for chaosface.

2. Install the Chatbot somewhere useful and run it. **ADD INSTRUCTIONS**

3. Open a tab in your browser and navigate to the main page. If you're running chaosface from the
   same computer as the web browser, this will be `http://localhost/Chaos`

4. In the `Connection Setup` tab of chaosface, set the address for the Pi to the value you found
   above. Do not change the talk/listen ports unless you know what you are doing.

5. Make sure any firewall on your computer isn't blocking local network access to the talk and
   listen ports (5555 and 5556). If you are running the interface on a Windows machine and it is
   set to be on a private network, you should not have any issues. If your computer is set to
   public network, you will have to manually open the listen port (5556 by default) to incomming
   connections. Note that you do *not* need to open these ports on your router unless you are
   trying to run the interface and the engine from different local networks.

Running for the First Time
--------------------------
The first time you run chaosface, you need to authorize it in your channel. You can run the bot
through a dedicated bot account that you control or through your main streamer account. Note that
whichever you choose, the bot will write messages in chat from that account.

1. If you're using a bot account, sign in to Twitch with that account.

2. Start chaosface. If you are using the default setup, this involves simply applying power to
   the Raspberry Pi and waiting for it to boot up.

3. Open a browser and navigate to the Chaos page:
   - If you're running chaosface on the Pi: http://raspberrypi.local/Chaos.
   - If you're running chaosface on the same computer as the browser, go to http://localhost/Chaos.

4. Enter your bot name and channel name.

4. On the "Connection Setup" tab, you will find a button labeled "Generate Token." Clicking that
   link that will take you to a Twitch authorization page. Twitch will ask you to grant permissions
   to "TCC Chaos Interfaceface." After you grant permission, you will be redirected to a page that
   shows you an Oauth token. Copy the complete token (including the 'oauth:' prefix) and paste it
   into the "Oauth Token" field in the setup page.  

5. Save the settings. If this is your first time entering credentials, the bot will attempt to connect.
   If you have already entered credentials, you will need to restart chaosface after saving in order to
   re-initialize the bot.


Configuring sources in OBS
--------------------------

Font and color adjustments
--------------------------

Operation
=========


Modifer Selections
------------------

Modifiers are chosen for voting randomly among the available mods. 

By default, the chaos bot uses a softmax algorithm to weight the probability that a mod should
be selected based on the frequency with which it has been used. In other words, the more often a
mod has been chosen in the past, the less likely it is to appear again. This feature helps reduce
the liklihood of the same mods being applied over and over. If you disable softmax, mods will
be selected with equal probability regardless of past use.

The modifier selection can also be limited by their type. Each modifier is assigned to one or
more groups (e.g., *render*, or *combat*). For each group, you can specify a maximum number of
mods to select for a vote. For example, you can limit render mods to no more than 1 at a time.


Voting Methods
--------------
The voting cycle occurs continuously as long as TCC is not paused. Each user gets one vote per
voting cycle. A second attempt to vote will be ignored.

By default, chaosface uses a proportional voting method to select the winner. When proportional
voting is enabled, the chances that a particular modifier will win the vote is proportional to
the percentage of votes that it receives. For example, if Mod A receives 66% of the votes, Mod B
receives 33% and Mod C receives 0%, Mod A has a 2/3 chance of winning, Mod B has a 1/3 chance, and
Mod C has no chance.

You can also configure vote counter to select the mod with the greatest number of votes (though
proportional voting adds chaos to the selection process, which is more in keeping with the spirit
of TCC).

Voting can also be turned off completely. In this case, a new mod will be chosen and applied every
minute without chat's input. This feature is intended mostly for testing and not recommended for
active play, since you are removing the chat interaction by doing this.


Applying Modifiers
------------------
You can apply a specific mod without waiting for it to win a vote with the command
`!apply <mod name>`. To redeem a mod, you need a mod credit. Credits can be issued in various ways,
which the streamer can choose to enable or diable individually:

* Channel-point redemption
* Bit donation
* Winning a raffle



Counters
--------
The chaos bot supports the ability to manage multiple counters and update them. By default, it
comes configured with two counters: a death counter (`!rip`) and a soft-lock counter (`!locked`).
These are intended to give an easy way to show your viewers the number of times you've died and 
been soft-locked as the result of chaos. You can change or delete these counters, as well as add
completely new ones.

Permission Levels
-----------------
Commands fall into two basic types: those that can be used by anyone who can type in chat and
those that are restricted to specific permission categories.

The streamer automatically has admin permission and can use all commands.

Each command is associated with one or more permission levels, so you can change who is allowed
to execute which commands.

You can manually add users to or remove them from roles, for example if you want to give one mod
admin permissions.


Commands
--------
*Important Note:* By default, only the information commands can be used by anyone in chat. All
commands to add modifiers, redeem credits, etc. are associated with specific permissions.
To add these 


No Permission Required:
* !chaos -- Get a general description of Twitch Controls Chaos
* !chaos vote -- Get an explanation of the voting method
* !mod <mod name> -- Describe the function of a specific mod
* !mods -- Link to list of available mods
* !mods active -- List currently active mods
* !mods voting -- List mods currently up for a vote

* !credits -- Tells the user how many mod credits they currently have

Requires 'apply_credits' permission
* !apply <mod name> -- Apply a mod (requires mod credit)

Requires 'join_raffles' permission:
* !join -- Join an active raffle

Requires 'apply_credits' permission:
* !addcredit <username> -- Give a specific user a credit to redeem a mofidier

Requires 'manage_raffles' permission:
* !raffle [time open] -- Start a raffle for a mod credit (if time is omitted, default raffle time is used)
* !scheduleraffle  [time open]

* !addcounter <name> <pattern> -- Create a counter
* !editcounter <name> <pattern> -- Change the pattern for an existing counter
* !delcounter <name> -- Delete a counter

*Note:* The Twitch chat bot is built upon the PythonTwitchBotFramework package. This framework
provides a full-featured framework that allows you to implement many other bot features beyond
those that are implemented here, and most of those can be configured by means of chat commands.
See the `PythonTwitchBotFramework documentation
<https://github.com/sharkbound/PythonTwitchBotFramework>`_ if you're interested in those
additional features, or if you want to reconfigure the default settings for features such as
permission levels for commands.


TODO List
=========
* Enable/disable individual mods from the interface
* Channel-point redemptions
* Bits redemptions
* Raffles
* Installationt scrip with configuration options
* Edit and load config files from the interface
