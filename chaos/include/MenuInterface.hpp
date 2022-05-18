/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#pragma once
#include <string>
#include <memory>

namespace Chaos {
  class MenuItem;
  class Sequence; 

  class MenuInterface {
  public:
    virtual std::shared_ptr<MenuItem> getMenuItem(const std::string& name) = 0;
    //virtual void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) = 0;
    //virtual void restoreMenuState(std::shared_ptr<MenuItem> item) = 0;
    virtual void correctOffset(std::shared_ptr<MenuItem> sender) = 0;
    virtual void addSequence(Sequence& sequence, const std::string& name) = 0;
    virtual void addSelectDelay(Sequence& sequence) = 0;
  };

};