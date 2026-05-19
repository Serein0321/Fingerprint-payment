#include "ApiClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>

#include "DeviceConfig.h"

namespace {
void clearText(char* destination, size_t length) {
	if (length > 0) {
		destination[0] = '\0';
	}
}

void copyDisplayText(char* destination, size_t length, const char* source) {
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

void setError(char* errorMessage, size_t errorLength, const char* value) {
	copyDisplayText(errorMessage, errorLength, value);
}

const char* mapErrorCode(const char* code) {
	if (code == nullptr || code[0] == '\0') {
		return "Req failed";
	}

	if (strcmp(code, "STUDENT_NOT_FOUND") == 0) {
		return "Student N/A";
	}
	if (strcmp(code, "FINGERPRINT_TEMPLATE_IN_USE") == 0) {
		return "Tpl in use";
	}
	if (strcmp(code, "STUDENT_ALREADY_BOUND") == 0) {
		return "Already bound";
	}
	if (strcmp(code, "INSUFFICIENT_BALANCE") == 0) {
		return "No balance";
	}
	if (strcmp(code, "FINGERPRINT_NOT_FOUND") == 0) {
		return "Finger N/A";
	}
	if (strcmp(code, "VALIDATION_ERROR") == 0) {
		return "Input error";
	}
	if (strcmp(code, "NOT_FOUND") == 0) {
		return "Not found";
	}
	if (strcmp(code, "INTERNAL_SERVER_ERROR") == 0) {
		return "Server error";
	}

	return "Req failed";
}

bool decodeJson(const String& payload, DynamicJsonDocument& document, char* errorMessage, size_t errorLength) {
	const DeserializationError error = deserializeJson(document, payload);
	if (error) {
		setError(errorMessage, errorLength, "Bad JSON");
		return false;
	}

	return true;
}

void addTimestamp(JsonDocument& document, const char* key) {
	const time_t now = time(nullptr);
	if (now < 100000) {
		return;
	}

	struct tm timeInfo;
	if (!localtime_r(&now, &timeInfo)) {
		return;
	}

	char isoBuffer[32];
	strftime(isoBuffer, sizeof(isoBuffer), "%Y-%m-%dT%H:%M:%S%z", &timeInfo);
	document[key] = isoBuffer;
}

void fillLookupResult(JsonVariantConst data, StudentLookupResult& result) {
	result.valid = true;
	copyDisplayText(result.studentId, sizeof(result.studentId), data["studentId"] | "");
	copyDisplayText(result.studentName, sizeof(result.studentName), data["studentName"] | "");
	copyDisplayText(result.balance, sizeof(result.balance), data["balance"] | "0.00");
}

void fillEnrollmentResult(JsonVariantConst data, EnrollmentResult& result) {
	result.success = true;
	copyDisplayText(result.studentId, sizeof(result.studentId), data["studentId"] | "");
	copyDisplayText(result.studentName, sizeof(result.studentName), data["studentName"] | "");
	result.fingerprintTemplateId = data["fingerprintTemplateId"] | 0;
	copyDisplayText(result.balance, sizeof(result.balance), data["balance"] | "0.00");
	copyDisplayText(result.message, sizeof(result.message), "Enroll saved");
}

void fillPaymentResult(JsonVariantConst data, PaymentResult& result) {
	const char* transactionType = data["transactionType"] | "";
	const char* message = data["message"] | "Done";

	result.success = true;
	copyDisplayText(result.transactionType, sizeof(result.transactionType), transactionType);
	copyDisplayText(result.studentId, sizeof(result.studentId), data["studentId"] | "");
	copyDisplayText(result.studentName, sizeof(result.studentName), data["studentName"] | "");
	result.fingerprintTemplateId = data["fingerprintTemplateId"] | 0;
	copyDisplayText(result.amount, sizeof(result.amount), data["amount"] | "0.00");
	copyDisplayText(result.balance, sizeof(result.balance), data["balance"] | "0.00");
	copyDisplayText(result.message, sizeof(result.message), message);
}

void fillTransactionRecord(JsonVariantConst item, TransactionRecordSummary& record) {
	copyDisplayText(record.transactionType, sizeof(record.transactionType), item["transactionType"] | "");
	copyDisplayText(record.amount, sizeof(record.amount), item["amount"] | "0.00");
	copyDisplayText(record.balanceAfter, sizeof(record.balanceAfter), item["balanceAfter"] | "0.00");
	copyDisplayText(record.result, sizeof(record.result), item["result"] | "");
	copyDisplayText(record.occurredAt, sizeof(record.occurredAt), item["occurredAt"] | "");
	copyDisplayText(record.message, sizeof(record.message), item["message"] | "");
}
}

ApiClient::ApiClient(const char* baseUrl)
	: _baseUrl(baseUrl) {}

bool ApiClient::healthCheck(char* errorMessage, size_t errorLength) {
	HTTPClient http;
	DynamicJsonDocument document(768);

	clearText(errorMessage, errorLength);
	http.begin(String(_baseUrl) + "/api/health");
	http.setTimeout(HTTP_TIMEOUT_MS);
	const int statusCode = http.GET();

	if (statusCode <= 0) {
		setError(errorMessage, errorLength, "API offline");
		http.end();
		return false;
	}

	const String responseBody = http.getString();
	http.end();

	if (!decodeJson(responseBody, document, errorMessage, errorLength)) {
		return false;
	}

	const char* serviceStatus = document["data"]["serviceStatus"] | "";
	if (statusCode == 200 && strcmp(serviceStatus, "ok") == 0) {
		return true;
	}

	setError(errorMessage, errorLength, strcmp(serviceStatus, "degraded") == 0 ? "DB degraded" : "API offline");
	return false;
}

bool ApiClient::lookupStudent(const char* studentId, StudentLookupResult& result, char* errorMessage, size_t errorLength) {
	HTTPClient http;
	DynamicJsonDocument document(1024);

	memset(&result, 0, sizeof(result));
	clearText(errorMessage, errorLength);
	http.begin(String(_baseUrl) + "/api/students/lookup/" + studentId);
	http.setTimeout(HTTP_TIMEOUT_MS);
	const int statusCode = http.GET();

	if (statusCode <= 0) {
		setError(errorMessage, errorLength, "Lookup fail");
		http.end();
		return false;
	}

	const String responseBody = http.getString();
	http.end();

	if (!decodeJson(responseBody, document, errorMessage, errorLength)) {
		return false;
	}

	if (statusCode == 200 && (document["success"] | false)) {
		fillLookupResult(document["data"], result);
		return true;
	}

	setError(errorMessage, errorLength, mapErrorCode(document["error"]["code"] | ""));
	return false;
}

bool ApiClient::enrollFingerprint(const char* studentId, int templateId, EnrollmentResult& result, char* errorMessage, size_t errorLength) {
	HTTPClient http;
	DynamicJsonDocument requestDocument(256);
	DynamicJsonDocument responseDocument(1024);
	String payload;

	memset(&result, 0, sizeof(result));
	clearText(errorMessage, errorLength);

	requestDocument["studentId"] = studentId;
	requestDocument["fingerprintTemplateId"] = templateId;
	requestDocument["deviceId"] = DEVICE_ID;
	addTimestamp(requestDocument, "enrolledAt");
	serializeJson(requestDocument, payload);

	http.begin(String(_baseUrl) + "/api/fingerprints/enroll");
	http.addHeader("Content-Type", "application/json");
	http.setTimeout(HTTP_TIMEOUT_MS);
	const int statusCode = http.POST(payload);

	if (statusCode <= 0) {
		setError(errorMessage, errorLength, "Enroll fail");
		http.end();
		return false;
	}

	const String responseBody = http.getString();
	http.end();

	if (!decodeJson(responseBody, responseDocument, errorMessage, errorLength)) {
		return false;
	}

	if ((statusCode == 200 || statusCode == 201) && (responseDocument["success"] | false)) {
		fillEnrollmentResult(responseDocument["data"], result);
		return true;
	}

	result.success = false;
	copyDisplayText(result.studentId, sizeof(result.studentId), studentId);
	result.fingerprintTemplateId = templateId;
	copyDisplayText(result.message, sizeof(result.message), mapErrorCode(responseDocument["error"]["code"] | ""));
	setError(errorMessage, errorLength, result.message);
	return false;
}

bool ApiClient::submitPayment(ActionType action, int templateId, const char* amount, PaymentResult& result, char* errorMessage, size_t errorLength) {
	HTTPClient http;
	DynamicJsonDocument requestDocument(256);
	DynamicJsonDocument responseDocument(1536);
	String payload;

	memset(&result, 0, sizeof(result));
	clearText(errorMessage, errorLength);

	requestDocument["fingerprintTemplateId"] = templateId;
	requestDocument["amount"] = amount;
	requestDocument["deviceId"] = DEVICE_ID;
	addTimestamp(requestDocument, "occurredAt");
	serializeJson(requestDocument, payload);

	http.begin(String(_baseUrl) + (action == ActionType::Topup ? "/api/payments/topup" : "/api/payments/consume"));
	http.addHeader("Content-Type", "application/json");
	http.setTimeout(HTTP_TIMEOUT_MS);
	const int statusCode = http.POST(payload);

	if (statusCode <= 0) {
		copyDisplayText(result.transactionType, sizeof(result.transactionType), action == ActionType::Topup ? "topup" : "consume");
		copyDisplayText(result.amount, sizeof(result.amount), amount);
		result.fingerprintTemplateId = templateId;
		copyDisplayText(result.message, sizeof(result.message), "Pay offline");
		setError(errorMessage, errorLength, result.message);
		http.end();
		return false;
	}

	const String responseBody = http.getString();
	http.end();

	if (!decodeJson(responseBody, responseDocument, errorMessage, errorLength)) {
		return false;
	}

	if (statusCode == 200 && (responseDocument["success"] | false)) {
		fillPaymentResult(responseDocument["data"], result);
		return true;
	}

	result.success = false;
	copyDisplayText(result.transactionType, sizeof(result.transactionType), action == ActionType::Topup ? "topup" : "consume");
	copyDisplayText(result.amount, sizeof(result.amount), amount);
	result.fingerprintTemplateId = templateId;
	copyDisplayText(result.studentId, sizeof(result.studentId), responseDocument["error"]["details"]["studentId"] | "");
	copyDisplayText(result.balance, sizeof(result.balance), responseDocument["error"]["details"]["currentBalance"] | "");
	copyDisplayText(result.message, sizeof(result.message), mapErrorCode(responseDocument["error"]["code"] | ""));

	if (result.studentId[0] != '\0') {
		StudentLookupResult lookup;
		char lookupError[16];
		if (lookupStudent(result.studentId, lookup, lookupError, sizeof(lookupError))) {
			copyDisplayText(result.studentName, sizeof(result.studentName), lookup.studentName);
		}
	}

	setError(errorMessage, errorLength, result.message);
	return false;
}

bool ApiClient::fetchRecentTransactions(const char* studentId, uint8_t limit, TransactionListResult& result, char* errorMessage, size_t errorLength) {
	HTTPClient http;
	DynamicJsonDocument document(2048);

	memset(&result, 0, sizeof(result));
	clearText(errorMessage, errorLength);
	http.begin(String(_baseUrl) + "/api/transactions?studentId=" + studentId + "&limit=" + String(limit));
	http.setTimeout(HTTP_TIMEOUT_MS);
	const int statusCode = http.GET();

	if (statusCode <= 0) {
		setError(errorMessage, errorLength, "Query fail");
		http.end();
		return false;
	}

	const String responseBody = http.getString();
	http.end();

	if (!decodeJson(responseBody, document, errorMessage, errorLength)) {
		return false;
	}

	if (statusCode == 200 && (document["success"] | false)) {
		const JsonArray transactions = document["data"]["transactions"].as<JsonArray>();
		result.success = true;
		copyDisplayText(result.studentId, sizeof(result.studentId), document["data"]["studentId"] | "");
		copyDisplayText(result.studentName, sizeof(result.studentName), document["data"]["studentName"] | "");
		result.count = 0;

		for (JsonVariantConst item : transactions) {
			if (result.count >= MAX_RECORDS) {
				break;
			}

			fillTransactionRecord(item, result.items[result.count]);
			++result.count;
		}

		return true;
	}

	setError(errorMessage, errorLength, mapErrorCode(document["error"]["code"] | ""));
	return false;
}

const char* ApiClient::getBaseUrl() const {
	return _baseUrl;
}
