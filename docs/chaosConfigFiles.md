# Twitch Congrols Chaos Configuration Files
This file documents the syntax for TCC configuration files. Each configuration file defines how
the modifiers and other information the Chaos engine needs to run TCC for a specific game.
The basic settings for the engine itself are controlled in the `chaosconfig.toml` file.

Configuration files use TOML format, which means they are plain-text files that can be modified
by any editor. For a full example of a configuration file in action, I recommend you study the
tlou2.toml configuration file.

## Basic Concepts

At the heart of the configuration file are the modifiers. Each modifier entry defines a
gameplay modifiaction that Twitch chatters can vote on and be applied to the user's
gameplay for a fixed period of time. Modifiers come in different types, which define
the category of action they perform. For example, some modifiers select options from the
game's menu. Others modify the signal or execute a predefined sequence of actions.

To support those modifiers, we also define a command map, sequences, and the game's menu
layout. The command map associates a user-friendly name with a specific controller signal
(e.g., the 'X' button or one axis of a joystick).

The configuration files are set up with the default binding of signals to named commands,
but if you prefer to play the game with custom settings, you can simply change this map
and all the modifiers will continue to behave as you would expect them to. For example,
a no-running modifier will actually block whatever signal you have bound to the run
command ('dodge/sprint' in TLOU2).

The menu-layout describes how to navigate the game's menu system to alter any necessary
game-setting options.

### Example Configuration File Entries

The following give some examples of how the configuration file works:

Associate a game command with a controller button:

```toml
[[command]]
name = "melee"
binding = "SQUARE"
```

Define a condition that can be monitored and acted on. The following defines a trigger that is reads
as true whenever the joystick controlling movement is deflected more than 20% of its maximum:

```toml
[[condition]]
name = "movement"
trueOn = [ "move forward/back", "move sideways" ]  
threshold = 0.2
thresholdType = "distance"
```

Define gameplay modifiers:

```toml
[[modifier]]
name = "No Melee"
description = "Disable melee attacks and stealth kills"
type = "disable"
groups = [ "combat" ]
appliesTo = [ "melee" ]

[[modifier]]
name = "Bad Stamina"
description = "Running is disabled after 2 seconds and takes 4 seconds to recharge."
type = "cooldown"
groups = [ "movement" ]
appliesTo = [ "dodge/sprint" ]
timeOn = 2.0
timeOff = 4.0

[[modifier]]
name = "Swap Joysticks"
description = "You may want to cross your thumbs to work with your muscle memory"
type = "remap"
remap = [
      {from = "RX", to = "LX"},
      {from = "RY", to = "LY"},
      {from = "LX", to = "RX"},
      {from = "LY", to = "RY"} ]

[[modifier]]
name = "Use Items"
description = "Shoots or throws 6 of whatever item is currently equipped. No effect for medkits."
type = "sequence"
groups = [ "combat", "inventory" ]
blockWhileBusy = [ "aiming", "reload/toss" ]
beginSequence = [
  { event="hold", command = "aiming", delay = 1.0 },
  { event="press", command = "shoot", repeat = 6, delay = 0.25 },
  { event="release", command = "aiming" } ]

[[modifier]]
name = "No Reticle"
description = "Headshots just got trickier."
type = "menu"
groups = [ "UI" ]
menu_items = [{entry = "reticle", value = 0 }]
```

## General Settings
The following settings apply to the operation of the engine for the whole game.

- chaos_toml: A version string for the TOML file format. Currently used only to verify that we're
  reading the right file. Currently, this should always be "1.0".

- game: User-friendly name of the game this configuration file defines.

- active_modifiers: The number of modifiers that can be in effect simultaneously

- time_per_modifier: Lifetime of an individual modifier, in seconds. The time that the engine is
  paused is deducted from the mod's total runtime, so this lifetime is the _active_ lifespan.

- button_press_time: Default time, in seconds, to hold down a button when issuing a command in a
  sequence.

- button_release_time: Default time, in seconds, to wait after releasing a button before going on
  to the next command in the sequence.

- touchpad_inactive_delay: Time (in seconds) since last received touchpad axis event before the
  touchpad will be reset to inactive.

