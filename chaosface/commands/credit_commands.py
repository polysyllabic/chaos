from twitchbot import Command, Message
import chaosface.config.globals as config

@Command('credits')
async def cmd_get_balance(msg: Message, *args):
  """ Report user's current mod-credit balance """
  await msg.reply(config.relay.get_balance_message(msg.author))

# Manually add a mod credit to a user's account
@Command('addcredit', permission='add_credits')
async def cmd_add_mod_credit(msg: Message, *args):
  if not args:
    await msg.reply('Usage: !addcredit <user>')
    return
  config.relay.step_balance(args[0], 1)
  await msg.replay(config.relay.get_balance_message(msg.author))


@Command('setbalance', permission='admin')
async def cmd_set_balance(msg: Message, *args):
  if not args or len(*args) < 2 or not args[1].isdigit():
    await msg.reply('Usage: !setbalance <user> <balance>')
    return
  user = args[0]
  new_balance = int(args[1])
  config.relay.set_balance(user, new_balance)
  await msg.replay(config.relay.get_balance_message(msg.author))
  

@Command('givecredit')
async def cmd_give_credit(msg: Message, *args):
  """
  Transfer a credit between users
  """
  if not args:
    await msg.reply('Usage: !givecredit <user>')
    return
  if config.relay.get_balance(msg.author) == 0:
    await msg.reply(config.relay.get_attribute('msg_no_credits'))
    return
  config.relay.step_balance(args[0], 1)
  config.relay.step_balance(msg.author, -1)

