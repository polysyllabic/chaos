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
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <toml++/toml.h>

namespace Chaos {

  /**
   * \brief The interface for a self-registering factory for child modifier classes.
   */
  template <class Base, class... Args> class Factory {
    
  public:
    /**
     * The builder function that creates the child class.
     */
    template <class ... T>
    static std::unique_ptr<Base> create(const std::string& name, T&&... args) {
      return factory().at(name)(std::forward<T>(args)...);
    }

    /**
     * The Registrar class registers the child class with the factory.
     */
    template <class T> struct Registrar : Base {      
      friend T;
      
      static bool registerType() {
	const auto name = T::name;
	Factory::factory()[name] = [](Args... args) -> std::unique_ptr<Base> {
	  return std::make_unique<T>(std::forward<Args>(args)...);
	};
	return true;
      }
      static bool registered;

    private:
      Registrar() : Base(Passkey{}) { (void) registered; }
    };

    friend Base;
    
  private:
    // We use a passkey to force the constructors to derive through the Registrar class
    // rather than from Modifier directly.
    class Passkey {
      Passkey(){};
      template <class T> friend struct Registrar;
    };
    
    using FuncType = std::unique_ptr<Base> (*)(Args...);
    Factory() = default;
    
    static auto& factory() {
      static std::unordered_map<std::string, FuncType> f;
      return f;
    }
  };

  template <class Base, class... Args>
  template <class T>
  bool Factory<Base, Args...>::Registrar<T>::registered =
    Factory<Base, Args...>::Registrar<T>::registerType();
  
};