- touchpad_velocity: Selects the algorithm used to translate touchpad events into joystick
  equivalents. If true, the joystick deflection is proportional to the change in axis value over
  time (i.e., your finger's velocity over the touchpad). An average value of the velocity over the
  previous 5 polling intervals is used. If false, we calculate the distance between the first point
  the finger touched and its curent position.

- touchpad_scale:  After the touchpad value is calculated by one of the two algorithms above, the
  resulting value is multiplied by this scaling factor. Scaling can be set separately for each
  axis. The first value in the list is for the x-axis; the second for the y-axis.

- touchpad_skew: Defines an offset to apply to the axis value when the derivative is non-zero. The
  sign of the skew is the same as the sign of the derivative. In other words, if the derivative is
  positive, the skew is added to the result, and the derivative is negative, the skew is
  subtractecd.

## Command Map

The command map defines a semantic map between the game's commands and the controller buttons/axes
that the game expects. If you have altered your command mapping in the game, you can change the
default values by altering the bindings here. Modifiers that reference command names rather than
hard-coded button or axis values should work as intended no matter how the controller might be
remapped.

These commands are currently always simple. They associate one button press or axis event with one
command. They do not currently support associating simultaneous presses of controls with one
command. Currently, those states are modeled by checking for a condition when a particular signal
comes in. For example, "shoot" in TLOU2 is equivalent to "reload/toss" with the condition "aiming".

A command is defined by specifying a name and controllor signal to bind to it. It has two parameters,
both required:
 - name: The label used to refer to the action in the elsewhere in the configuration file.
   Command names must be unique, but you can define multiple names to point to the same signal.
   This allows for aliases, so you can use different command names in different contexts.
 - binding: The controller signal (button, axis, etc.) attached to this command. These labels
   ("L1", "X", etc.) are hard-coded values and cannot be altered in the configuration file.

_Examples:_

```toml
[[command]]
name = "interact"
binding = "TRIANGLE"

[[command]]
name = "crouch/prone"
binding = "CIRCLE"
```

The uniqueness requirement means that currently you cannot give a single command name to
something that achieves the same effect, but often that circumstance will only arise in specific
contexts that TCC cannot detect. For example, when opening a safe in TLOU2, either the d-pad or
the left joystick will turn the dial, but TCC has know way of knowing when you're in that mode.

## Game Conditions
Game conditions are used to test the state of events coming from the controller. There are two
types of conditions: transient and persistent. A transient condition polls the current state of the
controller. In other words, it will be true or false depending on what the user is doing at that
moment. A persistent condition is set to true when a particular condition arrives and remains
true until a different condition arrives that clears the condition.

Transient conditions are defined in the `while` key and the condition will be true as long as the
current state of the controller exceeds the defined threshold value and false otherwise. More than
one command can appear in the while parameter, but how multiple commands are processed depends on
the threshold type.

Persistent events define one command in set_on that sets the state and a different command in
set_off that clears it. Multiple commands within set_on and set_off are not currently supported.
  
Transient and persistent are mutually exclusive states, so a condition must define either the
`while` parameter or the `set_on`/`clear_on` parameters, but not both.

The following parameters are used to define conditions:
 - name: The name by which the condition is identified in the TOML file (_Required_)

 - while: A list of commands that must be true according to the real-time state of the controller

 - set_on: A command that will set a persistent trigger to true when its incomming state exceeds
   the threshold

 - clear_on: A command that will set a persistent trigger to false when its incomming state
   exceeds the threshold

 - threshold: The threshold that a signal's magnitude must reach to trigger the condition,
   expressed as a proportion of the maximum value. (Optional. Default = 1)
   The threshold must be a number between -1 and 1 and indicates a proportion of the maximum
   value. The threshold is ignored for buttons and will always be 1.

 - thresholdType: The test applied to the threshold. Must be one of the following:
    - `above`: (Default) The cndition is true if the magnitude of the signal is greater than or
      equal to the threshold.
    - `below`: The condition is true if the magnitude of signal is less than the threshold.
    - `greater` True if the signed value of the signal is greater than or equal to the threshold.
    - `less`: True if the signed value of the signal is less than the threshold.
    - `distance`: Checks the Pythagorean distance of both axes from center. Requires 2 axes in
      the condition

_Example_
```toml
threshold = 0.5
thresholdType = below
```

In the case above, the condition will be true whenever the axis value is less than 50% of the
maximum deflection in either direction.

The "greater" and "less" threshold types are the only two types where a negative proportion makes
sense. These types compare the _signed_ value of the axis signal, so you can therefore use these
to have conditions that only apply to one direction of an axis. Axis values are negative for
left/up/forward and positive for right/down/backwards, so the following example will produce true
if the user deflects a joystick more than 20% to the left or up but false for anything less in
those directions or anything at all in the right/down directions.

 ```toml
 threshold = -0.2
 thresholdType = less
 ```

When chosing threshold values for axes, note that joysticks almost always produce some noise,
and with older controllers that noise can be substantial (stick drift). Therefore it's
recommended to avoid 0 thresholds, as the condition will constantly trip even when the user
is not touching the controller.

For all threhold types except `distance`, if more than one command is defined in `while`, the
same threshold applies to all. This means the test will only work if each command has the same
maximum value. If you have different categories of signal or different thresholds, you should
create separate conditions and chain them together in a condition trigger.

## Sequences
A sequence holds a series of predefined events that are sent as a batch.  Sequences can be
defined as a preset sequence for convenience or as part of a sequence modifier.

There are two mandatory and two optional arguments in a sequnce definition:

- `event`: The type of event (_Required_)
  - `hold`: turn on the command
  - `release`: turn off the command
  - `press`: Emulate pressing and releasing a button rapidly

- `command`: The command affected by the event. Must be a command created in the command
  definitions.  (_Required_)

- `value`: A specific value to set the command signal to. If not supplied, the command is set
  to its maximum value: 1 for buttons, or the axis maximum for axes.  (_Optional_)

- `repeat`: Number of times to issue this command. If the command is repeated, the delay
  applies to each iteration of the command. (_Optional, default = 1_)

- `delay`: Time in seconds to delay after this command before executing the next one.
  (_Optional, default = 0)

## Menuing

Modifiers can set options from the menu. To do so, TCC needs to know the layout of the menu.

Note that the features currently implemented were primarily designed to accommodate the menuing
system of TLOU2 and is likely missing features needed to support some other games. Please submit
requests for new features if TCC cannot navigate your game's menu with the current feature set.

TCC navigates the menu by issuing sequences of commands. If you use menuing options, your
configuraiton file must define those sequences under the specific names listed here. The
commands executed by each sequence can be customized to the appropriate button presses
for each game.

Required sequences:
- `open menu` Open the main menu
- `menu select` Select a menu item
- `menu exit` Back out of the current menu. This takes you to the parent menu if in a sub-menu
- `menu up` Navigate upward one item in a vertical list of menu options
- `menu down` Navigate down one item in a vertical list of menu options

These sequences do not need to be defined if the game does not need them, but using the features
that reference them require sequences with these names to be defined.

- `tab left` Tab left through a sub-menu on the parent menu screen
- `tab right` Tab right through a sub-menu on the parent menu screen
- `option greater` Increase a multi-state menu option one level
- `option less` Descrease a multi-state menu option one level
- `confirm` Confirm an option after selected by the `menu select` sequence

The following parameters define the menuing system:

- `use_menu`: True/false value indicating whether we should use the menu system. If this is set
  to false, you can skip all the menu layout, but menu mods will also not be available.

- `remember_last` True/false value indicating whether the menu system remembers the last position
  in a menu/submenu where the user leaves the cursor. 

- `hide_guarded_items` True/false value. If true, items that are protected by a guard are "hidden"
  from the menuing system while the guard is in place. This doesn't necessarily mean that the
  menu option is invisible to the user, only that scrolling through the menu items skips over this
  option while it is guarded.

- `layout` - An array of inline tables that defines the menuing hierarchy. Each inline table is a
  menu item, representing one entry in the game's menu.

### Menu Items
Each row in the layout table defines a menu item. The following keys help tell the game how to
navigate to the item and how to interact with it one we've reached the item:

- `name`: A unique name for this menu item (_Required_)

- `type`: The category of menu item (_Required_). The available types are:

- `menu`: Item is a submenu that appears in a standard list. These items serve as parents
  to other menu items.
      
- `command`: The menu item executes a command (e.g., restart encounter) rather than allowing
  a choice from a number of values. This type is a terminal leaf in the menu tree and
  declaring it as a parent of another menu item will generate an error when the
  configuration file is parsed.
      
- `option`: The menu item selects an option from among a discrete number of values. The selected
  option is represented as an unsigned integer, where 0 represents the minimum possible value
  achieved by executing the #option_less sequence until the first item in the list is reached.
  A maximum value is not currently specified. We rely on the configuration file to have correct
  values. This type is a terminal leaf of the menu structure and declaring it as a parent of
  another menu item will generate an error when the configuration file is parsed.

- `guard`: Item is a boolean option that protects options that appear beneath it in the
  same menu. The subordinate options are only active if the guard option is true. Note that
  if the guard is actually a sub-menu that must be selected to reveal the options, you should
  implement that with the "guarded" option in the "menu" type.

- `parent`: The parent menu for this item. If omitted, this item belongs to the root (main)
  menu. Parents must be declared before any children that reference them, and only items of
  the menu type can serve as parents. (_Optional_)

- `offset`: The number of actions required to reach this item from the first item in the list.
  Must be an integer. Positive values navigate down, negative values navigate up. If the
  menu contains potentially hidden items, the offset should be the value required to reach the
  item when all menu items are unhidden. The engine will adjust automatically for any hidden
  items. (_Required_)

- `tab`: An integer indicating a group of items within a parent menu. Items in this group are
  displayed together, and you must navigate from tab to tab with a different command from the
  normal menu up/down items. The value of the integer indicates the number of tab button
  presses required to reach this group from the default tab. Positive values mean tab right,
  negative values mean tab left. (_Optional_)

- `hidden`: A boolean value which, if true, indicates that the menu item is not reachable in
  the list currently. "Hidden" items may be completely concealed from the user or they may
  simply be unreachable, in that scrolling over them cannot land on the item. Menu items can
  be hidden or revealed as a result of selecting other menu items. The default value is false.
  (_Optional_)

- `confirm`: A boolean value which, if true, indicates that setting this option requies
  a confirmation after setting/selection. When a menu item with confirm set is selected, the
   (_Optional_)
    
- `initial`: The starting value for the menu item, expressed as an unsigned integer. For
  menu items of the option and guard type, this value indicates the number of `option greater`
  actions required to reach this value from the minimum value. When used with a menu item, the
  initial state is interpreted as the currently selected child that has this item as a parent.
  If omitted, the default value is 0. (_Optional_)

- `guard`: String value. If present, this item is guarded by the menu item of this name.
  (_Optional_) The guard is a kind of pseudo-parent item. It is an option item that must be
  enabled before this menu item can be set.

- `counter`: references a menu item and increments the counter for that menu item when the
  mod referencing this item is initialized, and decrements the counter when the mod referencing
  this item is finished. 
    
- `counter_action`: Action to take after this menu option's counter changes. The currently
  supported values are:

  - `reveal`: Menu option is shown when the counter is non-zero, hidden when zero

  - `zero_reset`: Restore the menu value to default only when the counter is zero.

Parent menu items and guards must be declared in the configuration file before they are
referenced, but the definition order is otherwise unconstrained.

## Modifiers

A modifier is a specific alteration of a game's operation, for example disabling a particular
control or inverting the direction of a joystick movement.

Within the configuration file, each mod is defined in a [[modifier]] table.
The following keys are defined for all modifiers:

- `name`: A unique string identifying this mod (_Required_)

- `description`: An explanatation of the mod for use by the chat bot (_Required_)

- `type`: The class of the modifier (_Required_). The type must be one of the following values:

    - `cooldown`: Allow a command for a set time then force a recharge period before it can be used
      again.
    
    - `delay`: Wait for a set amount of time before sending a command.

    - `disable`: Block specific commands from being sent to the console.

    - `formula`: Alter the magnitude and/or offset of the signal through a formula

    - `menu`: Apply a setting from the game's menus.
    
    - `parent`: A mod that contains other mods as children.

    - `remap`: Change which controls issue which commands.

    - `repeat`: Repeat a particular command at intervals.

    - `scaling`: Alter the signal by a scaling factor.

    - `sequence`: Issue an arbitrary sequence of actions.

- `group`: A group (separate from the type) to classify the mod for voting by the chatbot. If
  omitted, the mod will be assigned to the default "general" group. (_Optional_)

- `unlisted`: If true, the modifier's name will not be sent to the chat bot and it therefore
  cannot be voted on by chat. However it can still be inserted directly or exist as a child
  mod of a parent that _is_ listed.

The following additional parameters are available for use by all classes of modifiers. Their
specific use, whether they are required, optional, or unused depends on the type of modifier.

- `applies_to`: A list of commands affected by the mod. The reserved name "ALL" is a shortcut
  to indicate that the mod applies to all signals.

- `while`: A list of conditions whose total evaluation returns true or false. The modifier
  will perform its action when the while evaluation returns true.

- `unless`: A list of conditions whose total evaluation returns true or false. The modifier
  will perform its action only when the unless evaluation returns false.

- `begin_sequence`:  A sequence of commands to issue when the mod is activated, before the
  mod begins any ongoing tasks.

- `finish_sequence`: A sequence of commands to issue when the mod is deactivated to clean
  up any chaos we may have caused and restore the game to the state it had before the mod
  was applied.

TODO: Currently, `while` and `unless` require all conditions in the list to be true (for
while) or false (for unless) simultaneously. Add `while_comparison` and `unless_comparison`
that specify the type of comparison made. (Default to remain `all`)
- `all`: All conditions must be true (for while) or false (for unless) simultaneously
- `any`: Any one condition must be true (for while) or false (for unless) at one time
- `none`: None of the defined conditions must be true (for while) or false (for unless)

Other keys specific to individual types of modifiers are described in the sections below.

### Cooldown Modifiers

Cooldown modifiers provide for a periodic block of a signal. They allow an action for a set
period of time and then block it until a cooldown period has expired.

The `applies_to` key inticates which commands experience the cooldown, and the `while` key lists
the conditions that must be true for the cooldown to start.

In addition to the general keys described above, the following keys  are defined for this class of modifier:

- `time_on`: Length of time, in seconds, to allow the command before entering cooldown. (_Required_)

- `time_off`: The cooldown period. Length of time, in seconds, to block the command. (_Required_)

- `trigger`: An array of signals that can trigger the cooldown. When any of these signals is non-zero,
  we check the while condition, and if that is true, the time_on period begins. (_Required_)

- `cumulative`: If true, the counter for time_on accumulates only when the while condition is true. If false,
  the allow time begins at the first trigger signal, regardless of the while condition.
  (_Optional; Default = false_)

_Example:_

```toml
[[modifier]]
name = "Bad Stamina"
description = "Running is disabled after 2 seconds and takes 4 seconds to recharge."
type = "cooldown"
groups = [ "movement" ]
applies_to = [ "dodge/sprint" ]
while = [ "movement", "sprint" ]
trigger = [ "dodge/sprint", "vertical movement", "horizontal movement" ]
cumulative = true
time_on = 2.0
time_off = 4.0
```

This example waits for the user to begin sprinting and then lets them accumulate 2 seconds of running. Because
of the cumulative flag, starting and stoping within this window won't reset the time_on time. Once 2 seconds
is exceeded, we block the sprint button for 4 seconds but not movement, so the user can only walk.

The following example illustrates a cooldown modifier without the cumulative flag set:

```toml
[[modifier]]
name = "Snapshot"
description = "You can only aim for 1 second at a time, so take that shot quickly."
type = "cooldown"
groups = [ "combat" ]
applies_to = [ "aiming" ]
while = [ "aiming" ]
trigger = [ "aiming" ]
time_on = 1.0
time_off = 3.0
```

Because the cumulative flag is false for this modifier, the cooldown cycle will run and be reset even if
the user stops aiming. So for this mod, the you get an aiming window of 1 second that expires even if you
stop aiming. Then aiming is blocked for another 3 seconds and the modifier resets and starts looking for
the trigger condition again. This way, if the user gets a shot off within the 1-second window and stops
aiming, the cooldown process runs to completion so that they can have another full second to aim the
next time. If cumulative were set to true here and they got a shot off in say .5 seconds, the cooldown
would never trigger and they would only have .5 seconds the next time.

### Delay Modifiers

This type of modifier introduces a delay between when an event comes into the Chaos engine and when it
is passed to the controller. This modifier type applies the delay unconditionally to the defined
signals. It does not accept the `while` or `unless` condition lists.

The `applies_to` key inticates which commands experience the delay.

The `delay` key indicates the time, in seconds, to delay the lsited commands.

_Example:_
```toml
[[modifier]]
name = "Shooting Delay"
description = "Introduces a 0.5 second delay to shooting and throwing"
type = "delay"
groups = [ "debuff", "combat" ]
applies_to = [ "shoot/throw" ]	
delay = 0.5
```

### Disable Modifiers
This type of modifier disables a specified list of commands from being passed to the controller.

The `applies_to` key inticates which commands are blocked. (_Required_)

This class of modifier accepts the `while` and `unless` keys as options, and if one of these is set,
the signal will only be blocked if the `while` condition is true or the `unless` condition is false.

In addition to the general keys defined above, this class of modifier accepts the following additional
key:

- `filter`: Specifies what portion values to block. (_Optional_) It accepts the following values:

  - `all`: All non-zero values are blocked, i.e., set to zero. (_Default_)

  - `below`: Any values less than 0 are blocked

  - `above`: Any values greater than 0 are blocked

Setting the `filter` allows you to block partial inputs, for example stopping the user from moving
backwards.

_Example:_
```toml
[[modifier]]
name = "No Backward Movement"
description = "Moving backwards is not allowed"
type = "disable"
groups = [ "movement" ]
applies_to = [ "vertical movement" ]
filter = "above"
```

### Formula Modifier
Formula modifiers apply an offset to incoming signals according to a specified, time-dependent
formula. Currently, we don't have the ability to parse a general formula, so you must select
from a set of pre-defined formula types.

The `applies_to` key inticates which commands are affected. (_Required_) Applying a formula
only makes sense for axis signals. While you _can_ apply a formula to a single axis, to see
the pattern implied by the names circle or eight_curve, applies_to should contain pairs of
axis signals.

This class of modifier accepts the `while` and `unless` keys as options, and if one of these is set,
the signal will only be modified if the `while` condition is true or the `unless` condition is false.

The following additional keys are recognized by this modifier type:

- `formula_type`: The specific formula applied. The value must be one of the following:
    - `circle`: Offset changes in a circular pattern
    - `eight_curve`: Offset follows a figure-8 curve
    - `janky`: Offset follows an irregular pattern

- `amplitude`: Proportion of the maximum signal by which to multiply the formula (_Optional. Default = 1_)

- `period_length`: Time in seconds before the formula completes its cycle (_Optional. Default = 1_)

The basic output of the formulas will always be between =1 and 1 (`janky` has a narrower range) and this
value is multiplied by the maximum signal derived from the amplitude. 

The `janky` formula is used to generate a "drunken walk".

_Example:_
```toml
[[modifier]]
name = "Mega Scope Sway"
description = "Good luck landing shots"
type = "formula"
groups = [ "combat", "view" ]
applies_to = [ "horizontal camera", "vertical camera" ]
while =  [ "aiming" ]
formula_type = "eight_curve"
period_length = 0.5 
amplitude = 0.5
```

### Menu Modifiers
Menu modifiers execute specified commands in the game's menu system. They can be used, for example,
to enable or disable particular game settings. Menu modifiers do not accept the `while` or `unless`
condition lists.

Menu modifiers execute once at the beginning of the mod's lifespan to set the menu settings and again
at the end of the mod's lifespan to restore the settings to their original state.

The syntax of menu modifiers is rather simple, because most of the logic to navigate through the
menu and set options properly is encapsulated in the menu layout described above.

Apart from the normal keys common to all modifiers, each menum modifier must contain a `menu_items` key.
This is an array of inline tables having the following format:

- `entry`: The name of a menu item defined in the menu layout. (_Required_)

- `value`: For option menu items, the value to set the option to. This value represents a number from
  0 to the option's maximum value, with 0 representing the minimum option (in the example TLOU2 file,
  this is the left-most option). The modifier will calculate the number of `option greater` or
  `option less` sequences required to reach this value from the current setting automatically. Note
  that the menu sytem does not enforce a maximum legal value for options, so if your value does not
  make sense for that option, you will likely have errors. In TLOU2, for example, the menu scrolls
  around for most but not all options.

