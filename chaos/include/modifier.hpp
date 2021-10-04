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
#ifndef MODIFIER_HPP
#define MODIFIER_HPP

#include <string>
#include <functional>
#include <map>
#include <mogi/math/systems.h>

#include "controller.hpp"

namespace Chaos {

  class ChaosEngine;
  /**
   * Implementation of the base Modifier class for TCC
   *
   * A modifier is a specific alteration of a game's operation, for example
   * disabling a particular control, or inverting the direction of a joystick
   * movement. Specific classes of modifiers are implemented as subclasses
   * of this, the generic modifier class.
   */
  class Modifier {
    friend ChaosEngine;
  protected:
    /**
     * Default constructor
     */
    Modifier();
    
    Mogi::Math::Time timer;
    Controller* controller;
    ChaosEngine* chaosEngine;
    /**
     * Contains "this" except in cases where there is a parent modifier
     */
    Modifier* me; 
    /**
     * Amount of time the engine has been paused.
     */
    double pauseTimeAccumulator;
	
  public:
    /**
     * Map linking mods to a specific name
     */
    static std::map<std::string, std::function<Modifier*()>> factory;
    static Modifier* build( const std::string& name );
    static std::string getModList( double timePerModifier );
	
    void setController(Controller* controller);
    void setChaosEngine(ChaosEngine* chaosEngine);
	
    void _update(bool isPaused);	// ChaosEngine call this, which then calls virtual update();
	
    virtual void begin();	        // called when first instantiated
    virtual void update();	// called regularly
    virtual void finish();	// called on cleanup
    virtual const char* description(); // A short description of this mod, for Twitch bot response
	
    double lifetime();
	
    virtual bool tweak( DeviceEvent* event );
	
    void setParentModifier(Modifier* parent);

    // Directly read the current controller state
    bool buttonPressed(ButtonID button);

    // Alter event comming from the controller
    void buttonOff(DeviceEvent* event, ButtonID button);
    void buttonOff(DeviceEvent* event, ButtonID button, ButtonID alsoPressed);
    void buttonOn(DeviceEvent* event, ButtonID button);
  
    void axisOff(DeviceEvent* event, AxisID axis);
    void axisMin(DeviceEvent* event, AxisID axis);
    void axisMax(DeviceEvent* event, AxisID axis);
    void axisInvert(DeviceEvent* event, AxisID axis);
    void axisPositive(DeviceEvent* event, AxisID axis);
    void axisNegative(DeviceEvent* event, AxisID axis);
    void axisAbsolute(DeviceEvent* event, AxisID axis);
    void axisNegAbsolute(DeviceEvent* event, AxisID axis);
  
    // Prevent further mods from tweaking this event
    bool axisDropEvent(DeviceEvent* event, AxisID axis);
};

};

#endif
