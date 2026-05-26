#include "FingerprintService.h"

#include <Adafruit_Fingerprint.h>

#include "DeviceConfig.h"

namespace {
HardwareSerial fingerprintSerial(2);
Adafruit_Fingerprint fingerprintSensor(&fingerprintSerial);
constexpr uint8_t MAX_CONSECUTIVE_PACKET_ERRORS = 80;
constexpr uint8_t FINGERPRINT_POLL_DELAY_MS = 200;
constexpr uint16_t FINGERPRINT_SETTLE_DELAY_MS = 300;
constexpr uint32_t FINGERPRINT_BAUD_CANDIDATES[] = {FINGERPRINT_BAUD_RATE, 115200, 19200, 9600};

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


bool waitForFingerImage(char* errorMessage, size_t errorLength) {
	uint8_t packetErrors = 0;

	while (true) {
		const uint8_t response = fingerprintSensor.getImage();

		if (response == FINGERPRINT_OK) {
			return true;
		}

		if (response == FINGERPRINT_NOFINGER) {
			packetErrors = 0;
			delay(FINGERPRINT_POLL_DELAY_MS);
			continue;
		}

		if (response == FINGERPRINT_PACKETRECIEVEERR) {
			packetErrors++;
			if (packetErrors >= MAX_CONSECUTIVE_PACKET_ERRORS) {
				copyText(errorMessage, errorLength, "Sensor packet");
				return false;
			}

			delay(FINGERPRINT_POLL_DELAY_MS);
			continue;
		}

		if (response != FINGERPRINT_NOFINGER) {
			copyText(errorMessage, errorLength, "Bad finger img");
			return false;
		}
	}
}

bool waitForFingerRelease(uint16_t timeoutMs, char* errorMessage = nullptr, size_t errorLength = 0) {
	const unsigned long startMs = millis();
	uint8_t packetErrors = 0;

	while (timeoutMs == 0 || millis() - startMs < timeoutMs) {
		const uint8_t response = fingerprintSensor.getImage();
		if (response == FINGERPRINT_NOFINGER) {
			return true;
		}

		if (response == FINGERPRINT_PACKETRECIEVEERR) {
			packetErrors++;
			if (packetErrors >= MAX_CONSECUTIVE_PACKET_ERRORS) {
				copyText(errorMessage, errorLength, "Sensor packet");
				return false;
			}
		} else {
			packetErrors = 0;
		}

		delay(FINGERPRINT_POLL_DELAY_MS);
	}

	copyText(errorMessage, errorLength, "Release finger");
	return false;
}

bool captureImageToBuffer(uint8_t bufferId, char* errorMessage, size_t errorLength) {
	if (!waitForFingerImage(errorMessage, errorLength)) {
		return false;
	}

	const uint8_t response = fingerprintSensor.image2Tz(bufferId);
	if (response != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, response == FINGERPRINT_IMAGEMESS ? "Messy print" : "Finger convert");
		return false;
	}

	return true;
}

int findFreeTemplateId(char* errorMessage, size_t errorLength) {
	const uint8_t countResponse = fingerprintSensor.getTemplateCount();
	if (countResponse != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, countResponse == FINGERPRINT_PACKETRECIEVEERR ? "Sensor packet" : "Sensor comm");
		return -1;
	}

	const int templateId = static_cast<int>(fingerprintSensor.templateCount) + 1;
	if (templateId <= 0 || templateId > FINGERPRINT_MAX_TEMPLATE_ID) {
		copyText(errorMessage, errorLength, "No free slot");
		return -1;
	}

	return templateId;
}

bool connectSensor() {
	uint32_t lastBaud = 0;

	for (size_t index = 0; index < sizeof(FINGERPRINT_BAUD_CANDIDATES) / sizeof(FINGERPRINT_BAUD_CANDIDATES[0]); ++index) {
		const uint32_t baud = FINGERPRINT_BAUD_CANDIDATES[index];
		if (baud == lastBaud) {
			continue;
		}

		fingerprintSerial.begin(baud, SERIAL_8N1, FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN);
		fingerprintSensor.begin(baud);
		delay(FINGERPRINT_SETTLE_DELAY_MS);

		if (fingerprintSensor.verifyPassword()) {
			return true;
		}

		lastBaud = baud;
	}

	return false;
}
}

FingerprintService::FingerprintService() {}

bool FingerprintService::begin(char* errorMessage, size_t errorLength) {
	if (!connectSensor()) {
		copyText(errorMessage, errorLength, "No FP sensor");
		return false;
	}

	copyText(errorMessage, errorLength, "");
	return true;
}

bool FingerprintService::searchFingerprint(int& templateId, char* errorMessage, size_t errorLength) {
	templateId = 0;
	if (!captureImageToBuffer(1, errorMessage, errorLength)) {
		return false;
	}

	const uint8_t response = fingerprintSensor.fingerFastSearch();
	waitForFingerRelease(5000);

	if (response != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, response == FINGERPRINT_NOTFOUND ? "No match" : "Search fail");
		return false;
	}

	templateId = fingerprintSensor.fingerID;
	return true;
}

bool FingerprintService::enrollFingerprint(int& templateId, char* errorMessage, size_t errorLength) {
	templateId = findFreeTemplateId(errorMessage, errorLength);
	if (templateId <= 0) {
		return false;
	}

	if (!captureImageToBuffer(1, errorMessage, errorLength)) {
		return false;
	}

	delay(2000);

	if (!waitForFingerRelease(0, errorMessage, errorLength)) {
		return false;
	}

	if (!captureImageToBuffer(2, errorMessage, errorLength)) {
		return false;
	}

	const uint8_t createResponse = fingerprintSensor.createModel();
	if (createResponse != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, createResponse == FINGERPRINT_ENROLLMISMATCH ? "Print mismatch" : "Model fail");
		return false;
	}

	const uint8_t storeResponse = fingerprintSensor.storeModel(templateId);
	if (storeResponse != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, "Store fail");
		return false;
	}

	waitForFingerRelease(5000);
	copyText(errorMessage, errorLength, "");
	return true;
}

bool FingerprintService::deleteTemplate(int templateId) {
	return fingerprintSensor.deleteModel(templateId) == FINGERPRINT_OK;
}

bool FingerprintService::clearDatabase(char* errorMessage, size_t errorLength) {
	const uint8_t response = fingerprintSensor.emptyDatabase();
	if (response != FINGERPRINT_OK) {
		copyText(errorMessage, errorLength, response == FINGERPRINT_PACKETRECIEVEERR ? "Sensor packet" : "Delete fail");
		return false;
	}

	copyText(errorMessage, errorLength, "");
	return true;
}