The key `reset_on_finish` can be used to override whether the menu system should be reset, moving the
menu cursor to the top of the menu, after setting up and tearing down the mod. By default this is
true, but you should set it to false for options that make it unnecessary. (_Optional, default = true_)

_Examples:_

```toml
[[modifier]]
name = "Restart Checkpoint"
description = "Best served prior to the end of a long encounter."
groups = [ "debuff" ]
type = "menu"
menu_items = [{entry = "restart" }]
reset_on_finish = false

[[modifier]]
name = "Close POV"
description = "Objects in mirror are closer than they appear."
type = "menu"
groups = [ "view" ]
menu_items = [{entry = "camera distance", value = 0},
            {entry = "field of view", value = 0}]
```

### Parent Modifiers
A parent modifier is a modifier that executes one or more other modifiers (child modifiers).
The children can be drawn from a specific list or selected at random. Working from a specified
list lets you create modifiers by chaining together multiple modifiers of different types to
achieve effects that would not be possible within a single modifier class.

In addition to the standard keys for all modifiers, the following keys are 
- `children`: A list of specific child modifiers. These modifiers must be defined earlier in the
  configuration file.

  `random`: A boolean value. If true, selects child mods at random. (_Optional, default = false_)

  `value`: If `random` is true, the number of child mods to select at random.
  (_Optional, default = 1_)

