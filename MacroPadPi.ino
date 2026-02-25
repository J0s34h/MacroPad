#include <Arduino.h>;

// Modules
#include "./include/constants/HIDDescriptor.h";
#include "./include/constants/Wiring.h";
#include "./include/yaml/ConfigParser.h";
#include "./include/storage/StorageController.h";
#include "./include/wheel/WheelController.h";
#include "./include/buttons/ButtonManager.h";
#include "./include/usb/HidManager.h";
#include "./include/video/GraphicsDriver.h";

//ThirdParty

#include <SimpleFOC.h>;
#include <U8g2lib.h>;
#include <Adafruit_TinyUSB.h>;
#include <SPI.h>;
#include <FastLED.h>;
#include <SD.h>;
// TODO: - Implement Flashing button
//#include <pico/bootrom.h>;

// MARK: - Initiating variables

// ==== HARDWARE ====
StorageController storage = StorageController();

// Wheel vairables
Encoder encoder = Encoder(ENCODER_A_PIN, ENCODER_B_PIN, ENCODER_PPR);
BLDCMotor motor = BLDCMotor(MOTOR_POLE_PAIRS);
BLDCDriver6PWM driver = BLDCDriver6PWM(DRV_AH, DRV_AL, DRV_BH, DRV_BL, DRV_CH, DRV_CL);

// Wheel controller instance
WheelController* WheelController::instance_ = nullptr;
WheelController *wheelController = WheelController::initialize(driver, motor, encoder);

//Keyboars
Adafruit_USBD_HID usb_keyboard(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_KEYBOARD, 2, false);
Adafruit_USBD_HID usb_mouse(desc_mouse_report, sizeof(desc_mouse_report), HID_ITF_PROTOCOL_MOUSE, 4, false);

//Display
U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI u8g2(U8G2_R0, DISPLAY_SCK, DISPLAY_MOSI, DISPLAY_CS, DISPLAY_DC, DISPLAY_RESET);

// U8G2_SSD1309_128X64_NONAME0_1_4W_HW_SPI u8g2(U8G2_R0 , uint8_t DISPLAY_CS, uint8_t DISPLAY_DC, uint8_t DISPLAY_RESET)

// HID manager
HidManager hidManager;

// Display Driver
GraphicsDriver graphicsDriver = GraphicsDriver(u8g2);

// MARK: - Application State Machine

// Application State
enum ApplicationState {
    ERROR,
    PROFILE_SELECTION,
    ACTIVE_PROFILE
};

ApplicationState appState = ERROR;

// ADDED: Display sleep management
volatile unsigned long lastActivityTime = 0;          // last user activity timestamp (ms)
volatile bool displayOff = false;                     // current display power state
volatile bool wakeDisplayFlag = false;                 // set by core0 to request display wake
const unsigned long DISPLAY_TIMEOUT = 100000;          // 5 minutes in ms

// Forwarder used as a plain function pointer for ButtonManager
void sendKeyForward(const uint16_t* codes, size_t count) {
    // ADDED: activity detected
    lastActivityTime = millis();
    wakeDisplayFlag = true;
    
    hidManager.handleHidKeys(codes, count);
}

//
void sendProfileChangeCallback(ProfileButtonAction action) {
    // ADDED: activity detected
    lastActivityTime = millis();
    wakeDisplayFlag = true;

    switch (action) {
        case CANCEL_ACTION:
            rp2040.rebootToBootloader();
            
            if (appState == ERROR) {
                return;
            }

            graphicsDriver.setState(DisplayState::STATE_PROFILE_INTERFACE);
            wheelController->setProfileSelectionMode(false);
            break;  
        case SET_ACTION:
            switch (appState) {
            case ACTIVE_PROFILE:
                graphicsDriver.setState(DisplayState::STATE_PROFILE_SELECTION);
                appState = PROFILE_SELECTION;
                wheelController->setProfileSelectionMode(true);

                break;
            case PROFILE_SELECTION:
                appState = ACTIVE_PROFILE;
                // Set new Profile
                wheelController->setProfileSelectionMode(false);
                setupNewProfile(graphicsDriver.getSelectedProfileIdx());

                graphicsDriver.setState(DisplayState::STATE_PROFILE_INTERFACE);
                break;
            default:
                break;
            }
    }
}

