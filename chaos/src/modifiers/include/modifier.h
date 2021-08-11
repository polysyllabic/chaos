/**
 * @file modifier.h
 *
 * @brief Definition of the Modifier class
 *
 * @author blegas78
 * @author polysyl
 *
 */

/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 blegas78.
 * Additional code copyright 2021 polysyl
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
#ifndef MODIFIER_H
#define MODIFIER_H

#include <mogi/math/systems.h>

#include <string>
#include <functional>

#include "controller.h"

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
    bool is_pressed(unsigned char button);

    // Alter event comming from the controller
    void button_off(DeviceEvent* event, ButtonID button);
    void button_on(DeviceEvent* event, ButtonID button);
  
    void axis_off(DeviceEvent* event, AxisID axis);
    void min_axis(DeviceEvent* event, AxisID axis);
    void max_axis(DeviceEvent* event, AxisID axis);
    void invert_axis(DeviceEvent* event, AxisID axis);
    void only_positive(DeviceEvent* event, AxisID axis);
    void only_negative(DeviceEvent* event, AxisID axis);
    void absolute_axis(DeviceEvent* event, AxisID axis);
    void neg_absolute_axis(DeviceEvent* event, AxisID axis);
  
    // Prevent further mods from tweaking this event
    bool drop_axis_event(DeviceEvent* event, AxisID axis);
};

};

#endif
