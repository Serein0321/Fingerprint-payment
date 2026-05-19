#include "OledUi.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "DeviceConfig.h"

namespace {
Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

const char* const MENU_ITEMS[] = {
	"WiFi Connect",
	"Enroll FP",
	"Consume",
	"Top Up",
	"Records",
	"Sys Info"
};

void sanitizeText(const char* source, char* destination, size_t length) {
	if (length == 0) {
		return;
	}

	if (source == nullptr) {
		destination[0] = '\0';
		return;
	}

	size_t writeIndex = 0;

	for (size_t index = 0; source[index] != '\0' && writeIndex + 1 < length; ++index) {
		const unsigned char ch = static_cast<unsigned char>(source[index]);
		destination[writeIndex++] = (ch >= 32 && ch <= 126) ? static_cast<char>(ch) : '?';
	}

	destination[writeIndex] = '\0';
}

void drawHeader(const char* title) {
	char safeTitle[22];
	sanitizeText(title, safeTitle, sizeof(safeTitle));
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.println(safeTitle);
	display.drawLine(0, 9, OLED_SCREEN_WIDTH - 1, 9, SSD1306_WHITE);
}

void drawLineText(int16_t x, int16_t y, const char* text) {
	char safeText[24];
	sanitizeText(text, safeText, sizeof(safeText));
	display.setCursor(x, y);
	display.print(safeText);
}

void drawKeyHint(const char* leftHint, const char* rightHint) {
	display.drawLine(0, 55, OLED_SCREEN_WIDTH - 1, 55, SSD1306_WHITE);
	drawLineText(0, 57, leftHint);
	drawLineText(72, 57, rightHint);
}

void formatRecordTime(const char* rawValue, char* output, size_t length) {
	if (length == 0) {
		return;
	}

	output[0] = '\0';

	if (rawValue == nullptr || rawValue[0] == '\0') {
		return;
	}

	if (strlen(rawValue) >= 16 && rawValue[4] == '-' && rawValue[7] == '-') {
		snprintf(output, length, "%c%c-%c%c %c%c:%c%c",
			rawValue[5], rawValue[6], rawValue[8], rawValue[9], rawValue[11], rawValue[12], rawValue[14], rawValue[15]);
		return;
	}

	sanitizeText(rawValue, output, length);
}
}

bool OledUi::begin() {
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
		return false;
	}

	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE);
	display.cp437(true);
	display.display();
	return true;
}

void OledUi::renderStandby(const StatusSnapshot& status) {
	char wifiLine[24];
	char apiLine[24];

	display.clearDisplay();
	drawHeader("Standby");
	display.setTextSize(2);
	drawLineText(6, 15, status.timeSynced ? status.timeText : "--:--:--");
	display.setTextSize(1);
	drawLineText(22, 36, status.timeSynced ? status.dateText : "Time not synced");
	snprintf(wifiLine, sizeof(wifiLine), "WiFi:%s", status.wifiConnected ? "ON" : "OFF");
	snprintf(apiLine, sizeof(apiLine), "API :%s", status.serverOnline ? "ON" : "OFF");
	drawLineText(0, 46, wifiLine);
	drawLineText(66, 46, apiLine);
	drawKeyHint("D Menu", status.wifiConnected ? status.ssid : "No SSID");
	display.display();
}

void OledUi::renderMenu(uint8_t selectedIndex) {
	display.clearDisplay();
	drawHeader("Main Menu");

	for (uint8_t index = 0; index < 6; ++index) {
		display.setCursor(0, 12 + (index * 7));
		display.print(index == selectedIndex ? '>' : ' ');
		display.print(MENU_ITEMS[index]);
	}

	drawKeyHint("C Back", "D Open");
	display.display();
}

void OledUi::renderWifiList(const WifiNetworkInfo* networks, uint8_t count, uint8_t selectedIndex) {
	display.clearDisplay();
	drawHeader("WiFi Scan");

	if (count == 0) {
		drawLineText(0, 18, "No AP found");
		drawLineText(0, 28, "Press D rescan");
		drawKeyHint("C Back", "D Rescan");
		display.display();
		return;
	}

	const uint8_t visibleRows = 5;
	uint8_t startIndex = 0;
	if (selectedIndex >= visibleRows) {
		startIndex = selectedIndex - visibleRows + 1;
	}

	for (uint8_t row = 0; row < visibleRows && (startIndex + row) < count; ++row) {
		const uint8_t index = startIndex + row;
		char line[24];
		char ssid[14];
		sanitizeText(networks[index].ssid, ssid, sizeof(ssid));
		snprintf(line, sizeof(line), "%c%-12s %3ld", index == selectedIndex ? '>' : ' ', ssid, static_cast<long>(networks[index].rssi));
		drawLineText(0, 12 + (row * 9), line);
	}

	drawKeyHint("C Back", "D Select");
	display.display();
}

void OledUi::renderWifiPassword(const char* ssid, const char* password) {
	display.clearDisplay();
	drawHeader("WiFi Password");
	drawLineText(0, 14, "SSID:");
	drawLineText(30, 14, ssid);
	drawLineText(0, 28, "PWD:");
	drawLineText(30, 28, password[0] == '\0' ? "<empty>" : password);
	drawLineText(0, 40, "0-9 only");
	drawKeyHint("# Del C Back", "D Join");
	display.display();
}

