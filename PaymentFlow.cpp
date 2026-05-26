#include "PaymentFlow.h"

#include <cstring>

namespace {
constexpr uint8_t MENU_COUNT = 7;

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

const char* getScreenTitle(ActionType action) {
	switch (action) {
		case ActionType::Enroll:
			return "Enroll FP";
		case ActionType::Records:
			return "Records";
		case ActionType::Topup:
			return "Top Up";
		case ActionType::Consume:
			return "Consume";
		default:
			return "Input";
	}
}

bool hasDecimalPoint(const char* value) {
	return strchr(value, '.') != nullptr;
}

size_t decimalDigits(const char* value) {
	const char* point = strchr(value, '.');
	if (point == nullptr) {
		return 0;
	}

	return strlen(point + 1);
}
}

PaymentFlow::PaymentFlow(KeypadInput& keypadInput, OledUi& oledUi, NetworkService& networkService, ApiClient& apiClient, FingerprintService& fingerprintService)
	: _keypadInput(keypadInput),
		_oledUi(oledUi),
		_networkService(networkService),
		_apiClient(apiClient),
		_fingerprintService(fingerprintService),
		_screen(AppScreen::Standby),
		_pendingAction(ActionType::None),
		_resultType(ResultType::None),
		_serverOnline(false),
		_lastStandbyRefreshMs(0),
		_lastHealthCheckMs(0),
		_menuIndex(0),
		_wifiCount(0),
		_wifiIndex(0),
		_recordPage(0) {
	clearBuffer(_entryBuffer, sizeof(_entryBuffer));
	clearBuffer(_passwordBuffer, sizeof(_passwordBuffer));
	memset(_wifiNetworks, 0, sizeof(_wifiNetworks));
	memset(&_studentLookup, 0, sizeof(_studentLookup));
	memset(&_enrollmentResult, 0, sizeof(_enrollmentResult));
	memset(&_paymentResult, 0, sizeof(_paymentResult));
	memset(&_transactionList, 0, sizeof(_transactionList));
}

void PaymentFlow::begin() {
	_keypadInput.begin();//初始化键盘输入系统，准备读取用户的按键操作。
	const bool oledReady = _oledUi.begin();//初始化OLED显示屏，准备显示用户界面，并返回是否成功。
	_networkService.begin();

	char fingerprintError[24];//初始化指纹传感器，并在失败时获取错误信息。
	if (!_fingerprintService.begin(fingerprintError, sizeof(fingerprintError))) {
		Serial.print("Fingerprint init failed: ");
		Serial.println(fingerprintError);
		if (oledReady) {
			_oledUi.renderMessage("Startup", "FP sensor err", fingerprintError);
			delay(1200);
		}
	}

	updateServerStatus(true);
	renderCurrentScreen();
}

void PaymentFlow::tick() {
	_networkService.tick();
	updateServerStatus(false);

	if (_screen == AppScreen::Standby && millis() - _lastStandbyRefreshMs >= STANDBY_REFRESH_INTERVAL_MS) {
		renderCurrentScreen();
	}

	const KeyAction action = _keypadInput.readAction();
	if (action == KeyAction::None) {
		return;
	}

	handleAction(action);
}

void PaymentFlow::handleAction(KeyAction action) {
	switch (_screen) {
		case AppScreen::Standby:
			handleStandby(action);
			break;
		case AppScreen::MainMenu:
			handleMainMenu(action);
			break;
		case AppScreen::WifiList:
			handleWifiList(action);
			break;
		case AppScreen::WifiPassword:
			handleWifiPassword(action);
			break;
		case AppScreen::StudentIdInput:
			handleStudentInput(action);
			break;
		case AppScreen::AmountInput:
			handleAmountInput(action);
			break;
		case AppScreen::Records:
			handleRecords(action);
			break;
		case AppScreen::SystemInfo:
			handleSystemInfo(action);
			break;
		case AppScreen::Result:
			handleResult(action);
			break;
	}
}

void PaymentFlow::handleStandby(KeyAction action) {
	if (action == KeyAction::Confirm) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
	}
}

void PaymentFlow::handleMainMenu(KeyAction action) {
	if (action == KeyAction::Up) {
		_menuIndex = (_menuIndex == 0) ? (MENU_COUNT - 1) : (_menuIndex - 1);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Down) {
		_menuIndex = (_menuIndex + 1) % MENU_COUNT;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Back) {
		_screen = AppScreen::Standby;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm) {
		openMenuSelection();
	}
}

