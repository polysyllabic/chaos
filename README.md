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
that console. However, the same setup should generally work for PC games that can be played with a
controller. Mouse-and-keyboard remapping is not (yet) supported.

_PC Note:_ If you are trying to use this system for a PC game, note that Steam Input can cause
serious issues with lag, especially while starting up the game. This is definitely true with
_The Last of Us: Part II_ and likely true for other games as well. To avoid this problem, disable
Steam Input for the relevant PC games. From your Steam library, right-click on the game, then
select `Properties -> Controller -> Override -> Disable Steam Input`.

Chaos Unbound is currently in beta status: The engine compiles and runs (mostly) but its
functionality is not yet fully tested. Consider using Blegas's original version if you want
to play TCC with TLOU2 right now.

Note also that if you attempt to use this system in a competitive multiplayer game, it could
be seen as cheating, since the fundamental processes here (intercept and modify the signals
coming from the controller) are parallel to those that cheaters use to do things such as
eliminate recoil or patch in an aim bot. Of course cheating mods make the game easier to play,
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
events that the chatbot cannot track, such as reaching some custom channel goal.

## Limitations

TCC requires specific hardware to run, including a Raspberry Pi 4 and a DualShock controller (the
controller for the PlayStation 4), which may constitute a significant expense. To play PS5-only
games on the PlayStation, you will need still more hardware.

Chaos only sees the incoming pattern of controller signals. It has no idea what is actually
happening in the game. During cutscenes, death animations, or other places where the ordinary
controls are not available, the engine will not be able to apply new mods. For this reason, it's
important to remember to hit pause when such events occur. If you forget and a vote ends during
this period, some categories of mods, including all those that require setting menu options, may
put the game in an inconsistent state. Old mods may not be removed correctly, and new ones may not
be applied. In this case, you may need to pause the game and manually set the necessary options.

*Important Security Note:* By default, TCC runs without any security. This is only safe if you
are running it on a local network that is completely under your control (i.e., you manage the
router that connects you to the internet yourself and you trust all the devices within your home
network). If that is not the case, be sure to enable password protection for the UI and enable
TLS.


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
- Controller Drift: Add controller drift to the left joystick

The low-level usb interception should be more stable in this version. In particular, hot-plugging
the controller is now supported.

## Required Hardware

