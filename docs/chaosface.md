***************
Chaos Interface
***************

Overview
========

The Chaos interface, *Chaosface*, is one of two main parts of the Twitch Controls Chaos (TCC)
system for allowing Twitch chat to alter various aspects of gameplay. Chaosface is a Python
application that provides the connection between viewers in Twitch chat and the chaos engine
that directly controls gameplay modifications (mods). This file provides reference documentation
for the chaos interface. For a general overview and quick guide, see the general README file for
Twitch Controls Chaos.

Chaosface consists of three major parts:

- A voting algorithm to select mods for voting and inform the engine of winning mods
- A chatbot to monitor Twitch chat for votes and other commands
- A user interface for the streamer to alter TCC settings and monitor TCC's status

Chaosface can run along with the chaos engine on your Raspberry Pi, or you can host it on
another computer.

Installation and Setup
======================

The default installation of Chaosface runs on the Raspberry Pi along with the engine. This is
the simplest setup and is recommended if you're uncomfortable with setting up networking
See the main TCC documentation for those steps. This section explains the steps you need to take
if you plan to run Chaosface on a separate computer.

Installing Chaosface on Another Computer
----------------------------------------

To install Chaosface on a separate computer from the Chaos engine, you will need to know the
IP address assigned to the Pi and to the computer that you're running Chaosface on. Note that
there's a good chance that your router assigns these IP addresses dynamically, which over time
can mean that the IP address will change. To avoid having to reset IP addresses periodically,
it's recommended that you have your router assign static IP addresses to both the Pi and the
computer that will host Chaosface.

If you have already installed TCC with Chaosface on the Pi, you must re-run the `install.sh`
script and choose the options for remote installation. This will stop the chatbot from running
from the Pi and allow you to configure the correct interface address.

Setting up the computer for Chaosface:

1. Install Python 3.10 or newer on the computer that will run Chaosface.

2. Download or clone this repository onto that computer.

3. Run the dedicated Chaosface remote installer. This is separate from the Raspberry-Pi `install.sh`
   and can be run by itself.

   Linux / macOS:
   - Open a terminal in the repository root.
   - Run: `./chaosface/install/install_chaosface.sh`

   Windows (Command Prompt or PowerShell):
   - Open a terminal in the repository root.
   - Run: `chaosface\install\install_chaosface.bat`

   Notes:
   - By default this installs to `~/chaosface-runtime` (or `%USERPROFILE%\chaosface-runtime` on Windows).
   - To choose a different install directory, pass `--install-dir <path>` to either installer.

4. Start Chaosface from the installed runtime directory:

   Linux / macOS:
   - `<install-dir>/run_chaosface.sh`

   Windows:
   - `<install-dir>\run_chaosface.bat`

5. Open a browser and navigate to the main page. If you're running Chaosface from the
   same computer as the web browser, this will be `http://localhost/`

6. In the `Connection Setup` tab of Chaosface, set the address for the Pi to the value you found
   above. Do not change the talk/listen ports unless you know what you are doing.

7. Make sure any firewall on your computer isn't blocking local network access to the talk and
   listen ports (5555 and 5556). If you are running the interface on a Windows machine and it is
   set to be on a private network, you should not have any issues. If your computer is set to
   be on a public network, you will have to manually open the listen port (5556 by default) to
   incoming connections. Note that you do *not* need to open these ports on your router unless you
   are trying to run the interface and the engine from different local networks.

Running for the First Time
--------------------------

The first time you run Chaosface, you need to authorize it in your channel. You can run the bot
through a dedicated bot account that you control or through your main streamer account. Note that
whichever you choose, the bot will write messages in chat from that account.

1. If you are going to run Chaosface from your streamer account, just follow the instructions
   below while you are logged in to your main account rather than a bot account.

2. Start Chaosface. If you are using the default setup, this involves simply applying power to
   the Raspberry Pi and waiting for it to boot up.

3. Open a browser and navigate to the Chaos page:
   - If you're running Chaosface on the Pi, the default is http://raspberrypi.local/.
   - If you're running Chaosface on the same computer as the browser, go to http://localhost/.