void PaymentFlow::handleWifiList(KeyAction action) {
	if (action == KeyAction::Back) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm) {
		if (_wifiCount == 0) {
			scanWifiNetworks();
			return;
		}

		clearBuffer(_passwordBuffer, sizeof(_passwordBuffer));
		_screen = AppScreen::WifiPassword;
		renderCurrentScreen();
		return;
	}

	if (_wifiCount == 0) {
		return;
	}

	if (action == KeyAction::Up) {
		_wifiIndex = (_wifiIndex == 0) ? (_wifiCount - 1) : (_wifiIndex - 1);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Down) {
		_wifiIndex = (_wifiIndex + 1) % _wifiCount;
		renderCurrentScreen();
	}
}

void PaymentFlow::handleWifiPassword(KeyAction action) {
	if (KeypadInput::isDigit(action)) {
		appendDigit(_passwordBuffer, sizeof(_passwordBuffer), KeypadInput::toDigit(action));
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Delete) {
		removeLastCharacter(_passwordBuffer);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Back) {
		_screen = AppScreen::WifiList;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm) {
		connectSelectedWifi();
	}
}

void PaymentFlow::handleStudentInput(KeyAction action) {//处理学生ID输入界面的按键操作，包括数字输入、删除、返回和确认操作，以便用户能够正确输入学生ID并进行后续的注册或记录查询操作。
	if (KeypadInput::isDigit(action)) {
		appendDigit(_entryBuffer, sizeof(_entryBuffer), KeypadInput::toDigit(action));
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Delete) {
		removeLastCharacter(_entryBuffer);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Back) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm && _entryBuffer[0] != '\0') {
		if (_pendingAction == ActionType::Enroll) {
			processEnrollment();
		} else if (_pendingAction == ActionType::Records) {
			processRecordLookup();
		}
	}
}

void PaymentFlow::handleAmountInput(KeyAction action) {
	if (KeypadInput::isDigit(action) || action == KeyAction::Decimal) {
		appendAmountCharacter(action);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Delete) {
		removeLastCharacter(_entryBuffer);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Back) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm && _entryBuffer[0] != '\0' && strcmp(_entryBuffer, ".") != 0) {
		processPayment(_pendingAction);
	}
}

void PaymentFlow::handleRecords(KeyAction action) {
	if (action == KeyAction::Back || action == KeyAction::Confirm) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
		return;
	}

	if (_transactionList.count <= 1) {
		return;
	}

	if (action == KeyAction::Up) {
		_recordPage = (_recordPage == 0) ? (_transactionList.count - 1) : (_recordPage - 1);
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Down) {
		_recordPage = (_recordPage + 1) % _transactionList.count;
		renderCurrentScreen();
	}
}

void PaymentFlow::handleSystemInfo(KeyAction action) {
	if (action == KeyAction::Back) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
		return;
	}

	if (action == KeyAction::Confirm) {
		updateServerStatus(true);
		renderCurrentScreen();
	}
}

void PaymentFlow::handleResult(KeyAction action) {
	if (action == KeyAction::Back || action == KeyAction::Confirm) {
		_screen = AppScreen::MainMenu;
		renderCurrentScreen();
	}
}

void PaymentFlow::openMenuSelection() {
	switch (_menuIndex) {
		case 0:
			scanWifiNetworks();
			break;
		case 1:
			_pendingAction = ActionType::Enroll;
			clearBuffer(_entryBuffer, sizeof(_entryBuffer));
			_screen = AppScreen::StudentIdInput;
			renderCurrentScreen();
			break;
		case 2:
			_pendingAction = ActionType::Consume;
			clearBuffer(_entryBuffer, sizeof(_entryBuffer));
			_screen = AppScreen::AmountInput;
			renderCurrentScreen();
			break;
		case 3:
			_pendingAction = ActionType::Topup;
			clearBuffer(_entryBuffer, sizeof(_entryBuffer));
			_screen = AppScreen::AmountInput;
			renderCurrentScreen();
			break;
		case 4:
			_pendingAction = ActionType::Records;
			clearBuffer(_entryBuffer, sizeof(_entryBuffer));
			_screen = AppScreen::StudentIdInput;
			renderCurrentScreen();
			break;
		case 5:
			clearFingerprintDatabase();
			break;
		case 6:
			_screen = AppScreen::SystemInfo;
			renderCurrentScreen();
			break;
		default:
			break;
	}
}

void PaymentFlow::scanWifiNetworks() {
	_oledUi.renderMessage("WiFi", "Scanning...");
	_wifiCount = _networkService.scanNetworks(_wifiNetworks, MAX_WIFI_NETWORKS);
	_wifiIndex = 0;
	_screen = AppScreen::WifiList;
	renderCurrentScreen();
}

