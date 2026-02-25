#include "../include/storage/StorageController.h"
#include "../include/constants/Wiring.h"
#include <SPI.h>

// Static constants
const char *StorageController::CONFIG_PATH = "/config.yaml";
const char *StorageController::LAST_PROFILE_PATH = "/lastProfile";

// Constructor
StorageController::StorageController()
    : sdDetected(false), configFileExists(false), profileCount(0),
      currentProfileIndex(0), currentProfile(nullptr),
      currentLedState(nullptr) {

  // Initialize profile names array
  memset(profileNames, 0, sizeof(profileNames));

  // Initialize current scroll config
  currentScrollConfig.type = ScrollType::VERTICAL;
  currentScrollConfig.key1 = 0;
  currentScrollConfig.key2 = 0;
}

// Initialize storage controller
bool StorageController::initialize() {
  Serial.println("[Storage] Initializing...");

  // Initialize SD card
  if (!initializeSDCard()) {
    Serial.println("[Storage] SD card initialization failed!");
    return false;
  }

  // Check for config file
  if (!SD.exists(CONFIG_PATH)) {
    Serial.println("[Storage] No config.yaml found!");
    configFileExists = false;
    return false;
  }

  configFileExists = true;

  // Load configuration from file
  if (!loadConfiguration()) {
    Serial.println("[Storage] Failed to load configuration!");
    return false;
  }

  // Load profile names
  if (!loadProfileNames()) {
    Serial.println("[Storage] Failed to load profile names!");
    return false;
  }

  // Load last active profile
  if (!loadLastProfileIndex()) {
    Serial.println("[Storage] No last profile found, using profile 0");
    currentProfileIndex = 0;
  }

  // Validate profile index
  if (currentProfileIndex >= profileCount) {
    Serial.printf("[Storage] Profile index %d out of bounds, using profile 0\n",
                  currentProfileIndex);
    currentProfileIndex = 0;
  }

  // Switch to current profile
  if (!switchProfile(currentProfileIndex)) {
    Serial.println("[Storage] Failed to switch to current profile!");
    return false;
  }

  Serial.println("[Storage] Initialization complete");
  return true;
}

// Initialize SD card
bool StorageController::initializeSDCard() {
  // Setup SPI pins from Wiring.h
  SPI.setRX(SD_MISO);
  SPI.setTX(SD_MOSI);
  SPI.setSCK(SD_SCK);

  // End any previous SD session
  SD.end();

  // Initialize SD card with chip select pin from Wiring.h
  if (!SD.begin(SD_CS)) {
    sdDetected = false;
    return false;
  }

  sdDetected = true;

  // Test SD card by checking root directory
  File root = SD.open("/");
  if (!root) {
    sdDetected = false;
    return false;
  }
  root.close();

  Serial.println("[Storage] SD card initialized successfully");
  return true;
}

// Reinitialize SD card
bool StorageController::reinitializeSD() {
  Serial.println("[Storage] Reinitializing SD card...");
  sdDetected = false;
  return initializeSDCard();
}

// Load configuration from YAML file
bool StorageController::loadConfiguration() {
  if (!sdDetected || !configFileExists) {
    Serial.println("[Storage] SD not detected or config file doesn't exist");
    return false;
  }

  File configFile = SD.open(CONFIG_PATH);
  if (!configFile) {
    Serial.println("[Storage] Failed to open config file");
    return false;
  }

  Serial.println("[Storage] Parsing configuration...");

  // Parse configuration using ConfigParser
  bool success = parser.parseFromFile(configFile);

  configFile.close();

  if (!success) {
    Serial.printf("[Storage] Configuration parsing failed: %s\n",
                  parser.getError());
    return false;
  }

  // Get profile count
  profileCount = parser.getProfileCount();
  if (profileCount == 0) {
    Serial.println("[Storage] No profiles found in configuration");
    return false;
  }

  Serial.printf("[Storage] Configuration loaded: %d profiles\n", profileCount);
  return true;
}

