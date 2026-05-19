#pragma once

#include <Arduino.h>

enum class KeyAction : uint8_t {
  None,
  Digit0,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
  Digit7,
  Digit8,
  Digit9,
  Decimal,
  Up,
  Down,
  Back,
  Delete,
  Confirm
};

class KeypadInput {
public:
  KeypadInput();

  void begin();
  KeyAction readAction();
  char readRawKey();

  static bool isDigit(KeyAction action);
  static char toDigit(KeyAction action);
};
