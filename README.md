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

- The Chaos interface, a chatbot written in Python, monitors the streamer's Twitch chat,
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

TCC requires specific hardware to run, including a Raspberry Pi 4 running a 32-bit kernel and a
DualShock controller (the controller for the PlayStation 4), which may constitute a significant
expense. To play PS5-only games on the PlayStation, you will need even more hardware.

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

Streamers can now choose to allow chat to acquire credits that entitle the person with a credit to
apply a modifier of their choosing. The modifier is applied outside of the normal voting and occurs
immediately (or after a cooldown period that can be configured). This allows a member of chat to
apply the most chaotic modifier at a strategic time, rather than being subjected to the vagaries of
the voting process.

Credits can be acquired by any of the following means, which the streamer can enable and configure
individually:

- Channel points redemption
- Bits donation
- Raffle
- Gifting from those who have credits already

The low-level usb interception should be more stable in this version. In particular, hot-plugging
the controller is now supported.

## Required Hardware

Apart from a PlayStation (or PC if you're playing a PC game), you will need the following
hardware (for all setups):

- A DualShock 4 model CUH-ZCT2U controller 
- A Raspberry Pi 4
- A microSD card 32GB or larger (This is your storage device for the Pi)
- A USB card reader for microSD cards
- A micro USB-A to micro USB cable (the cable type that comes with the PS 4 DualShock)
- A USB-C to USB-A cable (the cable type that comes with the PS 5 DualSense)
- (recommended) An ethernet cable (cat 5 or higher)
- (optional) A case for the Raspberry Pi.

To play PS5 games such as TLOU1 Remake or TLOU2 Remaster, which do not let you play with
a DualShock controller, you will also need an authenticator device to let you use a non-PS5
controller without having the console reject your commands. 

*Authenticator devices known to work*

- Besavior controller: This is a regular PS5 controller that has been customized with a backpack
  device that lets you chain another controller into it. This is an excellent controller in its
  own right, but you won't be using it as a controller in this configuraiton. It will just sit
  on your desk between the Pi and the PS5. It is also expensive.

- Besavior P5Mate Pro 1000Hz: This is essentially just an authenticator/translation device. It's
  plugged in the same way that the Besavior controller is, sitting between the Pi and the console.
  Since it doesn't have the buttons and joysticks of a real controller, it's much less expensive
  than the Besavior controller.

Note that for either setup, the console alone likely doesn't provide enough power through the
USB port to run the authenticator device and the Pi, so you will need to provide additional power
to the Pi. See the cabling instructions for how to accomplish this. If you're playing a PS4 game
where you don't need the authenticator, the console provides enough power to run the Pi.

For either of these devices, see the [Besavior website](https://www.besavior.com/)

*Authenticator devices that will probably work*

Any device that transparently acts as a man-in-the-middle authenticator in the way that the
two devices above do will likely also work, but I have not tested them. The Brook Wingman P5
is an example of such a device.

*Authenticator devices that definitely will not work*

The P5General is another product from Besavior. It's the lowest cost option from that company,
but you should _not_ buy it for use with TCC. The P5General isn't a passthrough device that
translates an arbitrary controller's messages to the PS5 format. Instead, it provides services
to authenticate USB packets before sending them on to the PlayStation. In essence, it gives
other devices the tools they need to pretend they are PS5 controllers. Supporting this would
require a significant effort, and since there are already several other ways to achieve the
same functionality with much less work, I have no plans to add support for it at this time.

**Important:** Currently, you must use the DualShock 4 Generation 2, model CUH-ZCT2U. Some
3rd-party equivalents may also work, but this has not been carefully tested. Gen-1 DualShocks
connect to the console only through wifi, and a wired USB connection is needed to intercept the
signals.

Supporting XBox and DualSense controllers to play PC games in on the to-do list for version 2.1.

## Installation and Configuration

To set up the required hardware and install the Twitch Controls Chaos software on the Pi, see
the [installation guide](docs/installation.md).

To set up the chatbot so that it can run in your channel, and to set up OBS so you can stream
TCC, see the [Chaosface configuration file](docs/chaosface.md).

## Running the Game

1. Connect the hardware as described in the [installation guide](docs/installation.md) and
turn on your console/PC and external power, if needed. Once power is applied, the Raspberry Pi
will boot up. After the boot sequence and the Chaos engine is running, the system (either console
or PC) should recognize the controller. If you've turned off bluetooth on the console, note that
you will need to press the power button on the console itself.

3. Open your Chaosface in browser. Use the host name you chose when setting up the Pi. For example,
if you used `raspberrypi`, then you should go to `http://raspberrypi/` if TLS is not enabled, or
`https://raspberrypi/` if TLS is enabled. If you've set a password, log in. You will then be
directed to the streamer-interface page.

This interface provides you, the streamer, with information on the vote timer, the currently
active modifiers, and also an indicator of the state of the TCC engine. It deliberately does not
tell you what modifiers are being voted on. This way (assuming you're not looking at the OBS
overlays), what chat inflicts on you should come as a surprise. From this page you can also
access tabs that let you configure how Chaos runs. 

When Chaos is paused, the status indicator flashes a large box to remind you to unpause when
you are ready. It's meant to be noticeable in your peripheral vision, so you can position the
browser page on a secondary monitor. It is highly recommended to use this interface and not
the OBS scene that your viewers see to monitor TCC.

4. If you have never selected a game to play, or if you want to switch games, go to the
`Game Settings` tab of the interface and select the game you want to play from the
available-games list. Click `confirm game` to have TCC load the new game. Note that there
can be multiple configuration files for a single game to support variations that only exist
on certain platforms. For example, the original version of _The Last of Us: Part II_ has
a slightly different menu layout than the remastered version, and the PC version of the game
has still further differences. Even for the same game, there are certain mods that can only
run on specific platforms. For example, the 30fps mod is only available if you are playing
the original version of TLOU2 on a PS5. Make sure you pick the right version.

If you've loaded a game previously, TCC will try to load the same game automatically, so
you only need to select a new game if you want to switch.

5. If OBS was already running, refresh your browser sources. The overlays should be active.

6. Test that the controller is connected properly. Start up and load into your game so that
you are controlling your character. After you press the "Share" button, the vote-timer
progress bar should start moving. If the timer runs and you can control your character, then
the engine is running correctly and communicating with the interface.

7. Finally, test the voting. Pull up your chat and try to vote while the engine is running. A vote
should appear on your overlay. If all these tests are OK, you are ready to go!

## Pausing and Resuming the Game

Chaos initializes in a paused state. When Chaos is paused, modifiers do not run and all signals are
passed through to the console unaltered. The vote timer is also suspended, but any votes cast during
the pause *will* be counted. Pausing chaos will let you navigate things like your console's system
menu without wreaking true havoc.

- To resume Chaos, press the *Share* button.  
- To pause Chaos, press the *Option* button.

*Note:* This means that you will actively need to resume Chaos whenever you enter your game's pause
menu. The streamer UI flashes annoyingly to remind you when you are paused.

Because the Chaos engine requires a dedicated button to resume, the *Share* button's signal is
never passed on to the controller. For most games, this should not create problems, since this
button is used for console-related functions that are independent of the game itself. If you
need access to this button for a particular game, send me the details and I'll see what I can do.

## Game-Specific Usage

TCC was originally developed for *The Last of Us Part 2*. Detailed instructions on how to run that
game will be found in the [README file](docs/TLOU2/README.md) for that game.


## Design

The core of TCC involves the Chaos engine, written in C++ for speed. At the lowest level, the Chaos
engine works by forwarding USB protocols using the Linux raw-gadget kernel module. For every USB
request, the engine duplicates the request and passes it along. However, in the case of messages
corresponding to controller buttons/joysticks, the data is passed to other processes that can meddle
with the data. This forwarding infrastructure is done by through a custom library (usb-sniffify)
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

## Supporting New Games
Individual games are supported through configuration files. If you want to use TCC with a new game,
you merely need to create an appropriate configuration file. A complete description of the syntax
for these configuration files, as well as notes on utilities to help in the process of supporting
a new game, can be found [here](docs/chaosConfigFiles.md).

When creating new configuration files or new modifiers for existing games, there are several
utility programs that may be useful. They will be found in the `chaos/utils` directory.

- `make_modlist.sh` Generates a text list of all the mods available for chat to vote on for a
  game. If you put this somewhere accessible on the web, the chatbot can provide a link to users
  in response to the !chaoscmd command.

- `parse_game_config.sh` Attempts to load a game configuration file and reports any errors it
  encounters. Running this provides a syntax check on the configuration file before you try to use
  it for real.

- `validate_mod.sh` Runs a single modifier as the Chaos engine would, but without communication
  with the chatbot, and by default issuing debugging messages that will tell you how the mod is
  being processed. Running the game itself while testing the mod is optional.

- `gamepad_test.sh` Monitors the same USB passthrough traffic path used by the engine. This will
  give you a list of all the events that the engine would see passed between the controller and
  the console.

Documentation explaining their use will be found in the [README](chaos/util/README.md) file in
the same directory.

## Questions That People Might Frequently Ask (if anyone asked questions)

*Why can't I use a first-generation DualShock controller?*

The first-generation DualShock 4 controller can only send control inputs over bluetooth, making
it unusable for this project.

*Why can't I use the DualSense (PS5) controller with TCC?*

The short story is that Sony wants to lock down its platform so you are encouraged to buy one of
their controllers. (There is only one licensed 3rd-party controller that I know of: the Razer
Wolverine.)

The DualSense controller for the PS5 sends signals to the console along with an encrypted checksum
that indicates what those signals should be. If the signals don't match the checksum, the input is
rejected, and because this checksum is encrypted, there's no easy way for us to update it to match.
This is not a problem with PS4 games, since the console will allow you use a DualShock controller.
For PS5 games, however, Sony wants you to use a DualSense, which won't work with TCC.

For ways to remove this restriction, see the discussion of authenticator devices above.

*What about using remote play?*

Some people get around the PS5 controller restriction by using remote play. However this is laggy
and unreliable. I have no plans to support it.

*Does this work with the PC version of TLOU2?*

If you plug the DualShock + Pi into your PC instead of your console, you _can_ use TCC, but you
must select the game configuration file designed for the PC, since the menu options differ
slightly from the console version of the game. Playing this way will _not_ intercept signals
coming from mouse and keyboard, so you still need to use a DualShock.

If you're asking for a version that works _without_ a Raspberry Pi to handle the signal conversions,
that would require a major effort. In essence, we'd need to create some sort of driver to intercept
not only controller input but mouse-and-keyboard inputs as well. I might consider attempting this at
some point, after the Pi version is stable, but it's not a priority for me.

*Do I have to use a Raspberry Pi 4?*

Currently the Raspberry Pi 4 is the only device tested. Some other Raspberry Pi models *may* work
(see below), but a regular computer running Linux will not. Raspberry Pi 5-class devices are not a
good fit right now because Raspberry Pi OS keeps them on a 64-bit kernel, while raw-gadget
currently requires a 32-bit kernel.

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

If you do get TCC running on a different model, please drop me a line and let me know.

## Troubleshooting
*The console isn't responding to the controller*

Before you do anything else, test that each USB cable in your setup is working. Not only do
cables break, but a surprisingly large number of cables are e-waste from the beginning.

  - Test each cable separately.
  - If you're using long cables, try the shortest ones that will work for your setup.
  - For USB-C cables, make sure they provide a dual, power-and-data connection. Power-only cables
    will not work.
  - If you're using an adapter dongle (e.g., USB-A to USB-C), try reversing the device it's
    attached to. In other words, if it's currently attached at the console end, move it to the
    other device and reverse the cord, or vice versa. Sometimes a connection will work one way but
    not the other.

Verify that everything is correctly cabled, power is applied to the Pi, and the chaos engine is
running. When the engine is operating and the DualShock is plugged in to the correct USB port on
the Pi, the light on the back of the DualShock should be on.

If your cables are good and your cabling is correct, try connecting to the console through a
different USB port. (In particular, try using one of the USB-A ports on the back of the console
rather than a front-side USB-C port.)

If you're attempting to run this setup on a PS5 with an authentication device in the middle,
verify that the authenticator works without the Pi present. Connect a DualShock to the
authenticator and try to open and run a PS5 game. If this fails, you will need to configure the
authenticator, following whatever instructions the manufacturer provides.

*I can navigate through the PlayStation's system menu but I can't play a game.*

This is a bug that _should_ have been fixed in most if not all cases. Please report this issue if
you encounter it, but for an immediate work-around, open the game you want to play and then
restart the engine from the Pi's commandline: `sudo systemctl restart chaos`.

*When the Pi is plugged in to my PC, loading the game takes an eternity*

Steam Input can cause serious issues with lag, especially while starting up the game. This is
definitely true with _The Last of Us: Part II_ and likely true for other games as well. To avoid
this problem, disable Steam Input for the relevant PC games. From your Steam library, right-click
on the game, then select `Properties -> Controller -> Override -> Disable Steam Input`.

*TCC is selecting the wrong menu options / Some mods are still enabled*

*The Last of Us: Part II* uses a menu system that remembers the last place you exited the menu. If
you forget to return all menus to the top of each menu page, TCC will put the items in the wrong
place. It can also get off if the chaos engine attempts to start or remove a mod when the game is
not accepting commands (e.g., in a death animation). Remember, the engine has no way of knowing
what's going on in the game, so it's on you to remember to pause the game when you enter one of
these input-blocking moments. If you forget, you'll need to go into the menus and reset things to
their proper state by hand.

*The Pi suddenly reboots*

The most likely reason is insufficient power to the Pi. The USB output from a PlayStation normally
supplies enough power to run the Pi directly. However, if you have adopted a chained hardware
setup, e.g., though a Besavior controller, it does _not_ have enough power to run both the Besavior
and the Pi. It may deliver enough juice for you to boot the Pi, but it will likely fail once chaos
and chaosface start actively running, which will increase the power draw. To fix this, you need to
supply external power to the Pi. See the instructions above for the two Besavior products for
how to accomplish this.

*Chaos stops working after you update your Pi*

TCC uses a hacked version of the raw-gadget kernel module. If you've updated the kernel to a newer
version, the old kernel module will now be incompatible with the new one. You can manually rebuild
the module with these commands:

```bash
cd
cd chaos/raw-gadget-timeout/raw_gadget/
make
```

*Running the OG TCC after updates*
In the (unlikely) case that anyone is still trying to create a new installation of Blegas's original
TCC, note that the original version of the sniffify library that is used to intercept USB messages
depends on a specific, older version of the linux kernel headers that were altered in later releases.
To get that library working, you may need to revert to an older image of the OS, which is
[available here](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2021-05-28/).
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

### General
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

### Modifier Ideas
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
