/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#ifndef GAMEMENU_HPP
#define GAMEMENU_HPP
#include <iostream>
#include <toml++/toml.h>

#include "sequence.h"

class GameMenu {
private:
  GameMenu();

  toml::table menu;
  
  Chaos::Sequence sequence;
    
  // used to save the menu state

  void selectMenu()
  void deselectMenu();
    
  void moveToMenuItem( int difference );
	
  void sendSequence(Chaos::Controller* controller);
    
public:
  static Menuing* getInstance();

  void setMenuItem(bool enable, Chaos::Controller* controller);
  
};


#endif