4. Go to the "Connection Setup" tab and enter your bot name and channel name.

5. Next you need an OAuth token for Chaosface. The bot OAuth token lets Chaosface connect to your
   channel and send and receive chat messages. Without it, the interface will not work. Note that
   this token does not grant that account any special permissions. For example, if you want your
   bot to be a mod, you must grant it that permission on Twitch.
   
    A. If you're using a bot account, sign in to Twitch with that account. Otherwise, stay logged
       in to your main streaming account.
    B. Click **Start bot OAuth login** in the Connection Setup tab. This opens Twitch auth in a
       new tab and uses the callback URL shown in the UI (do not hard-code localhost unless that is
       actually where Chaosface is running).
    C. After permission is granted, Chaosface stores the generated token from the callback page.
    D. Click **Load generated tokens** and then **Save** in Connection Setup.

6. An EventSub OAuth token is only required if you want to allow chat members to redeem channel
   points or bits to acquire modifier credits. If you don't plan to use either feature, you can
   leave the EventSub OAuth Token field blank. To get this token, you need to be logged in to your
   main account and *not* your bot account. This is because you, as the streamer, must grant the
   bot permissions to read EventSub events about channel-point redemptions and bit donations.

    A. Log in to Twitch with your main account.
    B. From the "Connection Setup" tab, click **Start EventSub OAuth login**.
    C. An authorization dialog from Twitch will appear asking if you want to give permissions to
       "TCC Chaos Interfaceface." Grant the permissions.
    D. After permission is granted, Chaosface stores the generated token from the callback page.
       Click **Load generated tokens** and then **Save**.

7. If you enable self-signed TLS in Connection Setup, use **Generate self-signed cert** to create
   a certificate/key on the Chaosface host. Save settings and restart Chaosface to apply TLS
   changes.
8. Save the settings. If this is your first time entering credentials, the bot waits to connect
   until after you have entered your credentials. After that, saving updated credentials will
   automatically restart the chatbot to re-initialize the Twitch connection.

If the OAuth tokens expire or are manually reset, you will need to repeat these steps.

Configuring sources in OBS
--------------------------
The Chaos interface generates three browser sources that you can add as overlays to show the
current status of Chaos to your viewers:

* Active Mods: Shows the mods currently in effect with progress bars indicating how much time remains for each mod.
* Votes: Shows the mods currently available to be voted on, along with the number of votes each mod has currently received.
* Vote Timer: A progress bar showing the time left for the current voting cycle.

To add these overlays to OBS or SLOBS, perform the following steps:

* Make a copy of the scene you normally use to stream PlayStation games. Name it something like "Twitch Controls Chaos".

* To this new scene, add each of the following as a browser source. The default URLs are as follows.

  - Active Mods: http://raspberrypi.local/ActiveMods
  - Votes: http://raspberrypi.local/CurrentVotes
  - Vote Timer: http://raspberrypi.local/VoteTimer

If you are running Chaosface from a different computer, adapt the URL accordingly. 

It's recommended to set these browser sources to refresh when not displayed so that they can easily
be refreshed.


Font and Color Adjustments
--------------------------
TODO: Let this be configurable through the UI

Operation
=========

When Chaosface begins, it logs into your stream's chat and attempts to communicate with the Chaos
engine to get the game information. Until both connections are made, you cannot start playing chaos.

If the interface does not receive a response from the engine, it retries every 30 seconds
until a response is received.

You can monitor the operation of Chaos from your browser.
  - If you're running Chaosface on the Pi: http://raspberrypi.local/.
  - If you're running Chaosface on the same computer as the browser, go to http://localhost/

The default tab on this page ("Streamer Interface") shows you what mods are currently active,
whether chaos is running or paused, and a few other diagnostic features. It *does not* show what
mods are currently being voted on. This allows chat to surprise you with their choice of modifiers,
assuming you're not peeking at the sources in OBS.

The following sections explain the basic concepts of the Chaos system and how it can be
customized. To change settings from their default values, go to the "Game Settings" tab of
the Chaos browser page.

Voting Cycle
------------

