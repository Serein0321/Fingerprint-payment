#include <Arduino.h>
#include <Wire.h>

#include "ApiClient.h"
#include "DeviceConfig.h"
#include "FingerprintService.h"
#include "KeypadInput.h"
#include "NetworkService.h"
#include "OledUi.h"
#include "PaymentFlow.h"

KeypadInput keypadInput;
OledUi oledUi;
NetworkService networkService;
ApiClient apiClient(API_BASE_URL);
FingerprintService fingerprintService;
PaymentFlow paymentFlow(keypadInput, oledUi, networkService, apiClient, fingerprintService);

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  paymentFlow.begin();
}

void loop() {
  paymentFlow.tick();
  delay(20);
}
