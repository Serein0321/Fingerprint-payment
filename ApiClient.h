#pragma once

#include <Arduino.h>

#include "DeviceState.h"

class ApiClient {
public:
  explicit ApiClient(const char* baseUrl);

  bool healthCheck(char* errorMessage, size_t errorLength);
  bool lookupStudent(const char* studentId, StudentLookupResult& result, char* errorMessage, size_t errorLength);
  bool enrollFingerprint(const char* studentId, int templateId, EnrollmentResult& result, char* errorMessage, size_t errorLength);
  bool submitPayment(ActionType action, int templateId, const char* amount, PaymentResult& result, char* errorMessage, size_t errorLength);
  bool fetchRecentTransactions(const char* studentId, uint8_t limit, TransactionListResult& result, char* errorMessage, size_t errorLength);

  const char* getBaseUrl() const;

private:
  const char* _baseUrl;
};
