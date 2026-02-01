# Twitch Controls Chaos (Chaos Unbound Edition)

![Chaos](https://github.com/polysyllabic/chaos/blob/main/docs/images/chaos.jpg?raw=true)

## What is Chaos?
Twitch Controls Chaos, TCC or Chaos for short, lets Twitch chat interfere with a streamer's
game-play. It uses a Raspberry Pi to modify controller
input based on a series of gameplay modifiers that chat can select by voting for their favorites.

[Watch a minute-long video explanation of how users can interact with a controller.](https://www.twitch.tv/blegas78/clip/SmellyDepressedPancakeKappaPride-llz6ldXSKjSJLs9s)
![Twitch Clip](https://github.com/polysyllabic/chaos/blob/main/docs/images/explain.png?raw=true)

The original version of this project was written by [Blegas78](https://www.twitch.tv/blegas78). He
was inspired by watching Twitch user [DarkViperAU](https://www.twitch.tv/DarkViperAU) stream
"Twich Controls Chaos" using [ChaosModV](https://github.com/gta-chaos-mod/ChaosModV).

Unlike the ChaosModV version, this implementation directly intercepts controller inputs, so it does
not rely on integration with game mods. It can therefore run with non-modded console games. It does,
however, require hardware capable of acting as a USB host and device, such as a Raspberry Pi 4.

Blegas78's original version was hard-coded for *The Last Of Us Part 2*. That game is well suited
for controller-based Chaos because of the many accessibility options, gameplay modifiers, and render
modes, but the low-level programming made it non-trivial to extend to other games.

This version of Chaos (Chaos Unbound) is a major rewrite of the code by
[Polysyl](https://www.twitch.tv/polysyl) to add a number of enhancements, the most important
being that the Chaos engine is now a general-purpose gameplay modification system that uses a text
file to specify gameplay modifiers. You can now bring Chaos to PlayStation games (or PC games that
are controller-based) simply by creating an appropriate configuration file. Knowledge of C++ or
Python is not required.

When the original version was written (and when work began on Chaos Unbound), TLOU2 was a
PlayStation exclusive, and so the design of the system was specifically intended to work with
that console. However, the same setup should work fine for PC games that can be played with a
controller. Mouse-and-keyboard remapping is not (yet) supported.

Chaos Unbound is currently in alpha status: The engine compiles and runs (probably) but don't
expect it to be functional just yet. If you want to play Chaos% now, you should, for the time
being, continue to use Blegas's original version.

Note also that if you attempt to use this system in a competitive multiplayer game, it could
be seen as cheating, since the fundamental processes here (intercept and modify the signals
coming from the controller) are parallel to those that cheaters use to do things such as
eliminate reoil or patch in an aim bot. Of course cheating mods make the game easier to play,
whereas TCC makes the game substantially more difficult (and amusing for your audience), but
given that _any_ modification runs against the spirit of fair play in a multiplayer context,
I wouldn't recommend that you try it with such games.

## Examples of Chaos

"[I liked watching your stream, those mods you have are awesome!](https://clips.twitch.tv/AggressiveSuspiciousWalrusFutureMan-RZRm3iwICUuD8rgY)" - Shannon Woodward aka [ShannonIsLive](https://www.twitch.tv/shannonislive), Voice Actress for Dina in TLOU2

This is a collection of clips from streamers who have used chaos on their streams:

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

Twitch Controls Chaos inserts a Raspberry Pi (a small computer) between your controller and
console. The Raspberry Pi intercepts signals coming from the controller and selectively passes them
on to the console. It can change the signals, block them, and even insert completely new commands
that weren't sent from the controller.

The TCC software consists of two programs. By default, they both run on the Raspberry Pi:

- The Chaos engine, written in C++, does the actual work of intercepting signals from the
controller, altering them or creating new signals, and passing them on to the console.

- The Chaos interface, a chatbot written in Python, monitors the streamer's twitch chat,
conducts the votes on which mods to apply, and tells the engine what the winning mods are.
The chatbot can be run on a separate computer, if you like.

Modifiers and other information necessary to control a specific game are defined in configuration
files. Each game has its own file, and you can switch games by loading a new configuration file.

When Chaos is active, the chatbot selects modifiers at random from the list of available mods and
holds a vote. Anyone in chat can vote for their preferred mod by typing the number of their
preferred candidate mode in chat. The winning modifier is applied automatically, and the voting
process repeats with a new set of candidate modifiers. By default, each modifier is active for 3
minutes, and 3 mods are active at any one time, so a new vote is held every minute.

By default, the winning mod is selected *proportionally*. That is, the chance that a particular
mod wins the vote is equal to the proportion of votes that it received. So a mod that receives 25%
of the votes has a 25% chance of winning. The streamer can also configure voting to be majority
rule. In this case, the mod with the most votes always wins and any ties are decided by random
selection. Voting can even be disabled altogether, so that modifiers can only be applied manually.
This last system may be useful if you want to apply some specific modifier as the result of
events that the chatbot cannot track, such as reaching some custom channel goel.

## Limitations

TCC requires specific hardware to run, including a Raspberry Pi 4 and a DualShock controller (the
controller for the PlayStation 4), which may constitue a significant expense. To play PS5-only
games on the PlayStation, you will require still more hardware.

Chaos only sees the incoming pattern of controller signals. It has no idea what is actually
happening in the game. During cutscenes, death animations, or other places where the ordinary
controls are not available, the engine will not be able to apply new mods. For this reason, it's
important to remember to hit pause when such events occur. If you forget and a vote ends during
this period, some categories of mods, including all those that require setting menu options, may
put the game in an inconsistent state. Old mods may not be removed correctly, and new ones may not
be applied. In this case, you may need to pause the game and manually set the necessary options.

## What's New In Chaos Unbound

This version of Chaos is called "Chaos Unbound" because the system is no longer hard-coded to a
specific game. Modifiers and other aspects of gameplay are now defined in a configuration file,
which the engine reads when it initializes. Altering this configuration file allows you to add new
modifiers easily, or even create support for an entirely new game, without needing to write any C++
or Python code. The rules for creating  
[configuration files](https://github.com/polysyllabic/chaos/blob/unbound/chaos/examples/tlou2.toml)
can be found in the [documentation files](https://github.com/polysyllabic/chaos/blob/unbound/chaos/docs/chaosConfigFiles.md).

In the previous version of Chaos, a modifier that advertised itself, for example, as "no melee"
actually functioned by blocking the square button. One drawback of this implementation is that
ther mods can also scramble the controls, for example by swapping the square and triangle buttons.
If those two mods were active simultaneously, this could result in melee becoming active again,
depending on the order in which the modifiers were applied.

In Chaos Unbound, modifiers are defined based on game commands, and any remapping of controller
inputs is performed before the other modifiers are applied. This has the effect that any modifiers
that specify alterations to a particular command will continue to operate regardless of what
control that command is currently mapped to.

Streamers can now choose to allow chat to acquire "redemption" credits that entitle the person with
that credit to apply a modifier of their choosing. The modifier is applied outside of the normal
voting and occurs immediately (or after a cooldown period that can be configured). This allows
a member of chat to apply the most chaotic modifier at a strategic time, rather than being
subjected to the vagaries of the voting process.

Credits can be acquired by any of the following means, which the streamer can enable and configure
individually:

- Channel points redemption
- Bits donation
- Raffle
- Gifting from those who have credits already

The switch to a configuration-file initialization for modifiers means that it is easy to create many
types of new modifiers with only a few settings. Some examples of new modifiers included in the
new TLOU2 configuration file include the following:

- Sir Robin: Forced backwards running
- No Aim Movement: Cannot move while aiming
- No Aiming Camera: Cannot move the camera while aiming
- Snapshot: You have 1 second to aim before it's canceled
- Sniper Speed: Wait 5 seconds between each shot
- Criss-Cross Joysticks: Vertical and horizontal axes of both joysticks are swapped
- Drift Left: Add controller drift

## Required Hardware

Apart from a PlayStation (or PC if you're playing a PC game), you will need the following
hardware (for all steups):

- A DualShock 4 model CUH-ZCT2U controller 
- A Raspberry Pi 4
- A 32GB microSD card (This is your storage device for for the Pi)
- A USB card reader for microSD cards
- A micro USB A to micro USB B cable (the cable type that comes with the PS 4 DualShock)
- A USB C to USB A cable (the cable type that comes with the PS 5 DualSense)
- (recommended) A cat 5 ethernet cable
- (optional) A case for the Raspberry Pi.

To play PS5-only games such as TLOU1 Remake or TLOU2 Remaster, you will also need hardware to let
you use a non-PS5 controller without having the console reject your commands. I successfully
used a [Besavior PS5 controller](https://www.beloader.com/products/besavior-1.html).

Most of the instructions here also assume that you also use streaming software such as OBS, which will
require you to have a capture card as well. Note that is **is** possible to run Chaos without OBS,
meaning that it should be possible to play Chaos while streaming directly from your PlayStation,
or even without streaming at all, although you will not be able to use overlays to show the current
status of votes or the modifiers in effect. (This has not been tested, but suggestions for what
to try will be found below.)

**Important:** You must use the DualShock 4 Generation 2, model CUH-ZCT2U. This version has the
lightbar visible from the front at the top of the touchpad. Gen-1 DualShocks connect to the console
only through wifi, and a wired USB connection is needed to intercept the signals. Note also that you
need this controller *in addition* to the Besavior to play PS5-only games.

*Note:* You may be able to adapt TCC to run on other setups, particularly using different models
of the Raspberry Pi or alternatives to the Besavior, but these are untested. See the notes below
for specific details explaining the requirements if you want to try something different.

## Installation

Currently this setup is only supported on a Raspberry Pi 4 with 32-bit Raspberry Pi OS, though other
setups may work.

### Configuring the Pi

1. Flash Raspbian OS Lite (32-Bit) to your SD card. 

This software tool is convenient to flash SD cards: [Raspberry Pi Imager](https://www.raspberrypi.org/software/)

- Connect your SD card to your computer using an SD card reader
- Select the SD card in the Raspberry Pi Imager
- Under "Choose OS" select Raspberry Pi OS (other) -> Raspberry Pi OS Lite (32-bit). This is the version without
  a desktop environment (which you don't need).
- In advanced options:

    - Set image customization options to "to always use":
    - Check "Set hostname" and accept raspberrypi.local. (If you need this to be something else,
      it's OK to change this. Just substitute the hostname you choose for 'raspberrypi.local'
      wherever that occurs in the instructions.)
    - Check Set username and password. The original version of Chaos was hard-coded to assume the
      username was pi. Chaos unbound removes this restriction, but these instructions use the
      default username as an example.
    - If you want to be able to access your Pi without plugging in a keyboard and monitor, check
      "Enable SSH" and choose your method of authentication. If you don't know how to set up
      public-key authentication, you can stick with a password, but for security you should
      change the password from the default. If you need more help setting up SSH,
      [follow the instructions here](https://www.raspberrypi.com/documentation/computers/remote-access.html#setting-up-an-ssh-server).
      Note that the SSH shell is not required for ordinary operation of Chaos. Both the engine and
      the chatbot start automaticlly when the power is applied.
    - If you cannot plug your Pi into your network with an ethernet cable, enable WAN. Note:
      this may have a performance impact. Ethernet cable is recommended whenever possible.

- Click on the "Write" button.  If writing fails, simply try it again.

2. Install the SD card into your Pi. The slot for this is on the bottom side of the board.

3. IF you did not configure SSH, connect a monitor and keyboard. This is only required during the
setup process, or later if you need to update anything and have not set up an SSH shell. After the
setup is complete, you can run Chaos without either a keyboard or monitor connected to the Pi.

4. (*Highly recommended*) Connect the Raspberry Pi to your router with an ethernet cable. If you
cannot use wired ethernet, or prefer not to, you will need to set up WiFi below.

5. Apply power over the Pi's USB-C connector by plugging the cable into the PlayStation and turning
it on. (You can use an external USB-C power supply as well, but it should supply at least 3A.)

6. When your Pi boots, log in using the credentials you specified above. If you stuck with the
system defaults, they will be the following:
- Username: pi
- Password: raspberry

This will start a bash terminal session. If you are connecting over SSH, try rapberypi.local as
the host address. If that does not work, you may need to find the local IP address that your
router has assigned to the Pi. Go to your router's admin page and look for the list of connected
devices. 

Note: The password field will look like nothing is being typed, but it will be reading the password
as you type it.

7. Update your OS and system tools, and install the version-control system *git*, which is used to
manage all the files you will need to run TCC. After you have logged in to the Pi for the first
time, run the the following command:

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install git -y
```

This will ensure that you are running the latest stable versions of all the necessary tools. The
process may take a while. Be patient.

9. Next, get TCC from the online repository. The following command will fetch the latest version
and copy all the files you need to build TCC into the directory 'chaos':

```bash
git clone https://github.com/polysyllabic/chaos.git
```

10. To build TCC and install it, run the installation script with these commands:

```bash
cd chaos
./install.sh
```

The script will ask you a number of questions. The most important one is whether you plan to
run the chatbot/UI chaosface from the Raspberry Pi or from a different computer. If you run
it from the Pi, the install process will set it up to run automatically each time you apply
power to the Pi. If you choose to run it from a different computer, it will ask you what
that computer's IP address is. If you don't know this at the moment, you can enter a random
value and change the setting in the chaosconfig.toml file later. 

You will also be asked if you want to develop TCC on this device. Answering yes will install
additional packages to help developers (including doxygen), but unless you're planning to
do development work, this just increases the time it takes to install everything.

If you have installed a recent version of the OS (buster or later), a reboot will be required
halfway through the installation process. You will be prompted to re-run the install script
after you have rebooted. The installation will resume where it left off.

At the end of the installation, you will have to reboot one more time. If you choose not to
reboot when prompted, enter the following command when you are ready to reboot.

```bash
sudo reboot
```

After this final reboot, chaos should be installed and will run automatically each time you turn
on the Raspberry Pi.

### Configuring the Console

Your console will need to be set up to prefer USB communication rather than bluetooth for your
controllers. Open the system menu on your console.

- On a PS5, navigate to Settings -> Accessories -> Controllers -> Communication Method -> Use USB Cable 
- On a PS4, Navigate to Settings -> Devices -> Controllers -> Communication Method -> Use USB Cable

### Configuring the Chatbot
*Note:* If you are installing chaosface on a separate computer, see the installation instructions
in the [chaosface documentation](docs/chaosface.md). You should install and run chaosface from
that computer before performing the next steps.

Now that everything is installed, power up your Raspberry Pi and follow these steps:

1. If you have not already created a bot account, do so here.
TODO: bot account instructions

If you prefer not to create a secondary account for your bot, you can still run most features
of Chaos by entering the information for the account you broadcast from. In this case, however,
mod redemptions with channel points will not be available.

2. On the same local network as the Raspberry Pi, open a browser and navigate to to
[raspberrypi.local](http://raspberrypi.local), assuming you did not change the Pi's hostname.
If you did set the hostname during configuration, replace "raspberrypi" with whatever name
you chose.

If this address does not work, you may need to find your Pi's IP address and use it directly.
You can discover this address by going to your router's admin page, finding the list of
connected devices, and seeing if you can identify which one is the Pi. Alternatively, run
the command `hostname -I` on your Pi to get your Pi's IP address. Input this address in your
browser instead.

3. Click on the "Settings" tab. Enter your channel name and bot account credentials, or the
credentials for your broadcaster account if you do not have a secondary custom bot account.

4. Configure the overlay appearance to your liking.

### Configuring OBS/SLOBS

The Chaos interface generates three browser sources that you can add as overlays to show the current
status of Chaos to your viewers:

- Active Mods: Shows the mods currently in effect with progress bars indicating how much time
remains for each mod.
- Votes: Shows the mods currently available to be voted on, along with the number of votes each mod
has currently received
- Vote Timer: A progress bar showing the time left for the current voting cycle.

To add these overlays to OBS or SLOBS, perform the following steps:

1. Make a copy of the scene you normally use to stream PlayStation games. Name it something like
"Twitch Controls Chaos".

2. To this new scene, add each of the following as a browser source. The default URLs are as follows.
- Active Mods: http://raspberrypi.local/ActiveMods/
- Votes: http://raspberrypi.local/Votes/
- Vote Timer: http://raspberrypi.local/VoteTimer/

It's recommended to set these browser sources to refresh when not displayed so that they can
easily be refreshed. Detailed setup instructions for OBS are available in the
[stream setup](docs/streamSetup.md) guide.

### Running TCC Without Overlays
If you are streaming directly from a console and cannot display local browser-source overlays,
you can still run TCC, but you will need to make sure that you turn on the chatbot settings that
announceme in chat which mods are available to vote on and which mods are currently active
each time a new voting cycle begins.

Note that you do not even need to be streaming for the chatbot to work. Merely by virtue of
having a Twitch account, you have a chat, which can work even when you're offline. So in theory,
you could have a group of friends over to your house and just play the game on your TV while
they joined your twitch chat on their phones and voted without you ever needing to stream
the game.

## Running the Game

### Cabling

The hardware chain differs depending on whether you are playing a PS4 game or a PS5 game.

For all games:

1. Connect the DualShock controller (the PS4 controller) through a USB cable to the
lower left USB port on your Raspberry Pi. This is the USB 2.0 port furthest from the ethernet port
and closest to circuit board.

2. Connect the Raspberry Pi's ethernet port to your router, unless you're using WiFi.

[Cabling the Raspbery Pi](https://github.com/polysyllabic/chaos/blob/unbound/docs/images/chaos_plugging.jpg)

3. The next step depends on whether you are playing a PS4 or PS5 game. If you're playing a PS4 game
on a PS5 console, you can use the simpler, PS4 setup:

For PS4 games:

Use a USB-C to USB-A cable to connect the Raspberry Pi's USB-C port to the console. The
Pi will get its power from this connection, as well as using it to communicate with the console.

For PS5-only games:

3a. Connect the Besavior controller (or equivalent device) to the PlayStation 5 with a USB-C to USB-A cable.

3b. Connect the USB-C to USB-A adaptor that comes with the Besavior to the USB-C port on the Beloaderth
device on the bottom of the Besavior controller. This is a short cable with a second USB-C port on
the side.

3c. Connect the Besavior to the Raspberry Pi's USB-C port with a USB-A to USB-C cable.

3d. Connect another USB-C cable into this side port on the adapter and connect it to an external DC
power source that can supply at least 3A. *Note:* If you use a lower-rated power supply, you may
experience brown-outs when the Pi starts trying to draw more power than is available under load. In
this case, you may find the Pi hanging and rebooting at inconvenient times.

The sequence of devices is DualShock -> Raspberry Pi -> Besavior -> PlayStation 5


### Startup

1. Turn on your console.  If using the Besavior setup, also ensure that the external power is
connected. Once power is applied, the Raspberry Pi will boot up. During the Pi's boot sequence, the
controller's bluetooth connection will let you navigate to your game. At some point, you'll see a USB
connectivity notification, meaning that TCC is active.

2. If OBS was already running, refresh your browser sources. The overlays should be active.

3. Next, check that the controller is connected properly. Start up and load into your game so that
you are controlling your character. After you press the "Share" button, the VoteTimer progress bar
should start moving. If the timer runs and you can control your character, then the engine is
running correctly and communicating with the interface.

4. Finally, test the voting. Pull up your chat and try to vote while the engine is running. A vote
should appear on your overlay. If all these tests are OK, you are ready to go!

### Playing the Game

1. The [main interface page](http://raspberrypi.local) will contain several tabs that will let you
alter specific settings. The default tab is designed to show you the current status of Chaos without
revealing information about which mods are winning the voting. This way (assuming you're not looking
at the OBS overlays), what chat inflicts on you should come as a surprise.

### Pausing

Chaos initializes in a paused state. When Chaos is paused, modifiers do not run and all signals are
passed through to the console unaltered. The vote timer is also suspended, but any votes cast during
the pause *will* be counted. Pausing chaos will let you navigate things like your console's system
menu without wreaking true havoc.

- To resume Chaos, press the *Share* button.  
- To pause Chaos, press the *Option* button.

>Note: This means that you will actively need to resume Chaos whenever you enter your game's pause menu.

### Game-Specific Usage

Currently TLOU2 is the only supported game.  See TLOU2 specific instructions here:

[TLOU2 Usage](docs/TLOU2/README.md)


## Known Issues

### No USB Hot-Plugging
If the controller becomes disconnected, you must restart the chaos service or reboot the Pi. This is
annoying, and needs to be fixed. An overhaul to usb-sniffify to allow hot-plugging is necessary.

### USB crashing or delaying inputs over time on PS5
This can usually only currently be corrected with a reboot of the Pi. A check of dmesg will show
many errors, and they are generally fine, but eventually the kernel will prevent raw-gadget from
running. This may be a limitation of the raw-gadget Linux kernel module.

## Design

The core of TCC involves the Chaos engine, written in C++ for speed. At the lowest level, the Chaos
engine works by forwarding USB protocols using the Linux raw-gadget kernel module. For every USB
request, the engine duplicates the request and passes it along. However, in the case of messages
corresponding to controller buttons/joysticks, the data is passed to other processes that can meddle
with the data. This forwarding infrastructure is done by using
[usb-sniffify](https://github.com/blegas78/usb-sniffify), a library that combines
[raw-gadget](https://github.com/xairy/raw-gadget) and [libusb](https://libusb.info).

The Chatbot, stream overlays, and vote tracking are written in Python. The chatbot's basic
Twitch connectivity is implemented with [PythonTwitchBotFramework](https://github.com/sharkbound/PythonTwitchBotFramework).
The chatbot's GUI and overlays are built using [Flexx](https://github.com/flexxui/flexx).

Chaos uses [softmax](https://en.wikipedia.org/wiki/Softmax_function) to select modifiers for the
voting pool. The effect is that the more times a particular modifier has previously won a vote,
the less likely it is to appear in the voting list. If savvy viewers vote only for buffs or visual
modifiers during a game's slow segments, this makes it more likely that the more painful modifiers
will wind up in the voting list during combat sections.

The Chaos engine listens to the Python voting system using ZMQ with [zmqpp](https://github.com/zeromq/zmqpp).
When a winning modifier comes in, the engine adds the modifier to list of active modifiers. After
a set amount of time, the engine will remove the modifier.

Each modifier can perform a set of actions at each of these transitions. There can be unique actions
performed as the modifier is added, while it's currently active, when it ends, and can also perform
asynchronous actions that are controller event-based. This effectively follows UML-style state machine
with entry/do/exit/event actions.

The Chaos engine offers the following capabilities for reading and altering controller data:

- Sniffing - Read input packets being sent from the controller
- State Sampling - See live controller state (as opposed to what is trying to be sent)
- Interception - Block commands in the pipeline
- Modification - Alter data including the command type, id, and value
- Injection - Generate arbitrary messages without any controller events
- Direct Control - Send commands directly to the output

Chaos is capable of behaviors including creating macros and remmaping controls. The same framework can
be used as a TAS device for "Twitch Plays ..." streams, or for modifying/boosting controller performance.
(This could be considered cheating in multiplayer games.)

### Configuration=Based Game Support
Individual games are supported through configuration files. If you want to use TCC with a new game, you
merely need to create an appropriate configuration file.


## Frequently Asked Questions

*Why can't I use a first-generation DualShock controller?*

The first-generation DualShock 4 controller can only send control inputs over bluetooth, making
it unusable for this project.

*I have a PlayStation 5. Why can't I use that controller?*

The DualSense controller for the PS 5 sends signals to the console with an encrypted checksum, and
the private key for that remains, well, private. Although at least one key was hacked and released,
Sony has patched that out. This meaning that any attempts to alter the signals would result in a
signal with a mismatched checksum, since we don't know how to spoof a valid one, and the signal will
be rejected by the console.

One way to work around this limitation is with the Besavior, which is a custom-built PS5 controller
that supports chaining other controllers into it. This means that, instead of plugging the Raspberry
Pi directly into the PlayStation 5, you plug it into the Besavior and the Besavior into the
PlayStation. The Besavior authenticates itself with the console as an authentic controller and handles
the remapping of DualShock data to the DualSense format. Note that this method requires external
power for the Raspberry Pi. The Besavior comes with an adaptor cable that contains a side-port for this
power, so you should not need to supply that power through the Pi's GPIO pins.

*Does this work with the PC version of TLOU2?*

If you simply plug the DualShock + Pi into your PC instead of a console, it _should_ work with a few
tweaks to the configuration file. (Testing this is high on my to-do list.) If you're asking for a
version that works _without_ a Raspberry Pi to handle the signal conversions, that would require a
major effort. In essence, we'd need to create some sort of driver to intercept not only controller
input but mouse-and-keyboard inputs as well. I might consider attempting this at some point, after
the Pi version is stable, but it's not a priority for me.

*Do I have to use a Rapberry Pi 4?*

Currently the Raspberry Pi 4 is the only device tested. Some other Rasberry Pi models *may* work
(see below), but a regular computer running Linux will not.

The communication to the PlayStation requires the Pi to act as a USB client instead of a USB host.
That requires special hardware peripherals which the Pi 4 has on its USB-C power port. Many older
Pi models--and most, if not all, ordinary PCs--lack this hardware and so cannot run the chaos
engine.

There are other Pi variants that could work, but they are untested. The Raspberry Pi 0W has the
right hardware and is much less expensive than the Pi 4, but this has only one USB device to act as
a client and lacks an ethernet plug, meaning that WiFi is the only method to connect to the network
This means that the controller has to connect over Bluetooth, using the same device that's also used
for its network connection. There may be performance issues going that route, including controller
lag and disconnections, as well as chat/OBS overlay issues. Some of these performance issues may
be reduced by running the chatbot on a different computer, to lighten the Pi's workload.

*Why did Chaos stop working after I updated my Pi?*
TCC uses a hacked version of the raw-gadget kernel module. If you updated the kernel to a newer
version, the old kernel module will now be incompatible with the new one. You can manually
rebuild the module with these commands:

```bash
cd
cd chaos/raw-gadget-timeout/raw_gadget/
make
```

Note that the original version of Chaos, or rather the sniffify library, depends on a specific,
older version of the linux kernel headers that were altered in later releases. To get that
library working, you may need to revert to an older image of the OS, which is
[available here].(https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2021-05-28/)
Once flashed and booted, downloaded the corresponding kernel using this command:

```bash
wget https://archive.raspberrypi.org/debian/pool/main/r/raspberrypi-firmware/raspberrypi-kernel-headers_1.20210430-1_armhf.deb 
```

Install the headers with the following:

```bash
sudo dpkg --install raspberrypi-kernel-headers_1.20210430-1_armhf.deb 
```

After that, you should be able to install git and chaos orginarily. But you should not run "sudo apt-get upgrade".


## Contributors
The original version of controller-based Chaos is entirely due to the amazing work of
[Blegas78](https://www.twitch.tv/blegas78). [Polysyl](https://www.twitch.tv/polysyl) extensively rewrote most
aspects of the code for this version, but without Blegas's efforts, none of this would have been possible.

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

