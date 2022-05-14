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
#include <memory>
#include <string>
#include <unordered_map>

namespace Chaos {

  /**
   * \brief The interface for a self-registering factory for child modifier classes.
   * 
   * The configuration here allows you to add new types of modifiers without needing to
   * edit a factory class. See the #Modifier base class for more information.
   */
  template <class Base, class... Args> class Factory {
    
  public:
    /**
     * The builder function that creates the child class.
     */
    template <class ... T>
    static std::shared_ptr<Base> create(const std::string& mod_type, T&&... args) {
      return factory().at(mod_type)(std::forward<T>(args)...);
    }
    static bool hasType(std::string& mod_type) {
      return (factory().count(mod_type) == 1);
    }
    /**
     * The Registrar class registers the child class with the factory.
     */
    template <class T> struct Registrar : Base {      
      friend T;
      
      static bool registerType() {
        const auto mod_type = T::mod_type;
        Factory::factory()[mod_type] = [](Args... args) -> std::shared_ptr<Base> {
        return std::make_shared<T>(std::forward<Args>(args)...);
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
    
    using FuncType = std::shared_ptr<Base> (*)(Args...);
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
