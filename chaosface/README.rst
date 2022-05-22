===============
Chaos Interface
===============

Overview
--------

The chaos interface (chaosface) is one of two main parts of the Twitch Controls
Chaos (TCC) system for allowing Twitch chat to alter various aspects of
gameplay in a Playstation game. Chaosface is a Python application that provides
the connection between viewers in Twitch chat and the chaos engine that
directly controls gameplay modifications (mods). This file provides reference
documentation for the chaos interface. For a general overview and quick guide,
see the general README file for Twitch Controls Chaos.

Chaosface consists of three major parts:

- A voting algorithm to select mods for voting and inform the engine of winning mods
- A chatbot to monitor Twitch chat for votes and other commands
- A user interface for the streamer to alter TCC settings and monitor TCC's status

Chaosface runs along with the chaos engine on your Raspberry Pi. You can interact
with the program through a browser interface that 

Installation
------------

Chaosface should be set up automatically during the overall installation process.
If you need to set it up manually for some reason, use the following steps:


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


Voting Methods
--------------
Voting occurs continuously 
By default, chaosface uses a proportional voting method to select the winner.


Restricting Mod Selections
~~~~~~~~~~~~~~~~~~~~~~~~~~


Applying Modifiers
------------------
You can apply a specific mod without waiting for it to win a vote with the
command !apply <mod name>. redemption process. To redeem a mod, you need a mod credit. Credits can be
issued in various ways:

* Channel-point redemption
* Bit donation
* Winning a raffle


Commands
--------
Commands available to all users in chat
* !chaos -- Get a general description of Twitch Controls Chaos
* !chaos vote -- Get an explanation of the voting method
* !mod <mod name> -- Describe the function of a specific mod
* !mods -- List all available mods
* !mods active -- List currently active mods
* !mods voting -- List mods currently up for a vote
* !mods credit -- Tells the user how many mod credits they currently have
* !apply <mod name> -- Apply a mod (requires mod credit)
* !join -- Join an active raffle
Admin-only commands
* !addcredit <username> -- Give a specific user a credit to redeem a mofidier
* !raffle [time] -- Start a raffle for a mod credit
