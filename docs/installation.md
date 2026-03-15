# Installing and Updating Twitch Controls Chaos
The TCC engine has been confirmed on a Raspberry Pi 4 running a 32-bit kernel.
Other setups may work, but the device will need the appropriate hardware.

## Configuring the Pi
First you must set up your Raspberry Pi. Assuming you are working with a new Raspbery Pi, do
the following steps.

1. Install the OS onto your SD card.

Download the [Raspberry Pi Imager](https://www.raspberrypi.org/software/) to your PC to help
with this process. Note: over time, the imager software has undergone multiple changes to
how it guides you through the setup process. The instructions here were current as of March
2026, but if you're following this guide substantially after that, some details may have
changed.

- Connect your SD card to your computer using an SD card reader.
- Choose the model of your Pi
- Choose your OS version. Select `Raspberry Pi OS (other) -> Raspberry Pi OS Lite (32-bit)`.
  Unless you also plan to use the Pi as a desktop, the Lite image is recommended because it skips
  the desktop environment, which you do not need for TCC operation. On a Raspberry Pi 4,
  Raspberry Pi OS Bookworm 32-bit is currently the safest choice. Recent Trixie 32-bit images on
  the Pi 4 still boot a 64-bit kernel by default, and the official packages no longer ship the
  32-bit Pi 4 kernel image (`kernel7l.img`) that TCC needs. If you use Trixie, you will need to
  provide a custom 32-bit Pi 4 kernel before the installer can continue.
- Select your storage device. This should appear as something like "Mass Storage Device USB Device"
- Choose a hostname, e.g., `raspberrypi`.
- Set localization for your region.
- Choose a username and password. The default username is `pi` and password is `raspberry`, but
  it's recommended that you change at least the password.
- If you only plan to connect to the Pi with an ethernet cable, you can skip Wi-Fi setup.
  Otherwise, enter your SSID (the network name your router broadcasts) and password here.
- If you want to be able to access your Pi without plugging in a keyboard and monitor, check
  "Enable SSH" and choose your method of authentication. If you don't know how to set up
  public-key authentication, you can stick with a password, but make sure you've changed the
  password from the default. Note that you don't need the SSH shell (or keyboard/monitor) for
  ordinary operation of Chaos. It's only necessary if you need to perform an upgrade or do
  your own development.
- If you want to enable Rapberry Pi Connect, you can, but it's unnecessary.
- Click on the "Write" button to copy the OS, with the settings you have chosen, to the SD.

2. Insert the SD card into your Pi. The slot for this is on the bottom side of the board.

3. If you did not configure SSH, connect a monitor and keyboard. This is only required during the
setup process, or later if you need to update anything and have not set up an SSH shell. After the
setup is complete, you can run Chaos without either a keyboard or monitor connected to the Pi.

4. (*Recommended*) Connect the Raspberry Pi to your router with an ethernet cable. If you
cannot use wired ethernet, or prefer not to, Wi-Fi must be set up.

5. Apply power over the Pi's USB-C connector by plugging the cable into the PlayStation and
turning the console on. (You can also use an external USB-C power supply, but it should supply
at least 3A.) Note that at this stage, you only need to power the Pi. Connecting the controller
can wait until you're ready to play a game with TCC.

6. When your Pi boots, log in using the credentials you specified above. If you stuck with the
system defaults, they will be the following:
- Username: pi
- Password: raspberry

This will start a bash terminal session. If you are connecting over SSH and did not alter the
default hostname, try rapberypi.local as the host address. If that does not work, you may need to
find the local IP address that your router has assigned to the Pi. Go to your router's admin page
and look for the list of connected devices. 

Note: The password field will look like nothing is being typed, but it will be reading the password
as you type it.

7. Update your OS and system tools and install the version-control system *git*, which is used to
manage all the files you will need to run TCC. After you have logged in to the Pi for the first
time, run the following commands:

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
On Bookworm-based Pi 4 installs, a reboot may be required halfway through the installation process
so the Pi can switch back to the 32-bit kernel that raw-gadget currently requires. You will be
prompted to re-run the install script after you have rebooted. The installation will resume where
it left off. On current Trixie Pi 4 images, the installer will stop earlier and tell you to use a
Bookworm image or supply a custom `kernel7l.img` instead.

At the end of the installation, you will have to reboot one more time. If you choose not to
reboot when prompted, enter the following command when you are ready to reboot.

```bash
sudo reboot
```

After this final reboot, chaos should be installed and will run automatically each time you turn
on the Raspberry Pi. Note that if you chose to run the chatbot from a different computer, it
is your responsibility to start it yourself. Automatic initialization of the chatbot only occurs
when you run it from the Pi.

## Configuring the Console

Your console will need to be set up to prefer USB communication rather than bluetooth for your
controllers. Open the system menu on your console.

- On a PS5, navigate to Settings -> Accessories -> Controllers -> Communication Method -> Use USB Cable
- On a PS4, Navigate to Settings -> Devices -> Controllers -> Communication Method -> Use USB Cable

_Note:_ This seting on its own will mean that the console uses the cable when available but
will still use bluetooth in other contexts, for example when the Chaos engine is down. To fully
disable bluetooth, go into Settings -> Accessories -> General -> Advanced Settings and turn off
bluetooth. This will be necessary if you're using an authenticator device, since you need to make
sure that your controller only connects to the console through the authenticator.


## Configuring the Chatbot
Before attempting to play TCC for the first time, you will need to configure the chatbot and
authorize it in your channel.

See the [chaosface documentation](docs/chaosface.md) for those steps.

## Configuring OBS/SLOBS
The Chaos interface generates three browser sources that you can add as overlays to show the
current status of Chaos to your viewers:

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

You should set these browser sources to refresh when not displayed so that they can easily be
refreshed. Detailed setup instructions for OBS are available in the
[stream setup](docs/streamSetup.md) guide.

### Running TCC Without OBS/SLOBS
If you are streaming directly from a console and cannot display local browser-source overlays,
you can still run TCC, but you will need to rely more heavily on the chatbot's chat interface.
From the game-settings tab of Chaosface, check the options `Announce candidates in chat` and
`Announce winner in chat`. This way, your chat will be informed each voting cycle what mods
they can vote on and what mod won the last round. Even if you stream with sources, this option
could also be handy if you have people in chat who cannot see the sources.

Note that you do not even need to be streaming for the chatbot to work. Merely by virtue of
having a Twitch account, you have a chat, which can work even when you're offline. So in theory,
you could have a group of friends over to your house and just play the game on your TV while
they joined your twitch chat on their phones and voted without you ever needing to stream
the game.

## Cabling

Before applying power to your Pi, ensure that everything is connected correctly. The hardware
chain differs depending on whether you are playing a PS4 game or a PS5 game.

_For all games:_

1. Connect the DualShock controller (the PS4 controller) through a USB cable to one of
the USB-A ports on your Raspberry Pi. _Note:_ In the original version of TCC, you
had to connect to the lower-left port, but this restriction has been removed from
Chaos Unbound. 

2. Connect the Raspberry Pi's ethernet port to your router, unless you're using WiFi.

[Cabling the Raspbery Pi](https://github.com/polysyllabic/chaos/blob/unbound/docs/images/chaos_plugging.jpg)

The next step depends on what type of game you are playing.

_For PS4 games (whether played on a PS4 or a PS5):_

3. Use a USB-C to USB-A cable to connect the Raspberry Pi's USB-C port to the console. The Pi will
get its power from this connection, as well as using it to communicate with the console.

_For PC games:_

3. Connect the Pi's USB-C port to a USB port on your PC (either USB-A or USB-C will work).

_For PS5-only games:_

You need an authenticator device to convince your console that you're using a controller approved
for the PS5.

3. First connect the authenticator device to the PlayStation 5.

Different devices require different cable types. Please read the instructions that came with your
device.

In theory you can use any USB port on the console, but in practice the USB-A ports are much more
reliable. On a PS5 Pro, these are on the back of the console only.

Also connect supplementary power. For the P5Mate, there is a port for this on the lower right side
of the device. The Besavior controller comes with a special USB-A to USB-C cable that has an
additional USB-C port on one side for supplementary power. Connect one end of this adapter to
the USB-C port on the backpack device mounted on the under side of the Besavior controller, connect
the other (USB-A) end to a regular USB-A to USB-C cable, and connect the USB-C end of that cable 
to the Raspberry Pi's USB-C port. Supply supplementary power with another USB cable connected to
an independent power source such as a power brick. It must be able to supply at least 3A.

The full sequence of devices is DualShock -> Raspberry Pi -> Authenticator Device -> PlayStation 5

*Note:* If you don't supply external power, the Pi may still boot up, but you are likely to
experience brown-outs when the Pi runs under load. In this case, you may find the Pi hanging and
rebooting at inconvenient times.


## Updating an Existing Installation

If changes are made to TCC after you have installed it, you need to first fetch the updated
repository and then install the updates.

To get the most recent version of TCC, on your Pi go to the repository root (default `~/chaos/`)
and issue the command `git pull`.

There are a variety of update scripts to help simplify the process. From the repository root,
the following scripts are available:

```bash
./scripts/update_engine.sh
./scripts/update_chaosface.sh
./scripts/update_chaos.sh
./scripts/update_services.sh
```

- `update_engine.sh` rebuilds/reinstalls the engine and refreshes the `chaos` systemd unit files.
- `update_chaosface.sh` redeploys Chaosface to `/usr/local/chaos`.
- `update_chaos.sh` runs both engine and Chaosface updates in sequence.
- `update_services.sh` syncs `startchaos.sh` and service unit files without rebuilding engine or
   redeploying Chaosface.

Optional flags:

- `./scripts/update_engine.sh --restart`
- `./scripts/update_chaosface.sh --skip-deps --restart`
- `./scripts/update_chaos.sh --skip-deps --restart`
- `./scripts/update_services.sh --restart`

The --restart flag will stop the old version of the engine and/or chaosface and start the new one
immediately after the update. If you omit this flag, you will need either to restart the services
manually or to reboot the Pi.

The --skip-deps flag will skip checks for alterations in dependencies and will speed up updates
when there are no changes to any of the 3rd-party libraries that these programs rely upon.
