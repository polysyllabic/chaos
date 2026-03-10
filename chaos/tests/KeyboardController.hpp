#pragma once
#include <Controller.hpp>
#include <GameMenu.hpp>
#include <DeviceEvent.hpp>

class KeyboardController : public Chaos::Controller {
private:
  bool applyHardware();
  void doAction();
public:
  /**
   * \brief Construct keyboard-backed controller test double.
   */
  KeyboardController();

  /**
   * \brief Initialize keyboard input handling for tests.
   */
  void initialize();
};
