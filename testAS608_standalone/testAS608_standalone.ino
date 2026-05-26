#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

namespace {
constexpr uint32_t DEBUG_BAUD_RATE = 115200;
constexpr uint32_t SENSOR_BAUD_RATE_DEFAULT = 57600;
constexpr uint8_t SENSOR_RX_PIN = 16;
constexpr uint8_t SENSOR_TX_PIN = 17;
constexpr uint8_t IMAGE_RETRY_LIMIT = 80;

HardwareSerial fingerprintSerial(2);
Adafruit_Fingerprint finger(&fingerprintSerial);
uint32_t activeSensorBaud = SENSOR_BAUD_RATE_DEFAULT;

unsigned long lastHintAt = 0;

const char* resultText(uint8_t code) {
  switch (code) {
    case FINGERPRINT_OK:
      return "OK";
    case FINGERPRINT_NOFINGER:
      return "No finger";
    case FINGERPRINT_PACKETRECIEVEERR:
      return "Communication error";
    case FINGERPRINT_IMAGEFAIL:
      return "Imaging error";
    case FINGERPRINT_IMAGEMESS:
      return "Image too messy";
    case FINGERPRINT_FEATUREFAIL:
      return "Could not find fingerprint features";
    case FINGERPRINT_INVALIDIMAGE:
      return "Invalid image";
    case FINGERPRINT_ENROLLMISMATCH:
      return "Fingerprints did not match";
    case FINGERPRINT_BADLOCATION:
      return "Bad location";
    case FINGERPRINT_FLASHERR:
      return "Flash error";
    case FINGERPRINT_NOTFOUND:
      return "Not found";
    default:
      return "Unknown";
  }
}

void printMenu() {
  Serial.println();
  Serial.println("========= AS608 测试菜单 =========");
  Serial.println("i: 验证模块");
  Serial.println("p: 读取模块参数");
  Serial.println("c: 查询模板数量");
  Serial.println("a: 录入指纹");
  Serial.println("s: 搜索指纹");
  Serial.println("d: 删除指纹");
  Serial.println("e: 清空指纹库");
  Serial.println("h: 显示菜单");
  Serial.println("=================================");
}

bool connectSensor() {
  const uint32_t candidateBauds[] = {57600, 115200, 19200, 9600};

  for (size_t i = 0; i < sizeof(candidateBauds) / sizeof(candidateBauds[0]); ++i) {
    const uint32_t baud = candidateBauds[i];

    fingerprintSerial.begin(baud, SERIAL_8N1, SENSOR_RX_PIN, SENSOR_TX_PIN);
    finger.begin(baud);
    delay(300);

    if (finger.verifyPassword()) {
      activeSensorBaud = baud;
      return true;
    }
  }

  return false;
}

uint8_t readNumber(uint8_t defaultValue = 1) {
  while (true) {
    while (!Serial.available()) {
      delay(10);
    }
    const int value = Serial.parseInt();
    if (value > 0) {
      return static_cast<uint8_t>(value);
    }
    if (defaultValue > 0) {
      return defaultValue;
    }
  }
}

void printFingerParameters() {
  Serial.println("[参数] 读取模块参数...");
  const uint8_t result = finger.getParameters();
  Serial.print("[参数] 结果 = ");
  Serial.print(resultText(result));
  Serial.print(" (0x");
  Serial.print(result, HEX);
  Serial.println(")");
  if (result != FINGERPRINT_OK) {
    return;
  }

  Serial.print("[参数] status_reg = 0x");
  Serial.println(finger.status_reg, HEX);
  Serial.print("[参数] system_id = 0x");
  Serial.println(finger.system_id, HEX);
  Serial.print("[参数] capacity = ");
  Serial.println(finger.capacity);
  Serial.print("[参数] security_level = ");
  Serial.println(finger.security_level);
  Serial.print("[参数] device_addr = 0x");
  Serial.println(finger.device_addr, HEX);
  Serial.print("[参数] packet_len = ");
  Serial.println(finger.packet_len);
  Serial.print("[参数] baud_rate = ");
  Serial.println(finger.baud_rate);
}

void verifySensor() {
  Serial.println();
  Serial.println("[INIT] 正在连接 AS608...");

  if (!connectSensor()) {
    Serial.println("[INIT] 失败: 未找到指纹模块");
    Serial.println("[INIT] 请检查供电、GND、模块 TX->D16、模块 RX->D17");
    return;
  }

  Serial.println("[INIT] 成功: 指纹模块在线");
  Serial.print("[INIT] 当前波特率 = ");
  Serial.println(activeSensorBaud);
  printFingerParameters();
}

void waitForFinger() {
  Serial.println("[采图] 请把手指按在传感器上...");
  while (true) {
    const uint8_t result = finger.getImage();
    if (result == FINGERPRINT_OK) {
      Serial.println("[采图] 采图成功");
      return;
    }
    if (result == FINGERPRINT_NOFINGER) {
      delay(200);
      continue;
    }
    Serial.print("[采图] 失败: ");
    Serial.print(resultText(result));
    Serial.print(" (0x");
    Serial.print(result, HEX);
    Serial.println(")");
    return;
  }
}

void waitForFingerRelease() {
  Serial.println("[提示] 请抬起手指");
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    delay(200);
  }
}

void readTemplateCount() {
  const uint8_t result = finger.getTemplateCount();
  Serial.print("[数量] 结果 = ");
  Serial.print(resultText(result));
  Serial.print(" (0x");
  Serial.print(result, HEX);
  Serial.println(")");
  if (result == FINGERPRINT_OK) {
    Serial.print("[数量] 已存模板数 = ");
    Serial.println(finger.templateCount);
  }
}