void wheelActionCallback(WheelAction action) {
    // ADDED: activity detected
    lastActivityTime = millis();
    wakeDisplayFlag = true;

    switch (action) {
        case SCROLL_DOWN:
            graphicsDriver.scrollDown();
            break;    
        case SCROLL_UP:
            graphicsDriver.scrollUp();
            break;
    }
}

// Button manager instance
ButtonManager buttonManager;

// MARK: - Setup Core 0
void setup() {
    usb_keyboard.begin();
    usb_mouse.begin();
   
    // Initialize USB
    Serial.begin(115200);

    u8g2.begin();

    graphicsDriver.setState(DisplayState::STATE_ERROR);

    // Initialize storage
    if (!storage.initialize()) {
        Serial.println("Failed to initialize storage!");
        Serial.print("Error: ");
        Serial.println(storage.getError());
        
        // Try to reinitialize SD card
        delay(1000);
        if (storage.reinitializeSD()) {
            Serial.println("SD card reinitialized, retrying...");
            if (!storage.initialize()) {
                Serial.println("Still failed. Check SD card and config file.");
                while (1) delay(1000); // Halt if storage fails
            }
        } else {
            Serial.println("SD card reinitialization failed.");
            while (1) delay(1000);
        }        
    }

    appState = ACTIVE_PROFILE;

    // Storage is initialized, print config for debugging
    // Print configuration for debugging
    storage.printConfig();

    // Print current profile
    storage.printCurrentProfile();
    // Initialize ButtonManager and register key-send callback
    // Initialize HID manager and wire into wheel/button modules
    hidManager.begin(&usb_keyboard, &usb_mouse);

    wheelController->setHidManager(&hidManager);
    wheelController->setWheelActionCallback(wheelActionCallback);

    buttonManager.setButtonPressCallback(sendKeyForward);
    buttonManager.setProfileChangePressCallback(sendProfileChangeCallback);

    graphicsDriver.setProfileList(storage.getProfileNames(), storage.getProfileCount(), storage.getCurrentProfileIndex());
    graphicsDriver.setSelectedProfile(storage.getCurrentProfileIndex());

    setupNewProfile(storage.getCurrentProfileIndex());

    // Sleep timer
    lastActivityTime = millis();
    displayOff = false;
    wakeDisplayFlag = false;
}

// MARK: - Setup Core 1
void setup1() {
    // Hardware
    wheelController->setup();
}

// MARK: - Loop functions

void loop() {
    buttonManager.poll();  
}

void loop1() {
    wheelController->poll();
    graphicsDriver.poll();

    // ADDED: display sleep handling (core1 only)
    // Wake display if requested
    if (wakeDisplayFlag) {
        if (displayOff) {
            u8g2.setPowerSave(false);   // turn on
            delay(100);
            graphicsDriver.setNeedsDisplay();
            graphicsDriver.setState(graphicsDriver.currentState);
            wheelController->setPowerSave(false);
            displayOff = false;
        }
        wakeDisplayFlag = false;
    }

    // Check timeout if display is on
    if (!displayOff) {
        unsigned long now = millis();
        // Handle millis overflow (wrap-around safe)
        if (now - lastActivityTime >= DISPLAY_TIMEOUT) {
            u8g2.setPowerSave(true);    // turn off
            wheelController->setPowerSave(true);
            displayOff = true;
        }
    }
}

// MARK: - Helpers

// setsUpWithProfile
void setupNewProfile(int index) { 
    storage.switchProfile(index);

    buttonManager.setProfile(storage.getCurrentProfile());
    wheelController->setProfile(storage.getCurrentProfile());

    graphicsDriver.setProfile(storage.getCurrentProfile());
    graphicsDriver.setProfileIcons(storage.getProfileIcons());

    graphicsDriver.setState(DisplayState::STATE_PROFILE_INTERFACE);
};