Apart from a PlayStation (or PC if you're playing a PC game), you will need the following
hardware (for all setups):

- A DualShock 4 model CUH-ZCT2U controller 
- A Raspberry Pi 4
- A 32GB microSD card (This is your storage device for the Pi)
- A USB card reader for microSD cards
- A micro USB A to micro USB B cable (the cable type that comes with the PS 4 DualShock)
- A USB C to USB A cable (the cable type that comes with the PS 5 DualSense)
- (recommended) A cat 5 ethernet cable
- (optional) A case for the Raspberry Pi.

To play PS5-only games such as TLOU1 Remake or TLOU2 Remaster, you will also need hardware to let
you use a non-PS5 controller without having the console reject your commands. I successfully
used a [Besavior PS5 controller](https://www.beloader.com/products/besavior-1.html). Similar
devices such as the same company's Besavior P5Mate Pro will likely work as well. Note that in
these setups, the console alone doesn't provide enough power to run the intermediate device
and the Pi, so you will need to provide additional power to the Pi. Without the extra hardware,
the console provides enough power to run the Pi.

Most of the instructions here also assume that you also use streaming software such as OBS, which will
require you to have a capture card as well. Note that is **is** possible to run Chaos without OBS,
meaning that it should be possible to play Chaos while streaming directly from your PlayStation,
or even without streaming at all, although you will not be able to use overlays to show the current
status of votes or the modifiers in effect. (This has not been tested, but suggestions for what
to try will be found below.)

**Important:** You must use the DualShock 4 Generation 2, model CUH-ZCT2U, or a compatible 3rd-party
equivalent. Gen-1 DualShocks connect to the console only through wifi, and a wired USB connection is
needed to intercept the signals. Note also that you need this controller *in addition* to the
Besavior to play PS5-only games.

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
time, run the following command:

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

### Updating an Existing Installation

After the initial install, use the update scripts from the repository root:

```bash
./scripts/update_engine.sh
./scripts/update_chaosface.sh
./scripts/update_chaos.sh
./scripts/update_services.sh
```

- `update_engine.sh` rebuilds/reinstalls the engine and refreshes the `chaos` systemd unit files.
- `update_chaosface.sh` redeploys Chaosface to `/usr/local/chaos`.
- `update_chaos.sh` runs both engine and Chaosface updates in sequence.
- `update_services.sh` syncs `startchaos.sh` and service unit files without rebuilding engine or redeploying Chaosface.

Optional flags:

- `./scripts/update_engine.sh --restart-service`
- `./scripts/update_chaosface.sh --skip-deps --restart-service`
- `./scripts/update_chaos.sh --skip-chaosface-deps --restart-services`
- `./scripts/update_services.sh --restart-services`

The --restart-service and --restart-services will stop the old version of the engine and/or chaosface and start the new
one immediately after the update. If you omit this flag, you will need either to restart the services manually or
reboot the Pi.

The --skip-deps and --skip-chaosface-deps flags will skip checks for alterations in dependencies and will speed up
updates when there are no changes to any of the 3rd-party libraries that these programs rely upon.

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

1. If you don't have one already, create a separate Twitch account for the bot.

If you prefer not to create a secondary account for your bot, you can still run most features
of Chaos by entering the information for the account you broadcast from. In this case, however,
mod redemptions with channel points will not be available.

2. On the same local network as the Raspberry Pi, open a browser and navigate to
[raspberrypi.local](http://raspberrypi.local), assuming you did not change the Pi's hostname.
If you did set the hostname during configuration, replace "raspberrypi.local" with whatever
name you chose.

If this address does not work, you may need to find your Pi's IP address and use it directly.
You can discover this address by going to your router's admin page, finding the list of
connected devices, and seeing if you can identify which one is the Pi. Alternatively, run
the command `hostname -I` on your Pi to get your Pi's IP address. Input this address in your
browser instead.

3. Click on the "Settings" tab. Enter your channel name and bot account credentials, or the
credentials for your broadcaster account if you do not have a secondary custom bot account.

4. While logged in to your bot account, get an OAuth token. (See the
[chaosface instructions](docs/chaosface.md) for details.)

5. (Optional) if you want to use channel-point redemptions or bits to give users modifier
credits, get an EvenSub OAuth token while logged in to your main streamer's account.
(See the [chaosface instructions](docs/chaosface.md) for details.)

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

1. Connect the DualShock controller (the PS4 controller) through a USB cable to the lower left
USB port on your Raspberry Pi. This is the USB 2.0 port furthest from the ethernet port and
closest to circuit board.

2. Connect the Raspberry Pi's ethernet port to your router, unless you're using WiFi.

[Cabling the Raspbery Pi](https://github.com/polysyllabic/chaos/blob/unbound/docs/images/chaos_plugging.jpg)

The next step depends on what type of game you are playing.

For PS4 games (whether played on a PS4 or a PS5):

3. Use a USB-C to USB-A cable to connect the Raspberry Pi's USB-C port to the console. The Pi will
get its power from this connection, as well as using it to communicate with the console.

For PC games:

3. Connect the Pi's USB-C port to a USB port on your PC (either USB-A or USB-C will work).

For PS5-only games:

You need a device to convince your console that you're using a DualSense (PS5) controller. We'll call
this the "imposter" device such as a Besavior controller.

3. First connect the imposter device to the PlayStation 5 with a USB cable. For the Besavior
controller, this means using the USB-C port that is in the same place as the port on an ordinary
controller.

Next, connect the impostor device to the Raspberry Pi's USB-C port.

If you're using the Besavior controller, you should use plug the special cable that comes with the
Besavior that has a male USB-C port on one side and a connector with both a female USB-C and a
female USB-A port on the other. Connect the male USB-C side to the USB-C port on the device
mounted below the Besavior controller. Connect the the Raspberry Pi's USB-C port to the USB-A end
of the special connector with a USB-C to USB-A cable. Finally, connect another USB-C cable into
the side port of the special adapter and connect it to an external DC power source that can supply
at least 3A. *Note:* If you use a lower-rated power supply, you may experience brown-outs when the
Pi starts trying to draw more power than is available under load. In this case, you may find the
Pi hanging and rebooting at inconvenient times.

The sequence of devices is DualShock -> Raspberry Pi -> Impostor Device -> PlayStation 5


### Startup

1. Turn on your console.  If using the Besavior setup, also ensure that the external power is
connected. Once power is applied, the Raspberry Pi will boot up. One the boot sequence and the
Chaos engine is running, the system (either console or PC) should recognize the controller.

2. If OBS was already running, refresh your browser sources. The overlays should be active.

3. Next, check that the controller is connected properly. Start up and load into your game so that
you are controlling your character. After you press the "Share" button, the VoteTimer progress bar
should start moving. If the timer runs and you can control your character, then the engine is
running correctly and communicating with the interface.

4. Finally, test the voting. Pull up your chat and try to vote while the engine is running. A vote
should appear on your overlay. If all these tests are OK, you are ready to go!

### Playing the Game

1. The main interface page will contain several tabs that will let you alter specific settings. The
default tab is designed to show you the current status of Chaos without revealing information about
which mods are winning the voting. This way (assuming you're not looking at the OBS overlays), what
chat inflicts on you should come as a surprise.

### Pausing

Chaos initializes in a paused state. When Chaos is paused, modifiers do not run and all signals are
passed through to the console unaltered. The vote timer is also suspended, but any votes cast during
the pause *will* be counted. Pausing chaos will let you navigate things like your console's system
menu without wreaking true havoc.

- To resume Chaos, press the *Share* button.  
- To pause Chaos, press the *Option* button.

*Note:* This means that you will actively need to resume Chaos whenever you enter your game's pause
menu.

Because the Chaos engine requires a dedicated button to resume, the *Share* button's signal is
never passed on to the controller. For most games, this should not create problems, since this
button is used for console-related functions that are independent of the game itself. If you
need access to this button for a particular game, send me the details and I'll see what I can do.

### Game-Specific Usage

TCC was originally developed for The Last of Us Part 2. Detailed instructions on how to run that
game will be found in the [README file](docs/TLOU2/README.md) for that game.

## Design

The core of TCC involves the Chaos engine, written in C++ for speed. At the lowest level, the Chaos
engine works by forwarding USB protocols using the Linux raw-gadget kernel module. For every USB
request, the engine duplicates the request and passes it along. However, in the case of messages
corresponding to controller buttons/joysticks, the data is passed to other processes that can meddle
with the data. This forwarding infrastructure is done by through the a custom library (usb-sniffify)
that combines [raw-gadget](https://github.com/xairy/raw-gadget) and [libusb](https://libusb.info).

The Chatbot is written primarily in Python, with some HTML and JavaScript for the sources.

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
asynchronous actions that are controller event-based.

The Chaos engine offers the following capabilities for reading and altering controller data:

- Sniffing - Read input packets being sent from the controller
- State Sampling - See live controller state (as opposed to what is trying to be sent)
- Interception - Block commands in the pipeline
- Modification - Alter data including the command type, id, and value
- Injection - Generate arbitrary messages without any controller events
- Direct Control - Send commands directly to the output

### Supporting New Games
Individual games are supported through configuration files. If you want to use TCC with a new game,
you merely need to create an appropriate configuration file using the correct
[configuration file syntax](docs/chaosConfigFiles.md).

When creating new games, or new mods for existing games, there are several utility programs that
may be useful. They will be found in the `chaos/utils` directory. To see all the options available
with these utilities, run them with -h or --help as the parameter.

Each of these utilities is run from a shell script. For C++ utilities, those scripts ensure the
build tree is configured and run an incremental rebuild each time before execution.

To build C++ utilities manually from the repository root, follow these steps:

  `cmake -S chaos -B chaos/build`
  `cmake --build chaos/build --target chaos_parse_game_config validate_mod -j4`

The examples below assume you're running them from the repository root.

- `./scripts/make_modlist.sh` Generates a text list of all the mods available for chat to vote on for this
  game. If you put this somewhere accessible on the web, the chatbot can provide a link to users in
  response to the !chaoscmd command.

_Basic usage:_
`./scripts/make_modlist.sh <game-config.toml> <output.txt>`

_Example:_
`./scripts/make_modlist.sh chaos/examples/tlou2.toml chaos/examples/modlists/tlou2_mods.txt`

_The option -g also lists mods by their group:_
`./scripts/make_modlist.sh -g chaos/examples/tlou2.toml chaos/examples/modlists/tlou2_mod_groups.txt`

- `parse_game_config` Attempts to load a game configuration file and reports any errors it
  encountered. Running this provides a syntax check on the configuration file before you try to use
  it for real.

_Basic usage:_
`./chaos/utils/parse_game_config.sh <game-config.toml>`

_Example:_
`./chaos/utils/parse_game_config.sh chaos/examples/tlou2.toml`

- `validate_mod` Runs a single modifier as the Chaos engine would, but without communication with the chatbot,
and by default issuing debugging messages that will tell you how the mod is being processed. Running the
game itself while testing the mod is optional.

_Basic usage:_
`./chaos/utils/validate_mod.sh -g <game-config.toml> -m "<modifier name>"`

_Example:_
`./chaos/utils/validate_mod.sh -g chaos/examples/tlou2.toml -m "Aimbot"`

_Write logs to a file with -o:_
`./chaos/utils/validate_mod.sh -g chaos/examples/tlou2.toml -m "Aimbot" -o /tmp/validate_mod.log`


## Questions That People Might Frequently Ask (if anyone asked questions)

*Why can't I use a first-generation DualShock controller?*

The first-generation DualShock 4 controller can only send control inputs over bluetooth, making
it unusable for this project.

*Why can't I use the DualSense (PS5) controller with TCC?*

The DualSense controller for the PS5 sends signals to the console along with an encrypted checksum
that indicates what those signals should be. If the signals don't match the checksum, the input is
rejected, and because this checksum is encrypted, there's no way for us to update the checksum to
match. This is not a problem with PS4 games, since the console will allow you use a DualShock
controller. For PS5 games, however, Sony wants you to use a DualSense, which won't work with TCC.

The way to work around this limitation is either to play the PC version of the game (if one exists)
or to play a PS5 game on the PlayStation with a device that allows other controllers to pretend
that they are a DualSense. The best-known examples of such hardware are products from Besavior. 
If you're interested in going this route, I recommend you check out
[this YouTube video](https://youtu.be/j-GCoZs3qVM?si=7RSLYm3dREmNkMga) before buying anything so
that you understand the trade-offs involved between the various devices.


Supporting other controllers (e.g., XBox) would be extremely useful, especially for PC games. This
is on my to-do list.

*Does this work with the PC version of TLOU2?*

If you can plug the DualShock + Pi into your PC instead of your console, you _can_ use TCC, but you
must select the game configuration file designed for the PC, since the menu options differ somewhat.
Playing this way will _not_ intercept signals coming from mouse and keyboard, so you still need to
use a DualShock.

If you're asking for a version that works _without_ a Raspberry Pi to handle the signal conversions,
that would require a major effort. In essence, we'd need to create some sort of driver to intercept
not only controller input but mouse-and-keyboard inputs as well. I might consider attempting this at
some point, after the Pi version is stable, but it's not a priority for me.

*Do I have to use a Raspberry Pi 4?*

Currently the Raspberry Pi 4 is the only device tested. Some other Raspberry Pi models *may* work
(see below), but a regular computer running Linux will not.

The communication to the PlayStation requires the Pi to act as a USB client instead of a USB host.
That requires special hardware peripherals which the Pi 4 has on its USB-C power port. Many older
Pi models--and most, if not all, ordinary PCs--lack this hardware and so cannot run the chaos
engine.

There are other Pi variants that could work, but they are untested. The Raspberry Pi 0W has the
right hardware and is much less expensive than the Pi 4, but this has only one USB device to act as
a client and lacks an ethernet plug, meaning that WiFi is the only method to connect to the network.
This means that the controller has to connect over Bluetooth, using the same device that's also used
for its network connection. There may be performance issues going that route, including controller
lag and disconnections, as well as chat/OBS overlay issues. Some of these performance issues may
be reduced by running the chatbot on a different computer to lighten the Pi's workload.

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
