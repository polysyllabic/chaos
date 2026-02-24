Commands
--------
Vote for modifiers with a number only (1, 2, 3)

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
* !active -- Alias for !mods active
* !mods voting -- List modifiers currently up for a vote
* !candidates -- Alias for !mods voting

Modifier Management Commands (require manage_modifiers permission):
* !enable <mod name> -- Enable a modifier for voting/selection
* !disable <mod name> -- Disable a modifier for voting/selection
* !resetmods -- Request an immediate reset of active modifiers

Voting Commands (require manage_voting permission):
* !startvote (time) -- Manually open a new vote. If time omitted, default vote time is used
* !newvote (time) -- Alias for !startvote
* !endvote -- End an open vote immediately and choose a winner

Modifier Credit Commands:
* !credits (user) -- Reports the number of modifier credits that the user currently has
* !addcredits <user> (amount) -- Add credits to user's balance. If amount omitted, add 1. Requires 'manage_credits' permission.
* !setcredits <user> <amount> -- Sets user's balance to the specified amount. Requires 'manage_credits' permission.
* !givecredits <user> (amount) -- Give some of your modifier credits to the specified user. If amount omitted, transfers 1 credit.
* !givecredit <user> (amount) -- Alias for !givecredits

Raffle Commands:
* !join -- Enters the user into an open raffle
* !joinchaos -- An alias for !join
* !raffle (time) -- Start a raffle for a modifier credit (if time is omitted, default raffle time is used) Requires 'manage_raffles' permission
* !chaosraffle -- An alias for !raffle

Manage Permissions (require manage_permissions permission):
* !addgroup <group> Create a new permission group.
* !addmember <group> <user> Add a user to a permission group.
* !addperm <group> <permission> Add a permission to a group.
* !delgroup <group> Remove an existing permission group.
* !delmember <group> <user> Remove a user from a group.
* !delperm <group> <permission> Remove a permission from a group.
* !permgroups List all permission groups.
* !permgroup <group> List permissions and users in this group.

Permissions
-----------

* admin: Can execute all chat commands. (Streamer is in this group by default)
* manage_raffles: Start raffles
* manage_credits: Set users' modifier-credit balances to arbitrary values
* manage_modifiers: Update modifiers directly
* manage_voting: Start/end votes manually
* manage_permissions: Create/delete permission groups and add/remove users and permissions from them