void PaymentFlow::connectSelectedWifi() {
	char errorMessage[24];

	_oledUi.renderMessage("WiFi", "Connecting...", _wifiNetworks[_wifiIndex].ssid);
	if (!_networkService.connectToWifi(_wifiNetworks[_wifiIndex].ssid, _passwordBuffer, errorMessage, sizeof(errorMessage))) {
		showTemporaryMessage("WiFi", "Connect fail", errorMessage);
		return;
	}

	_oledUi.renderMessage("WiFi", "Syncing clock...");
	const bool timeSynced = _networkService.syncClock(errorMessage, sizeof(errorMessage));

	char apiError[24];
	_serverOnline = _apiClient.healthCheck(apiError, sizeof(apiError));
	_screen = AppScreen::Standby;
	showTemporaryMessage(
		"WiFi",
		"Connected",
		timeSynced ? "Time synced" : errorMessage,
		_serverOnline ? "API online" : apiError,
		1500UL
	);
}

void PaymentFlow::clearFingerprintDatabase() {
	char errorMessage[24];

	_oledUi.renderMessage("Delete FP DB", "Deleting...");
	if (!_fingerprintService.clearDatabase(errorMessage, sizeof(errorMessage))) {
		showTemporaryMessage("Delete FP DB", "Delete failed", errorMessage);
		return;
	}

	showTemporaryMessage("Delete FP DB", "All prints deleted", "", "", 1500UL);
}

void PaymentFlow::processEnrollment() {//处理学生注册流程，包括验证WiFi连接、查询学生信息、指纹录入和注册，以及根据结果显示相应的界面和信息。
	char errorMessage[24];
	int templateId = 0;

	if (!ensureWifiConnection()) {//首先检查WiFi连接，如果未连接则提示用户并返回。
		return;
	}

	_oledUi.renderMessage("Enroll FP", "Lookup student", _entryBuffer);//显示正在查询学生信息的界面。
	if (!_apiClient.lookupStudent(_entryBuffer, _studentLookup, errorMessage, sizeof(errorMessage))) {//调用API客户端查询学生信息，如果失败则显示错误信息并返回。
		showTemporaryMessage("Enroll FP", errorMessage);
		return;
	}

	_oledUi.renderMessage("Enroll FP", _studentLookup.studentId, _studentLookup.studentName[0] == '\0' ? "Name unread" : _studentLookup.studentName, "Place/lift/place");//显示指纹录入界面，提示用户放置手指进行录入。
	delay(900);

	if (!_fingerprintService.enrollFingerprint(templateId, errorMessage, sizeof(errorMessage))) {//
		showTemporaryMessage("Enroll FP", errorMessage);
		return;
	}

	if (!_apiClient.enrollFingerprint(_studentLookup.studentId, templateId, _enrollmentResult, errorMessage, sizeof(errorMessage))) {
		memset(&_enrollmentResult, 0, sizeof(_enrollmentResult));
		_enrollmentResult.success = false;
		copyText(_enrollmentResult.studentId, sizeof(_enrollmentResult.studentId), _studentLookup.studentId);
		copyText(_enrollmentResult.studentName, sizeof(_enrollmentResult.studentName), _studentLookup.studentName);
		_enrollmentResult.fingerprintTemplateId = templateId;
		copyText(_enrollmentResult.balance, sizeof(_enrollmentResult.balance), _studentLookup.balance);
		copyText(_enrollmentResult.message, sizeof(_enrollmentResult.message), errorMessage[0] == '\0' ? "Enroll fail" : errorMessage);
		_fingerprintService.deleteTemplate(templateId);
	}

	_resultType = ResultType::Enrollment;
	_screen = AppScreen::Result;
	renderCurrentScreen();
}

void PaymentFlow::processPayment(ActionType action) {
	char errorMessage[24];
	int templateId = 0;

	if (!ensureWifiConnection()) {
		return;
	}

	_oledUi.renderMessage(getScreenTitle(action), "Place finger", "Scanning...");
	if (!_fingerprintService.searchFingerprint(templateId, errorMessage, sizeof(errorMessage))) {
		showTemporaryMessage(getScreenTitle(action), errorMessage);
		return;
	}

	if (!_apiClient.submitPayment(action, templateId, _entryBuffer, _paymentResult, errorMessage, sizeof(errorMessage))) {
		if (_paymentResult.amount[0] == '\0') {
			copyText(_paymentResult.amount, sizeof(_paymentResult.amount), _entryBuffer);
		}
		if (_paymentResult.message[0] == '\0') {
			copyText(_paymentResult.message, sizeof(_paymentResult.message), errorMessage[0] == '\0' ? "Payment fail" : errorMessage);
		}
	}

	_resultType = ResultType::Payment;
	_screen = AppScreen::Result;
	renderCurrentScreen();
}

