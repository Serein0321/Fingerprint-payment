#pragma once

#include <Arduino.h>

constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 4;

static const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

static const byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {13, 12, 14, 27};
static const byte KEYPAD_COL_PINS[KEYPAD_COLS] = {26, 25, 33, 32};

constexpr uint8_t OLED_SCREEN_WIDTH = 128;
constexpr uint8_t OLED_SCREEN_HEIGHT = 64;
constexpr int8_t OLED_RESET_PIN = -1;
constexpr uint8_t OLED_SDA_PIN = 21;
constexpr uint8_t OLED_SCL_PIN = 22;

constexpr uint8_t FINGERPRINT_RX_PIN = 17;
constexpr uint8_t FINGERPRINT_TX_PIN = 16;
constexpr uint32_t FINGERPRINT_BAUD_RATE = 57600;
constexpr uint16_t FINGERPRINT_MAX_TEMPLATE_ID = 127;

constexpr unsigned long WIFI_SCAN_TIMEOUT_MS = 12000UL;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000UL;
constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 15000UL;
constexpr unsigned long HEALTH_CHECK_INTERVAL_MS = 30000UL;
constexpr unsigned long STANDBY_REFRESH_INTERVAL_MS = 1000UL;
constexpr uint8_t MAX_WIFI_NETWORKS = 8;
constexpr uint8_t MAX_PASSWORD_LENGTH = 20;
constexpr uint8_t MAX_STUDENT_ID_LENGTH = 20;
constexpr uint8_t MAX_AMOUNT_LENGTH = 12;
constexpr uint8_t MAX_RECORDS = 3;
constexpr uint16_t HTTP_TIMEOUT_MS = 8000;

static const char DEVICE_ID[] = "esp32-fp-01";
static const char API_BASE_URL[] = "http://192.168.1.100:3000";
static const char NTP_SERVER_PRIMARY[] = "pool.ntp.org";
static const char NTP_SERVER_SECONDARY[] = "time.nist.gov";
constexpr long GMT_OFFSET_SECONDS = 8 * 3600;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
