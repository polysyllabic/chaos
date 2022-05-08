# Twitch Controls Chaos (Chaos Unbound Edition)

![Chaos](https://github.com/polysyllabic/chaos/blob/main/docs/images/chaos.jpg?raw=true)

## What is Chaos?
Twitch Controls Chaos, TCC or Chaos for short, lets Twitch chat interfere with a streamer
playing a PlayStation game. It uses a Raspberry Pi to modify controller input based on a
series of gameplay modifiers that chat can select by voting for their favorites.

[Watch a minute long video explanation of how users can interact with a controller.](https://www.twitch.tv/blegas78/clip/SmellyDepressedPancakeKappaPride-llz6ldXSKjSJLs9s)
![Twitch Clip](https://github.com/polysyl/chaos/blob/main/docs/images/explain.png?raw=true)

The original version of this project was written by [Blegas78](https://www.twitch.tv/blegas78). It
was inspired after he watched Twitch user [DarkViperAU](https://www.twitch.tv/DarkViperAU) stream
"Twich Controls Chaos" using [ChaosModV](https://github.com/gta-chaos-mod/ChaosModV).

Unlike the ChaosModV version, this implementation directly intercepts controller inputs, so it does
not rely on integration with game mods. It can therefore be run even on non-modded console games.
It does, however, require hardware capable of acting as a USB host and device, such as a Raspberry
Pi 4.

Blegas78's original version was hard-coded for The Last Of Us Part 2. That game is well suited for
controller-based Chaos because of the many accessibility options, gameplay modifiers, and render
modes, but the low-level programming made it non-trivial to extend to other games.

This version of Chaos (Chaos Unbound) is a major rewrite of the code by
[Polysyl](https://www.twitch.tv/polysyl) to add a number of enhancements, the most important
being that the Chaos engine is now a general-purpose gameplay modification system that uses a text
file to specify gameplay modifiers. You can now bring Chaos to most PlayStation games simply by
creating an appropriate configuration file. Knowledge of C++ or Python is not required.

*Important*: Chaos Unbound is currently in alpha status: The engine compiles and runs, but don't
expect it to be functional just yet. If you want to play Chaos% now, you should, for the time being,
continue to use Blegas's original version.

## Examples of Chaos

"[I liked watching your stream, those mods you have are awesome!](https://clips.twitch.tv/AggressiveSuspiciousWalrusFutureMan-RZRm3iwICUuD8rgY)" - Shannon Woodward aka [ShannonIsLive](https://www.twitch.tv/shannonislive), Voice Actress for Dina in TLOU2

This is a collection of clips from streamers who have used chaos on their streams

[blegas78](https://www.twitch.tv/blegas78)
- [Use Items Works Perfectly](https://www.twitch.tv/blegas78/clip/BeautifulNiceDurianGivePLZ-freRXy_EDPVJ1KcB)

Blegas78's [YouTube channel](https://www.youtube.com/channel/UCDJVtdkaxW1GHextweQuPug) also has videos
showing extended gameplay with Chaos.

[inabox44](https://www.twitch.tv/inabox44)
- [Helium Audio Dino Climb](https://clips.twitch.tv/ConcernedInventiveTortoiseDansGame-vaLowhLl-MLk4Qk8)

[RachyMonster](https://www.twitch.tv/rachymonster)
- [Snake Ellie](https://www.twitch.tv/videos/1066095946) - TLOU1 Run Glitch breaks prone animations

[Polysyl](https://www.twitch.tv/polysyl)
- [Nice stroll through the horde](https://www.twitch.tv/polysyl/clip/WanderingCreativeKleeBleedPurple-j8P4Yij2IZupQNOD) - Run/dodge disabled


## How it Works

Twitch Controls Chaos inserts a Raspberry Pi 4 (a small but capable computer) between your
controller and the console. Two programs are running on the Pi:

- The chaos engine, written in C++, which does the actual work of intercepting signals from the
controller, altering them or creating new signals, and passing them on to the console.

- The chaos interface, a chatbot written in Python, which monitors the streamer's twitch chat,
conducts the votes on which mods to apply, and tells the engine what the winning mods are.

- Modifiers and other information necessary to control a specific game are defined in configuration
files. Each game has its own file, and you can switch games by loading a new configuration file.

When Chaos is active, the chatbot selects 3 mods at random from the list of available mods and
holds a vote. Anyone in chat can vote for their preferred mod by typing 1, 2, or 3. The winning
mod is applied and the voting process repeats with a new set of candidate mods. By default, each
mod is active for 3 minutes, and 3 mods are active at any one time, so a new mod will be applied
every minute.

By default, the winning mod is selected *proportionally*. That is, the chance that a particular mod
wins the vote is equal to the proportion of votes that it received. So a mod that receives 25% of
the votes has a 25% chance of winning. The streamer can also configure voting to be majority rule.
In this case, the mod with the most votes always wins and any ties are decided by random selection.

## Limitations

Chaos only supports DualShock 4 controllers. You can still play on a PS 5 console, but Chaos is only
workable with games that support that controller. A PS5-only game such as Returnal will not work,
since that game requires features found only on the unsupported DualSense controller.

Chaos only sees the incoming pattern of controller signals. It has no idea what is actually happening
in the game. During cutscenes, death animations, or other places where the ordinary controls are
not available, the engine will not be able to apply new mods. For this reason, it's important to
remember to hit "pause" when such events occur. If you forget and a vote ends during this period,
some categories of mods, especially those that require setting menu options, the game may wind up
in an inconsistent state. Old mods may not be removed correctly, and new ones may not be applied. In
this case, you may need to pause the game and manually set the necessary options.

## What's New In Chaos Unbound

The following features have been implemented with this version of Chaos

#### Game-Independent Chaos Engine

The Chaos engine is no longer hard-coded specifically to TLOU2. Modifiers and other aspects of
gameplay are now defined in a configuration file, which the engine reads when it initializes.
Altering this configuration file easily allows you to add new mods, or even to create support for
an entirely new game. The
[configuration file for TLOU2](https://github.com/polysyllabic/chaos/blob/unbound/chaos/examples/tlou2.toml)
contains extensive comments that document the available options, and can serve as a template for other games.

#### Semantic Mapping of Controls

In earlier versions of Chaos, a modifier that advertised itself, for example, as "no melee" actually
functioned by blocking the square button. Other mods could also scramble the controls, for example
swapping the square and triangle buttons. If those two mods were active simultaneously, this could
result in melee becoming active again (since it's mapped to a different control), depending on the
order in which the maps were applied.

In the current version, modifiers operate on game commands rather than low-level controls, and any
remapping of signals occurs before the modifiers work their magic. This has the effect that any
alterations to a particular command will continue to operate regardless of what control that command
is currently mapped to.

#### Modifier Redemption

Streamers can now choose to allow chat to acquire "redemption" credits that entitle the person with
that credit to apply a modifier of their choosing. The modifier is applied outside of the normal
voting and occurs immediately (or after a cooldown period that you can be configured). This allows
a member of chat to apply the most chaotic mod at a strategic time, rather than being subjected to
the vagaries of the voting process.

Credits can be acquired by any of the following means, which the streamer can enable and configure
individually:

- Channel points redemption
- Bits donation
- Raffle
- Gifting from those who have credits already

#### New Modifiers

The switch to a configuration-file initialization for modifiers means that it is easy to create many
types of new modifiers with only a few settings. The TLOU2 configuration file includes the following
new modifiers (in addition to all those available in the older version of Chaos):

- No Aim Movement: Cannot move while aiming
- No Aiming Camera: Cannot move the camera while aiming
- Snapshot: You have 1 second to aim before it's canceled
- Criss-Cross Joysticks: Vertical and horizontal axes of both joysticks are swapped

#### Voting Methods

You can configure whether you want the winning mod to be selected proportionally or by majority vote.



## Required Hardware

In order to stream with Chaos, you will need a PlayStation 4 or 5 console. Most of the instructions
here assume that you also use streaming software such as OBS, which will require you to have a
capture card as well. Note that is *is* possible to run Chaos without OBS, meaning that it should be
possible to play Chaos while streaming directly from your PlayStation, or even without streaming at
all, although you will not be able to use overlays to show the current status of votes or the
modifiers in effect.

What else you need:

- A DualShock 4 model CUH-ZCT2U controller 
- A Raspberry Pi 4
- A 32GB microSD card (This is your storage device for for the Pi)
- A USB card reader for microSD cards
- A micro USB A to micro USB B cable (the cable type that comes with the PS 4 DualShock)
- A USB C to USB A cable (the cable type that comes with the PS 5 DualSense)
- (recommended) A cat 5 ethernet cable
- (optional) A case for the Raspberry Pi.

Here is an [Amazon shopping list](https://a.co/fp7VGcb) with items that are used to install and run
chaos.

Important: The only controller supported is the Dualshock 4 Generation 2, model CUH-ZCT2U. This version
has the lightbar visible from the front at the top of the touchpad. This controller can be used even if
you're playing on a PlayStation 5.

## Installation

Currently this setup is only supported on a Raspberry Pi 4 with 32-bit Raspberry Pi OS, though other
setups may work.

Also, the engine and chatbot both expect the "chaos" directory to be in your
home directory, so . If you're using so make
sure to check out this project into your default account home directory.  This may change later
depending on project interest.

#### Configuring the Pi

1. Flash Raspbian OS Lite (32-Bit) to your SD card. 

- This software tool is convenient to flash SD cards: [Raspberry Pi Imager](https://www.raspberrypi.org/software/)
- - Connect your SD card to your computer using an SD card reader
- - Select the SD card in the Raspberry Pi Imager
- - Under "Choose OS" select Raspberry Pi OS (other) -> Raspberry Pi OS Lite (32-bit)
- - Click on the "Write" button.  If writing fails, simply try it again.

2. Install the SD card into your Pi. The slot for this is on the bottom side of the board.

3. Connect a monitor and keyboard. This is only required during the setup process, or later if you need
to update anything and have not set up an SSH shell. After the setup is complete, you can run Chaos without
either a keyboard or monitor connected to the Pi.

4. Apply power over the Pi's USB-C connector by plugging the cable into the PlayStation and turning it on.

5. (*Highly recommended*) Connect the Raspberry Pi to your router with an ethernet cable. If you cannot use
wired ethernet, or prefer not to, you will need to set up WiFi below.

6. When your Pi boots, log in using the following credentials:
- Username: pi
- Password: raspberry

This will start a bash terminal session.

>Note: The password field will look like nothing is being typed, but it will be reading the password as you type it.

7. (*If using WiFi only*). Configure your WiFi connection using the
[WiFi instructions here](https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md).

8. Update your OS and system tools, and install the version-control system git, which is used to
manage all the files you will need to run TCC. After you have logged in to the Pi for the first time,
run the the following command:

```bash
sudo apt update && sudo apt install git -y
```

Updating may take a while. You might want to get a coffee while you wait.

9. Next, get the latest version of Chaos and install it. Run the following commands to get the
necessary files and install them:

```bash
cd
git clone https://github.com/polysyl/chaos.git
cd chaos
./install.sh
```

TODO: Add alternate instructions for installation variants

Running the install command above will take ~5 minutes. During the first part of this process, you
may not see any progress messages for a while. This silent period occurs while git is downloading
all the necessary files. Once the program starts compiling, you should see regular progress
notifications.

6. (Optional) If you want to access the Pi without needing to plug in a keyboard and monitor,
you can set up SSH access. SSH is disabled by default. 
[Follow the instructions here](https://www.raspberrypi.com/documentation/computers/remote-access.html#setting-up-an-ssh-server).

>Note that the SSH shell is not required for ordinary operation of Chaos. Both the engine and the
chatbot start automaticlly when the power is applied.

7. A reboot is required to enable USB communication to hosts:

```bash
sudo reboot
```

Chaos should now be installed!  Now all that is needed is configuration and setup for OBS and your
particular game.


#### Configuring the Console

Your console will need to be set up to prefer USB communication rather than bluetooth for your
controllers. Open the system menu on your console.

- On a PS5, navigate to Settings -> Accessories -> Controllers -> Communication Method -> Use USB Cable 
- On a PS4, Navigate to Settings -> Devices -> Controllers -> Communication Method -> Use USB Cable


#### Configuring the Chatbot

Now that everything is installed, power up your Raspberry Pi and follow these steps:

1. If you have not already created a bot account, do so here
TODO: bot account instructions

If you prefer not to create a secondary account for your bot, you can still run most features
of Chaos by entering the information for the account you broadcast from. In this case, however,
mod redemptions with channel points will not be available.

2. On the same local network as the Raspberry Pi, open a browser and navigate to to
[raspberrypi.local](http://raspberrypi.local).

If the address raspberripi.local does not work, you may need to find your Pi's IP address
and use it directly. You can discover this address by going to your router's admin page, finding
the list of connected devices, and see if you can identify which one is the Pi. Alternatively, run
the command `hostname -I` on your Pi to get your Pi's IP address. Input this address in your
browser instead.

3. Click on the "Settings" tab. Enter your channel name and bot account credentials, or the
credentials for your broadcaster account if you do not have a secondary custom bot account.

4. Configure the overlay appearance to your liking. You can adjust the colors of the progress bars,
the font, font size, and font colors to your taste.

#### Configuring OBS/SLOBS

The Chaos interface generates three browser sources that you can add as overlays to show the current
status of Chaos to your viewers:

- Active Mods: Shows the mods currently in effect with progress bars indicating how much time remains for each mod.
- Votes: Shows the mods currently available to be voted on, along with the number of votes each mod has currently received
- Vote Timer: A progress bar showing the time left for the current voting cycle.

To add these overlays to OBS or SLOBS, perform the following steps:

1. Make a copy of the scene you normally use to stream PlayStation games. Name it something like
"Twitch Controls Chaos".

2. To this new scene, add each of the following as a browser source. The default URLs are as follows.
- Active Mods: http://raspberrypi.local/ActiveMods/
- Votes: http://raspberrypi.local/Votes/
- Vote Timer: http://raspberrypi.local/VoteTimer/

It's recommended to set these browser sources to refresh when not displayed so that they can
easily be refreshed. Detailed setup for OBS instruction are available in the
[stream setup](docs/streamSetup.md) guide.


## Running the Game

#### Initial Startup

1. Each time you want to start Chaos, you must connect your controller through a USB cable to the
lower left USB port on your Raspberry Pi. This is the USB 2.0 port furthest from the ethernet port
and closest to circuit board. Then use a USB-C to USB-A cable to connect the Raspberry Pi's power
to your console. Also connect the Raspberry Pi's ethernet port to your router, unless you're using
WiFi.

![Cabling the Raspbery Pi](https://github.com/polysyl/chaos/blob/main/docs/images/chaos_plugging.png?raw=true)

2. Turn on your console.  On a PlayStation, pressing the PS button will turn on the console, which
will then power the Raspberry Pi. During the Pi's boot process, the controller's bluetooth connection
will let you navigate to your game. At some point, you'll see a USB connectivity notification, meaning
that chaos is active.

3. If OBS was already running, refresh your browser sources. The overlays should be active.

4. Next, check that the controller is connected properly. Start up and load into your game so that
you are controlling your character. After you press the "Share" button, the VoteTimer progress bar
should start moving. If the timer runs and you can control your character, then the engine is
running correctly and communicating with the interface.

5. Finally, test the voting. Pull up your chat and try to vote while the engine is running. A vote
should appear on your overlay. If all these tests are OK, you are ready to go!

#### Playing the Game

1. The [main interface page](http://raspberrypi.local) will contain several tabs that will let you
alter specific settings. The default tab is designed to show you the current status of Chaos without
revealing information about which mods are winning the voting. This way (assuming you're not looking
at the OBS overlays), what chat inflicts on you should come as a surprise.

#### Pausing
Chaos initializes in a paused state. When Chaos is paused, modifiers do not run and all signals are
passed through to the console unaltered. The vote timer is also suspended, but any votes cast during
the pause *will* be counted. Pausing chaos will let you navigate things like your console's system
menu without wreaking true havoc.

  - To resume Chaos, press the *Share* button.  
  - To pause Chaos, press the *Option* button.

>Note: This means that you will actively need to resume Chaos whenever you enter your game's pause menu.

#### Game-Specific Usage

Currently TLOU2 is the only supported game.  See TLOU2 specific instructions here:

[TLOU2 Usage](docs/TLOU2/README.md)


## Known Issues

#### No USB Hot-Plugging
If the controller becomes disconnected, the chaos service must be restarted or a reboot must be performed.  This is certainly annoying and can be fixed in code with an overhaul to usb-sniffify to allow hot-plugging.

#### USB crashing or delaying inputs over time on PS5
This can usually only currently be corrected with a reboot of the Pi.  dmesg will show many errors and they are fine, but eventually the kernel will prevent raw-gadget from running.  This may be a limitation of the raw-gadget Linux kernel module.

## TODO List

#### Video feedback
Implementing video feedback in a similar form to video-based load removers can aid in providing game-state information so that Chaos can know when it can/cannot perform certain actions.  This would be a major overhaul and add complication to the game setup, so this is certainly a long-term stretch goal.


## Design

The core of this Chaos implementation involves the Chaos engine, written in C++ for speed. At the lowest level, the Chaos engine works by forwarding USB protocols using the Linux raw-gadget kernel module. For every USB request, the engine duplicates the request and passes it along. However, in the case of messages corresponding to controller buttons/joysticks, the data is passed to other processes that can meddle with the data. This forwarding infrastructure is done by using [usb-sniffify](https://github.com/blegas78/usb-sniffify), a library that combines [raw-gadget](https://github.com/xairy/raw-gadget) and [libusb](https://libusb.info).

The Twitch chatbot, stream overlays, and vote tracking are built using Python.
- The chatbot is built  GUI and overlays are built using [PySimpleGui](https://github.com/PySimpleGUI/PySimpleGUI).  The IRC client code is unnecessarily custom (a library should be used).  

Chaos uses [softmax](https://en.wikipedia.org/wiki/Softmax_function) to select modifiers for the voting pool.  This is implemented to reduce the probability that modifiers appear based on the number of times it has won.  This reduces the likelihood that modifiers repeat to create more diversity in each moment however it is still possible to see repeat modifiers.  Some viewers like [Chaos Coach Cholocco](https://www.twitch.tv/cholocco) will inform users to only vote on buff/visual modifiers during slow segments so that painful modifiers are more likely to appear during combat sections.

The ChaosEngine listens to the Python voting system using ZMQ with [zmqpp](https://github.com/zeromq/zmqpp).  When a winning modifier comes in, ChaosEngine adds the modifier to list of active modifiers.  After a set amount of time, the ChaosEngine will remove the modifier.

Each modifier can perform a set of actions at each of these transitions.  There can be unique actions performed as the modifier is added, while it's currently active, when it ends, and can also perform asynchronous actions that are controller event-based.  This effectively follows UML-style state machine with entry/do/exit/event actions.

ChaosEngine is designed to allow for flexibility in modifiers using the following principles:

- Sniffing - Can read input being sent along the way
- Interception - Can block pipelined commands
- Modification - Replace message data including the type, id, and value
- Injection - Can generate and send arbitrary messages without any controller events
- State Sampling - See what is being applied versus what is trying to be sent
- Direct Control - Can send commands directly to the output

Some other included helper utilities for modifiers include sequencers.  With the above list and utilities, Chaos is capable of behaviors including button macros and remmaping.  Such a framework can potentially be used as a TAS device, for "Twitch Plays ..." streams, or for modifying/boosting controller performance  (could be considered cheating in multiplayer games).

#### Example Modifiers

Example modifier for applying inverted mode (Event Modification):

## Frequently Asked Questions

*Why can't I use a first-generation DualShock controller?*

- The first-generation DualShock 4 controller can only send control inputs over bluetooth, making it unusable for this project.

*I have a PlayStation 5. Why can't I use that controller?*
- The DualSense controller for the PS 5 sends signals to the console with an encrypted checksum, and the private key for that remains, well, private. This meaning that any attempts to alter the signals would result in a signal with a mismatched checksum, since we don't know how to spoof a valid one, and the signal will be rejected by the console. It should be theoretically possible to use this controller by using the chaos engine to emulate a DualShock controller, but this would require significant additional work. 

*Do I have to use a Rapberry Pi 4?*

Currently the Raspberry Pi 4 is the only device tested. Some other Rasberry Pi models *may* work
(see below), but a regular computer running Linux will not.

The communication to the PlayStation requires the Pi to act as a USB client instead of a USB host.
That requires special hardware peripherals which the Pi 4 has on its USB-C power port. Many older
Pi models--and most, if not all, ordinary PCs--lack this hardware and so cannot run the chaos
engine.

There are other Pi variants that could work, but they are untested. The Raspberry Pi 0 W has the
right hardware and is much less expensive than the Pi 4, but this has only one USB device to act as
a client and lacks an ethernet plug, meaning that WiFi is the only method to connect to the network
This means that the controller has to connect over Bluetooth, using the same device that's also used
for its network connection. There may be performance issues going that route, including controller
lag and disconnections, as well as chat/OBS overlay issues. Some of these performance issues may
be reduced by running the chatbot on a different computer, to lighten the Pi's workload.

## Contributors

The original version of controller-based Chaos is entirely due to the amazing work of
[Blegas78](https://www.twitch.tv/blegas78).

Many other people in the community have contributed ideas. Chaos would not be nearly as colorful or
effective without their contributions, so thanks to everyone that has made this project better!

#### General
 - Aeathone [Twitch](https://www.twitch.tv/aeathone)
 - - Informs me of my terrible curly braces <3
 - - Suggested great design improvements to handling things like menuing
 - EspritDeCorpse [Github](https://github.com/EspritDeCorpse) [Twitch](https://www.twitch.tv/espritdecorpse)
  - - Reviewer
  - PrincessDiodes [Github](https://github.com/ash3rz) [Twitch](https://www.twitch.tv/princessdiodes)
  - - Server-side gamepad viewer implementation
  - RachyMonster [Twitch](https://www.twitch.tv/rachymonster)
  - - Figured out how to make readable and spoiler free chat in the [Stream Setup](docs/streamSetup.md) guide
 - ners_14 [Twitch](https://www.twitch.tv/ners_14)
  - - Informed the idea of proportional voting from GTAV chaos

#### Modifier Ideas
 - [KillstreekDaGeek](https://www.twitch.tv/killstreekdageek) AKA Prototoxin187
 - - Stay Scoped
 - - Strafe Only
 - - Moonwalk
 - - Force Aim
 - - Mystery
 - - Min Sensitivity
 - - Button remap randomizer
 - [joshuatimes7](https://www.twitch.tv/joshuatimes7), [JustForSaft](https://www.twitch.tv/JustForSaft)
 - - Swap Dpad/Left Joystick
 - [JustForSaft](https://www.twitch.tv/JustForSaft)
 - - Input Delay
 - - Chaos Mod
 - [HeHathYought](https://www.twitch.tv/HeHathYought)
 - - Anthony Caliber
 - - TLOU1 Run Glitch
 - [RachyMonster](https://www.twitch.tv/rachymonster)
 - - Moose
 - - Use Equipped Items
 - [PrincessDiodes](https://www.twitch.tv/princessdiodes),  [DJ_Squall_808](https://www.twitch.tv/DJ_Squall_808), [cloverfieldmel](https://www.twitch.tv/cloverfieldmel:)
 - - Controller Mirror
 - [hipsterobot](https://www.twitch.tv/hipsterobot)
 - - Controller Flip
 - [gabemusic](https://www.twitch.tv/gabemusic)
- - Max Sensitivity
 - [carnalgasyeah](https://www.twitch.tv/carnalgasyeah)
- - Zoolander
- [inabox44](https://www.twitch.tv/inabox44)
- - Mute certain game volumes
- [crescenterra](https://www.twitch.tv/crescenterra)
- - 30fps

