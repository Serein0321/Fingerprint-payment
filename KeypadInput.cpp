#include "KeypadInput.h"

#include <Keypad.h>

#include "DeviceConfig.h"

namespace {
Keypad keypad = Keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS, KEYPAD_COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);
}

KeypadInput::KeypadInput() {}

void KeypadInput::begin() {}

char KeypadInput::readRawKey() {
  return keypad.getKey();
}

KeyAction KeypadInput::readAction() {
  const char key = readRawKey();

  switch (key) {
    case '0': return KeyAction::Digit0;
    case '1': return KeyAction::Digit1;
    case '2': return KeyAction::Digit2;
    case '3': return KeyAction::Digit3;
    case '4': return KeyAction::Digit4;
    case '5': return KeyAction::Digit5;
    case '6': return KeyAction::Digit6;
    case '7': return KeyAction::Digit7;
    case '8': return KeyAction::Digit8;
    case '9': return KeyAction::Digit9;
    case '*': return KeyAction::Decimal;
    case 'A': return KeyAction::Up;
    case 'B': return KeyAction::Down;
    case 'C': return KeyAction::Back;
    case '#': return KeyAction::Delete;
    case 'D': return KeyAction::Confirm;
    default: return KeyAction::None;
  }
}

bool KeypadInput::isDigit(KeyAction action) {
  return action >= KeyAction::Digit0 && action <= KeyAction::Digit9;
}

char KeypadInput::toDigit(KeyAction action) {
  if (!isDigit(action)) {
    return '\0';
  }

  return static_cast<char>('0' + (static_cast<uint8_t>(action) - static_cast<uint8_t>(KeyAction::Digit0)));
}