// Load last profile index from file
bool StorageController::loadLastProfileIndex() {
  if (!sdDetected) {
    return false;
  }

  if (!SD.exists(LAST_PROFILE_PATH)) {
    return false;
  }

  File file = SD.open(LAST_PROFILE_PATH);
  if (!file) {
    return false;
  }

  // Read the profile index
  String content = file.readString();
  file.close();

  content.trim();
  int index = content.toInt();

  if (index >= 0 && index < profileCount) {
    currentProfileIndex = index;
    Serial.printf("[Storage] Loaded last profile index: %d\n",
                  currentProfileIndex);
    return true;
  }

  return false;
}

// Load profile names into cache
bool StorageController::loadProfileNames() {
  if (!parser.isValid() || profileCount == 0) {
    return false;
  }

  const DeviceConfig *config = getConfig();
  if (!config) {
    return false;
  }

  // Copy profile names
  for (uint8_t i = 0; i < profileCount && i < MAX_PROFILES; i++) {
    strncpy(profileNames[i], config->profiles[i].name, MAX_NAME_LENGTH - 1);
    profileNames[i][MAX_NAME_LENGTH - 1] = '\0';
  }

  Serial.printf("[Storage] Loaded %d profile names\n", profileCount);
  return true;
}

// Load profile Icons
bool StorageController::loadProfileIcons() {
  if (!parser.isValid() || profileCount == 0) {
    return false;
  }

  const DeviceConfig *config = getConfig();
  if (!config) {
    return false;
  }

  // Get current profile name
  const char *profileName = config->profiles[currentProfileIndex].name;

  // Load icons numbered 1-6 from folder named after profile
  for (uint8_t iconNum = 1; iconNum <= 6; iconNum++) {
    char path[64];
    snprintf(path, sizeof(path), "/%s/%d.bmp", profileName, iconNum);

    if (!SD.exists(path)) {
      Serial.printf("[Storage] Icon file not found: %s\n", path);
      memset(icons[iconNum - 1], 0, sizeof(icons[iconNum - 1]));
      continue;
    }

    if (!loadBMP16x16(path, icons[iconNum - 1])) {
      Serial.printf("[Storage] Failed to load icon: %s\n", path);
      memset(icons[iconNum - 1], 0, sizeof(icons[iconNum - 1]));
      continue;
    }
  }

  return true;
}

bool StorageController::loadBMP16x16(const char *filename,
                                     uint8_t icon[16][2]) {
  File bmp = SD.open(filename);
  if (!bmp)
    return false;

  if (read16(bmp) != 0x4D42) { // "BM"
    bmp.close();
    return false;
  }

  bmp.seek(10);
  uint32_t pixelOffset = read32(bmp);

  bmp.seek(18);
  int32_t width = read32(bmp);
  int32_t height = read32(bmp);

  bmp.seek(28);
  uint16_t depth = read16(bmp);

  if (width != 16 || abs(height) != 16 || depth != 1) {
    bmp.close();
    return false;
  }

  bool bottomUp = height > 0;

  for (int row = 0; row < 16; row++) {
    int srcRow = bottomUp ? (15 - row) : row;
    bmp.seek(pixelOffset + srcRow * 4);

    uint8_t rowData[4];
    bmp.read(rowData, 4);

    // Copy only the first 2 bytes (16 pixels)
    icon[row][0] = rowData[0];
    icon[row][1] = rowData[1];
  }

  bmp.close();
  return true;
}

uint16_t StorageController::read16(File &f) {
  return f.read() | (f.read() << 8);
}

uint32_t StorageController::read32(File &f) {
  return (uint32_t)f.read() | ((uint32_t)f.read() << 8) |
         ((uint32_t)f.read() << 16) | ((uint32_t)f.read() << 24);
}

// Get profile name by index
const char *StorageController::getProfileName(uint8_t index) const {
  if (index < profileCount && index < MAX_PROFILES) {
    return profileNames[index];
  }
  return "Unknown";
}

