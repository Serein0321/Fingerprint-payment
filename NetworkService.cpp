#include "NetworkService.h"

#include <WiFi.h>
#include <time.h>

#include "DeviceConfig.h"

namespace {
void copyText(char* destination, size_t length, const char* source) {
	if (length == 0) {
		return;
	}

	if (source == nullptr) {
		destination[0] = '\0';
		return;
	}

	strncpy(destination, source, length - 1);
	destination[length - 1] = '\0';
}

void setError(char* errorMessage, size_t errorLength, const char* value) {
	copyText(errorMessage, errorLength, value);
}
}

NetworkService::NetworkService()
	: _timeSynced(false) {}

void NetworkService::begin() {
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.begin();
}

void NetworkService::tick() {}

uint8_t NetworkService::scanNetworks(WifiNetworkInfo* results, uint8_t maxResults) {
	const int found = WiFi.scanNetworks();
	const uint8_t count = found < 0 ? 0 : static_cast<uint8_t>(found > maxResults ? maxResults : found);

	for (uint8_t index = 0; index < count; ++index) {
		copyText(results[index].ssid, sizeof(results[index].ssid), WiFi.SSID(index).c_str());
		results[index].rssi = WiFi.RSSI(index);
		results[index].encrypted = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
	}

	WiFi.scanDelete();
	return count;
}

bool NetworkService::connectToWifi(const char* ssid, const char* password, char* errorMessage, size_t errorLength) {
	setError(errorMessage, errorLength, "");
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(200);
	WiFi.begin(ssid, password);

	const unsigned long startMs = millis();
	while (millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
		if (WiFi.status() == WL_CONNECTED) {
			return true;
		}

		delay(250);
	}

	switch (WiFi.status()) {
		case WL_NO_SSID_AVAIL:
			setError(errorMessage, errorLength, "SSID not found");
			break;
		case WL_CONNECT_FAILED:
			setError(errorMessage, errorLength, "Wrong password");
			break;
		case WL_CONNECTION_LOST:
			setError(errorMessage, errorLength, "Link lost");
			break;
		default:
			setError(errorMessage, errorLength, "WiFi timeout");
			break;
	}

	return false;
}

bool NetworkService::syncClock(char* errorMessage, size_t errorLength) {
	setError(errorMessage, errorLength, "");
	configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);

	const unsigned long startMs = millis();
	while (millis() - startMs < NTP_SYNC_TIMEOUT_MS) {
		struct tm timeInfo;
		if (getLocalTime(&timeInfo, 200)) {
			_timeSynced = true;
			return true;
		}

		delay(200);
	}

	_timeSynced = false;
	setError(errorMessage, errorLength, "NTP timeout");
	return false;
}

bool NetworkService::isWifiConnected() const {
	return WiFi.status() == WL_CONNECTED;
}

bool NetworkService::isTimeSynced() const {
	return _timeSynced;
}

void NetworkService::fillStatusSnapshot(StatusSnapshot& status, bool serverOnline) const {
	struct tm timeInfo;
	const bool hasTime = getLocalTime(&timeInfo, 10);

	status.wifiConnected = isWifiConnected();
	status.serverOnline = serverOnline;
	status.timeSynced = _timeSynced && hasTime;

	if (status.timeSynced) {
		strftime(status.timeText, sizeof(status.timeText), "%H:%M:%S", &timeInfo);
		strftime(status.dateText, sizeof(status.dateText), "%m-%d %a", &timeInfo);
	} else {
		copyText(status.timeText, sizeof(status.timeText), "--:--:--");
		copyText(status.dateText, sizeof(status.dateText), "No NTP");
	}

	copyText(status.ssid, sizeof(status.ssid), status.wifiConnected ? WiFi.SSID().c_str() : "Offline");
	getIpAddress(status.ipAddress, sizeof(status.ipAddress));
}

void NetworkService::getIpAddress(char* buffer, size_t length) const {
	if (!isWifiConnected()) {
		copyText(buffer, length, "IP:0.0.0.0");
		return;
	}

	const IPAddress ipAddress = WiFi.localIP();
	snprintf(buffer, length, "IP:%u.%u.%u.%u", ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]);
}
