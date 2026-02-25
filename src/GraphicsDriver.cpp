#include "../include/video/GraphicsDriver.h"

void GraphicsDriver::poll() {
  // Check if we need to redraw the display
  if (needsDisplay) {
    draw();
    needsDisplay = false;
  }
}

void GraphicsDriver::setState(DisplayState newState) {
  if (currentState != newState) {
    currentState = newState;
    needsDisplay = true; // Trigger redraw on state change
  }
}

void GraphicsDriver::setErrorMessage(const String &message) {
  errorMessage = message;
  setState(STATE_ERROR);
}

void GraphicsDriver::setSelectedProfile(int profileIndex) {
  if (profileIndex >= 0 && profileIndex < totalProfiles) {
    selectedProfile = profileIndex;
    needsDisplay = true; // Trigger redraw
  }
}

void GraphicsDriver::draw() {
  // Use page buffer mode (based on your working example)
  u8g2.firstPage();
  do {
    switch (currentState) {
    case STATE_ERROR:
      drawError();
      break;
    case STATE_PROFILE_SELECTION:
      drawProfileSelection();
      break;
    case STATE_PROFILE_INTERFACE:
      drawProfileInterface();
      break;
    }
  } while (u8g2.nextPage());
}

void GraphicsDriver::drawError() {
  // Set font for error display
  u8g2.setFont(u8g2_font_5x7_tf);

  // Clear area (by drawing a filled rectangle as background)
  u8g2.setDrawColor(0); // Clear
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(1); // Draw

  // Calculate center position for error message
  int msgWidth = u8g2.getStrWidth(errorMessage.c_str());
  int xPos = (128 - msgWidth) / 2;

  // Draw error message
  u8g2.drawStr(xPos, 30, errorMessage.c_str());
}

void GraphicsDriver::drawProfileSelection() {
  // Clear display
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(1);

  // Title
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(45, 8, "SELECT");

  // Separator line
  u8g2.drawLine(0, 12, 128, 12);

  // Calculate animated offset for smooth scrolling
  int animatedOffset = scrollOffset;
  if (isScrolling && scrollAnimation < 100) {
    // If scrolling down, adjust offset for animation
    // For simplicity, we'll just use the scrollOffset directly
    // You could add pixel-level animation here if desired
  }

  // Draw visible profiles with selection highlight
  for (int i = 0; i < visibleProfiles; i++) {
    int profileIndex = scrollOffset + i;

    if (profileIndex >= 0 && profileIndex < totalProfiles) {
      // Calculate vertical position with optional animation offset
      int yPos = 20 + (i * 15);

      // Apply scrolling animation
      if (isScrolling) {
        // You can add pixel-level animation here
        // For example: yPos += (scrollAnimation * direction) / 100;
      }

      // Check if this is the selected profile
      bool isSelected = (profileIndex == selectedProfile);

      drawProfileListItem(i, profileIndex, isSelected);
    }
  }

  // Draw profile counter
  char counter[16];
  sprintf(counter, "%d/%d", selectedProfile + 1, totalProfiles);
  int counterWidth = u8g2.getStrWidth(counter);
  u8g2.drawStr(128 - counterWidth - 5, 8, counter);
}

void GraphicsDriver::drawProfileListItem(int listPosition, int profileIndex,
                                         bool isSelected) {
  int yPos = 20 + (listPosition * 15);

  // Draw background for selected item
  if (isSelected) {
    u8g2.drawBox(10, yPos - 8, 108, 12);
    u8g2.setDrawColor(0); // White text on black background
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, yPos + 1, profileNames[profileIndex]);
    u8g2.setDrawColor(1); // Restore normal
  } else {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(15, yPos + 1, profileNames[profileIndex]);
  }
}

void GraphicsDriver::drawProfileInterface() {
  // Draw the grid (background)
  drawGrid();

  // Now draw interface elements on top of the grid
  u8g2.setFont(u8g2_font_u8glib_4_tf);

  // Top row
  u8g2.drawStr(20 - (u8g2.getStrWidth(buttonLabels[0]) / 2), 23,
               buttonLabels[0]); // Top-left
  u8g2.drawStr(64 - (u8g2.getStrWidth(buttonLabels[1]) / 2), 23,
               buttonLabels[1]); // Top-center
  u8g2.drawStr(108 - (u8g2.getStrWidth(buttonLabels[2]) / 2), 23,
               buttonLabels[2]); // Top-right

  // Bottom row
  u8g2.drawStr(20 - (u8g2.getStrWidth(buttonLabels[3]) / 2), 47,
               buttonLabels[3]); // Bottom-left
  u8g2.drawStr(64 - (u8g2.getStrWidth(buttonLabels[4]) / 2), 47,
               buttonLabels[4]); // Bottom-center
  u8g2.drawStr(108 - (u8g2.getStrWidth(buttonLabels[5]) / 2), 47,
               buttonLabels[5]); // Bottom-right

  // Profile name at top
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 36, profileNames[selectedProfile]);

  drawButtonIcons();
}