Parent modifiers do not accept the `while` or `unless` condition lists.

The random-selection process will exclude picking any parent modifiers that use random
selection, so recursively selecting the oneself is prevented.

The `children` key is optional if `random` is true, but if it is false you must have at least
one modifier listed there.

During the lifetime of the mod, child modifiers will be processed in the order that they are
specified in the configuration file. If both specific child mods and random mods are specified,
the random mods will be processed after the explicit child mods.

_Examples:_

```toml
[[modifier]]
name = "Chaos"
description = "Applies 5 random modifiers"
type = "parent"
random = true
value = 5

[[modifier]]
name = "Drunk"
description = "Random joystick motion. Also don't push things too far, or you may stumble (go prone)"
type = "parent"
groups = [ "movement", "view" ]
children = ["Random Movement", "Stumblebum"]
```

### Remap Modifiers
A remap modifier changes the signals comming from the controller so that one or more signals from
the controller appear to the game to be different signals.

It's important to note that remapping operates on the actual controller signals, not on the
comands, which are semantic mappings of signals. The reason for this difference is that remaps
are processed differently from ordinary modifiers. All active remaps are stored in a table that is
processed at the start of the modifier loop, before any other modifiers have done their work.
Ordinary modifiers therefore operate on the remapped signals. Since the remapped signals are what
the game will see, having other modifiers work on them will preserve the intended semantics of
those modifiers. For example, if touchpad aiming is active (touchpad remapped to the right
joystick) while camera movement is being blocked, you will be unable to move the camera with
the touchpad.