uint8_t enrollFingerprint(uint16_t id) {
  int p = -1;
  uint8_t retries = 0;
  Serial.print("[录入] 正在录入 ID #");
  Serial.println(id);

  Serial.println("[录入] 第一次放置手指");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      continue;
    }
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("[录入] 通信错误，继续等待");
      delay(200);
      retries++;
      if (retries >= IMAGE_RETRY_LIMIT) {
        Serial.println("[录入] 连续通信错误过多，请检查接线或波特率");
        return p;
      }
      continue;
    }
    if (p != FINGERPRINT_OK) {
      Serial.print("[录入] 采图失败: ");
      Serial.println(resultText(p));
      return p;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.print("[录入] 特征转换失败: ");
    Serial.println(resultText(p));
    return p;
  }

  Serial.println("[录入] 移开手指");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.println("[录入] 再次放置同一手指");
  p = -1;
  retries = 0;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      continue;
    }
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("[录入] 通信错误，继续等待");
      delay(200);
      retries++;
      if (retries >= IMAGE_RETRY_LIMIT) {
        Serial.println("[录入] 连续通信错误过多，请检查接线或波特率");
        return p;
      }
      continue;
    }
    if (p != FINGERPRINT_OK) {
      Serial.print("[录入] 第二次采图失败: ");
      Serial.println(resultText(p));
      return p;
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.print("[录入] 第二次特征转换失败: ");
    Serial.println(resultText(p));
    return p;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.print("[录入] 合并失败: ");
    Serial.println(resultText(p));
    return p;
  }

  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.print("[录入] 存储失败: ");
    Serial.println(resultText(p));
    return p;
  }

  Serial.println("[录入] 成功");
  return FINGERPRINT_OK;
}

void searchFingerprint() {
  Serial.println("[搜索] 请把手指按在传感器上...");
  uint8_t p = FINGERPRINT_NOFINGER;
  uint8_t retries = 0;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      continue;
    }
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("[搜索] 通信错误，继续等待");
      delay(200);
      retries++;
      if (retries >= IMAGE_RETRY_LIMIT) {
        Serial.println("[搜索] 连续通信错误过多，请检查接线或波特率");
        return;
      }
      continue;
    }
    if (p != FINGERPRINT_OK) {
      Serial.print("[搜索] 采图失败: ");
      Serial.println(resultText(p));
      return;
    }
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.print("[搜索] 特征转换失败: ");
    Serial.println(resultText(p));
    return;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("[搜索] 找到 ID #");
    Serial.print(finger.fingerID);
    Serial.print(" 置信度 = ");
    Serial.println(finger.confidence);
  } else {
    Serial.print("[搜索] 未找到匹配: ");
    Serial.println(resultText(p));
  }

  waitForFingerRelease();
}

void deleteFingerprint() {
  Serial.print("[删除] 请输入要删除的 ID(1~127): ");
  const uint16_t id = readNumber();
  const uint8_t result = finger.deleteModel(id);
  Serial.print("[删除] 结果 = ");
  Serial.print(resultText(result));
  Serial.print(" (0x");
  Serial.print(result, HEX);
  Serial.println(")");
}

void emptyDatabase() {
  Serial.println("[清空] 正在清空指纹库...");
  const uint8_t result = finger.emptyDatabase();
  Serial.print("[清空] 结果 = ");
  Serial.print(resultText(result));
  Serial.print(" (0x");
  Serial.print(result, HEX);
  Serial.println(")");
}

void handleCommand(char command) {
  switch (command) {
    case 'i':
    case 'I':
      verifySensor();
      break;
    case 'p':
    case 'P':
      printFingerParameters();
      break;
    case 'c':
    case 'C':
      readTemplateCount();
      break;
    case 'a':
    case 'A': {
      Serial.print("[录入] 请输入 ID(1~127): ");
      const uint16_t id = readNumber();
      if (id < 1 || id > 127) {
        Serial.println("[录入] ID 超出范围");
        break;
      }
      enrollFingerprint(id);
      break;
    }
    case 's':
    case 'S':
      searchFingerprint();
      break;
    case 'd':
    case 'D':
      deleteFingerprint();
      break;
    case 'e':
    case 'E':
      emptyDatabase();
      break;
    case 'h':
    case 'H':
      printMenu();
      break;
    case '\r':
    case '\n':
      break;
    default:
      Serial.print("[命令] 未知命令: ");
      Serial.println(command);
      printMenu();
      break;
  }
}
}

void setup() {
  Serial.begin(DEBUG_BAUD_RATE);
  while (!Serial) {
    delay(10);
  }

  Serial.println();
  Serial.println("AS608 串口测试启动");
  Serial.print("调试串口波特率: ");
  Serial.println(DEBUG_BAUD_RATE);
  Serial.print("指纹串口: Serial2, RX=");
  Serial.print(SENSOR_RX_PIN);
  Serial.print(", TX=");
  Serial.print(SENSOR_TX_PIN);
  Serial.print(", 波特率=");
  Serial.println(SENSOR_BAUD_RATE_DEFAULT);

  verifySensor();
  readTemplateCount();
  printMenu();
}

void loop() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    handleCommand(command);
    lastHintAt = millis();
  }

  if (millis() - lastHintAt > 10000) {
    Serial.println("[提示] 输入 h 查看菜单，输入 s 测试搜索");
    lastHintAt = millis();
  }
}
