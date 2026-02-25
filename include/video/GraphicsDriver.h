#ifndef GRAPHICSDRIVER_H
#define GRAPHICSDRIVER_H

#include "../constants/Wiring.h"
#include "../yaml/ConfigParser.h"

#pragma once
#include <U8g2lib.h>

enum DisplayState {
  STATE_ERROR,
  STATE_PROFILE_SELECTION,
  STATE_PROFILE_INTERFACE
};

class GraphicsDriver {
private:
  // Display Controller
  U8G2 &u8g2; // Reference to display object

  bool needsDisplay = true;

  // Profile selection state
  int totalProfiles = 5;   // Example: 8 profiles total
  int visibleProfiles = 3; // Number of profiles visible at once
  int scrollOffset = 0;    // Which profile is at the top of the visible list
  int selectedProfile = 0; // Currently selected profile

  // Animation/Scrolling variables (Profile Selection)
  int scrollAnimation = 0; // For smooth scrolling animation (0-100%)
  bool isScrolling = false;
  unsigned long scrollStartTime = 0;
  const int scrollDuration = 300; // ms for scroll animation

  // Error state data
  String errorMessage = "Initial error message";

  // Profile data
  char (*profileNames)[MAX_NAME_LENGTH] = nullptr;
  char (*buttonLabels)[MAX_BUTTONS] = nullptr;
  uint8_t (*profileIcons)[16][2] = nullptr;

public:
  GraphicsDriver(U8G2 &display) : u8g2(display) {
    delay(100);

    // Hardware reset sequence
    pinMode(DISPLAY_RESET, OUTPUT);
    digitalWrite(DISPLAY_RESET, LOW);  // assert reset
    delay(10);                         // hold reset for 10 ms
    digitalWrite(DISPLAY_RESET, HIGH); // release reset
    delay(50);                         // wait for display to initialize

    // u8g2.setBusClock(400000);  // 100 kHz instead of 400 kHz

    u8g2.begin();
  }

  DisplayState currentState = STATE_ERROR;

  // Profile selection navigation
  void scrollUp();
  void scrollDown();

  void poll();
  void setState(DisplayState newState);
  void setErrorMessage(const String &message);
  void setSelectedProfile(int profileIndex);

  void setProfile(const Profile *p);
  void setProfileList(char (*profiles)[MAX_NAME_LENGTH], int count,
                      int selectedProfile);
  void setProfileIcons(uint8_t (*icons)[16][2]);

  int getSelectedProfileIdx();

  void setNeedsDisplay() { needsDisplay = true; };

private:
  void draw();
  void drawError();
  void drawProfileSelection();
  void drawProfileInterface();
  void drawGrid();
  void drawButtonIcons();

  // Helper functions
  void ensureSelectionVisible();
  void updateScrollAnimation();
  void drawProfileListItem(int listPosition, int profileIndex, bool isSelected);
  void drawIcon16x16(int x, int y, const uint8_t icon[16][2]);
};

#endif