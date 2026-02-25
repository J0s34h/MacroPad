#ifndef WIRING_H
#define WIRING_H

#include <Arduino.h>

// ===== DATA CONFIGURATION =====

#define MAX_PROFILES 128
#define MAX_BUTTONS 6
#define MAX_KEY_SEQUENCE 3
#define MAX_NAME_LENGTH 32
#define MAX_BUTTON_NAME 16

// ===== LED CONFIGURATION =====
const int LED_NUM = 20;
const int LED_DATA_PIN = 15;

// ===== SD CARD CONFIGURATION =====
// SD Card SPI pins
const int SD_MISO = 4;
const int SD_MOSI = 3;
const int SD_CS = 5;
const int SD_SCK = 2;

// ===== DISPLAY CONFIGURATION (U8G2) =====
// SSD1309 128x64 OLED using 4-wire software SPI
const int DISPLAY_SCK = 28;  // Clock
const int DISPLAY_MOSI = 22; // Data
const int DISPLAY_CS = 6;    // Chip Select
const int DISPLAY_DC = 7;    // Data/Command
const int DISPLAY_RESET = 8; // Reset

// ===== MOTOR & ENCODER CONFIGURATION (SimpleFOC) =====
// BLDCMotor configuration
const int MOTOR_POLE_PAIRS = 7;
const int POWER_SUPPLY_VOLTAGE = 5;

// 6PWM Driver pins
const int DRV_AH = 20; // Phase A high-side
const int DRV_AL = 21; // Phase A low-side
const int DRV_BH = 18; // Phase B high-side
const int DRV_BL = 19; // Phase B low-side
const int DRV_CH = 16; // Phase C high-side
const int DRV_CL = 17; // Phase C low-side

// Encoder pins
const int ENCODER_A_PIN = 27;
const int ENCODER_B_PIN = 26;

// Updated PPR
const int ENCODER_PPR = 1024; // Pulses per revolution

// ===== BUTTON CONFIGURATION =====
const int BUTTON_COUNT = 8;
const int BUTTON_PINS[BUTTON_COUNT] = {9, 1, 0, 12, 11, 10, 13, 14};

#endif