
_chaos_decript = ("Twitch Controls Chaos lets chat interfere with a streamer playing a PlayStation "
  "game. It uses a Raspberry Pi to modify controller input based on gameplay modifiers that chat "
  "votes on. For a video explanation of how Chaos works, see here: "
  "https://www.twitch.tv/blegas78/clip/SmellyDepressedPancakeKappaPride-llz6ldXSKjSJLs9s")

_chaos_how_to_vote = ("Each voting cycle you can choose one of {options} modifiers. Type the number "
  "of the mod you want (and nothing else) in chat. You can vote once per cycle. Votes are counted "
  "while the game is paused. ")

_chaos_how_to_redeem = ("With a mod credit, apply a modifier of your choice immediately "
"(subject to a {cooldown}-second cooldown) with '!apply <mod name>' (ignored if in "
"cooldown), or queue the mod to run once the cooldown is over with '!queue <mod name>'")

_chaos_credit_methods = ("Earn mod credits in these ways: {activemethods}. Give another user one "
  "of your credits with '!givecredit <username>'")

# Dictionary to hold all the default settings. Used when the json configuration file does not exist
# or when we reset to default. All times are in seconds. 
chaosDefaults = {
  'obs_overlays': True,
  'use_gui': True,
  'ui_rate': 20.0,               # Hz
  'ui_port': 80,
  'active_modifiers': 3,
  'modifier_time': 180.0,
  'softmax_factor': 33,
  'vote_options': 3,
  'info_cmd_cooldown': 15.0,
  'apply_mod_cooldown': 600.0,
  'points_redemption': False,
  'bit_redemption': False,
  'bits_per_credit': 500,
  'multiple_credits': True,      # Multiple credits from larger donations
  'raffle_enabled': False,
  'raffle_time' : 60.0,
  'raffle_reminder_interval': 15.0,
  'raffle_cooldown': 600.0,
  'announce_mods': False,
  'voting_enabled': True,
  'proportional_voting': True,
  'log_votes': True,
  'about_chaos_descrip': _chaos_decript,
  'active_mods_msg': "The active mods are ",
  'voting_mods_msg': "You can currently vote for these mods: ",
  'how_to_vote_msg': _chaos_how_to_vote,
  'proportional_vote_msg': "The chances of a mod winning are proportionally to the number of votes it receives.",
  'majority_vote_msg': "The mod with the most votes wins. In the case of ties, the winner is selected randomly.",
  'no_voting_msg': "Voting is currently disabled.",
  'how_to_redeem': _chaos_how_to_redeem,
  'how_to_earn': _chaos_credit_methods,
  'points_msg': "redeeming channel points",  
  'bits_msg': "donating bits (1 credit for {restriction} bits in a single donation)",
  'multiple_credits_msg': "every {amount}",
  'no_multiple_credits_msg': "{amount} or more",
  'raffle_msg': "winning raffles",
  'no_redemption': "Mod credits and redemption are currently disabled.",
  'mod_not_found': "I can't find a mod named {modname}.",
  'mod_text_style': "color: white; text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;",
  'title_text_style': "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;",
  'progress_style': "background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; width:1050px;",
}