In addition to the standard keys for all modifiers, the following keys are used to define remap
modifier:

- `disable_signals`: An array of signals that should be cleared (set to 0) when the mod is
  initialized.

- `remap` An array of the signals that will be remapped. Each remap definition should be contained
  in an inline table with the following possible keys:
    - `from` (_required_): The signal we intercept from the controller

    - `to` (_required_): The signal we pass to the console. For axis-to-button mapping, this is the
      button that receives positive signals.

    - `to_neg` (_optional_): For axis-to-button mapping, this is the button that receives negative
      signals.
      
    - `invert` (_optiopnal_): If true, flip the sign of the signal. Up becomes down, left becomes
      right, etc.

    - `threshold` (_optional_): The absolute value of the signal that must be received in order to
      trigger the remapping.
    
    - `to_min` (_optional_): When mapping buttons-to-axis, if true, the axis is set to the joystick
      minimum. If false, it is set to the joystick maximum.
    
    - `sensitivity` (_optional_): A divisor that scales the incomming accelerometer signal down.
    
- `random_remap`: A list of controls that will be randomly remapped among one another. (_Optioonal_)

A remap modifier may specify *either* a fixed list of remaps with the "remap" key *or* a list
of controls that will be randomly scrambled with the "random_remap" key, but not both.

