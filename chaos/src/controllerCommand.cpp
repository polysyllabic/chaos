/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "controllerCommand.hpp"

using namespace Chaos;

ControllerCommand::ControllerCommand(GPInputType gptype, uint8_t first, uint8_t second) :
  input_type{gptype}, button_id{first}, second_id{second} {
  switch (gptype) {
  case GPInputType::BUTTON:
    index = ((int) TYPE_BUTTON << 8) + (int) first;
    break;
  case GPInputType::HYBRID:
    index = ((int) TYPE_BUTTON << 8) + (int) first;
    index2 = ((int) TYPE_AXIS << 8) + (int) second;
    break;
  default:
    index = ((int) TYPE_AXIS << 8) + (int) first;
  }
}

std::unordered_map<std::string, GPInput> ControllerCommand::buttonNames;

/**
 * Initialize the static definitions of button/axis mappings. This defines the strings that must
 * be used in the TOML file to identify the particular controller command.
 */
void ControllerCommand::initialize() {

  buttonNames["X"] =  GPInput::X;
  buttonNames["CIRCLE"] =  GPInput::CIRCLE;
  buttonNames["TRIANGLE"] =  GPInput::TRIANGLE;
  buttonNames["SQUARE"] =  GPInput::SQUARE;
  buttonNames["L1"] =  GPInput::L1;
  buttonNames["R1"] =  GPInput::R1;
  buttonNames["L2"] =  GPInput::L2;
  buttonNames["R2"] =  GPInput::L2;
  buttonNames["SHARE"] =  GPInput::SHARE;
  buttonNames["OPTIONS"] =  GPInput::OPTIONS;
  buttonNames["PS"] =  GPInput::PS;
  buttonNames["L3"] =  GPInput::L3;
  buttonNames["R3"] =  GPInput::R3;
  buttonNames["TOUCHPAD"] =  GPInput::TOUCHPAD;
  buttonNames["TOUCHPAD_ACTIVE"] =  GPInput::TOUCHPAD_ACTIVE;
  buttonNames["TOUCHPAD_ACTIVE_2"] =  GPInput::TOUCHPAD_ACTIVE_2;
  buttonNames["LX"] =  GPInput::LX;
  buttonNames["LY"] =  GPInput::LY;
  buttonNames["RX"] =  GPInput::RX;
  buttonNames["RY"] =  GPInput::RY;
  buttonNames["DX"] =  GPInput::DX;
  buttonNames["DY"] =  GPInput::DY;
  buttonNames["ACCX"] =  GPInput::ACCX;
  buttonNames["ACCY"] =  GPInput::ACCY;
  buttonNames["ACCZ"] =  GPInput::ACCZ;
  buttonNames["GYRX"] =  GPInput::GYRX;
  buttonNames["GYRY"] =  GPInput::GYRY;
  buttonNames["GYRZ"] =  GPInput::GYRZ;
  buttonNames["TOUCHPAD_X"] =  GPInput::TOUCHPAD_X;
  buttonNames["TOUCHPAD_Y"] =  GPInput::TOUCHPAD_Y;
  buttonNames["TOUCHPAD_X_2"] =  GPInput::TOUCHPAD_X_2;
  buttonNames["TOUCHPAD_Y_2"] =  GPInput::TOUCHPAD_Y_2;

};
