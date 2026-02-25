#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "../constants/HIDDescriptor.h"
#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

class HidManager {
public:
  HidManager();

  // Initialize with TinyUSB HID instances
  void begin(Adafruit_USBD_HID *keyboardHid, Adafruit_USBD_HID *mouseHid);

  /* =========================================================
     MAIN ENTRY
     Accepts dynamic number of encoded key codes.
     Automatically parses:
       - 0x00XX → Keyboard usage
       - 0x01XX → Consumer/Media usage
       - 0x02XX → Modifier bitmask
     Sends press + release (momentary)
     ========================================================= */
  void handleHidKeys(const uint16_t *codes, size_t count);

  /* =========================================================
     RAW PRESS (no automatic release)
     Useful for toggle / hold behavior
     ========================================================= */
  void pressRaw(const uint16_t *codes, size_t count);

  /* =========================================================
     Release all keyboard keys + modifiers
     ========================================================= */
  void releaseAll();

  /* =========================================================
     Mouse
     ========================================================= */
  void sendMouseReport(const mouse_report_t *report);

private:
  Adafruit_USBD_HID *usb_kb;
  Adafruit_USBD_HID *usb_mouse;

  void sendMediaUsage(uint16_t usage);
};

#endif // HID_MANAGER_H
