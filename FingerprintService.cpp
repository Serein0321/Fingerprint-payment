#include "FingerprintService.h"

#include <Adafruit_Fingerprint.h>

#include "DeviceConfig.h"

namespace {
HardwareSerial fingerprintSerial(2);
Adafruit_Fingerprint fingerprintSensor(&fingerprintSerial);

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

bool waitForFingerImage(uint16_t timeoutMs, char* errorMessage, size_t errorLength) {
	const unsigned long startMs = millis();

	while (millis() - startMs < timeoutMs) {
		const uint8_t response = fingerprintSensor.getImage();

		if (response == FINGERPRINT_OK) {
			return true;
		}

		if (response != FINGERPRINT_NOFINGER && response != FINGERPRINT_PACKETRECIEVEERR) {
			copyText(errorMessage, errorLength, "Bad finger img");
			return false;
		}

		delay(80);
	}

	copyText(errorMessage, errorLength, "Finger timeout");
	return false;
}

bool waitForFingerRelease(uint16_t timeoutMs) {
	const unsigned long startMs = millis();

	while (millis() - startMs < timeoutMs) {
		if (fingerprintSensor.getImage() == FINGERPRINT_NOFINGER) {
			return true;
		}

		delay(80);
	}

	return false;
}

bool captureImageToBuffer(uint8_t bufferId, char* errorMessage, size_t errorLength) {
	if (!waitForFingerImage(15000, errorMessage, errorLength)) {
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
	for (uint16_t templateId = 1; templateId <= FINGERPRINT_MAX_TEMPLATE_ID; ++templateId) {
		const uint8_t response = fingerprintSensor.loadModel(templateId);

		if (response == FINGERPRINT_OK) {
			continue;
		}

		if (response == FINGERPRINT_PACKETRECIEVEERR) {
			copyText(errorMessage, errorLength, "Sensor packet");
			return -1;
		}

		return templateId;
	}

	copyText(errorMessage, errorLength, "No free slot");
	return -1;
}
}

FingerprintService::FingerprintService() {}

bool FingerprintService::begin(char* errorMessage, size_t errorLength) {
	fingerprintSerial.begin(FINGERPRINT_BAUD_RATE, SERIAL_8N1, FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN);
	fingerprintSensor.begin(FINGERPRINT_BAUD_RATE);
	delay(200);

	if (!fingerprintSensor.verifyPassword()) {
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

	if (!waitForFingerRelease(10000)) {
		copyText(errorMessage, errorLength, "Release finger");
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
