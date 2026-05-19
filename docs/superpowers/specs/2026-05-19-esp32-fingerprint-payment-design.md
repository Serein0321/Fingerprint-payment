# ESP32 指纹支付系统设计

## 1. 项目目标

本项目基于 ESP32-WROOM-32、4x4 矩阵键盘、0.96 寸 I2C OLED 和 AS608 指纹模块，实现一个可联网的指纹支付终端。

系统目标如下：

- 设备待机时显示网络校时后的时钟
- 设备可通过矩阵键盘进入功能菜单
- 设备可扫描附近 WiFi，并通过数字密码完成连接
- 设备可录入学生指纹，绑定学号
- 设备可通过键盘输入金额，显示在 OLED 上，再通过指纹发起充值或消费请求
- 后端使用 Node.js 对接 MySQL，统一处理账户余额、扣款、充值和交易流水
- 数据库中维护学生账户表、学号姓名对照表、交易记录表、指纹绑定表

## 2. 硬件定义

### 2.1 矩阵键盘

- 行引脚：D13、D12、D14、D27
- 列引脚：D26、D25、D33、D32

键位定义按从左到右、从上到下：

| 键位 | 含义 |
| --- | --- |
| 1 | 数字 1 |
| 2 | 数字 2 |
| 3 | 数字 3 |
| A | 上 |
| 4 | 数字 4 |
| 5 | 数字 5 |
| 6 | 数字 6 |
| B | 下 |
| 7 | 数字 7 |
| 8 | 数字 8 |
| 9 | 数字 9 |
| C | 返回 |
| * | 小数点 |
| 0 | 数字 0 |
| # | 删除 |
| D | 确认 |

### 2.2 OLED

- 分辨率：128x64
- 协议：I2C
- SDA：D21
- SCL：D22

### 2.3 AS608 指纹模块

- TX -> D16
- RX -> D17
- 使用 ESP32 的硬件串口进行通信

## 3. 系统架构

系统采用三层结构：

1. 设备层：ESP32 负责本地输入、屏幕显示、WiFi 联网、指纹录入与比对
2. 服务层：Node.js 提供 REST API，负责业务校验、账务处理和日志记录
3. 数据层：MySQL 保存学生信息、指纹绑定关系和交易记录

原则如下：

- ESP32 不直接连接 MySQL
- 所有充值、消费、绑定行为都先经过后端 API
- 设备只负责采集输入、显示结果和上传业务请求
- 余额的最终状态只以数据库为准

## 4. 设备端界面与交互流程

### 4.1 待机时钟页

默认页面显示：

- 当前日期时间
- WiFi 连接状态
- 后端服务连通状态

时间来源为 NTP 自动校时。若设备尚未联网，则显示“未校时”或保留上次校时结果。

### 4.2 主菜单

按确认键进入菜单，菜单项定义为：

1. 时钟
2. WiFi 连接
3. 录入指纹
4. 消费
5. 充值
6. 记录查询
7. 系统信息

交互规则：

- 上键：向上移动
- 下键：向下移动
- 返回键：返回上一层
- 确认键：进入或提交

### 4.3 WiFi 连接流程

流程如下：

1. 进入 WiFi 菜单后扫描附近热点
2. OLED 显示 SSID 列表
3. 用户通过上/下键选择目标热点
4. 按确认进入密码输入页
5. 用户使用数字键输入纯数字密码
6. 删除键删除最后一位
7. 确认键发起连接
8. 连接成功后执行 NTP 校时
9. 校时成功后调用后端心跳接口验证服务在线状态

限制：

- 当前版本仅支持纯数字 WiFi 密码
- 若连接失败，OLED 显示错误原因并允许重试

### 4.4 录入指纹流程

流程如下：

1. OLED 提示“请输入学号”
2. 用户通过键盘输入学号并确认
3. 设备调用后端接口查询学号姓名对照表
4. 若学号存在，则 OLED 显示姓名并提示录入指纹
5. 用户按压指纹两次，AS608 完成模板创建
6. 设备获取指纹模板 ID
7. 设备将学号、模板 ID、录入时间发送到后端
8. 后端创建账户或完成绑定，初始余额固定为 0

异常处理：

- 学号不存在：提示“学号未登记”
- 指纹录入失败：提示重试
- 学号已绑定其他模板：后端拒绝并返回原因
- 模板 ID 已被其他学号使用：后端拒绝并返回原因

### 4.5 充值和消费流程

流程如下：

