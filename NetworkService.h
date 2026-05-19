#pragma once

#include <Arduino.h>

#include "DeviceState.h"

class NetworkService {
public:
  NetworkService();

  void begin();
  void tick();

  uint8_t scanNetworks(WifiNetworkInfo* results, uint8_t maxResults);
  bool connectToWifi(const char* ssid, const char* password, char* errorMessage, size_t errorLength);
  bool syncClock(char* errorMessage, size_t errorLength);

  bool isWifiConnected() const;
  bool isTimeSynced() const;
  void fillStatusSnapshot(StatusSnapshot& status, bool serverOnline) const;
  void getIpAddress(char* buffer, size_t length) const;

private:
  bool _timeSynced;
};
