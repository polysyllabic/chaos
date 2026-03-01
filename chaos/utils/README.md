# Utilities

This folder contains various utilities that may be useful in creating and testing
game-configuration files.

Each of these utilities is run from a shell script. For C++ utilities, those scripts will
also run an incremental rebuild each time they execute to ensure that the latest version
is being executed.

Source files for utility apps are under `chaos/utils/src`.

To build C++ utilities manually from the repository root, follow these steps:

  `cmake -S chaos -B chaos/build`
  `cmake --build chaos/build --target chaos_parse_game_config validate_mod gamepad_test -j4`

## make_modlist

This utility reads the game-configuration file to generate a plain text file containing an
alphabetical list of all the mods available for chat to vote on for this game. Modifiers
that are set to `unlisted` are not included in this list. Its primary purpose is to support
the !chaoscmd function. Since a full list of modifiers would be much too long to post in
chat, !chaoscmd sends users to an external page. Put the generated file somewhere accessible
on the web and set its URI in the Chaosface interface.

_Basic usage:_
`make_modlist.sh <game-config.toml> <output.txt>`

_Example:_
`make_modlist.sh chaos/examples/tlou2.toml chaos/examples/modlists/tlou2_mods.txt`

_The option -g also lists mods by their group after the alphabetical list:_
`make_modlist.sh -g chaos/examples/tlou2.toml chaos/examples/modlists/tlou2_mod_groups.txt`

_Windows usage:_
`make_modlist.bat <game-config.toml> <output.txt>`

Note that this utility is written in Python, so you can run it on any computer that has Python
installed.

## parse_game_config

This utility loads a game configuration file and reports any errors it has encountered. Running
it will let you verify the syntax of the configuration file before you try to use it with the
Chaos engine.

_Basic usage:_
`./chaos/utils/parse_game_config.sh <game-config.toml>`

_Example:_
`./chaos/utils/parse_game_config.sh chaos/examples/tlou2.toml`

This is a Linux commandline utility. It can be run from the Pi or from any other computer
running Linux.

## validate_mod

This utility runs a single modifier as the Chaos engine would but without communication with
the chatbot and prints the same log messages that the engine would. By default, the log level
is set to 5 (debug), so you will see a messages indicating how the modifier is processing
the signals it's receiving from the controller.

_Basic usage:_
`./chaos/utils/validate_mod.sh -g <game-config.toml> -m "<modifier name>"`

_Example:_
`./chaos/utils/validate_mod.sh -g chaos/examples/tlou2.toml -m "Aimbot"`

_Options:_
 -g, --game-config <path> Game configuration TOML file (required)
 -m, --mod <name>         Modifier name to validate (required)
 -v, --verbosity <0-6>    Logging verbosity level (default: 5/debug)
 -o, --output <path|->    Log output destination (default: stdout)
 -t, --time <seconds>     Override modifier lifespan
     --sleep-us <us>      Loop sleep interval in microseconds (default: 500)
 -h, --help               Show help message

By default, the modifier lifespan is the same as set in the config file (probably 180
seconds). For things like menu modifiers, which only fire at the beginning and end
of their lifecycle, you probably want to override the time.

This utility must be run from the Pi's command line. Because it tests interaction with the
controller, the controller must be plugged in to the Pi.

## gamepad_test

This utility monitors the same USB passthrough traffic path used by the engine and outputs
a log of the activity seen. By default, the log verbosity is set to verbose (6), which will
includes handshake/control-transfer details.

It can run in one of two modes. In _translate mode_, the default, incoming data is translated
into controller events and the signals that the engine would act on (i.e., only those events
from the controller where a value has changed) are printed (`<signal name>=<value>`).

To reduce the amount of irrelevant noise from the output, accelerometer signals (`ACCX`, `ACCY`,
`ACCZ`) are suppressed by default. Small joystick values (`LX`, `LY`, `RX`, `RY`) are also
filtered to block low-level noise from joystick drift. You can set the size of this filter
with the `--fuzz` option.

In _raw mode_ the raw data packets are printed in their numeric form.

_Basic usage (translate mode, default):_
`sudo gamepad_test.sh`

_Options:_
    --accel            In translate mode, print accelerometer changes
-e, --endpoint <value> USB endpoint to monitor, hex or decimal (default: 0x84)
    --fuzz=<int>       In translate mode, ignore joystick values when abs(value) < fuzz (default: 10)
    --no-data          Show only handshake/control and transport logs (suppress ordinary data output)
-o, --output <path|->  Log output destination (default: stdout)
    --sleep-us <us>    Poll sleep interval in microsecons (default: 10000)
    --print-repeats    In raw mode, print repeated incoming packets (default: off)
-v, --verbosity <0-6>  Log verbosity (default: 6/verbose)
-h, --help             Show help

This utility must be run on the Pi with the controller plugged in and the Chaos engine stopped.
It must also run with superuser privileges (i.e., use `sudo`).