1. 进入充值或消费菜单
2. 用户通过键盘输入金额
3. OLED 实时显示当前输入金额
4. 确认后提示“请按压指纹”
5. 设备调用 AS608 搜索指纹模板
6. 识别出模板 ID 后，设备将业务类型、金额、模板 ID、设备时间发送给后端
7. 后端根据模板 ID 查到学号，再根据学号在对照表查询姓名
8. 后端完成充值或扣款，并写入交易流水
9. 设备显示姓名、学号、金额、余额和处理结果

业务规则：

- 消费时若余额不足，后端拒绝交易
- 充值时余额直接增加
- 金额必须大于 0
- 金额保留两位小数

### 4.6 记录查询

当前版本建议仅提供“最近一条结果回显”或“最近几条交易概览”，避免在 128x64 屏上展示过多明细。

## 5. 服务端设计

### 5.1 技术栈

- Node.js
- Express
- MySQL
- mysql2
- dotenv
- cors
- helmet
- morgan

若需要更清晰的输入校验，可增加 zod 或 joi。

### 5.2 服务职责

后端负责：

- 查询学号姓名对照信息
- 保存指纹模板与学号绑定关系
- 处理充值请求
- 处理消费请求
- 查询账户余额
- 保存所有交易流水
- 提供心跳接口给设备检测服务是否在线

### 5.3 API 设计

#### 健康检查

- `GET /api/health`
  - 返回服务状态、数据库连接状态、服务时间

#### 学生对照表查询

- `GET /api/students/lookup/:studentId`
  - 根据学号查询姓名和基本状态
  - 用于录入指纹前校验学号是否已登记在对照表中

#### 指纹绑定

- `POST /api/fingerprints/enroll`
  - 请求体：`studentId`, `fingerprintTemplateId`, `deviceId`, `enrolledAt`
  - 行为：校验学号是否存在、模板是否已被占用、是否需要创建账户、保存绑定关系

#### 充值

- `POST /api/payments/topup`
  - 请求体：`fingerprintTemplateId`, `amount`, `deviceId`, `occurredAt`
  - 行为：通过模板 ID 找到学号，给账户加款，写交易记录，返回最新余额

#### 消费

- `POST /api/payments/consume`
  - 请求体：`fingerprintTemplateId`, `amount`, `deviceId`, `occurredAt`
  - 行为：通过模板 ID 找到学号，检查余额，扣款并写交易记录，返回最新余额

#### 余额查询

- `GET /api/accounts/:studentId/balance`
  - 返回账户余额和最近更新时间

#### 交易记录查询

- `GET /api/transactions?studentId=...&limit=...`
  - 返回指定学号的最近交易记录

## 6. 数据库设计

### 6.1 student_name_map 学号姓名对照表

用于维护学号与姓名的固定对应关系，是录入前置表。

字段建议：

- `id` bigint 主键
- `student_id` varchar(32) 唯一，学号
- `student_name` varchar(64)，姓名
- `created_at` datetime
- `updated_at` datetime

约束：

- `student_id` 唯一

### 6.2 student_accounts 学生账户表

用于保存账户余额和开户状态。

字段建议：

- `id` bigint 主键
- `student_id` varchar(32) 唯一
- `balance` decimal(10,2) 默认 0.00
- `status` varchar(16) 默认 `active`
- `created_at` datetime
- `updated_at` datetime

约束：

- `student_id` 关联对照表中的学号
- 新账户初始余额必须为 0.00

### 6.3 fingerprint_bindings 指纹绑定表

用于维护 AS608 模板 ID 与学号的关系。

字段建议：

- `id` bigint 主键
- `student_id` varchar(32)
- `fingerprint_template_id` int 唯一
- `device_id` varchar(64)
- `status` varchar(16) 默认 `active`
- `enrolled_at` datetime
- `created_at` datetime
- `updated_at` datetime

约束：

- `fingerprint_template_id` 唯一
- `student_id` 可设置唯一，表示一个学生只绑定一个指纹模板；若后续允许多指纹，再去掉该唯一约束

### 6.4 transaction_records 交易记录表

用于保存充值和消费记录。

字段建议：

- `id` bigint 主键
- `student_id` varchar(32)
- `student_name` varchar(64)
- `fingerprint_template_id` int
- `transaction_type` varchar(16)，取值 `topup` 或 `consume`
- `amount` decimal(10,2)
- `balance_before` decimal(10,2)
- `balance_after` decimal(10,2)
- `device_id` varchar(64)
- `result` varchar(16)，取值 `success` 或 `failed`
- `message` varchar(255)
- `occurred_at` datetime
- `created_at` datetime

说明：

- 即使失败交易，也建议记录到表中，便于排错和审计

## 7. 推荐建表 SQL

