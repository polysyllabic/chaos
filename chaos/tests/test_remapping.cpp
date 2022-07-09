/**
 * Testing cascading remaps
 * 
 * Modifier A (D-Pad Rotate)
 *   1. DX -> DY
 *   2. DY -> DX invert
 * 
 * Table After A
 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * DX                -> DY         ; NONE        ; false  ; false  ; 0         ; 1
 * DY                -> DX         ; NONE        ; false  ; true   ; 0         ; 1
 * 
 * Modifier B (Swap D-Pad/Left Joystick):
 *   1. LX -> DX
 *   2. LY -> DY
 *   3. DX -> LX
 *   4. DY -> LY
 * 
 *   1. LX -> DX
 *   2. LY -> DY
 *   3. DX -> LX
 *   4. DY -> LY
 * 
 * Table After A & B
 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * LX                -> DX         ; NONE        ; false  ; false  ; 0         ; 1
 * LY                -> DY         ; NONE        ; false  ; false  ; 0         ; 1
 * DX                -> LY         ; NONE        ; false  ; false  ; 0         ; 1
 * DY                -> LX         ; NONE        ; false  ; true   ; 0         ; 1
 * 
 *  Modifier C (Controller Flip):
 *   1. DY -> DY invert
 *   2. LY -> LY invert
 *   3. RY -> RX invert
 *   4. R1 -> R2
 *   5. L1 -> L2
 *   6. R2 -> R1
 *   7. L2 -> L1
 *   8. TRIANGLE -> X
 *   9. X -> TRIANGLE
 *
 *   1. DY -> DY invert -> LY -> DX invert
 *   2. LY -> LY invert
 *   3. RY -> RX invert
 *   4. R1 -> R2
 *   5. L1 -> L2
 *   6. R2 -> R1
 *   7. L2 -> L1
 *   8. TRIANGLE -> X
 *   9. X -> TRIANGLE
 * 
 * 
 * Table After A, B, and C
 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * X                 -> TRIANGLE   ; NONE        ; false  ; false  ; 0         ; 1
 * TRIANGLE          -> X          ; NONE        ; false  ; false  ; 0         ; 1
 * L1                -> L2         ; NONE        ; false  ; false  ; 0         ; 1
 * R1                -> R2         ; NONE        ; false  ; false  ; 0         ; 1
 * L2                -> L1         ; NONE        ; false  ; false  ; 0         ; 1
 * R2                -> R1         ; NONE        ; false  ; false  ; 0         ; 1
 * LX                -> DX         ; NONE        ; false  ; false  ; 0         ; 1
 * LY                -> DY         ; NONE        ; false  ; false  ; 0         ; 1
 * RX                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * RY                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * DX                -> LY         ; NONE        ; false  ; false  ; 0         ; 1
 * DY                -> LX         ; NONE        ; false  ; true   ; 0         ; 1
 * 
 *  Modifier D (Swap Shapes/Right Joystick)
 *   1. RX -> SQUARE neg->CIRCLE threshold=0.5
 *   2. RY -> X neg->TRIANGLE threshold=0.5
 *   3. SQUARE -> RX to_min
 *   4. CIRCLE -> RX
 *   5. TRIANGLE -> RY to_min
 *   6. X -> RY

 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * X                 -> RY         ; NONE        ; false  ; false  ; 0         ; 1
 * CIRCLE            -> RX         ; NONE        ; false  ; false  ; 0         ; 1
 * TRIANGLE          -> RY         ; NONE        ; true   ; false  ; 0         ; 1
 * SQUARE            -> RX         ; NONE        ; true   ; false  ; 0         ; 1
 * RX                -> SQUARE     ; CIRCLE      ; false  ; false  ; 0.5       ; 1
 * RY                -> x          ; TRIANGLE    ; true   ; false  ; 0         ; 1
 * 
 *  Modifier E (Motion Control Aiming)
 *   1. ACCX -> RX scale=24
 *   2. ACCZ -> RY scale=24
 *   3. RX -> NOTHING
 *   4. RY -> NOTHING
 * 
 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * X                 -> RY         ; NONE        ; false  ; false  ; 0         ; 1
 * CIRCLE            -> RX         ; NONE        ; false  ; false  ; 0         ; 1
 * TRIANGLE          -> RY         ; NONE        ; true   ; false  ; 0         ; 1
 * SQUARE            -> RX         ; NONE        ; true   ; false  ; 0         ; 1
 * RX                -> SQUARE     ; CIRCLE      ; false  ; false  ; 0.5       ; 1
 * RY                -> x          ; TRIANGLE    ; true   ; false  ; 0         ; 1
 * ACCX              -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * ACCZ              -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * 
 * Remap Table (No remaps applied)
 * actual             ; to-console ; to-negative ; to-min ; invert ; threshold ; scale
 * X                 -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * CIRCLE            -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * TRIANGLE          -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * SQUARE            -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * L1                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * R1                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * L2                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * R2                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * L3                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * R3                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * LX                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * LY                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * RX                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * RY                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * DX                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * DY                -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * ACCX              -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * ACCZ              -> NONE       ; NONE        ; false  ; false  ; 0         ; 1
 * 
 * Table B and C (a removed)
 * 
 */
#include <string>
#include <iostream>
#include <ControllerInjector.hpp>
#include <DeviceEvent.hpp>
#include <ControllerInputTable.hpp>
#include <timer.hpp>

class TestInjector : public Chaos::ControllerInjector {

  public:
    bool sniffify(const Chaos::DeviceEvent& input, Chaos::DeviceEvent& output) {
      output = input;
      bool valid = true;

      return valid;
    }
};


int main(int argc, char const* argv[]) {

}