#ifndef CONTROLLER_STATE_DUALSENSE_H
#define CONTROLLER_STATE_DUALSENSE_H

#include <cstdint>

#include "controller.hpp"

namespace Chaos {

  class ControllerStateDualsense : public ControllerState {
    friend ControllerState;
  private:
    void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events);
	
  protected:
    ControllerStateDualsense();
	
  public:
    void applyHackedState(unsigned char* buffer, short* chaosState);
	
    ~ControllerStateDualsense();
  };

typedef struct
{
  uint8_t  reportId;                 // Report ID = 0x01 (1)
  uint8_t  GD_GamePadX;              // Usage 0x00010030: X, Value = 0 to 255
  uint8_t  GD_GamePadY;              // Usage 0x00010031: Y, Value = 0 to 255
  uint8_t  GD_GamePadZ;              // Usage 0x00010032: Z, Value = 0 to 255
  uint8_t  GD_GamePadRz;             // Usage 0x00010035: Rz, Value = 0 to 255
  uint8_t  GD_GamePadRx;             // Usage 0x00010033: Rx, Value = 0 to 255
  uint8_t  GD_GamePadRy;             // Usage 0x00010034: Ry, Value = 0 to 255
  uint8_t  VEN_GamePad0020;          // Usage 0xFF000020: , Value = 0 to 255
  uint8_t  GD_GamePadHatSwitch : 4;  // Usage 0x00010039: Hat switch, Value = 0 to 7, Physical = Value x 45 in degrees
  uint8_t  BTN_GamePadButton1 : 1;   // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton2 : 1;   // Usage 0x00090002: Button 2 Secondary, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton3 : 1;   // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton4 : 1;   // Usage 0x00090004: Button 4, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton5 : 1;   // Usage 0x00090005: Button 5, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton6 : 1;   // Usage 0x00090006: Button 6, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton7 : 1;   // Usage 0x00090007: Button 7, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton8 : 1;   // Usage 0x00090008: Button 8, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton9 : 1;   // Usage 0x00090009: Button 9, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton10 : 1;  // Usage 0x0009000A: Button 10, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton11 : 1;  // Usage 0x0009000B: Button 11, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton12 : 1;  // Usage 0x0009000C: Button 12, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton13 : 1;  // Usage 0x0009000D: Button 13, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton14 : 1;  // Usage 0x0009000E: Button 14, Value = 0 to 1, Physical = Value x 315
  uint8_t  BTN_GamePadButton15 : 1;  // Usage 0x0009000F: Button 15, Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad0021 : 1;      // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00211 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00212 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00213 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00214 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00215 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00216 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00217 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00218 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad00219 : 1;     // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad002110 : 1;    // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad002111 : 1;    // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad002112 : 1;    // Usage 0xFF000021: , Value = 0 to 1, Physical = Value x 315
  uint8_t  VEN_GamePad0022[52];      // Usage 0xFF000022: , Value = 0 to 255, Physical = Value x 21 / 17
	
  // 12-14 counter , perhaps something additional in the highest bits of byte 14
  // 15: value 0x27	After reconnect: 0x1C
  // 16-17: gyro X
  // 18-19: gyro
  // 20-21: gyro
  // 22-23: accel
  // 24-25: accel
  // 26-27: accel
  // 28-29: change rapidly, unknown
  // 30-31: appears to be a counter
  // 32: value 0x14, also 0x15, also 0x13, 0x12  Battery perhaps?
  // 33: bit 0x80 set if touchpad not touched, cleared if touched.  bits 0-7 correspond to a touch count
  // 34-37:  touch location
  // 38-41:  Same as the above 2 notes with touch event/location, but for a second finger
  //  	     If one finger touches, then second finger touches, and first finger releases, then second data still tracks
  // 42: 0x09
  // 43: 0x09
  // 44-48: 0x00
  // 49-50: appears to increment, perhaps as a part of more precise timing for below:
  // 51-52: counter, matches 30-31
  // 53: value 0x29
  // 54: value 0x08	-> becomes 9 when headphones are attached
  // 55: value 0x00
  // 56-63: 8-bytes of seeming random data
} inputReport01_t;

};

#endif
