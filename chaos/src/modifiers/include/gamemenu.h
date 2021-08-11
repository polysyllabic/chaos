/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78
*
* Refactor the hard-coded menuing system so that we can setup things with
* a toml configuration file.
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#ifndef GAMEMENU_H
#define GAMEMENU_H
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
