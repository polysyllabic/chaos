# Twitch Controls Chaos: Chaosface

The Chaos interface, *Chaosface*, is one of two main parts of the Twitch Controls Chaos (TCC)
system for allowing Twitch chat to alter various aspects of gameplay. Chaosface is a Python
application that provides the connection between viewers in Twitch chat and the chaos engine
that directly controls gameplay modifications (mods). This file provides reference documentation
for the chaos interface.

Chaosface consists of three major parts:

- A voting algorithm to select mods for voting and inform the engine of winning mods
- A chatbot to monitor Twitch chat for votes and other commands
- A user interface for the streamer to alter TCC settings and monitor TCC's status

Chaosface can run along with the chaos engine on your Raspberry Pi, or you can host it on
another computer.

## Installation

The default installation of Chaosface runs on the Raspberry Pi along with the engine. This is
the simplest setup and is recommended if you're uncomfortable with setting up networking. If
you choose this route, Chaosface will be installed along with the other components of TCC. In
that case, you can skip the rest of these installation instructions and proceed with
initial configuration.

To install Chaosface on a separate computer from the Chaos engine, you will need to know the
IP address assigned to the Pi and to the computer that you're running Chaosface on. Note that
there's a good chance that your router assigns these IP addresses dynamically, which over time
can mean that the IP address will change. To avoid having to reset IP addresses periodically,
it's recommended that you have your router assign static IP addresses to both the Pi and the
computer that will host Chaosface.

To install Chaosface on a computer other than the Pi, you must follow these steps:

1. Python 3.10 or newer is required. If it's not yet installed on the computer that will run
   Chaosface, install it first.

2. Download or clone the TCC repository onto the computer that will run Chaosface. If you've
   downloaded the repository, unpack the archive to whatever directory you desire. To clone
   the repository, you must have Git installed on this computer

3. Run the standalone installer script to install Chaosface by itself. This is separate from
   the Raspberry-Pi `install.sh` script, which you should _only_ run from the Pi.

   Linux / macOS:
   - Open a terminal in the repository root.
   - Run: `./chaosface/install/install_chaosface_standalone.sh`

   Windows (Command Prompt or PowerShell):
   - Open a terminal in the repository root.
   - Run: `chaosface\install\install_chaosface_standalone.bat`

   Notes:
   - By default this installs to `~/chaosface-runtime` (or `%USERPROFILE%\chaosface-runtime` on Windows).
   - To choose a different install directory, pass `--install-dir <path>` to either installer.
   - Do not run the remote installer with `sudo` unless you also pass `--install-dir`; otherwise
     the default `~` resolves to `/root` and the runtime lands under `/root/chaosface-runtime`.

## Running Chaosface

When hosted on the Pi, Chaosface starts automatically when the Pi boots up. If for some reason you
need to restart Chaosface without rebooting the Pi, log in to the Pi and issue the following
command: `sudo systemd restart chaosface`

If you are running Chaosface from another computer, you must start it yourself. From the installed
runtime directory:

   Linux / macOS:
   - `<install-dir>/run_chaosface.sh`

   Windows:
   - `<install-dir>\run_chaosface.bat`

## Initial Configuration

