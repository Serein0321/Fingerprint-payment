#pragma once

#include <Arduino.h>

#include "DeviceState.h"

class OledUi {
public:
  bool begin();

  void renderStandby(const StatusSnapshot& status);
  void renderMenu(uint8_t selectedIndex);
  void renderWifiList(const WifiNetworkInfo* networks, uint8_t count, uint8_t selectedIndex);
  void renderWifiPassword(const char* ssid, const char* password);
  void renderTextInput(const char* title, const char* prompt, const char* value);
  void renderAmountInput(const char* title, const char* amountValue);
  void renderMessage(const char* title, const char* line1, const char* line2 = "", const char* line3 = "", const char* line4 = "");
  void renderEnrollmentResult(const EnrollmentResult& result);
  void renderPaymentResult(const PaymentResult& result);
  void renderTransactions(const TransactionListResult& result, uint8_t pageIndex);
  void renderSystemInfo(const StatusSnapshot& status, const char* deviceId, const char* baseUrl, bool serverOnline);
};
