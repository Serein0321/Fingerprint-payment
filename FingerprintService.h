#pragma once

#include <Arduino.h>

class FingerprintService {
public:
  FingerprintService();

  bool begin(char* errorMessage, size_t errorLength);
  bool searchFingerprint(int& templateId, char* errorMessage, size_t errorLength);
  bool enrollFingerprint(int& templateId, char* errorMessage, size_t errorLength);
  bool deleteTemplate(int templateId);
  bool clearDatabase(char* errorMessage, size_t errorLength);
};
