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

Stop the interface from running on the Pi:

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
   be on a public network, you will have to manually open the listen port (5556 by default) to
   incomming connections. Note that you do *not* need to open these ports on your router unless you
   are trying to run the interface and the engine from different local networks.

Running for the First Time
--------------------------
The first time you run chaosface, you need to authorize it in your channel. You can run the bot
through a dedicated bot account that you control or through your main streamer account. Note that
whichever you choose, the bot will write messages in chat from that account.

1.  If you are going to run
   chaosface from your streamer account, just do everything instructed to for the bot account
   while you are logged in to your main account.

2. Start chaosface. If you are using the default setup, this involves simply applying power to
   the Raspberry Pi and waiting for it to boot up.

3. Open a browser and navigate to the Chaos page:
   - If you're running chaosface on the Pi: http://raspberrypi.local/Chaos.
   - If you're running chaosface on the same computer as the browser, go to http://localhost/Chaos.

4. Go to the "Connection Setup" tab and enter your bot name and channel name.

5. Next you need an OAuth token for chaosface. The bot Oauth token lets chaosface connect to your
   channel and send and receive chat messages. Without it, the interface will not work. Note that
   this token does not grant that account any special permissions. For example, if you want your
   bot to be a mod, you must grant it that permission on Twitch.
   
    A. If you're using a bot account, sign in to Twitch with that account. Otherwise, stay logged
       in to your main streaming account.
    B. Click on the link to get the bot's OAuth token. You will be taken to a Twitch page that
       creates the token for you.
    C. Copy the token that Twitch gives you, including the initial 'oauth:' prefix, and paste it
       in the 'Bot OAuth Token' field.  

6. A PubSub OAuth token is only required if you want to allow chat members to redeem channel points
   or bits to acquire modifier credits. If you don't plan to use either feature, you can leave the
   PubSub OAuth Token field blank. To get this token, you need to be logged in to your main account
   and *not* your bot account. This is because you, as the streamer, must grant the bot permissions
   to see messages about channel-point redepmtions and bit donations.

    A. Log in to twitch with your main account.
    B. From the "Connection Setup" tab, clink to get a PubSub token.
    C. An authorization dialog from Twitch will appear asking if you want to give permissions to
       "TCC Chaos Interfaceface." Grant the permissions.
    D. After you grant permission, you will be redirected to a page that shows you an OAuth token.
       Copy the complete token (including the 'oauth:' prefix) and paste it into the "PubSub OAuth
       Token" field.  

7. Save the settings. If this is your first time entering credentials, the bot wait to connect
   until after you have entered your credentials. After a complete set of credentials have been
   entered for the first time, you will need to restart chaosface after saving new credentials in
   order to re-initialize the bot.


Configuring sources in OBS
--------------------------
The Chaos interface generates three browser sources that you can add as overlays to show the
current status of Chaos to your viewers:

* Active Mods: Shows the mods currently in effect with progress bars indicating how much time remains for each mod.
* Votes: Shows the mods currently available to be voted on, along with the number of votes each mod has currently received
* Vote Timer: A progress bar showing the time left for the current voting cycle.

To add these overlays to OBS or SLOBS, perform the following steps:

* Make a copy of the scene you normally use to stream PlayStation games. Name it something like "Twitch Controls Chaos".

* To this new scene, add each of the following as a browser source. The default URLs are as follows.

  - Active Mods: http://raspberrypi.local/ActiveMods/
  - Votes: http://raspberrypi.local/Votes/
  - Vote Timer: http://raspberrypi.local/VoteTimer/

If you are running chaosface from a different computer, adapt the URL accordingly. 

It's recommended to set these browser sources to refresh when not displayed so that they can easily
be refreshed.


Font and color adjustments
--------------------------
<to write>

Operation
=========
When the chaos interface begins, it will log into chat and attempt to communicate with the chaos
engine to get the game information. Until both connections are made, you cannot start playing
chaos.