The voting cycle is the normal loop in which gameplay modifiers are chosen and applied. It runs
continuously as long as Chaos is not paused. In each cycle, a set of modifiers is selected, chat
(normally) has the opportunity to vote on them, and a winner is chosen. You can modify many
parameters of the voting cycle to customize how and when voting occurs.

With the default settings, a new vote occurs once each minute, and the winning modifier is active
for 3 minutes. This has the effect of keeping 3 modifiers always applied to your gameplay after the
initial votes have populated the modifier list.

Modifier Selections
------------------

When voting begins, a set of modifiers is chosen randomly among the available mods and presented
in a list. The number of options per voting cycle is set with the "Vote options" parameter.

By default, the chaos bot uses a softmax algorithm to weight the probability that a mod should
be selected based on the frequency with which it has been used. In other words, the more often a
mod has been chosen in the past, the less likely it is to appear again. This feature helps reduce
the likelihood of the same mods being applied over and over.

You can adjust the selection weighting by altering the "Chance of repeat modifier" setting. The
lower the setting, the less likely a previously used modifier is to be chosen. Setting this value to
100 has the effect of disabling softmax weighting, in which case mods will be selected with equal
probability regardless of past use.

Voting Period
-------------

The voting period specifies when a voting cycle begins and how long votes last. Each user gets
one vote per voting cycle. A second attempt to vote will be ignored.

There are several options for the timing of the voting cycle.

* Interval
* Continuous (*Default*)
* Random
* Triggered
* Disabled

In interval voting votes occur at set intervals. You specify the length of time that a vote
is open ("Time to vote") and the delay between the end of the previous vote and the start of the
next one ("Time between votes"). For example, you could have the vote open for 2 minutes and a gap
of 2 minutes between votes. With the default length of a 3-minute modifier, this would have the
effect of applying 1 mod, for 3 minutes, once every 5 minutes, leaving you a few minutes of
un-modded gameplay before chaos resumes.

In continuous voting, the default method, the next voting cycle begins immediately after the
previous vote has concluded. This option is a form of interval voting where the delay between
votes is automatically set to 0 seconds and the vote length to [mod time / number of active mods].
In other words, the winner of each vote will immediately replace the oldest mod, and you will
always have a fixed number of mods active.

Random voting is a form of interval voting where the gap between votes is randomly chosen to be
between 0 seconds and the length of time specified by the "Time between votes." In other words,
the gap time functions as the maximum time between votes.

With triggered voting, a new vote must be started manually with the `!startvote` command
(`!newvote` is an alias). It remains open for the time specified in the "Time to vote"
parameter, unless it is ended early with `!endvote`.

Disabling voting prevents all votes from being held, including those started with the `!startvote`
or `!newvote` commands. If voting is disabled, modifiers can only be applied manually with the
`!apply` command. This mode is largely intended for testing new modifiers, but it might be useful
if you wanted to apply chaos to a game where you need to manually apply modifiers only at times
the interface cannot predict.

Voting Methods
--------------

There are three different methods available for selecting a winning modifier:

* Proportional (*Default*)
* Majority
* Authoritarian

By default, Chaosface uses a proportional voting method to select the winner. When proportional
voting is enabled, the chances that a particular modifier will win the vote are proportional to
the percentage of votes that it receives. For example, if Mod A receives 66% of the votes, Mod B
receives 33% and Mod C receives 0%, Mod A has a 2/3 chance of winning, Mod B has a 1/3 chance, and
Mod C has no chance.

With majority voting, the modifier with the greatest number of votes will always win. Ties are
broken by random selection among those with the greatest votes.

The 'authoritarian' mode doesn't let chat vote at all. Instead, at the end of each voting cycle,
chaos chooses a modifier for you at random. This feature is mostly intended for testing. If you
use it for active play, note that you are removing the 'twitch controls' from the chaos by doing
this.


Applying Modifiers
------------------

You can apply a specific modifier without waiting for it to win a vote with the command
`!apply <mod name>`. To execute this command, everyone except the streamer needs a modifier credit.
Credits can be issued in various ways, which the streamer can choose to enable or disable
individually:

