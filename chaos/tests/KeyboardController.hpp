#pragma once
#include <Controller.hpp>
#include <GameMenu.hpp>
#include <DeviceEvent.hpp>

class KeyboardController : public Chaos::Controller {
private:
  bool applyHardware();
  void doAction();
public:
  KeyboardController();
  void initialize();
};