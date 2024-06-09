# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Handle PubSub events
"""
import logging

from twitchbot import (Mod, PubSubData, PubSubPointRedemption, PubSubTopics,
                       get_pubsub)
from twitchbot.pubsub.bits_model import PubSubBits

import chaosface.config.globals as config


class PubSubSubscriber(Mod):
  async def on_connected(self):
    await get_pubsub().listen_to_channel(config.relay.get_attribute('channel'),
      [PubSubTopics.channel_points, PubSubTopics.bits],
      access_token=config.relay.get_attribute('pubsub_oauth'))

  async def on_pubsub_received(self, raw: PubSubData):
    logging.debug(raw.raw_data)


  async def on_pubsub_custom_channel_point_reward(self, raw: PubSubData, data: PubSubPointRedemption):
    logging.debug(f'User {data.user_display_name} redeemed "{data.reward_title}"')
    channel = await data.get_channel()
    if (data.reward_title == config.relay.get_attribute('points_reward_title')):
      if config.relay.get_attribute('points_redemptions'):
        balance = config.relay.step_balance(data.user_display_name, 1)
        logging.debug(f'{data.user_display_name} has a new balance of {balance}')
        await channel.send_message(config.relay.get_attribute('msg_user_balance').format(data.user_display_name, balance))
      else:
        await channel.send_message('Points redemptions currently disabled')

  async def on_pubsub_bits(self, raw: PubSubData, data: PubSubBits):
    # Ignore anonymous cheers
    if not data.user_id:
      return
    logging.debug(f'{data.username} donated {data.bits_used} bits')
    channel = await data.get_channel()
    if config.relay.get_attribute('bits_redemptions') and data.bits_used >= config.relay.bits_per_credit:
      credits = (int) (data.bits_used / config.relay.bits_per_credit) if config.relay.multiple_credits else 1
      balance = config.relay.step_balance(data.username, credits)
      await channel.send_message(config.relay.get_attribute('msg_user_balance').format(data.user_display_name, balance))

