#include "../include/buttons/ButtonManager.h"
#include "../include/constants/Wiring.h"

ButtonManager::ButtonManager() {
  mappedCount = 0;
  sendKey = nullptr;
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    lastState[i] = true; // INPUT_PULLUP default
    lastDebounce[i] = 0;
    // ensure mapping cleared
    mapping[i].name[0] = '\0';
    mapping[i].sequenceLength = 0;
    mapping[i].pressType = PressType::SHORT;
    mapping[i].delay = 0;

    toggleActive[i] = false;
  }
}

void ButtonManager::setButtonPressCallback(
    void (*sendKeyCb)(const uint16_t *codes, size_t count)) {
  sendKey = sendKeyCb;
  Serial.println("ButtonManager: configuring pins...");
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    bool state = digitalRead(BUTTON_PINS[i]);
    lastState[i] = state;
    lastDebounce[i] = millis();
    Serial.printf("  Button %d -> pin %d initial: %s\n", i, BUTTON_PINS[i],
                  state ? "HIGH" : "LOW");
  }
}

void ButtonManager::setProfileChangePressCallback(
    void (*sendProfilePressCb)(ProfileButtonAction action)) {
  sendProfilePress = sendProfilePressCb;
}

void ButtonManager::setProfile(const Profile *p) {
  if (!p) {
    mappedCount = 0;
    return;
  }
  // Copy up to BUTTON_COUNT buttons from profile
  mappedCount = min((uint8_t)BUTTON_COUNT, p->buttonCount);
  for (uint8_t i = 0; i < mappedCount; i++) {
    mapping[i] = p->buttons[i];
  }
  // Clear remaining
  for (uint8_t i = mappedCount; i < BUTTON_COUNT; i++) {
    mapping[i].name[0] = '\0';
    mapping[i].sequenceLength = 0;
  }
  Serial.printf("ButtonManager: profile '%s' mapped %d buttons\n", p->name,
                mappedCount);
}

void ButtonManager::handlePresses(const uint8_t *indices, size_t count) {
  uint16_t keyCodes[6] = {0};
  size_t keyCount = 0;

  for (size_t j = 0; j < count; ++j) {
    uint8_t index = indices[j];
    if (index >= BUTTON_COUNT)
      continue;

    // Profile buttons
    if (index >= 6) {
      switch (index) {
      case 6:
        sendProfilePress(ProfileButtonAction::SET_ACTION);
        break;
      case 7:
        sendProfilePress(ProfileButtonAction::CANCEL_ACTION);
        break;
      }
      continue;
    }

    // No mapping
    if (index >= mappedCount || mapping[index].sequenceLength == 0) {
      Serial.printf("ButtonManager: button %d pressed (no mapping)\n", index);
      continue;
    }

    // SHORT press -> momentary
    if (mapping[index].pressType == PressType::SHORT) {
      keyCount = mapping[index].sequenceLength;
      if (keyCount > 3)
        keyCount = 3;

      for (size_t k = 0; k < keyCount; k++) {
        keyCodes[k] = mapping[index].sequence[k];
      }

      if (sendKey)
        sendKey(keyCodes, keyCount);
    }

    // TOGGLE press -> start/stop repeating
    else if (mapping[index].pressType == PressType::TOGGLE) {

      // Toggle OFF if same button pressed again
      if (toggleActive[index]) {
        toggleActive[index] = false;
        toggleIndex = 0xFF;
        toggleCount = 0;
        memset(toggleCodes, 0, sizeof(toggleCodes));

        Serial.printf("ButtonManager: button %d TOGGLE OFF\n", index);
        continue;
      }

      // If another toggle is active, turn it off
      if (toggleIndex != 0xFF) {
        toggleActive[toggleIndex] = false;
        toggleIndex = 0xFF;
        toggleCount = 0;
        memset(toggleCodes, 0, sizeof(toggleCodes));
      }

      // Activate this toggle
      toggleActive[index] = true;
      toggleIndex = index;

      toggleCount = mapping[index].sequenceLength;
      if (toggleCount > 3)
        toggleCount = 3;

      memset(toggleCodes, 0, sizeof(toggleCodes));
      for (size_t k = 0; k < toggleCount; k++) {
        toggleCodes[k] = mapping[index].sequence[k];
      }

      // Repeat interval from mapping.delay (0 => default 333ms)
      toggleIntervalMs =
          (mapping[index].delay == 0) ? 10 : mapping[index].delay;
      toggleLastMillis = millis();

      // Immediately send first burst
      if (sendKey)
        sendKey(toggleCodes, toggleCount);

      Serial.printf("ButtonManager: button %d TOGGLE ON (interval=%lums)\n",
                    index, toggleIntervalMs);
    }
  }
}

void ButtonManager::poll() {
  unsigned long now = millis();
  uint8_t pressedIndices[BUTTON_COUNT];
  size_t pressCount = 0;

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    bool raw = digitalRead(BUTTON_PINS[i]);

    if (raw != lastState[i]) {
      if (now - lastDebounce[i] > DEBOUNCE_MS) {
        lastDebounce[i] = now;
        lastState[i] = raw;
        bool pressed = (raw == LOW);

        if (pressed) {
          pressedIndices[pressCount++] = i;
        } else {
          Serial.printf("ButtonManager: button %d released\n", i);
        }
      }
    } else {
      lastDebounce[i] = now;
    }
  }

  if (pressCount > 0) {
    handlePresses(pressedIndices, pressCount);
  }

  // Handle repeating toggle
  if (toggleIndex != 0xFF && toggleActive[toggleIndex]) {
    if (now - toggleLastMillis >= toggleIntervalMs) {
      toggleLastMillis = now;
      if (sendKey)
        sendKey(toggleCodes, toggleCount);
    }
  }
}
