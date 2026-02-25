#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "../constants/Wiring.h"
#include "../yaml/ConfigParser.h"
#include <Arduino.h>

// ButtonManager encapsulates button wiring, debouncing and mapping to
// profile outputs. It is intentionally independent from USB HID
// specifics — a send callback is used to emit keypresses.

// Example `usb_hud` instances you might use (commented):
// #include <Adafruit_TinyUSB.h>
// Adafruit_USBD_HID usb_hud(desc_hid_report, sizeof(desc_hid_report),
// HID_ITF_PROTOCOL_KEYBOARD, 2, false); or a dedicated HUD interface object if
// you have one.

//
enum ProfileButtonAction { CANCEL_ACTION, SET_ACTION };

class ButtonManager {
public:
  ButtonManager();

  // setButtonPressCallback: provide a callback used to send a single 16-bit
  // keycode when a button is activated. The callback may be a function that
  // forwards that key through your USB HID instance (e.g. `usb_hud`).
  void setButtonPressCallback(void (*sendKeyCb)(const uint16_t *codes,
                                                size_t count));

  // setProfileChangePressCallback: provide a callback used to send when profile
  // change buttons are pressed (either 0 for left, or 1 for right)
  void setProfileChangePressCallback(
      void (*sendProfilePressCb)(ProfileButtonAction action));

  // Set current profile; ButtonManager will copy button mappings from
  // the supplied Profile pointer. Passing nullptr clears mappings.
  void setProfile(const Profile *p);

  // poll: call frequently from main loop to process buttons
  void poll();

private:
  bool lastState[BUTTON_COUNT];
  unsigned long lastDebounce[BUTTON_COUNT];
  static const unsigned long DEBOUNCE_MS = 1;

  // Copy of the profile's button config used for output mapping
  ButtonConfig mapping[BUTTON_COUNT];
  uint8_t mappedCount;

  // Track toggle state per button
  bool toggleActive[BUTTON_COUNT];
  uint8_t toggleIndex;            // which button is toggled ON
  uint16_t toggleCodes[6];        // stored sequence for repeat
  size_t toggleCount;             // how many codes
  unsigned long toggleLastMillis; // last time repeated
  unsigned long toggleIntervalMs; // repeat rate

  void (*sendKey)(const uint16_t *codes, size_t count);
  void (*sendProfilePress)(ProfileButtonAction btn);

  void handlePresses(const uint8_t *indices, size_t count);
};

#endif // BUTTON_MANAGER_H
