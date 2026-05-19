#pragma once

#include <Arduino.h>
#include <stddef.h>

#include "ApiClient.h"
#include "DeviceConfig.h"
#include "DeviceState.h"
#include "FingerprintService.h"
#include "KeypadInput.h"
#include "NetworkService.h"
#include "OledUi.h"

class PaymentFlow {
public:
  PaymentFlow(KeypadInput& keypadInput, OledUi& oledUi, NetworkService& networkService, ApiClient& apiClient, FingerprintService& fingerprintService);

  void begin();
  void tick();

private:
  KeypadInput& _keypadInput;
  OledUi& _oledUi;
  NetworkService& _networkService;
  ApiClient& _apiClient;
  FingerprintService& _fingerprintService;

  AppScreen _screen;
  ActionType _pendingAction;
  ResultType _resultType;
  bool _serverOnline;
  unsigned long _lastStandbyRefreshMs;
  unsigned long _lastHealthCheckMs;
  uint8_t _menuIndex;
  uint8_t _wifiCount;
  uint8_t _wifiIndex;
  uint8_t _recordPage;
  char _entryBuffer[MAX_STUDENT_ID_LENGTH + 1];
  char _passwordBuffer[MAX_PASSWORD_LENGTH + 1];
  WifiNetworkInfo _wifiNetworks[MAX_WIFI_NETWORKS];
  StudentLookupResult _studentLookup;
  EnrollmentResult _enrollmentResult;
  PaymentResult _paymentResult;
  TransactionListResult _transactionList;

  void handleAction(KeyAction action);
  void handleStandby(KeyAction action);
  void handleMainMenu(KeyAction action);
  void handleWifiList(KeyAction action);
  void handleWifiPassword(KeyAction action);
  void handleStudentInput(KeyAction action);
  void handleAmountInput(KeyAction action);
  void handleRecords(KeyAction action);
  void handleSystemInfo(KeyAction action);
  void handleResult(KeyAction action);

  void openMenuSelection();
  void scanWifiNetworks();
  void connectSelectedWifi();
  void processEnrollment();
  void processPayment(ActionType action);
  void processRecordLookup();

  void updateServerStatus(bool force);
  bool ensureWifiConnection();
  void showTemporaryMessage(const char* title, const char* line1, const char* line2 = "", const char* line3 = "", unsigned long pauseMs = 1200UL);
  void clearBuffer(char* buffer, size_t length);
  void removeLastCharacter(char* buffer);
  void appendDigit(char* buffer, size_t length, char digit);
  void appendAmountCharacter(KeyAction action);
  void renderCurrentScreen();
};
