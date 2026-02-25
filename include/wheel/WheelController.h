#ifndef WHEELCONTROLLER_H
#define WHEELCONTROLLER_H

#include "../constants/Wiring.h"
#include "../usb/HidManager.h"
#include "../yaml/ConfigParser.h"
#include <Arduino.h>
#include <FastLED.h>
#include <SimpleFOC.h>

enum WheelAction { SCROLL_UP, SCROLL_DOWN };

class WheelController {
public:
  // ---- External initialization (must be called once before using callbacks)
  // ----
  static WheelController *initialize(BLDCDriver6PWM &driver, BLDCMotor &motor,
                                     Encoder &encoder) {
    if (!instance_) {
      instance_ = new WheelController(driver, motor, encoder);
    }
    return instance_;
  }

  // Setup function, configures BLDC behaviour
  void setup();

  // Main wheel mode functions
  void poll();

  // Profile selection mode
  void setProfileSelectionMode(bool enabled) { profileSelectMenu = enabled; }

  // Action helpers
  void cancelWheelAction();

  // Callback
  void setWheelActionCallback(void (*sendWheelActionCb)(WheelAction action));

  // Optional integration with HidManager
  void setHidManager(HidManager *hm) { hidManager = hm; }

  // Set the active profile (copy of profile data)
  void setProfile(const Profile *p);

  // Send a profile-configured HID action based on wheel direction
  // direction > 0 : positive step, direction < 0 : negative step
  void sendProfileHidAction(int8_t direction);

  // Set power saving mode (encdoer still will report)
  void setPowerSave(bool state);

  // Simple getter (inline)
  float getCurrentAngle() const { return encoder.getAngle(); }

private:
  // Private constructor – only called from initialize()
  WheelController(BLDCDriver6PWM &driver, BLDCMotor &motorRef,
                  Encoder &encoderRef)
      : driver(driver), motor(motorRef), encoder(encoderRef) {}

  static WheelController *instance_;

  // References to externally managed hardware
  BLDCDriver6PWM &driver;
  BLDCMotor &motor;
  Encoder &encoder;

  // LED configuration
  CRGB leds[LED_NUM];

  // Wheel callback
  void (*sendWheelAction)(WheelAction action);

  // Wheel state
  WheelMode wheelMode;
  bool wheelModeChanged;

  // Angle tracking
  float target_angle = 0;
  float new_target_angle = 0;

  // Timing
  unsigned long wheelKeyTimer;

  // Wheel action
  bool wheelKeyPressed;

  // Profile selection
  bool profileSelectMenu = false;

  // Optional HID manager to send mouse/key reports
  HidManager *hidManager = nullptr;

  // Current profile data (copied from storage)
  Profile currentProfile;
  bool hasProfile = false;

  bool isPowerSave = false;

  // Private setup methods
  void setupNotchyMode();
  void setupFreespin();

  // Mode implementations
  void handleNotchyWheel();
  void handleClickyWheel(ClickyState state);

  // Encoder interrupts
  static void doA() { instance_->encoder.handleA(); }
  static void doB() { instance_->encoder.handleB(); }

  // LED management
  void setLedColor(CRGB color);
};

#endif