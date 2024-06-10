# Twitch Congrols Chaos Configuration Files
TCC configuration files are TOML files, which are written in plain text and can be modified by any
editor.

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
