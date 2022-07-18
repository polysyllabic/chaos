from twitchbot import (Command, Message, InvalidArgumentsError)
import chaosface.config.globals as config

MANAGE_CREDIT_PERMISSION = 'manage_credits'

#@Command('setcreditname', permission=MANAGE_CREDIT_PERMISSION, syntax='<new_name>',
#         help='sets the name for chaos modifier credits')
#async def cmd_set_currency_name(msg: Message, *args):
#    if len(args) != 1:
#        raise InvalidArgumentsError(reason=translate('missing_required_arguments'), cmd=cmd_set_currency_name)
#
#    set_currency_name(msg.channel_name, args[0])
#    await msg.reply(translate('currency_name_set', currency_name=get_currency_name(msg.channel_name).name))

def validate_user_from_message(user: str, msg: Message):
  """
  If user name starts with @, return without @. If no @, check against users in chat and return the name
  if that user is chatting, otherwise None.
  """
  if user[0] == '@':
    return user.lstrip('@').lower()
  return user.lower() if user in msg.channel.chatters else None

@Command('credits', syntax='(target)', help='gets the caller\'s (or target\'s if specified) balance')
async def cmd_get_balance(msg: Message, *args):
  if args:
    target = args[0].lstrip('@')
  else:
    target = msg.author
  await msg.reply(config.relay.get_balance_message(target))

# Manually add a mod credit to a user's account
@Command('addcredits', permission=MANAGE_CREDIT_PERMISSION, syntax='<target> (amount)',
  help='Add the specified amount (default 1) to the target\'s credit balance')
async def cmd_add_credits(msg: Message, *args):
  if len(args) < 1:
    raise InvalidArgumentsError('Must specify a target to receive the credits', cmd=cmd_add_credits)
  
  target: str = validate_user_from_message(args[0], msg)
  if not target:
    raise InvalidArgumentsError(f'The user {args[0]} doesn\'t appear to be in chat. Start the name with an @ if you\'re sure you want this person to receive the credits.', cmd=cmd_add_credits)

  amount = 1
  if len(args) > 1:
    try:
      amount = int(args[1])
    except ValueError:
      raise InvalidArgumentsError('The amount must be a valid integer', cmd=cmd_add_credits)
    if amount <= 0:
      raise InvalidArgumentsError('Ammount to add must be positive', cmd=cmd_add_credits)

  config.relay.step_balance(target, amount)
  await msg.reply(config.relay.get_balance_message(target))


@Command('setcredits', permission=MANAGE_CREDIT_PERMISSION, syntax='<target> <amount>',
  help='Set a user\'s credit balance to a specified amount')
async def cmd_set_credits(msg: Message, *args):
  if len(args) != 2:
    raise InvalidArgumentsError('Must specify a target and an amount', cmd=cmd_set_credits)
  
  target: str = validate_user_from_message(args[0], msg)
  if not target:
    raise InvalidArgumentsError(f'The user {args[0]} doesn\'t appear to be in chat. Start the name with an @ if you\'re sure you want this person to receive the credits.', cmd=cmd_set_credits)

  try:
    amount = int(args[1])
  except ValueError:
    raise InvalidArgumentsError('The amount must be a valid integer', cmd=cmd_set_credits)
  if amount < 0:
    raise InvalidArgumentsError('The ammount cannot be negative', cmd=cmd_set_credits)

  user = args[0]
  new_balance = int(args[1])
  config.relay.set_balance(user, new_balance)
  await msg.reply(config.relay.get_balance_message(msg.author))
  

@Command('givecredits', syntax='<target> (amount)',
  help='Give credits to another user. If amount is omitted, gives 1 credit')
async def cmd_give_credits(msg: Message, *args):
  """
  Transfer credits between users
  """
  if len(args) < 1:
    raise InvalidArgumentsError('Must specify a target to receive the credits', cmd=cmd_give_credits)

  target: str = validate_user_from_message(args[0], msg)
  if not target:
    raise InvalidArgumentsError(f'The user {args[0]} doesn\'t appear to be in chat. Start the name with an @ if you\'re sure you want this person to receive the credits.', cmd=cmd_give_credits)

  amount = 1
  if len(args) > 1:
    try:
      amount = int(args[1])
    except ValueError:
      raise InvalidArgumentsError('The amount must be a valid integer', cmd=cmd_give_credits)
    if amount < 1:
      raise InvalidArgumentsError('The ammount must be positive', cmd=cmd_give_credits)
  
  # Check for balance. Streamer can give infinite credits
  donor_balance = config.relay.get_balance(msg.author)
  plural = 's' if donor_balance == 1 else ''
  if donor_balance < amount and msg.author != msg.channel_name:
    await msg.reply(f'@{target} You do not have enough credits. Your current balance is {donor_balance} credit{plural}.')
    return

  config.relay.step_balance(target, amount)
  config.relay.step_balance(msg.author, -amount)
  plural = 's' if amount == 1 else ''
  await msg.reply(f'@{msg.author} has given {target} {amount} mod credit{plural}')


