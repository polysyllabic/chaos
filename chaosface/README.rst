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

Installation differs if you are running chaosface on the Raspberry Pi or on a separate computer.

*This section is just a skeleton with some guesses about the process. Actual instructions to be
added once we have a stable system.*

Installing Chaosface on the Raspberry Pi
----------------------------------------

If you chose to install chaosface on the Pi when installing TCC, you should not need to
do anything further. Chaosface should have been set up to run automatically. If you originally
chose to skip installing chaosface on the Pi and now want to add it, use the following steps:

*some configuration steps here*

Installing Chaosface on Another Computer
----------------------------------------

1.  Configure the Pi:
    - If this is the initial setup on the Pi, choose to *skip* installing chaosface.
    - If you originally installed chaosface on the Pi and want to switch to using it on a
      separate computer, log in to the Pi and run the following script: *some script*.

2. Make sure Python 3 is installed on your computer

3. Install the Chatbot somewhere useful


Initial Setup
-------------

Entering your credentials

Getting an Oauth key

Subscribing to pubsub events

Configuring sources in OBS

Font and color adjustments


Operation
---------

The default configuration sets things up to start both the chaos engine and
interface automatically when power is applied to your Raspberry Pi. 

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
By default, the chaos bot recognizes five user roles:
- Admin: The streamer is automatically assigned this role
- Moderator: Channel moderators are automatically assigned this role
- VIP: Channel VIPs are automatically assigned this role
- Subscriber: Channel subscribers are automatically assigned this role
- User: This is the default role of anyone who can chat

Each command is associated with one or more permission levels, so you can change who is allowed
to execute which commands.

You can also manually add users to or remove them from roles, for example if you want to give one
moderator admin permissions.


Commands
--------
User commands
* !chaos -- Get a general description of Twitch Controls Chaos
* !chaos voting -- Get an explanation of the voting method
* !mod <mod name> -- Describe the function of a specific mod
* !mods -- Link to list of available mods
* !mods active -- List currently active mods
* !mods voting -- List mods currently up for a vote
* !mods credit -- Tells the user how many mod credits they currently have
* !apply <mod name> -- Apply a mod (requires mod credit)
* !join -- Join an active raffle

Admin commands
* !addcredit <username> -- Give a specific user a credit to redeem a mofidier

Mod Commands:
* !raffle [time] -- Start a raffle for a mod credit
* !addcounter <name> <pattern> -- Create a counter
* !editcounter <name> <pattern> -- Change the pattern for an existing counter
* !delcounter <name> -- Delete a counter