When using random_remap, note that you can only scramble signals within the same basic class:
buttons or axes. Trying to scramble across input types will almost certainly break things,
since an axis must map to two buttons and vice versa.

When planning remapping, you must consider the type of signal. Mapping within a single class (e.g.,
button to button or axis to axis) requires only the `from` and `to` keys. To map between buttons and
axes requires 2 buttons for each axis.

Setting `to` or `to_neg` to "NOTHING" has the effect of blocking the signal from reaching the
console. When a remapping means that the old signal should not be used at all (e.g., using the
touchpad in place of a joystick), it's important to include lines to block the old signals.

### Repeat Modifiers
Repeat modifiers cycle a sequence of commands on and off repeatedly throughout the lifetime of
the modifier. These sequences can be set to repeat unconditionally or to begin only when triggered by a
particular input condition.

In addition to the keys defined for all modules, the following keys are available for this
modifier type:

- `applies_to`: An array  of the commands affected by the mod. (_Required_)

- `force_on`: An array of integers, corresponding to the commands in `applies_to`, indicating
  the values to set the commands to during the on period.

- `time_on`: Time in seconds to apply `force_on` values. (_Required_)

- `time_off`: Time in seconds to turn the command off. (_Required_)

- `repeat`: Number of times to repeat the on/off event before starting a new cycle. (_Optional_)