* Channel-point redemption
* Bit donation
* Winning a raffle

Channel-Point Redemptions
-------------------------

To configure channel-points redemptions and bit donations, you must have stored a valid EventSub
token for your channel. (See the setup instructions above.)

You will need to create a channel-points redemption in your Twitch channel. Set your desired
number of channel points and any restrictions you want on how often people can redeem those points
there. In general, it's probably a good idea to make this redemption relatively expensive.

From the "Game Settings" tab in the interface, enable channel-points redemptions and enter the
exact name of the redemption you created in the "Points Reward Title" field. (The default name is
'Chaos Credit').

If your EventSub token is entered, any channel-points redemptions done while the chatbot is
active will be recorded automatically. Note, however, that if you choose to leave this
redemption active when you are not running the chatbot and someone redeems that reward, you will
need either to give credits for those redemptions manually (with `!addcredits`) or to refund
those redemptions.

Note also that any cooldown you apply on the channel-point redemption applies only to users
*getting* modifier credits. There is a separate cooldown period for the `!apply` command, which
is in effect regardless of how you earn the credit.

Bit Donations
-------------

Bit donations work similarly to channel-points redemptions. You must have stored a valid EventSub
token for your channel. (See the setup instructions above.)

When enabled, this feature monitors incoming cheers, and bit donations above a certain threshold
will give the user modifier credits. You can set the number of bits required to earn a credit in
the "Bits per mod credit" field.

If you select "Allow multiple credits per cheer," the user can earn multiple credits by donating
multiples of the base amount. For example, if the default for a credit is 100 bits, and the user
donates 200 bits, they will earn 2 mod credits. If this option is disabled, the user will only
get 1 credit per donation over the minimum threshold, regardless of the size of the donation.

There is no record kept of odd numbers of bits in between donations. For example, if the
bits-per-credit setting is 100 and a user donates 69 bits in one donation and 31 in a second,
they will not earn a credit.

As with channel-point redemptions, credits are only applied automatically while Chaosface is
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
* !remove <mod name> -- Manually remove a modifier immediately (requires admin permission)
* !mod <mod name> -- Describe the function of a specific modifier. Not case sensitive.
* !mods -- Link to list of all available modifiers
* !mods active -- List currently active modifiers
* !mods voting -- List modifiers currently up for a vote

Voting Commands (these commands all require manage_voting permission):
* !startvote (time) -- Manually open a new vote. If time omitted, default vote time is used
* !newvote (time) -- Alias for !startvote
* !endvote -- End an open vote immediately and choose a winner

Modifier Credit Commands:
* !credits (user) -- Reports the number of modifier credits that the user (message author if user name omitted) currently has
* !addcredits <user> (amount) -- Add credits to user's balance. If amount omitted, add 1. Requires 'manage_credits' permission.
* !setcredits <user> <amount> -- Sets user's balance to the specified amount. Requires 'manage_credits' permission.
* !givecredits <user> (amount) -- Give some of your modifier credits to the specified user. If amount omitted, transfers 1 credit.

Raffle Commands:
* !join -- Enters the user into an open raffle
* !joinchaos -- An alias for !join
* !raffle (time) -- Start a raffle for a modifier credit (if time is omitted, default raffle time is used) Requires 'manage_raffles' permission
* !chaosraffle -- An alias for !raffle

Permissions
-----------
Some commands require additional permissions beyond the ability to type messages in chat. The
streamer automatically has admin permission and can use all commands. You can also give these 
same permissions to trusted individual users through chat commands.

The defined permissions are the following:
* admin: Can execute all chat commands. (Streamer is in this group by default)
* manage_raffles: Start raffles
* manage_credits: Set users' modifier-credit balances to arbitrary values
* manage_modifiers: Update modifiers directly
* manage_voting: Start/end votes manually
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
* Set font and colors from interface
* Edit and load config files from the interface
* Write counter data to files for OBS to display
* Allow a re-start of bot and/or engine from the interface
* Limit modifier selection by group
* Manage command permissions through interface
* Commands to configure game settings without UI
* Assign permissions to moderators as a group
* Heartbeat monitor for engine
