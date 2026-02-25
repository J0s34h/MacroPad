#include "../include/wheel/WheelController.h"
#include "../include/constants/HIDDescriptor.h"

// ===== Setup =====

void WheelController::setup() {
  // LED setup
  FastLED.addLeds<WS2812, 15, GRB>(leds, LED_NUM);

  // Motor setup
  driver.voltage_power_supply = 5;
  driver.voltage_limit = 5;
  driver.init();

  encoder.init();
  encoder.enableInterrupts(doA, doB);
  motor.linkSensor(&encoder);
  motor.linkDriver(&driver);
  motor.voltage_sensor_align = 3;

  motor.PID_velocity.D = 0;
  motor.voltage_limit = 3;
  motor.PID_velocity.output_ramp = 1000;
  motor.LPF_velocity.Tf = 0.025f;
  motor.P_angle.P = 20;
  motor.velocity_limit = 4;

  setupNotchyMode();

  motor.init();
  motor.initFOC();
  motor.enable();
}

// ===== Public Methods =====

void WheelController::setWheelActionCallback(
    void (*sendWheelActionCb)(WheelAction action)) {
  sendWheelAction = sendWheelActionCb;
}

// ===== Mode Setups =====

void WheelController::setupNotchyMode() {
  motor.controller = MotionControlType::angle;
  motor.PID_velocity.I = 0.5;
  motor.PID_velocity.P = 0.2;
  motor.enable();
}

void WheelController::setupFreespin() {
  motor.controller = MotionControlType::velocity;
  motor.PID_velocity.P = 0.0f;
  motor.PID_velocity.D = 0.002f;
  motor.PID_velocity.I = 0.0f;
  motor.voltage_limit = 2.0f;
  motor.velocity_limit = 20.0f;
  motor.LPF_velocity.Tf = 0.025f;
  motor.PID_velocity.output_ramp = 1000;
  motor.P_angle.P = 0;
}

// ===== Mode Functions =====

void WheelController::handleNotchyWheel() {
  motor.disable();
  motor.move(target_angle);
  float encoderAngle = encoder.getAngle();

  new_target_angle = round(encoderAngle / radians(40)) * radians(40);

  const float angle_step = 0.2;

  if (abs(new_target_angle - target_angle) > angle_step) {
    int scroll = round((target_angle - new_target_angle) / radians(40));

    if (hidManager && abs(scroll) > radians(40 * 0.8)) {
      int steps = abs(scroll);
      int8_t dir = (scroll > 0) ? 1 : -1;
      for (int i = 0; i < steps; ++i) {
        sendProfileHidAction(dir);
      }
    }

    target_angle = new_target_angle;
    motor.move(target_angle);
  } else {
    cancelWheelAction();
  }
}

void WheelController::handleClickyWheel(ClickyState state) {
  float encoderAngle = encoder.getAngle();

  if (abs(encoderAngle - target_angle) > 0.5) {
    if (encoderAngle - target_angle > 0) {
      setLedColor(state.color1);
      sendProfileHidAction(-1);
    } else {
      setLedColor(state.color2);
      sendProfileHidAction(1);
    }

    target_angle = encoderAngle;
  }
}

void WheelController::poll() {
  if (hasProfile) {
    switch (currentProfile.wheelConfig.type) {
    case WheelSimulation::CLICKY: {
      ClickyState state = currentProfile.wheelConfig.state.clicky;
      handleClickyWheel(state);
      break;
    }
    case WheelSimulation::NOTCHY:
      handleNotchyWheel();
      break;
    default:
      handleNotchyWheel();
      break;
    }
  } else {
    handleNotchyWheel();
  }

  motor.loopFOC();
}

void WheelController::sendProfileHidAction(int8_t direction) {
  Serial.printf("WheelController: sending HID action for direction %d\n",
                direction);

  if (profileSelectMenu) {
    sendWheelAction(direction > 0 ? SCROLL_UP : SCROLL_DOWN);
    return;
  }

  switch (currentProfile.scroll.type) {
  case ScrollType::VERTICAL: {
    mouse_report_t mr = {0, 0, 0, 0, 0, 0};
    mr.vertical = direction;
    hidManager->sendMouseReport(&mr);
    break;
  }
  case ScrollType::HORIZONTAL: {
    mouse_report_t mr = {0, 0, 0, 0, 0, 0};
    mr.horizontal = direction;
    hidManager->sendMouseReport(&mr);
    break;
  }
  case ScrollType::KEY_EMULATION: {
    uint16_t usage = (direction > 0) ? currentProfile.scroll.key1
                                     : currentProfile.scroll.key2;
    if (usage != 0) {
      hidManager->handleHidKeys(&usage, 1);
    }
    break;
  }
  default:
    break;
  }
}

// ===== Wheel Action Helpers =====

void WheelController::cancelWheelAction() {
  if (wheelKeyPressed && wheelKeyTimer + 100 < millis()) {
    if (hidManager) {
      hidManager->releaseAll();
    }
    wheelKeyPressed = false;
  }
}

void WheelController::setProfile(const Profile *p) {
  memcpy(&currentProfile, p, sizeof(Profile));
  hasProfile = true;

  Serial.printf("WheelConfig  %s", currentProfile.wheelConfig.type);

  if (true) {
    switch (currentProfile.wheelConfig.type) {
    case WheelSimulation::CLICKY:
      setLedColor(currentProfile.wheelConfig.type == WheelSimulation::CLICKY
                      ? CRGB(0xFF0000)
                      : CRGB(0x00FF00));
    case WheelSimulation::NOTCHY:
      wheelMode = (currentProfile.wheelConfig.type == WheelSimulation::CLICKY)
                      ? WHEEL_CLICKY
                      : WHEEL_NOTCHY;
      setLedColor(currentProfile.wheelConfig.type == WheelSimulation::CLICKY
                      ? CRGB(0xFF0000)
                      : CRGB(0x00FF00));
      setupNotchyMode();
      break;
    default:
      wheelMode = WHEEL_CLICKY;
      setupNotchyMode();
      break;
    }
  } else {
    wheelMode = WHEEL_CLICKY;
    setLedColor(CRGB(0xFFFFFF));
    setupFreespin();
  }

  wheelModeChanged = true;
}

void WheelController::setPowerSave(bool state) {
  isPowerSave = state;

  if (isPowerSave) {
    motor.disable();
    FastLED.clear();
    FastLED.show();
  } else {
    motor.enable();
    FastLED.setBrightness(85);
  }
}

void WheelController::setLedColor(CRGB color) {
  if (isPowerSave) {
    return;
  }

  for (int i = 0; i < LED_NUM; i++) {
    leds[i] = color;
  }
  FastLED.show();
}