void GraphicsDriver::drawButtonIcons() {
  drawIcon16x16(13, 0, profileIcons[0]);
  drawIcon16x16(58, 0, profileIcons[1]);
  drawIcon16x16(102, 0, profileIcons[2]);
  drawIcon16x16(13, 49, profileIcons[3]);
  drawIcon16x16(58, 49, profileIcons[4]);
  drawIcon16x16(102, 49, profileIcons[5]);
}

void GraphicsDriver::drawGrid() {
  // Draw horizontal lines (across full width)
  u8g2.drawHLine(0, 26, 128); // Top horizontal line
  u8g2.drawHLine(0, 39, 128); // Middle horizontal line (creates gap)

  // Draw vertical lines for top section
  u8g2.drawVLine(42, 0, 26); // Left vertical in top section
  u8g2.drawVLine(86, 0, 26); // Right vertical in top section

  // Draw vertical lines for bottom section
  u8g2.drawVLine(42, 39, 25); // Left vertical in bottom section
  u8g2.drawVLine(86, 39, 25); // Right vertical in bottom section
}

void GraphicsDriver::scrollUp() {
  if (currentState != STATE_PROFILE_SELECTION) {
    return;
  }

  if (selectedProfile > 0) {
    selectedProfile--;
    ensureSelectionVisible();

    // Start scroll animation
    isScrolling = true;
    scrollStartTime = millis();
    scrollAnimation = 0;
    needsDisplay = true;
  }
}

void GraphicsDriver::scrollDown() {
  if (currentState != STATE_PROFILE_SELECTION) {
    return;
  }

  if (selectedProfile < totalProfiles - 1) {
    selectedProfile++;
    ensureSelectionVisible();

    // Start scroll animation
    isScrolling = true;
    scrollStartTime = millis();
    scrollAnimation = 0;
    needsDisplay = true;
  }
}

void GraphicsDriver::ensureSelectionVisible() {
  // Ensure selected profile is within the visible range
  if (selectedProfile < scrollOffset) {
    scrollOffset = selectedProfile;
  } else if (selectedProfile >= scrollOffset + visibleProfiles) {
    scrollOffset = selectedProfile - visibleProfiles + 1;
  }

  // Clamp scroll offset
  if (scrollOffset < 0)
    scrollOffset = 0;
  if (scrollOffset > totalProfiles - visibleProfiles) {
    scrollOffset = totalProfiles - visibleProfiles;
  }
}

// Setter

void GraphicsDriver::setProfileList(char (*profiles)[MAX_NAME_LENGTH],
                                    int count, int selectedProfile) {
  this->profileNames = profiles;
  this->totalProfiles = (count < MAX_PROFILES) ? count : MAX_PROFILES;

  // Reset selection state
  selectedProfile = selectedProfile;
  scrollOffset = 0;
}

void GraphicsDriver::setProfileIcons(uint8_t (*icons)[16][2]) {
  // Currently not used in drawing, but can be stored for future use
  this->profileIcons = icons;
}

void GraphicsDriver::setProfile(const Profile *profile) {
  // Clear any existing labels
  if (buttonLabels != nullptr) {
    delete[] buttonLabels;
    buttonLabels = nullptr;
  }

  // Check if profile has any buttons
  if (profile == nullptr || profile->buttonCount == 0) {
    return;
  }

  // Allocate memory for button labels
  buttonLabels = new char[profile->buttonCount][MAX_BUTTONS];
  // Copy labels from profile buttons
  for (uint8_t i = 0; i < profile->buttonCount; i++) {
    // Assuming ButtonConfig has a char array member called 'label'
    // Adjust this based on the actual structure of ButtonConfig
    strncpy(buttonLabels[i], profile->buttons[i].name, MAX_BUTTONS - 1);
    buttonLabels[i][MAX_BUTTONS - 1] = '\0'; // Ensure null termination
  }
}

int GraphicsDriver::getSelectedProfileIdx() { return selectedProfile; }

// Helpers

void GraphicsDriver::drawIcon16x16(int x, int y, const uint8_t icon[16][2]) {
  u8g2.drawBitmap(x, y, 2, 16, (const uint8_t *)icon);
}