void OledUi::renderTextInput(const char* title, const char* prompt, const char* value) {
	display.clearDisplay();
	drawHeader(title);
	drawLineText(0, 14, prompt);
	drawLineText(0, 28, value[0] == '\0' ? "_" : value);
	drawLineText(0, 40, "Use # to delete");
	drawKeyHint("C Back", "D Submit");
	display.display();
}

void OledUi::renderAmountInput(const char* title, const char* amountValue) {
	display.clearDisplay();
	drawHeader(title);
	drawLineText(0, 14, "Amount:");
	display.setTextSize(2);
	drawLineText(0, 26, amountValue[0] == '\0' ? "0" : amountValue);
	display.setTextSize(1);
	drawLineText(0, 48, "*.=dot  #=del");
	drawKeyHint("C Back", "D Pay");
	display.display();
}

void OledUi::renderMessage(const char* title, const char* line1, const char* line2, const char* line3, const char* line4) {
	display.clearDisplay();
	drawHeader(title);
	drawLineText(0, 14, line1);
	drawLineText(0, 24, line2);
	drawLineText(0, 34, line3);
	drawLineText(0, 44, line4);
	display.display();
}

void OledUi::renderEnrollmentResult(const EnrollmentResult& result) {
	char templateLine[24];
	char balanceLine[24];

	display.clearDisplay();
	drawHeader(result.success ? "Enroll OK" : "Enroll Fail");
	drawLineText(0, 14, result.studentName[0] == '\0' ? "Name: n/a" : result.studentName);
	drawLineText(0, 24, result.studentId[0] == '\0' ? "ID: n/a" : result.studentId);
	snprintf(templateLine, sizeof(templateLine), "Tpl ID: %d", result.fingerprintTemplateId);
	snprintf(balanceLine, sizeof(balanceLine), "Balance: %s", result.balance[0] == '\0' ? "0.00" : result.balance);
	drawLineText(0, 34, templateLine);
	drawLineText(0, 44, balanceLine);
	drawKeyHint(result.message[0] == '\0' ? "C Menu" : result.message, "D Menu");
	display.display();
}

void OledUi::renderPaymentResult(const PaymentResult& result) {
	char amountLine[24];
	char balanceLine[24];

	display.clearDisplay();
	drawHeader(result.success ? "Payment OK" : "Payment Fail");
	drawLineText(0, 14, result.studentName[0] == '\0' ? "Name: n/a" : result.studentName);
	drawLineText(0, 24, result.studentId[0] == '\0' ? "ID: n/a" : result.studentId);
	snprintf(amountLine, sizeof(amountLine), "Amt: %s", result.amount[0] == '\0' ? "0.00" : result.amount);
	snprintf(balanceLine, sizeof(balanceLine), "Bal: %s", result.balance[0] == '\0' ? "0.00" : result.balance);
	drawLineText(0, 34, amountLine);
	drawLineText(0, 44, balanceLine);
	drawKeyHint(result.message[0] == '\0' ? "C Menu" : result.message, "D Menu");
	display.display();
}

void OledUi::renderTransactions(const TransactionListResult& result, uint8_t pageIndex) {
	char header[24];
	char timeLine[24];
	char amountLine[24];
	char balanceLine[24];

	display.clearDisplay();

	if (result.count == 0) {
		drawHeader("Records");
		drawLineText(0, 18, result.studentName[0] == '\0' ? result.studentId : result.studentName);
		drawLineText(0, 30, "No records");
		drawKeyHint("C Menu", "D Menu");
		display.display();
		return;
	}

	const TransactionRecordSummary& record = result.items[pageIndex];
	snprintf(header, sizeof(header), "Records %u/%u", static_cast<unsigned>(pageIndex + 1), static_cast<unsigned>(result.count));
	drawHeader(header);
	drawLineText(0, 12, result.studentName[0] == '\0' ? result.studentId : result.studentName);
	drawLineText(0, 22, record.transactionType);
	snprintf(amountLine, sizeof(amountLine), "Amt:%s %s", record.amount, record.result);
	snprintf(balanceLine, sizeof(balanceLine), "Bal:%s", record.balanceAfter);
	formatRecordTime(record.occurredAt, timeLine, sizeof(timeLine));
	drawLineText(0, 32, amountLine);
	drawLineText(0, 42, balanceLine);
	drawLineText(0, 52, timeLine[0] == '\0' ? record.message : timeLine);
	display.display();
}

void OledUi::renderSystemInfo(const StatusSnapshot& status, const char* deviceId, const char* baseUrl, bool serverOnline) {
	char apiLine[24];
	char urlLine1[22];
	char urlLine2[22];

	display.clearDisplay();
	drawHeader("System Info");
	drawLineText(0, 12, deviceId);
	drawLineText(0, 22, status.ipAddress[0] == '\0' ? "IP:0.0.0.0" : status.ipAddress);
	drawLineText(0, 32, status.wifiConnected ? "WiFi: Connected" : "WiFi: Offline");
	snprintf(apiLine, sizeof(apiLine), "API: %s", serverOnline ? "Online" : "Offline");
	drawLineText(0, 42, apiLine);
	sanitizeText(baseUrl, urlLine1, sizeof(urlLine1));
	urlLine2[0] = '\0';
	if (strlen(urlLine1) > 20) {
		strncpy(urlLine2, urlLine1 + 20, sizeof(urlLine2) - 1);
		urlLine2[sizeof(urlLine2) - 1] = '\0';
		urlLine1[20] = '\0';
	}
	drawLineText(0, 52, urlLine1);
	if (urlLine2[0] != '\0') {
		drawLineText(0, 60, urlLine2);
	}
	display.display();
}