- `cycle_delay`: Time in seconds to delay after one iteration of a cycle (all the repeats) before
  starting the next cycle. (_Optional, default = 0_)

- `block_while_busy`: List of commands to block while the sequence is on.

### Scaling Modifiers
Scaling modifiers tranform the incoming signal through a linear formula. If the value of the
incoming signal is `x`, the result will be mx + b, wheree m is an amplitude and b is an
offset. The output is also clipped to the min/max values of the signal.

In addition to the keys defined for all modules, the following keys are available for this
modifier type:

- `applies_to`: A vector of commands affected by the mod. (_Required_)

- `amplitude`: Amount to multiply the incoming signal by (_Optional, default = 1_)

- `offset`: Ammount to add to the incomming signal (_Optional, default = 0_)

_Examples:_

```toml
[[modifier]]
name = "Sideways Moonwalk"
description = "Go left to go right and go right to go left"
type = "scaling"
groups = [ "movement" ]
applies_to = [ "horizontal movement" ]
amplitude = -1

[[modifier]]
name = "Max Sensitivity"
description = "Goodbye precision aiming. Joystick postions multiplied by 5."
type = "scaling"
groups = [ "movement", "view" ]
applies_to = [ "horizontal movement", "vertical movement", "horizontal camera", "vertical camera"  ]
amplitude = 5.0
```