void PaymentFlow::processRecordLookup() {
	char errorMessage[24];

	if (!ensureWifiConnection()) {
		return;
	}

	_oledUi.renderMessage("Records", "Querying...", _entryBuffer);
	if (!_apiClient.fetchRecentTransactions(_entryBuffer, MAX_RECORDS, _transactionList, errorMessage, sizeof(errorMessage))) {
		showTemporaryMessage("Records", errorMessage);
		return;
	}

	_recordPage = 0;
	_screen = AppScreen::Records;
	renderCurrentScreen();
}

void PaymentFlow::updateServerStatus(bool force) {
	if (!_networkService.isWifiConnected()) {
		_serverOnline = false;
		_lastHealthCheckMs = millis();
		return;
	}

	if (!force && millis() - _lastHealthCheckMs < HEALTH_CHECK_INTERVAL_MS) {
		return;
	}

	char errorMessage[16];
	_serverOnline = _apiClient.healthCheck(errorMessage, sizeof(errorMessage));
	_lastHealthCheckMs = millis();
}

bool PaymentFlow::ensureWifiConnection() {
	if (_networkService.isWifiConnected()) {
		return true;
	}

	showTemporaryMessage("Network", "WiFi offline", "Use WiFi menu");
	return false;
}

void PaymentFlow::showTemporaryMessage(const char* title, const char* line1, const char* line2, const char* line3, unsigned long pauseMs) {
	_oledUi.renderMessage(title, line1, line2, line3);
	delay(pauseMs);
	renderCurrentScreen();
}

void PaymentFlow::clearBuffer(char* buffer, size_t length) {
	if (length > 0) {
		memset(buffer, 0, length);
	}
}

void PaymentFlow::removeLastCharacter(char* buffer) {
	const size_t currentLength = strlen(buffer);
	if (currentLength > 0) {
		buffer[currentLength - 1] = '\0';
	}
}

void PaymentFlow::appendDigit(char* buffer, size_t length, char digit) {
	const size_t currentLength = strlen(buffer);
	if (currentLength + 1 >= length) {
		return;
	}

	buffer[currentLength] = digit;
	buffer[currentLength + 1] = '\0';
}

void PaymentFlow::appendAmountCharacter(KeyAction action) {
	if (KeypadInput::isDigit(action)) {
		if (hasDecimalPoint(_entryBuffer) && decimalDigits(_entryBuffer) >= 2) {
			return;
		}

		appendDigit(_entryBuffer, MAX_AMOUNT_LENGTH + 1, KeypadInput::toDigit(action));
		return;
	}

	if (action == KeyAction::Decimal) {
		if (hasDecimalPoint(_entryBuffer)) {
			return;
		}

		if (_entryBuffer[0] == '\0') {
			appendDigit(_entryBuffer, MAX_AMOUNT_LENGTH + 1, '0');
		}

		appendDigit(_entryBuffer, MAX_AMOUNT_LENGTH + 1, '.');
	}
}

void PaymentFlow::renderCurrentScreen() {
	StatusSnapshot status = {};

	switch (_screen) {
		case AppScreen::Standby:
			_networkService.fillStatusSnapshot(status, _serverOnline);
			_oledUi.renderStandby(status);
			_lastStandbyRefreshMs = millis();
			break;
		case AppScreen::MainMenu:
			_oledUi.renderMenu(_menuIndex);
			break;
		case AppScreen::WifiList:
			_oledUi.renderWifiList(_wifiNetworks, _wifiCount, _wifiIndex);
			break;
		case AppScreen::WifiPassword:
			_oledUi.renderWifiPassword(_wifiNetworks[_wifiIndex].ssid, _passwordBuffer);
			break;
		case AppScreen::StudentIdInput:
			_oledUi.renderTextInput(getScreenTitle(_pendingAction), "Student ID", _entryBuffer);
			break;
		case AppScreen::AmountInput:
			_oledUi.renderAmountInput(getScreenTitle(_pendingAction), _entryBuffer);
			break;
		case AppScreen::Records:
			_oledUi.renderTransactions(_transactionList, _recordPage);
			break;
		case AppScreen::SystemInfo:
			_networkService.fillStatusSnapshot(status, _serverOnline);
			_oledUi.renderSystemInfo(status, DEVICE_ID, _apiClient.getBaseUrl(), _serverOnline);
			break;
		case AppScreen::Result:
			if (_resultType == ResultType::Enrollment) {
				_oledUi.renderEnrollmentResult(_enrollmentResult);
			} else {
				_oledUi.renderPaymentResult(_paymentResult);
			}
			break;
	}
}
