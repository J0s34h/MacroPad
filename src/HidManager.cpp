#include "../include/usb/HidManager.h"
#include <string.h>

HidManager::HidManager() : usb_kb(nullptr), usb_mouse(nullptr) {}

void HidManager::begin(Adafruit_USBD_HID *keyboardHid,
                       Adafruit_USBD_HID *mouseHid) {
  usb_kb = keyboardHid;
  usb_mouse = mouseHid;

  if (usb_kb)
    usb_kb->setPollInterval(1);
  if (usb_mouse)
    usb_mouse->setPollInterval(2);
}

/* =========================================================
   MAIN ENTRY
   Accepts ANY number of codes
   Enforces 6-key limit only at HID output
   ========================================================= */
void HidManager::handleHidKeys(const uint16_t *codes, size_t count) {
  if (!usb_kb || !codes || count == 0)
    return;

  keyboard_report_t kbReport;
  memset(&kbReport, 0, sizeof(kbReport));

  uint8_t keyIndex = 0;

  for (size_t i = 0; i < count; i++) {
    uint16_t code = codes[i];
    if (code == 0)
      continue;

    uint8_t page = (code >> 8) & 0xFF;
    uint8_t usage = code & 0xFF;

    switch (page) {

    // ==========================
    // KEYBOARD USAGE PAGE
    // ==========================
    case PAGE_KEYBOARD:
      Serial.printf("Sending kb %d \n", usage);
      if (keyIndex < HID_KEYBOARD_MAX_KEYS) {
        kbReport.keycode[keyIndex++] = usage;
      }
      break;

    // ==========================
    // MODIFIER PAGE
    // usage is bitmask (0x01, 0x02, etc)
    // ==========================
    case PAGE_MODIFIER:
      Serial.printf("Sending mod %d \n", usage);
      kbReport.modifiers |= usage;
      break;

    // ==========================
    // CONSUMER / MEDIA PAGE
    // ==========================
    case PAGE_CONSUMER:
      Serial.printf("Sending media %d \n", code & 0xFFF);
      sendMediaUsage(usage);
      break;

    default:
      Serial.printf("Unknown Page %d \n", page);
      // Unknown page — ignore safely
      break;
    }
  }

  // Send combined keyboard report (keys + modifiers)
  usb_kb->sendReport(1, &kbReport, sizeof(kbReport));

  // Short press model: auto release
  delay(10);

  memset(&kbReport, 0, sizeof(kbReport));
  usb_kb->sendReport(1, &kbReport, sizeof(kbReport));
}

/* =========================================================
   MEDIA (Consumer Page)
   ========================================================= */
void HidManager::sendMediaUsage(uint16_t usage) {
  if (!usb_kb)
    return;

  media_report_t report;

  if (usage == 0) {
    Serial.println("Rew");
    report.rewind = 1;
  }

  report.value = usage;

  usb_kb->sendReport(2, &report.value, sizeof(report.value));
  delay(2);

  report.value = 0;
  usb_kb->sendReport(2, &report.value, sizeof(report.value));
}

/* =========================================================
   OPTIONAL DIRECT CONTROL
   ========================================================= */
void HidManager::pressRaw(const uint16_t *codes, size_t count) {
  if (!usb_kb || !codes)
    return;

  keyboard_report_t kbReport;
  memset(&kbReport, 0, sizeof(kbReport));

  uint8_t keyIndex = 0;

  for (size_t i = 0; i < count; i++) {
    uint16_t code = codes[i];
    if (code == 0)
      continue;

    uint8_t page = (code >> 8) & 0xFF;
    uint8_t usage = code & 0xFF;

    if (page == PAGE_KEYBOARD && keyIndex < HID_KEYBOARD_MAX_KEYS) {
      kbReport.keycode[keyIndex++] = usage;
    } else if (page == PAGE_MODIFIER) {
      kbReport.modifiers |= usage;
    }
  }

  usb_kb->sendReport(1, &kbReport, sizeof(kbReport));
}

void HidManager::releaseAll() {
  if (!usb_kb)
    return;

  keyboard_report_t kbReport;
  memset(&kbReport, 0, sizeof(kbReport));
  usb_kb->sendReport(1, &kbReport, sizeof(kbReport));
}

/* =========================================================
   MOUSE
   ========================================================= */
void HidManager::sendMouseReport(const mouse_report_t *report) {
  if (!usb_mouse || !report)
    return;
  usb_mouse->sendReport(0, report, sizeof(mouse_report_t));
}