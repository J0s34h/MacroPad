#ifndef HID_DESCRIPTOR_H
#define HID_DESCRIPTOR_H

#include <Adafruit_TinyUSB.h>

uint8_t const desc_hid_report[] = {
    // Keyboard (Report ID 1)
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x01, //   Report ID (1)

    // Modifier keys (8 bits)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0xE0, //   Usage Minimum (224)
    0x29, 0xE7, //   Usage Maximum (231)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data, Variable, Absolute)

    // Reserved byte
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x01, //   Input (Constant)

    // LED output report
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (1)
    0x29, 0x05, //   Usage Maximum (5)
    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x91, 0x02, //   Output (Data, Variable, Absolute)

    // LED padding
    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x01, //   Output (Constant)

    // Key array (6 keys)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0xDD, //   Logical Maximum (101)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0x00, //   Usage Minimum (0)
    0x29, 0xDD, //   Usage Maximum (101)
    0x81, 0x00, //   Input (Data, Array)

    0xC0, // End Collection

    // Consumer Control - Media Keys (Report ID 2)
    0x05, 0x0C, // Usage Page (Consumer)
    0x09, 0x01, // Usage (Consumer Control)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x02, //   Report ID (2)

    // Media keys (16 bits)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x10, //   Report Count (16)

    // Media key usages
    0x0A, 0xCD, 0x00, //   Usage (Play/Pause)
    0x0A, 0xB5, 0x00, //   Usage (Scan Next Track)
    0x0A, 0xB6, 0x00, //   Usage (Scan Previous Track)
    0x0A, 0xE9, 0x00, //   Usage (Volume Increment)
    0x0A, 0xEA, 0x00, //   Usage (Volume Decrement)
    0x0A, 0xE2, 0x00, //   Usage (Mute)
    0x0A, 0xB7, 0x00, //   Usage (Stop)
    0x0A, 0xB1, 0x00, //   Usage (Fast Forward)
    0x0A, 0xB0, 0x00, //   Usage (Rewind)

    // New brightness controls
    0x0A, 0x6F, 0x00, //   Usage (Brightness Increment)
    0x0A, 0x70, 0x00, //   Usage (Brightness Decrement)

    0x81, 0x02, //   Input (Data, Variable, Absolute)
    0xC0,       // End Collection" What are acceptable values for consumer page
};

// Mouse descriptor
uint8_t const desc_mouse_report[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)

    // Buttons
    0x05, 0x09, //   Usage Page (Button)
    0x19, 0x01, //   Usage Minimum (0x01)
    0x29, 0x08, //   Usage Maximum (0x08)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x95, 0x08, //   Report Count (8)
    0x75, 0x01, //   Report Size (1)
    0x81, 0x02, //   Input (Data,Var,Abs)

    // Padding
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x01, //   Input (Const,Array,Abs)

    // X, Y
    0x05, 0x01, //   Usage Page (Generic Desktop)
    0x09, 0x30, //   Usage (X)
    0x09, 0x31, //   Usage (Y)
    0x15, 0x81, //   Logical Minimum (-127)
    0x25, 0x7F, //   Logical Maximum (127)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x02, //   Report Count (2)
    0x81, 0x06, //   Input (Data,Var,Rel)

    // Vertical Wheel
    0x09, 0x38, //   Usage (Wheel)
    0x15, 0x81, //   Logical Minimum (-127)
    0x25, 0x7F, //   Logical Maximum (127)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x01, //   Report Count (1)
    0x81, 0x06, //   Input (Data,Var,Rel)

    // Horizontal Wheel (AC Pan)
    0x05, 0x0C,       //   Usage Page (Consumer Devices)
    0x0A, 0x38, 0x02, //   Usage (AC Pan, 0x0238)
    0x15, 0x81,       //   Logical Minimum (-127)
    0x25, 0x7F,       //   Logical Maximum (127)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x06,       //   Input (Data,Var,Rel)

    0xC0 // End Collection
};

#define HID_KEYBOARD_MAX_KEYS 6

#define PAGE_KEYBOARD 0x00
#define PAGE_CONSUMER 0x10
#define PAGE_MODIFIER 0x20

// Keyboard report structure
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keycode[6];
} keyboard_report_t;

// Keyboard 'Media' report structure
typedef union {
  struct {
    uint16_t play_pause : 1;
    uint16_t next_track : 1;
    uint16_t prev_track : 1;
    uint16_t vol_up : 1;
    uint16_t vol_down : 1;
    uint16_t mute : 1;
    uint16_t stop : 1;
    uint16_t fast_forward : 1;
    uint16_t rewind : 1;
    uint16_t brightness_up : 1;   // new
    uint16_t brightness_down : 1; // new
    uint16_t reserved : 5;        // remaining 5 bits unused
  };
  uint16_t value;
} media_report_t;

// Mouse report structure
typedef struct {
  uint8_t buttons;   // 1 byte: 8 buttons
  uint8_t padding;   // 1 byte: Constant/padding (CRITICAL!)
  int8_t x;          // 1 byte: X movement (-127 to 127)
  int8_t y;          // 1 byte: Y movement (-127 to 127)
  int8_t vertical;   // 1 byte: Vertical scroll (-127 to 127)c
  int8_t horizontal; // 1 byte: Horizontal scroll (-127 to 127)
} __attribute__((packed)) mouse_report_t;

#endif // HID_DESCRIPTOR_H