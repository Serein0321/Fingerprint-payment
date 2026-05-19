#pragma once

#include <Arduino.h>

enum class AppScreen : uint8_t {
  Standby,
  MainMenu,
  WifiList,
  WifiPassword,
  StudentIdInput,
  AmountInput,
  Records,
  SystemInfo,
  Result
};

enum class ActionType : uint8_t {
  None,
  Enroll,
  Consume,
  Topup,
  Records
};

enum class ResultType : uint8_t {
  None,
  Enrollment,
  Payment
};

struct WifiNetworkInfo {
  char ssid[33];
  int32_t rssi;
  bool encrypted;
};

struct StudentLookupResult {
  bool valid;
  char studentId[24];
  char studentName[32];
  char balance[16];
};

struct EnrollmentResult {
  bool success;
  char studentId[24];
  char studentName[32];
  int fingerprintTemplateId;
  char balance[16];
  char message[48];
};

struct PaymentResult {
  bool success;
  char transactionType[12];
  char studentId[24];
  char studentName[32];
  int fingerprintTemplateId;
  char amount[16];
  char balance[16];
  char message[48];
};

struct TransactionRecordSummary {
  char transactionType[12];
  char amount[16];
  char balanceAfter[16];
  char result[12];
  char occurredAt[24];
  char message[32];
};

struct TransactionListResult {
  bool success;
  char studentId[24];
  char studentName[32];
  uint8_t count;
  TransactionRecordSummary items[3];
};

struct StatusSnapshot {
  bool wifiConnected;
  bool serverOnline;
  bool timeSynced;
  char timeText[16];
  char dateText[16];
  char ssid[33];
  char ipAddress[20];
};