If chaosface does not receive a response from the engine, it will re-try every 30 seconds until
a response is received.

Modifer Selections
------------------

Modifiers are chosen for voting randomly among the available mods. 

By default, the chaos bot uses a softmax algorithm to weight the probability that a mod should
be selected based on the frequency with which it has been used. In other words, the more often a
mod has been chosen in the past, the less likely it is to appear again. This feature helps reduce
the liklihood of the same mods being applied over and over. If you disable softmax, mods will
be selected with equal probability regardless of past use.


Voting Methods
--------------
The voting cycle occurs continuously as long as TCC is not paused. Each user gets one vote per
voting cycle. A second attempt to vote will be ignored.

There are four modes for modifier selection:

* Proportional (*Default*)
* Majority
* Authoriatian
* Disabled

By default, chaosface uses a proportional voting method to select the winner. When proportional
voting is enabled, the chances that a particular modifier will win the vote are proportional to
the percentage of votes that it receives. For example, if Mod A receives 66% of the votes, Mod B
receives 33% and Mod C receives 0%, Mod A has a 2/3 chance of winning, Mod B has a 1/3 chance, and
Mod C has no chance.

With majority voting, the modifier with the greatest number of votes will always win. Ties are
broken by random selection among those with the greatest votes.

The 'Authoritarian' mode doesn't let chat vote at all. Instead, at the end of each voting cycle,
a new modifier is chosen at random by the interface and applied. This feature is intended mostly
for testing. If you use it for active play, note that you are removing the 'twitch controls' from
the chaos by doing this.

If voting is completely disabled, new mods are only applied manually. Like the Authoritarian mode,
this mode is largely intended for testing new modifiers, but it might be useful if you wanted to
apply chaos to a game where you need to manually apply modifiers only at times the interface cannot
predict, e.g., at the beginning of a new PVP match.


Applying Modifiers
------------------
You can apply a specific modifier without waiting for it to win a vote with the command
`!apply <mod name>`. To execute this command, everyone except the streamer needs a modifier credit.
Credits can be issued in various ways, which the streamer can choose to enable or diable
individually:

* Channel-point redemption
* Bit donation
* Winning a raffle

Channel-Point Redemptions
-------------------------
To configure channel-points redemptions and bit donations, you must have stored a valid PubSub
token for your channel. (See the setup instructions above.)

You will need to create a channel-points redemption in your Twitch channel. Set your desired
number of channel points and any restrictions you want on how often people can redeem those points
there. In general, it's probably a good idea to make this redemption relatively expensive.

From the "Game Settings" tab in the interface, enable channel-points redemptions and enter the
exact name of the redemption you created in the "Points Reward Title" field. (The default name is
'Chaos Credit').

If your PubSub token is entered, any channel-points redemptions done while the chatbot is
active will be recorded automatically. Note, however, that if you choose to leave this
redemption active when you are not running the chatbot and someone redeems that reward, you will
need either to give credits for those redemptions manually (with !addcredits) or to refund
those redemptions.

Note also that any cooldown you apply on the channel-point redemption applies only to users
*getting* modifier credits. There is a separate cooldown period for the `!apply` command, which
is in effect regardless of how you earn the credit.

Bit Donations
-------------
Bit donations work similarly to channel-points redemptions. You must have stored a valid PubSub
token for your channel. (See the setup instructions above.)

When enabled, this feature monitors incomming cheers, and bit donations above a certain threshold
will give the user modifier credits. You can set the number of bits required to earn a credit in
the "Bits per mod credit" field.

If you select "Allow multiple credits per cheer", The user can earn multiple credits by donating
multiples of the base amount. For example, if the default for a credit is 100 bits, and the user
donates 200 bits, they will earn 2 mod credits. If this option is disabled, the user will only
get 1 credit per donation over the minimum threshold, regardless of the size of the donation.

