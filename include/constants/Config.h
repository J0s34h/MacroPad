#ifndef CONFIG.H
#define CONFIG .H

#include "./Wiring.h"

// ===== WHEEL MODE CONSTANTS =====
enum WheelMode : uint8_t { WHEEL_CLICKY = 0, WHEEL_NOTCHY = 1 };

// Whell Simulation Types
enum class WheelSimulation { NOTCHY, CLICKY, TWIST, NONE };

// Scroll Types
enum class ScrollType { VERTICAL, HORIZONTAL, KEY_EMULATION };

// Button Press Types
enum class PressType { TOGGLE, SHORT };

// LED State Structures
struct SolidState {
  uint8_t brightness;
  uint32_t color;
};

struct TwistState {
  uint32_t maxLeftColor;
  uint32_t neutralColor;
  uint32_t maxRightColor;
};

struct ClickyState {
  uint32_t color1;
  uint32_t color2;
};

struct WheelConfig {
  WheelSimulation type;
  bool hasConfiguration; // True if this LED state has valid configuration
  union {
    SolidState solid;
    TwistState twist;
    ClickyState clicky;
  } state;
};

// Button Structure
struct ButtonConfig {
  char name[MAX_BUTTON_NAME];          // Button name
  uint16_t sequence[MAX_KEY_SEQUENCE]; // Key sequence
  uint8_t sequenceLength;              // Actual number of keys (0-3)
  PressType pressType;
  uint16_t delay; // Only used for spam type
};

// Scroll Configuration
struct ScrollConfig {
  ScrollType type;
  uint16_t key1; // For KEY_EMULATION
  uint16_t key2; // For KEY_EMULATION
};

// Profile Structure
struct Profile {
  char name[MAX_NAME_LENGTH]; // Profile name
  WheelConfig wheelConfig;    // Profile-specific LED config
  ScrollConfig scroll;
  ButtonConfig buttons[MAX_BUTTONS];
  uint8_t buttonCount; // Actual number of buttons
};

// Main Configuration Structure
struct DeviceConfig {
  WheelConfig globalLedState; // Global/default LED state
  Profile profiles[MAX_PROFILES];
  uint8_t profileCount; // Actual number of profiles
};

#endif CONFIG.H