// Switch to a specific profile
bool StorageController::switchProfile(uint8_t profileIndex) {
  if (profileIndex >= profileCount) {
    Serial.printf("[Storage] Profile index %d out of bounds (max: %d)\n",
                  profileIndex, profileCount - 1);
    return false;
  }

  const DeviceConfig *config = getConfig();
  if (!config) {
    Serial.println("[Storage] No configuration available");
    return false;
  }

  // Get the profile
  const Profile *profile = &config->profiles[profileIndex];

  // Update current state
  currentProfileIndex = profileIndex;
  currentProfile = profile;
  currentLedState = parser.getProfileLedState(profileIndex);

  // Copy scroll config (since it's not a pointer in the structure)
  currentScrollConfig = profile->scroll;

  Serial.printf("[Storage] Switched to profile %d: %s\n", profileIndex,
                profile->name);

  // Store the new profile index
  storeCurrentProfile();
  loadProfileIcons();

  return true;
}

// Switch to next profile
bool StorageController::switchToNextProfile() {
  if (profileCount == 0) {
    return false;
  }

  uint8_t nextProfile = (currentProfileIndex + 1) % profileCount;
  return switchProfile(nextProfile);
}

// Switch to previous profile
bool StorageController::switchToPreviousProfile() {
  if (profileCount == 0) {
    return false;
  }

  uint8_t prevProfile =
      (currentProfileIndex == 0) ? profileCount - 1 : currentProfileIndex - 1;
  return switchProfile(prevProfile);
}

// Store current profile index to file
bool StorageController::storeCurrentProfile() {
  if (!sdDetected) {
    return false;
  }

  // Remove old file if exists
  if (SD.exists(LAST_PROFILE_PATH)) {
    if (!SD.remove(LAST_PROFILE_PATH)) {
      Serial.println("[Storage] Failed to remove old profile file");
      return false;
    }
  }

  File file = SD.open(LAST_PROFILE_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("[Storage] Failed to create profile file");
    return false;
  }

  file.print(currentProfileIndex);
  file.close();

  Serial.printf("[Storage] Stored current profile index: %d\n",
                currentProfileIndex);
  return true;
}

// Update current profile (internal helper)
void StorageController::updateCurrentProfile() {
  if (!parser.isValid() || currentProfileIndex >= profileCount) {
    currentProfile = nullptr;
    currentLedState = nullptr;
    return;
  }

  const DeviceConfig *config = getConfig();
  if (config) {
    currentProfile = &config->profiles[currentProfileIndex];
    currentLedState = parser.getProfileLedState(currentProfileIndex);
    currentScrollConfig = currentProfile->scroll;
  }
}

// Debug: Print full configuration
void StorageController::printConfig() const {
  if (!parser.isValid()) {
    Serial.println("[Storage] Configuration is not valid");
    return;
  }

  const DeviceConfig *config = getConfig();
  if (!config) {
    Serial.println("[Storage] No configuration available");
    return;
  }

  Serial.println("\n=== Full Device Configuration ===");

  // Print global LED state
  Serial.println("\n[Global LED State]");
  Serial.printf("  Type: %s\n",
                ConfigParser::ledTypeToString(config->globalLedState.type));
  if (config->globalLedState.hasConfiguration) {
    switch (config->globalLedState.type) {
    case WheelSimulation::NOTCHY:
      Serial.printf("  Brightness: %d\n",
                    config->globalLedState.state.solid.brightness);
      Serial.printf("  Color: 0x%06X\n",
                    config->globalLedState.state.solid.color);
      break;
    case WheelSimulation::CLICKY:
      Serial.printf("  Color 1: 0x%06X\n",
                    config->globalLedState.state.clicky.color1);
      Serial.printf("  Color 2: 0x%06X\n",
                    config->globalLedState.state.clicky.color2);
      break;
    }
  } else {
    Serial.println("  No configuration (will use defaults)");
  }

  // Print profiles
  Serial.printf("\n[Profiles: %d total]\n", profileCount);
  for (uint8_t i = 0; i < profileCount; i++) {
    const Profile &profile = config->profiles[i];
    Serial.printf("\n  Profile %d: %s\n", i, profile.name);

    // Print profile LED state if it has one
    if (profile.wheelConfig.hasConfiguration) {
      Serial.printf("    LED: %s\n",
                    ConfigParser::ledTypeToString(profile.wheelConfig.type));
    } else {
      Serial.println("    LED: (uses global)");
    }

    Serial.printf("    Scroll: %s\n",
                  ConfigParser::scrollTypeToString(profile.scroll.type));
    if (profile.scroll.type == ScrollType::KEY_EMULATION) {
      Serial.printf("      Keys: 0x%04X, 0x%04X\n", profile.scroll.key1,
                    profile.scroll.key2);
    }

    Serial.printf("    Buttons: %d\n", profile.buttonCount);
    for (uint8_t j = 0; j < profile.buttonCount; j++) {
      const ButtonConfig &button = profile.buttons[j];
      Serial.printf("      %s: [%s] ", button.name,
                    ConfigParser::pressTypeToString(button.pressType));

      Serial.print("Keys: [");
      for (uint8_t k = 0; k < button.sequenceLength; k++) {
        if (k > 0)
          Serial.print(", ");
        Serial.printf("0x%04X", button.sequence[k]);
      }
      Serial.print("]");

      Serial.println();
    }
  }

  Serial.println("\n=== End Configuration ===");
}

