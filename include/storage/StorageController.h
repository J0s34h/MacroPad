#ifndef STORAGE_CONTROLLER_H
#define STORAGE_CONTROLLER_H

#include "../constants/Wiring.h"
#include "../yaml/ConfigParser.h"
#include <Arduino.h>
#include <SD.h>

class StorageController {
public:
  // Constructor
  StorageController();

  // Initialize storage controller
  bool initialize();

  // Load configuration from SD card
  bool loadConfiguration();

  // Get current configuration
  const DeviceConfig *getConfig() const {
    return parser.isValid() ? &parser.getConfig() : nullptr;
  }

  // Get current profile
  const Profile *getCurrentProfile() const { return currentProfile; }

  // Get LED state for current profile
  const WheelConfig *getCurrentLedState() const { return currentLedState; }

  // Get scroll config for current profile
  const ScrollConfig *getCurrentScrollConfig() const {
    return &currentScrollConfig;
  }

  // Switch to a specific profile
  bool switchProfile(uint8_t profileIndex);

  // Switch to next profile
  bool switchToNextProfile();

  // Switch to previous profile
  bool switchToPreviousProfile();

  // Get current profile index
  uint8_t getCurrentProfileIndex() const { return currentProfileIndex; }

  // Get total number of profiles
  uint8_t getProfileCount() const { return profileCount; }

  // Get profile name by index
  const char *getProfileName(uint8_t index) const;

  // Get profile list
  char (*getProfileNames())[MAX_NAME_LENGTH];

  // Get profile icon data
  uint8_t (*getProfileIcons())[16][2] { return icons; }

  // Store current profile index to file
  bool storeCurrentProfile();

  // Check if configuration is valid
  bool isConfigValid() const { return parser.isValid(); }

  // Get error message
  const char *getError() const { return parser.getError(); }

  // Check if SD card is detected
  bool isSDDetected() const { return sdDetected; }

  // Check if config file exists
  bool configExists() const { return configFileExists; }

  // Reinitialize SD card
  bool reinitializeSD();

  // Debug: Print configuration
  void printConfig() const;
  void printCurrentProfile() const;
  void printAllProfileNames() const;

private:
  // Constants
  static const char *CONFIG_PATH;
  static const char *LAST_PROFILE_PATH;

  // Components
  ConfigParser parser;

  // State
  bool sdDetected;
  bool configFileExists;
  uint8_t profileCount;
  uint8_t currentProfileIndex;
  const Profile *currentProfile;
  const WheelConfig *currentLedState;
  ScrollConfig currentScrollConfig;

  char profileNames[MAX_PROFILES][MAX_NAME_LENGTH];
  uint8_t icons[MAX_BUTTONS][16][2];

  // Icons, loaded only current icon set (Images numbered from 1-6 in folder
  // named after profile)

  // Private methods
  bool initializeSDCard();
  bool loadLastProfileIndex();
  bool loadProfileNames();
  bool loadProfileIcons();
  void updateCurrentProfile();

  bool loadBMP16x16(const char *filename, uint8_t icon[16][2]);
  uint16_t read16(File &f);
  uint32_t read32(File &f);
};

#endif // STORAGE_CONTROLLER_H