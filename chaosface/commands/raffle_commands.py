import time
from asyncio import sleep, ensure_future
from random import choice

from twitchbot import (
  Channel,
  Command,
  Message,
  InvalidArgumentsError,
)

import chaosface.config.globals as config

MANAGE_RAFFLE_PERMISSION = 'manage_raffles'

in_raffle = []

@Command('rafflemsg', permission=MANAGE_RAFFLE_PERMISSION, syntax='<message>')
async def cmd_set_raffle_message(msg: Message, *args):
  if not args:
    raise InvalidArgumentsError('', cmd=cmd_start_raffle)
  raffle_msg = ' '.join(args)
    

@Command('raffle', permission=MANAGE_RAFFLE_PERMISSION, syntax='(time)',
         help='Opens a raffle, with the winner receiving a modifier credit. If time omitted, uses default raffle length')
async def cmd_start_raffle(msg: Message, *args):
  if config.relay.raffle_open:
    await msg.reply('A raffle is already open')
    return
  length = config.relay.get_attribute('raffle_time')
  if args:
    try:
      length = int(args[0])
    except ValueError:
      raise InvalidArgumentsError('The time must be an integer number of seconds', cmd=cmd_start_raffle)
    if length < 30:
      raise InvalidArgumentsError('Raffle time must be at least 30 seconds', cmd=cmd_start_raffle)

  task = ensure_future(_raffle_timer_loop(msg.channel, length))


@Command('join')
async def cmd_join_raffle(msg: Message, *args):
  if not config.relay.raffle_open:
    return
  if msg.author not in in_raffle:
    in_raffle.append(msg.author)

async def _raffle_timer_loop(channel: Channel, raffle_length):
  in_raffle = []
  config.relay.raffle_open = True
  config.relay.raffle_start_time = time.time()
  time_elapsed = 0
  while time_elapsed < raffle_length:
    if time_elapsed == 0:
      status = 'has begun'
    else: 
      status = f'will close in {int(raffle_length - time_elapsed)} seconds'
    notice: str = config.relay.get_attribute('msg_raffle_open')
    await channel.send_message(notice.format(status=status))

    # sleep until it's time for the next message
    await sleep(config.relay.get_attribute('raffle_message_interval'))
    time_elapsed = time.time() - config.relay.raffle_start_time
  
  # Timer expired
  config.relay.raffle_open = False
  if not in_raffle:
    await channel.send_message('No one joined the raffle :(')
  else:
    winner = choice(in_raffle)
    config.relay.step_balance(winner, 1)
    await channel.send_message(f'Congratulations {winner}! You\'ve won a modifier credit.')