// Debug: Print current profile
void StorageController::printCurrentProfile() const {
  if (!currentProfile) {
    Serial.println("[Storage] No current profile");
    return;
  }

  Serial.println("\n=== Current Profile ===");
  Serial.printf("Index: %d\n", currentProfileIndex);
  Serial.printf("Name: %s\n", currentProfile->name);

  // LED state
  if (currentLedState && currentLedState->hasConfiguration) {
    Serial.printf("LED Type: %s\n",
                  ConfigParser::ledTypeToString(currentLedState->type));
    switch (currentLedState->type) {
    case WheelSimulation::NOTCHY:
      Serial.printf("  Max Left: 0x%06X\n",
                    currentLedState->state.twist.maxLeftColor);
      Serial.printf("  Neutral: 0x%06X\n",
                    currentLedState->state.twist.neutralColor);
      Serial.printf("  Max Right: 0x%06X\n",
                    currentLedState->state.twist.maxRightColor);
      break;
    case WheelSimulation::CLICKY:
      Serial.printf("  Color 1: 0x%06X\n",
                    currentLedState->state.clicky.color1);
      Serial.printf("  Color 2: 0x%06X\n",
                    currentLedState->state.clicky.color2);
      break;
    }
  } else {
    Serial.println("LED: Using global configuration");
  }

  // Scroll
  Serial.printf("Scroll: %s\n",
                ConfigParser::scrollTypeToString(currentScrollConfig.type));
  if (currentScrollConfig.type == ScrollType::KEY_EMULATION) {
    Serial.printf("  Keys: 0x%04X, 0x%04X\n", currentScrollConfig.key1,
                  currentScrollConfig.key2);
  }

  // Buttons
  Serial.printf("Buttons: %d\n", currentProfile->buttonCount);
  for (uint8_t i = 0; i < currentProfile->buttonCount; i++) {
    const ButtonConfig &button = currentProfile->buttons[i];
    Serial.printf("  %d. %s [%s]: ", i + 1, button.name,
                  ConfigParser::pressTypeToString(button.pressType));

    Serial.print("[");
    for (uint8_t j = 0; j < button.sequenceLength; j++) {
      if (j > 0)
        Serial.print(", ");
      Serial.printf("0x%04X", button.sequence[j]);
    }
    Serial.print("]");

    Serial.println();
  }
  Serial.println("=== End Current Profile ===");
}

// Debug: Print all profile names
void StorageController::printAllProfileNames() const {
  Serial.println("\n=== Available Profiles ===");
  for (uint8_t i = 0; i < profileCount; i++) {
    if (i == currentProfileIndex) {
      Serial.printf("  * %d: %s (current)\n", i, getProfileName(i));
    } else {
      Serial.printf("    %d: %s\n", i, getProfileName(i));
    }
  }
  Serial.println("===========================");
}

// Getter

char (*StorageController::getProfileNames())[MAX_NAME_LENGTH] {
  return profileNames;
}