### Sequence Modifiers
Sequence modifiers execute arbitrary sequences of commands, either once or repeatedly throughout
the lifetime of the modifier. Seperate sequences can be defined to execute at the start and end
of the mod's lifetime.

In addition to the keys availabel for all modifiers, the following keys are available for thie
type of modifier:

- `block_while_busy`: Array of commands to disable while the sequence is being executed.
  The value can be either an array of commands or the single string "ALL". The latter is a
  shortcut to disable all defined game commands. (_Optional_)

- `begin_sequence`: A sequence of commands to execute once when the mod is enabled. This is an
  array of inline arrays. If you want the mod to execute a one-time action, put it here.
  (_Optional_)

- `repeat_sequence`: A sequence of commands to execute repeatedly for the lifetime of the mod
  This is an array of inline arrays. (_Optional_)
   
- `finish_sequence`: A sequence of commands to execute once when the mod is removed. This is an
  array of inline arrays. (_Optional_)

- `trigger`: An array of commands that, in conjunction with the while/unless conditions, will
  trigger the `repeat_sequence`. When the mod receives any of the commands listed in trigger,
  it checks if the while/unless conditions return true, and if they do, the sequence begins.
  If the trigger is empty, the `repeat_sequence` actions will loop for the lifetime of the
  mod. (_Optional_)

- `start_delay`: Time in seconds to wait after a cycle begins (or after the trigger condition
  becomes true) before sending the `repeat_sequence`. (_Optional, default = 0_)

- `cycle_delay`: The time in seconds between the conclusion of one `repeat_sequence` and the time
  we reset the sequence and begin the next iteration (or begin waiting for the next trigger).
  (_Optional, default = 0_)

This class of modifier accepts the `while` and `unless` keys as options, and if one of these is set,
the signal will only be modified if the `while` condition is true or the `unless` condition is false.

_Examples:_

```toml
[[modifier]]
name = "Double Tap"
description = "It's Rule #2. Everytime a shot is fired, another occurs in quick succession. For Rule #1, see Leeroy Jenkins."
type = "sequence"
groups = [ "combat" ]
block_while_busy = [ "aiming", "shoot" ]
while = [ "aiming" ]
trigger = [ "shoot" ]
start_delay = 0.2
repeat_sequence = [
  { event = "release", command = "shoot", delay = 1.0 },
  { event = "hold", command = "shoot", delay = 0.2 },
  { event = "release", command = "shoot" }
  ]

[[modifier]]
name = "Rubbernecking"
description = "Woah! What's behind you? Invokes a periodic, 180-degree quick turn."
type = "sequence"
groups = [ "movement" ]
block_while_busy = [ "jump", "aiming", "vertical movement", "horizontal movement" ]
start_delay = 6.0
cycle_delay = 0.1
repeat_sequence = [
   { event = "release", command = "jump" },
   { event = "release", command = "aiming" },
   { event = "release", command = "horizontal movement" },
   { event = "hold", command = "vertical movement", delay=0.1}, # default is JOYSTICK_MAX = 127, which we want
   { event = "hold", command = "jump", delay=0.1},
   { event = "release", command = "jump"}
   ]
```
