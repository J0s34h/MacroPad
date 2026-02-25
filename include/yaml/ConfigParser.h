#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "../constants/Config.h"
#include <Arduino.h>
#include <SD.h>

// Parser Class
class ConfigParser {
public:
  ConfigParser();

  // Parse YAML from File object
  bool parseFromFile(File &file);

  // Parse YAML from string (for testing)
  bool parseFromString(const char *yamlString);

  // Get parsed configuration
  const DeviceConfig &getConfig() const { return config; }

  // Check if parsing was successful
  bool isValid() const { return valid; }

  // Get error message if parsing failed
  const char *getError() const { return errorMessage; }

  // Get specific profile
  const Profile *getProfile(uint8_t index) const {
    return (index < config.profileCount) ? &config.profiles[index] : nullptr;
  }

  // Get global LED state
  const WheelConfig *getGlobalLedState() const {
    return &config.globalLedState;
  }

  // Get profile-specific LED state (returns global if profile doesn't have
  // specific)
  const WheelConfig *getProfileLedState(uint8_t profileIndex) const {
    if (profileIndex >= config.profileCount) {
      return &config.globalLedState;
    }

    const Profile &profile = config.profiles[profileIndex];
    return profile.wheelConfig.hasConfiguration ? &profile.wheelConfig
                                                : &config.globalLedState;
  }

  // Get total number of profiles
  uint8_t getProfileCount() const { return config.profileCount; }

  // Helper functions for debugging
  static const char *ledTypeToString(WheelSimulation type);
  static const char *scrollTypeToString(ScrollType type);
  static const char *pressTypeToString(PressType type);

private:
  DeviceConfig config;
  bool valid;
  char errorMessage[64];

  // File parsing state
  struct ParseState {
    char *buffer;
    char *current;
    char *end;
    int lineNumber;
  };

  // Type conversion
  WheelSimulation parseLedType(const char *str);
  ScrollType parseScrollType(const char *str);
  PressType parsePressType(const char *str);

  // Error handling
  void setError(const char *message);
  void resetConfig();

  // Initialize LED state to defaults
  void initLedState(WheelConfig &ledState);
};

#endif // CONFIG_PARSER_H