There is no record kept of odd numbers of bits in between donations. For example, if the
bits-per-credit setting is 100 is a user donates 69 bits in one donation and 31 in a second,
they will not earn a credit.

As with channel-point redemptions, credits are only applied automatically while chaosface is
running and connected to Twitch. If you want to give credits for donations that come in at
other times, you need to add the credits manually.

Raffles
-------
A raffle gives you a way to distribute modifier credits to users without them needing to spend
channel points or bits. To enable raffles, check the "Conduct raffles" box in the "Game
Settings" menu. You can set the default raffle time, in seconds, here.

The command `!raffle` opens a raffle of the default length. You can customize the raffle length
by adding a time, in seconds. For example, `!raffle 300` will open a 5-minute raffle. When the
raffle opens, and periodically throughout the raffle time, the chatbot will announce the raffle
and instruct users to enter the raffle with the `!join` command.

When the time expires, one winner will be selected at random from those who have joined and
will receive a modifier credit.

Commands
--------

General Information Commands:
* !chaos -- Get a general description of Twitch Controls Chaos
* !chaos apply -- Get an explanation of how to apply modifier credits
* !chaos credits -- Get an explanation of how to earn modifier credits
* !chaos voting -- Get an explanation of the voting method

Modifier Commands:
* !apply <mod name> -- Apply a modifier (requires modifier credit and subject to cooldown)
* !mod <mod name> -- Describe the function of a specific modifier. Not case sensitive.
* !mods -- Link to list of all available modifiers
* !mods active -- List currently active modifiers
* !mods voting -- List modifiers currently up for a vote

Modifier Credit Commands:
* !credits (user) -- Reports number of modifier credits that the user (message author if user name ommitted) currently has
* !addcredits <user> (amount) -- Add credits to user's balance. If amount omitted, add 1. Requires 'manage_credits' permission.
* !setcredits <user> <amount> -- Sets user's balance to the specified amount. Requires 'manage_credits' permission.
* !givecredits <user> (amount) -- Give some of your modifier credits to the specified user. If amount omitted, transfers 1 credit.

Raffle Commands:
* !join -- Enters the user into an open raffle
* !raffle (time) -- Start a raffle for a modifier credit (if time is omitted, default raffle time is used) Requires 'manage_raffles' permission


*Note:* The chat bot is built upon the PythonTwitchBotFramework package. This framework means you
can implement other features common to many bots by means of chat commands. See the
`Twitch bot framework documentation <https://github.com/sharkbound/PythonTwitchBotFramework>`_ 
if you're interested in those additional features, or if you want to reconfigure chatbot settings
for which there is no UI.

Permissions
-----------
Some commands require additional permissions beyond the ability to type messages in chat. The
streamer automatically has admin permission and can use all commands. You can also give these 
same permissions to trusted individual users through chat commands.

The defined permissions are the following:
* admin: Can execute all chat commands. (Streamer is in this group by default)
* manage_raffles: Start raffles
* manage_credits: Set users' modifier-credit balances to arbitrary values
* manage_modifiers: update modifiers directly
* manage_permissions: Create permission groups and add/remove users and permissions from them


To give these extra permissions, you must first create a permission group, and then assign both
permissions and individual users to that group. The following commands are available. All
require the 'manage_permissions' permission to execute:

* To add a permission group: !addgroup <group>
* To add a member to a group: !addmember <group> <user>
* To add a permission to a group: !addperm <group> <permission>
* To remove a member from a group: !delmember <group> <user>
* To remove a permission from a group: !delperm <group> <permission>


TODO List
=========
* Installation script with configuration options
* Set font and colors from interface
* Edit and load config files from the interface
* Write counter data to files for OBS to display
* Allow a re-start of bot and/or engine from the interface
* Limit modifier selection by group
* Manage command permissions through interface
* Commands to configure game settings without UI
* Assign permissions to moderators as a group
* Heartbeat monitor for engine