```sql
CREATE TABLE student_name_map (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL UNIQUE,
  student_name VARCHAR(64) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE student_accounts (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL UNIQUE,
  balance DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
  status VARCHAR(16) NOT NULL DEFAULT 'active',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_student_accounts_student_id
    FOREIGN KEY (student_id) REFERENCES student_name_map(student_id)
);

CREATE TABLE fingerprint_bindings (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL UNIQUE,
  fingerprint_template_id INT NOT NULL UNIQUE,
  device_id VARCHAR(64) NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'active',
  enrolled_at DATETIME NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_fingerprint_bindings_student_id
    FOREIGN KEY (student_id) REFERENCES student_name_map(student_id)
);

CREATE TABLE transaction_records (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL,
  student_name VARCHAR(64) NOT NULL,
  fingerprint_template_id INT NOT NULL,
  transaction_type VARCHAR(16) NOT NULL,
  amount DECIMAL(10, 2) NOT NULL,
  balance_before DECIMAL(10, 2) NOT NULL,
  balance_after DECIMAL(10, 2) NOT NULL,
  device_id VARCHAR(64) NOT NULL,
  result VARCHAR(16) NOT NULL,
  message VARCHAR(255) DEFAULT NULL,
  occurred_at DATETIME NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_transaction_student_id (student_id),
  INDEX idx_transaction_occurred_at (occurred_at)
);
```

## 8. 设备与后端的数据流

### 8.1 录入指纹

1. 设备采集学号
2. 后端查 `student_name_map`
3. 若学号存在，则设备执行指纹录入
4. 设备拿到模板 ID 后请求 `/api/fingerprints/enroll`
5. 后端写入 `fingerprint_bindings`
6. 若 `student_accounts` 不存在，则创建余额为 0 的账户

### 8.2 消费

1. 设备采集金额
2. 设备识别出模板 ID
3. 设备请求 `/api/payments/consume`
4. 后端查 `fingerprint_bindings` -> 学号
5. 后端查 `student_name_map` -> 姓名
6. 后端查 `student_accounts` -> 当前余额
7. 余额足够则扣款并写 `transaction_records`
8. 返回交易结果和新余额给设备显示

### 8.3 充值

1. 设备采集金额
2. 设备识别出模板 ID
3. 设备请求 `/api/payments/topup`
4. 后端查学号、姓名和账户
5. 后端加款并写 `transaction_records`
6. 返回交易结果和新余额给设备显示

## 9. 错误处理原则

设备端错误提示尽量简短明确，包括：

- WiFi 连接失败
- 服务未连接
- 学号未登记
- 指纹未识别
- 指纹录入失败
- 余额不足
- 请求超时
- 服务器处理失败

后端返回统一 JSON 结构：

```json
{
  "success": false,
  "message": "余额不足",
  "code": "INSUFFICIENT_BALANCE"
}
```

成功响应建议包含：

- `success`
- `message`
- `studentId`
- `studentName`
- `amount`
- `balance`
- `transactionType`
- `occurredAt`

## 10. 安全与实现边界

本阶段以演示可用为主，采用以下边界：

- WiFi 密码仅支持数字输入
- 暂不做 HTTPS 和 JWT 鉴权
- 暂不做复杂后台管理系统
- 暂不支持键盘直接输入中文姓名

建议至少保留以下基础防护：

- 后端开启基础请求日志
- 对金额、模板 ID、学号做严格校验
- 交易处理放在数据库事务里，防止余额和流水不一致

## 11. 测试建议

### 11.1 设备端

- 测试键盘扫描与按键映射是否正确
- 测试 OLED 菜单切换和金额输入显示
- 测试 WiFi 扫描和数字密码连接
- 测试 AS608 录入与搜索
- 测试接口请求成功、超时、失败的显示

### 11.2 后端

- 测试学号查询接口
- 测试指纹绑定接口的重复绑定拦截
- 测试充值接口余额累加是否正确
- 测试消费接口余额不足拦截
- 测试交易记录是否正确写入

## 12. 需要安装的库

### 12.1 Arduino / ESP32 端

- `WiFi.h`（ESP32 核心自带）
- `HTTPClient.h`（ESP32 核心自带）
- `Wire.h`（核心自带）
- `Keypad.h`
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `Adafruit Fingerprint Sensor Library`
- `ArduinoJson`
- `time.h`（ESP32 环境可直接使用）

### 12.2 Node.js 端

- `express`
- `mysql2`
- `dotenv`
- `cors`
- `helmet`
- `morgan`
- 可选：`zod`

## 13. 实现顺序建议

建议按以下顺序开发：

1. 先完成 ESP32 上的 OLED、键盘、菜单框架
2. 再完成 WiFi 连接和 NTP 校时
3. 再完成 AS608 录入与搜索封装
4. 然后完成 Node.js 后端和 MySQL 建表
5. 最后打通录入、充值、消费的完整链路