Once Chaosface is running, open a browser and navigate to the main page. If Chaosface is running
on the Pi and you've accepted the default hostname, the address will be
[raspberrypi.local](http://raspberrypi.local). If you're running Chaosface from the same computer
as your web browser, the address will be `http://localhost/`. In other scenarios, substitute the
appropriate host name.

The first time you navigate to this page, Chaosface will prompt you to set a password to access the
user interface. You can choose to continue without one, but if you do that, anyone on your local
network will be able to access the streamer's interface. (As long as your Internet connection is
behind a router's firewall, however, no one outside should be able to access it.)

If you do set a password, you will have to enter it each time you start a new session. In addition
to a password, you can also run Chaosface over TLS, or encrypted handling of traffic. (See below.)

### When Chaosface Runs On a Separate Computer

If you're running Chaosface on a separate computer, enter address of the Raspberry Pi in the
provided box. Do not change the talk/listen ports unless you know what you are doing.

If the default address (or whatever you chose) does not work, you may need to find your Pi's
IP address and use it directly. You can discover this address by going to your router's admin
page, finding the list of connected devices, and seeing if you can identify which one is the Pi.
Alternatively, run the command `hostname -I` on your Pi to get your Pi's IP address. Input this
address in your browser instead.

Make sure any firewall on your computer isn't blocking local network access to the talk and
listen ports (5555 and 5556). If you are running the interface on a Windows machine and it is
set to be on a private network, you should not have any issues. If your computer is set to
be on a public network, you will have to manually open the listen port (5556 by default) to
incoming connections. Note that you do *not* need to open these ports on your router unless you
are trying to run the interface and the engine from different local networks.

### Getting Twitch Authorization

The chatbot needs to log in to your chat in order to read and send messages. The security for this
communication requires you to get OAuth tokens from Twitch. These tokens prove to Twitch that
you've authorized the bot within your channel. Note that this token does not grant that account
any special permissions. For example, if you want your bot to be a mod, you must grant it that
permission on Twitch.

If you don't have a bot account already, or if you prefer to run Chaosface as an additional bot,
create a new Twitch account for the bot.

When you get an OAuth tokens, Twitch requires a callback location to sent it. Chaosface has been
configured to use `http://localhost:8080/` (NB: this is not a setting you can change), which
gives you access to the token without needing a separate, secure server on the Internet to
handle the callback. However, if Chaosface is not running on the same machine as your browser,
and it won't be if you're running it form the Pi, you will need to take one extra step before
getting these tokens.

 From a terminal, run the command
     `ssh -L 8080:localhost:80 pi@raspberrypi.local`
     
Substitue your username and/or address for the Pi if you've changed themn from the defaults.

This command creates a local SSH tunnel so you automatically can pass the token you receive
along to Chaosface. This tunnelling is only necessary to get OAuth tokens. Otherwise, you 
can run Chaosface without the ssh step.

Once the tunnel is established, open Chaosface and click on the "Settings" tab. Enter your
channel name and bot-account name. Although the bot can use your main streaming account to
chat, this is not recommended. If you do so, you will not be able to use channel points for
modifier redemptions.

Before doing the next step, make sure you are logged into Twitch with your _bot_ account.

A. Click **Start bot OAuth login** in the Connection Setup tab. This opens Twitch auth in a
new tab and uses the fixed localhost callback URI.

B. After you grant permission, Chaosface stores the generated token from the callback page.

C. Click **Load generated tokens** and then **Save** in Connection Setup.

An EventSub OAuth token is only required if you want to allow chat members to redeem channel
points or bits to acquire modifier credits. If you don't plan to use either feature, you can
leave the EventSub OAuth Token field blank. To get this token, you need to be logged in to your
main account and *not* your bot account. This is because you, as the streamer, must grant the
bot permissions to read EventSub events about channel-point redemptions and bit donations.

A. Log in to Twitch with your main account.
B. From the "Connection Setup" tab, click **Start EventSub OAuth login**.
C. An authorization dialog from Twitch will appear asking if you want to give permissions to
the TCC Chaos Interfaceface. Grant the permissions.
D. After permission is granted, Chaosface stores the generated token from the callback page.
Click **Load generated tokens** and then **Save**.

The bot will not try to connect to Twitch until you enter values for your channel name, bot name,
and bot OAuth token. Once those are saved, the bot will attempt to log in to Twitch immediately
after starting. Any time you update your credentials, the chatbot will restart to re-initialize
the Twitch connection.

If the OAuth tokens expire or are manually reset, you will need to repeat these steps.

## HTTPS / TLS
TLS lets you run Chaosface over an encrypted connection. It is not required, but if you
run without TLS your browser will likely complain that data you enter is insecure.

Chaosface supports three UI TLS modes from Connection Setup:

* ``off``: HTTP only
* ``self-signed``: HTTPS using an auto-generated self-signed certificate
* ``custom``: HTTPS using a provided certificate and private key

If you enable self-signed TLS in Connection Setup, use **Generate self-signed cert** to create
a certificate/key on the Chaosface host. Chaosface auto-generates files at
``configs/tls/selfsigned-cert.pem`` and ``configs/tls/selfsigned-key.pem`` if they do not already
exist.  Note that when you first try to access the UI with a self-signed certificate, the browser
will complain that it is insecure and you will need to manually override the security warning.

You can configure the self-signed certificate hostname in Connection Setup.

The UI port field can be set directly; by default it uses port 80 for HTTP and port 443 for TLS,
and switching TLS mode updates that default.

The **Overlay HTTP port** field controls an additional non-TLS listener for OBS overlays when
TLS is enabled for the UI (default `80`; set `0` to disable).

Custom certificate mode:

* Provide paths to your PEM certificate and private key in Connection Setup.
* Both files must exist and be readable by the ``chaosface`` process.

Example file install (systemd deployment):

.. code-block:: bash

   sudo mkdir -p /usr/local/chaos/certs
   sudo cp fullchain.pem /usr/local/chaos/certs/chaosface-cert.pem
   sudo cp privkey.pem /usr/local/chaos/certs/chaosface-key.pem
   sudo chown root:root /usr/local/chaos/certs/chaosface-*.pem
   sudo chmod 600 /usr/local/chaos/certs/chaosface-key.pem
   sudo chmod 644 /usr/local/chaos/certs/chaosface-cert.pem

Changes to the password or TLS settings require you to save and restart Chaosface to take
effect:

  `sudo systemctl restart chaosface`


## Configuring OBS Overlays
The Chaos interface generates three browser sources that you can add as overlays to show the
current status of Chaos to your viewers:

* Active Mods: Shows the mods currently in effect with progress bars indicating how much time
  remains for each mod.
* Votes: Shows the mods currently available to be voted on, along with the number of votes each
  mod has currently received.
* Vote Timer: A progress bar showing the time left for the current voting cycle.

To add these overlays to OBS or SLOBS, perform the following steps:

* Make a copy of the scene you normally use to stream PlayStation games. Name it something like
  "Twitch Controls Chaos".

* To this new scene, add each of the following as a browser source. The canonical paths are:

  - Active Mods: `http://raspberrypi.local/overlays/active-mods`
  - Votes: `http://raspberrypi.local/overlays/current-votes`
  - Vote Timer: `http://raspberrypi.local/overlays/vote-timer`

If you are running Chaosface from a different host, replace `raspberrypi.local` with your
hostname.

The same pages are also available at aliases (`<host>/ActiveMods`, `<host>/CurrentVotes`,
`<host>/VoteTimer`, and lowercase variants).

If you have enabled TLS for your UI, these sources are available at the same names but on `https`.
Note, however, that if you are using a self-signed certificate, OBS will silently reject https
pages with self-signed certificates. To work around this, we also continue to serve OBS sources
(but not the UI interface) on `http` even when TLS is enabled.

Recommended OBS browser-source dimensions (pixels):
- Active Mods (`/overlays/active-mods`): width `500`, height `160`
- Votes (`/overlays/current-votes`): width `500`, height `160`
- Vote Timer (`/overlays/vote-timer`): width `1920`, height `44`

These are the basic dimensions of the sources, but you can use obs to scale them however you want.

You should set these browser sources to refresh when not displayed so that they can easily be refreshed.

The `SOURCE CONFIGURATION` tab lets you tune source layout and colors:

For both the current-votes and active mods sources, you can position the text (which includes the
names of the relevant modifiers) to the left or right of the progress pars.

You can also set the text color and the bar color for all three sources.

Color fields accept standard CSS names, hex values, and `rgb(...)`/`rgba(...)` values or you
can use a color-picker control to find the color you want.

Note that with the original version of TCC, you had to adjust the colors of the sources by applying
filters in OBS. If you are converting over from that old setup and aren't seeing the colors you
expect, try deleting the old filters and setting new colors from Chaosface.

## Chat Filters

When you play TCC, your chat will be full of votes (1/2/3) as well as periodic commands asking
for definitions of mods that are up for voting, and potentially other commands that might give
you as the streamer a hint as to what is coming up. To preserve the surprise, and to reduce
the volume of messages you have to look at, you can implement chat filters. This will allow
you to avoid seeing these messages without changing the experience for your viewers.

### BTTV filters

If you use the BetterTTV browser extension, you can use its blacklist function to remove specific
commands and votes from your chat.

From your Twitch chat, click on the Chat Settings gear icon and scroll down to BetterTTV settings.
Then scroll down to "Blacklist Keywords" and add a new keyword for each item to block.

To block the votes, add each vote number as a separate keyword surrounded with angled brackets,
e.g., `<1>`.

The angled brackets mean that the entire message must be a single number. This will block messages
consisting solely of a single number but allow messages like "You have died 3 times!" to come
through. If you don't include those brackets, messages containing that number anywhere in them
will be filtered.

If you want to filter individual commands, you can do it the same way, e.g., `<!mods>`. To prevent
yourself from seeing !mod commands, include that command _without_ the brackets, i.e., `!mod`,
as a blacklisted keyword.

To block seeing responses from your chatbot (such as a reply to the !mod messages), after you
click "AddNew", click again on the "Message" type and select "Username" instead. Then fill
in the keyword field with the name of your chatbot.

Thanks to Twich streamer [RachyMonster](https://www.twitch.tv/rachymonster) for showing how
to use this blacklist. 

## FrankerFaceZ filters
If you have the FrankerFaceZ extension, you can accomplish the same thing in a similar way.
Open the FrankerFaceZ Control Center, go to Chat >> Filtering >> Block, and add the terms to
filter. Unlike the BetterTTV filters, you can use regular expressions to filter messages.

`^[1-3]$` will filter only messages that solely consist of a single number.

Add your chatbot to the Users list.

For all terms and users that you add, make sure to check the box "Remove matching messages from chat".

### Chat Overlays

BTTV and FrankerFaceZ filters only filter what you see in your normal chat window. If you
display an overlay of your chat on your screen, you will need to set up filters for that
separately.

If you use the Streamlabs Chat Box to show the chat as an overlay on screen, check
"Hide commands starting with '!'" and then add your bot name to the "MutedChatters" box.

In "Custom Bad Words" add the following expression: `regex:^[1-3]`

This will filter any messages that start with the number 1, 2, or 3 but will let messages
like "You have died 3 times!" through.

If you don't implement BTTV or FrankerFaceZ fitering and want to read filtered chat, note that you
can pull up the source for the chatbox widget in its own browser tab and read it directly without
looking at the OBS interface itself.

StreamElements also provides a chat-box overlay, but it can only block individual users from
chatting. So while you can stop messages from your bot from appearing here, it won't stop the votes
from appearing or specific commands.

# Operation

The following sections explain the basic concepts of the Chaos system and how it can be customized.
To change settings from their default values, go to the "Game Settings" tab of the Chaos browser page.

## Voting Cycle

The voting cycle is the normal loop in which gameplay modifiers are chosen and applied. It runs
continuously as long as Chaos is not paused. In each cycle, a set of modifiers is selected, chat
(normally) has the opportunity to vote on them, and a winner is chosen. You can modify many
parameters of the voting cycle to customize how and when voting occurs.

With the default settings, a new vote occurs once each minute, and the winning modifier is active
for 3 minutes. This has the effect of keeping 3 modifiers always applied to your gameplay after the
initial votes have populated the modifier list.

## Modifier Selections

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

## Voting Period

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

## Voting Methods

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

## Applying Modifiers

You can apply a specific modifier without waiting for it to win a vote with the command
`!apply <mod name>`. To execute this command, everyone except the streamer needs a modifier credit.
Credits can be issued in various ways, which the streamer can choose to enable or disable
individually:

* Channel-point redemption
* Bit donation
* Winning a raffle

## Channel-Point Redemptions

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

## Bit Donations

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

## Raffles

A raffle gives you a way to distribute modifier credits to users without them needing to spend
channel points or bits. To enable raffles, check the "Conduct raffles" box in the "Game
Settings" menu. You can set the default raffle time, in seconds, here.

The command `!raffle` opens a raffle of the default length. You can customize the raffle length
by adding a time, in seconds. For example, `!raffle 300` will open a 5-minute raffle. When the
raffle opens, and periodically throughout the raffle time, the chatbot will announce the raffle
and instruct users to enter the raffle with the `!join` command.

When the time expires, one winner will be selected at random from those who have joined and
will receive a modifier credit.

# Commands

General Information Commands:
* !chaos -- Get a general description of Twitch Controls Chaos
* !chaos apply -- Get an explanation of how to apply modifier credits
* !chaos credits -- Get an explanation of how to earn modifier credits
* !chaos voting -- Get an explanation of the voting method
* !chaoscmd -- Get a list of available commands for Twitch Controls Chaos

Modifier Commands:
* !apply <mod name> -- Apply a modifier (requires modifier credit and subject to cooldown)
* !remove <mod name> -- Manually remove a modifier immediately (requires admin permission)
* !mod <mod name> -- Describe the function of a specific modifier. Not case sensitive.
* !mods -- Link to list of all available modifiers
* !mods active -- List currently active modifiers
* !mods voting -- List modifiers currently up for a vote

Modifier Management Commands (require manage_modifiers permission):
* !enable <mod name> -- Enable a modifier for voting/selection
* !disable <mod name> -- Disable a modifier for voting/selection
* !resetmods -- Request an immediate reset of active modifiers
* !applycooldown <time> -- Set the apply command cooldown, in seconds (does not affect admin use)

Voting Commands (these commands all require manage_voting permission):
* !startvote (time) -- Manually open a new vote. If time omitted, default vote time is used
* !newvote (time) -- Alias for !startvote
* !endvote -- End an open vote immediately and choose a winner
* !votemethod <method> -- Set winner selection method. Legal methods are proportional, majority, authoritarian (resets any open vote)
* !votetime <time> -- Set default vote length, in seconds (resets any open vote)
* !votecycle <type> -- Set voting cycle type. Legal types are continuous, interval, random, triggered, disabled (resets any open vote unless voting is disabled)

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
* !raffletime <time> -- Set default raffle duration, in seconds. Requires 'manage_raffles' permission

## Command Aliases
Command aliases allow you to rename commands to your liking. Some default aliases are listed above. You can manage these command
names from the `Chatbot Commands` tab of the user interface. If you set an alias, it will be available as an alternate command.
If you also set the checkbox `Use alias only` then the default command name will be disabled and only the alias will be accepted.
You can use this feature if you already have a different bot that responds to these command names (e.g., !raffle) and want to keep
that other bot running as before.

## Permissions
Some commands require additional permissions beyond the ability to type messages in chat. The
streamer automatically has admin permission and can use all commands. You can also give these 
same permissions to trusted individual users through chat commands.

The defined permissions are the following:
* admin: Can execute all chat commands. (Streamer is in this group by default)
* manage_raffles: Start raffles
* manage_credits: Set users' modifier-credit balances to arbitrary values
* manage_modifiers: Update modifiers directly
* manage_voting: Start/end votes manually and configure vote settings
* manage_permissions: Create permission groups and add/remove users and permissions from them

To give these extra permissions, you must first create a permission group, and then assign both
permissions and individual users to that group. The following commands are available. All
require the 'manage_permissions' permission to execute:

* To add a permission group: !addgroup <group>
* To add a member to a group: !addmember <group> <user>
* To add a permission to a group: !addperm <group> <permission>
* To remove a member from a group: !delmember <group> <user>
* To remove a permission from a group: !delperm <group> <permission>
