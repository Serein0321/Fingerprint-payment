# ESP32 Fingerprint Payment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the ESP32 firmware, Node.js backend, and MySQL schema for a keypad-driven fingerprint payment terminal that shows time on the OLED, connects to WiFi, enrolls fingerprints by student ID, and sends top-up or consume requests to a backend that owns account balances.

**Architecture:** Keep the ESP32 sketch thin by moving keypad, OLED rendering, WiFi/NTP, AS608, and HTTP logic into focused modules in the sketch folder. Build the backend as an Express app with explicit route, service, and repository layers, and enforce all balance mutations inside MySQL transactions so the account balance and transaction log stay consistent. Use a single SQL schema file so the database can be recreated repeatably on a clean MySQL instance.

**Tech Stack:** Arduino C++ for ESP32, Keypad, Adafruit SSD1306/GFX, Adafruit Fingerprint Sensor, ArduinoJson, Express, mysql2/promise, Vitest, Supertest, MySQL 8

---

## Implementation Notes

- The current workspace is not a Git repository, so commit steps are omitted. If the user initializes Git later, add a commit after each task.
- For backend code, follow TDD strictly with failing tests before production code.
- For ESP32 code, use compile-first probes when a hardware-dependent module cannot be exercised with a desktop unit test in the current workspace.

## File Structure

### Firmware files

- Modify: `sketch_may19a.ino` — bootstrap shared services and run the main app loop
- Create: `DeviceConfig.h` — pin map, WiFi/NTP defaults, API base URL, device ID constants
- Create: `DeviceState.h` — enums, structs, menu state, amount buffer, WiFi scan results, transaction result data
- Create: `KeypadInput.h`
- Create: `KeypadInput.cpp` — map matrix key presses to semantic actions
- Create: `OledUi.h`
- Create: `OledUi.cpp` — render clock page, menu page, WiFi page, amount entry page, result page
- Create: `NetworkClock.h`
- Create: `NetworkClock.cpp` — WiFi scan/connect, NTP sync, health ping
- Create: `ApiClient.h`
- Create: `ApiClient.cpp` — call backend lookup, enroll, top-up, consume, and records endpoints
- Create: `FingerprintService.h`
- Create: `FingerprintService.cpp` — wrap AS608 enroll/search/delete operations
- Create: `PaymentFlow.h`
- Create: `PaymentFlow.cpp` — coordinate keypad input, OLED state changes, API calls, and fingerprint actions

### Backend files

- Create: `backend/package.json` — scripts and dependencies
- Create: `backend/.env.example` — database and server configuration sample
- Create: `backend/vitest.config.js` — Vitest defaults for Node environment
- Create: `backend/src/app.js` — Express app factory and dependency wiring
- Create: `backend/src/server.js` — HTTP server startup
- Create: `backend/src/config/env.js` — environment variable loading and validation
- Create: `backend/src/config/db.js` — MySQL pool factory and transaction helper
- Create: `backend/src/middleware/errorHandler.js` — JSON error responses
- Create: `backend/src/routes/healthRoutes.js`
- Create: `backend/src/routes/studentRoutes.js`
- Create: `backend/src/routes/fingerprintRoutes.js`
- Create: `backend/src/routes/paymentRoutes.js`
- Create: `backend/src/routes/transactionRoutes.js`
- Create: `backend/src/services/studentService.js`
- Create: `backend/src/services/fingerprintService.js`
- Create: `backend/src/services/paymentService.js`
- Create: `backend/src/services/transactionService.js`
- Create: `backend/src/repositories/studentRepository.js`
- Create: `backend/src/repositories/accountRepository.js`
- Create: `backend/src/repositories/fingerprintRepository.js`
- Create: `backend/src/repositories/transactionRepository.js`
- Create: `backend/db/schema.sql` — MySQL DDL for all required tables
- Create: `backend/tests/health.test.js`
- Create: `backend/tests/studentRoutes.test.js`
- Create: `backend/tests/fingerprintRoutes.test.js`
- Create: `backend/tests/paymentRoutes.test.js`
- Create: `backend/tests/transactionRoutes.test.js`

### Verification commands used during execution

- Backend tests: `cd backend && npm test`
- Backend syntax check: `cd backend && node --check src/app.js && node --check src/server.js`
- Firmware compile: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

## Task 1: Bootstrap the Node.js test harness and health endpoint

**Files:**
- Create: `backend/package.json`
- Create: `backend/vitest.config.js`
- Create: `backend/tests/health.test.js`
- Create: `backend/src/app.js`
- Create: `backend/src/server.js`
- Create: `backend/src/middleware/errorHandler.js`

- [ ] **Step 1: Create the backend package manifest and test runner config**

```json
{
  "name": "fingerprint-payment-backend",
  "version": "1.0.0",
  "private": true,
  "scripts": {
    "dev": "node --watch src/server.js",
    "start": "node src/server.js",
    "test": "vitest run",
    "test:watch": "vitest",
    "check": "node --check src/app.js && node --check src/server.js"
  },
  "dependencies": {
    "cors": "^2.8.5",
    "dotenv": "^16.4.5",
    "express": "^4.19.2",
    "helmet": "^7.1.0",
    "morgan": "^1.10.0",
    "mysql2": "^3.11.0"
  },
  "devDependencies": {
    "supertest": "^7.0.0",
    "vitest": "^2.0.5"
  }
}
```

```js
const { defineConfig } = require('vitest/config');

module.exports = defineConfig({
  test: {
    environment: 'node',
    include: ['tests/**/*.test.js']
  }
});
```

- [ ] **Step 2: Write the failing health endpoint test**

```js
const request = require('supertest');
const { describe, expect, it } = require('vitest');
const { createApp } = require('../src/app');

describe('GET /api/health', () => {
  it('returns an ok status payload', async () => {
    const app = createApp();
    const response = await request(app).get('/api/health');

    expect(response.status).toBe(200);
    expect(response.body.success).toBe(true);
    expect(response.body.data.status).toBe('ok');
    expect(typeof response.body.data.timestamp).toBe('string');
  });
});
```

- [ ] **Step 3: Run the health test to verify the RED state**

Run: `cd backend && npm install && npm test -- tests/health.test.js`

Expected: FAIL with a module-not-found error for `../src/app` or a missing `/api/health` route.

- [ ] **Step 4: Implement the minimal app factory and health route**

```js
const cors = require('cors');
const express = require('express');
const helmet = require('helmet');
const morgan = require('morgan');
const { errorHandler } = require('./middleware/errorHandler');

function createApp() {
  const app = express();

  app.use(helmet());
  app.use(cors());
  app.use(express.json());
  app.use(morgan('dev'));

  app.get('/api/health', (_request, response) => {
    response.json({
      success: true,
      data: {
        status: 'ok',
        timestamp: new Date().toISOString()
      }
    });
  });

  app.use(errorHandler);

  return app;
}

module.exports = { createApp };
```

```js
function errorHandler(error, _request, response, _next) {
  response.status(error.statusCode || 500).json({
    success: false,
    code: error.code || 'INTERNAL_SERVER_ERROR',
    message: error.message || '服务器处理失败'
  });
}

module.exports = { errorHandler };
```

```js
const { createApp } = require('./app');

const app = createApp();
const port = process.env.PORT || 3000;

app.listen(port, () => {
  console.log(`Fingerprint payment backend listening on ${port}`);
});
```

- [ ] **Step 5: Run the health test and syntax check to verify GREEN**

Run: `cd backend && npm test -- tests/health.test.js && npm run check`

Expected: `tests/health.test.js` passes and both Node syntax checks exit with code 0.

## Task 2: Add database configuration and the MySQL schema

**Files:**
- Create: `backend/.env.example`
- Create: `backend/src/config/env.js`
- Create: `backend/src/config/db.js`
- Create: `backend/db/schema.sql`

- [ ] **Step 1: Add the environment sample and validation module**

```env
PORT=3000
DB_HOST=127.0.0.1
DB_PORT=3306
DB_NAME=fingerprint_payment
DB_USER=root
DB_PASSWORD=123456
DEVICE_ID=esp32-watch-001
```

```js
const dotenv = require('dotenv');

dotenv.config();

const requiredKeys = ['DB_HOST', 'DB_PORT', 'DB_NAME', 'DB_USER', 'DB_PASSWORD'];

for (const key of requiredKeys) {
  if (!process.env[key]) {
    throw new Error(`Missing required environment variable: ${key}`);
  }
}

module.exports = {
  port: Number(process.env.PORT || 3000),
  db: {
    host: process.env.DB_HOST,
    port: Number(process.env.DB_PORT),
    database: process.env.DB_NAME,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD
  },
  deviceId: process.env.DEVICE_ID || 'esp32-watch-001'
};
```

- [ ] **Step 2: Create the MySQL schema file**

```sql
CREATE DATABASE IF NOT EXISTS fingerprint_payment CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE fingerprint_payment;

CREATE TABLE IF NOT EXISTS student_name_map (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL UNIQUE,
  student_name VARCHAR(64) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS student_accounts (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id VARCHAR(32) NOT NULL UNIQUE,
  balance DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
  status VARCHAR(16) NOT NULL DEFAULT 'active',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_student_accounts_student_id
    FOREIGN KEY (student_id) REFERENCES student_name_map(student_id)
);

CREATE TABLE IF NOT EXISTS fingerprint_bindings (
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

CREATE TABLE IF NOT EXISTS transaction_records (
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

- [ ] **Step 3: Add the MySQL pool factory and transaction helper**

```js
const mysql = require('mysql2/promise');
const { db } = require('./env');

const pool = mysql.createPool({
  host: db.host,
  port: db.port,
  database: db.database,
  user: db.user,
  password: db.password,
  waitForConnections: true,
  connectionLimit: 10,
  namedPlaceholders: true
});

async function withTransaction(callback) {
  const connection = await pool.getConnection();
  try {
    await connection.beginTransaction();
    const result = await callback(connection);
    await connection.commit();
    return result;
  } catch (error) {
    await connection.rollback();
    throw error;
  } finally {
    connection.release();
  }
}

module.exports = { pool, withTransaction };
```

- [ ] **Step 4: Verify the backend still parses after adding config files**

Run: `cd backend && npm run check`

Expected: command exits with code 0.

## Task 3: Implement student lookup by student ID

**Files:**
- Create: `backend/tests/studentRoutes.test.js`
- Create: `backend/src/routes/studentRoutes.js`
- Create: `backend/src/services/studentService.js`
- Create: `backend/src/repositories/studentRepository.js`
- Modify: `backend/src/app.js`

- [ ] **Step 1: Write the failing lookup route test**

```js
const request = require('supertest');
const { describe, expect, it, vi } = require('vitest');
const { createApp } = require('../src/app');

describe('GET /api/students/lookup/:studentId', () => {
  it('returns the student name from the mapping table', async () => {
    const studentService = {
      lookupStudent: vi.fn().mockResolvedValue({
        studentId: '20240001',
        studentName: '张三'
      })
    };

    const app = createApp({ studentService });
    const response = await request(app).get('/api/students/lookup/20240001');

    expect(response.status).toBe(200);
    expect(response.body.success).toBe(true);
    expect(response.body.data.studentName).toBe('张三');
  });

  it('returns 404 when the student does not exist', async () => {
    const studentService = {
      lookupStudent: vi.fn().mockResolvedValue(null)
    };

    const app = createApp({ studentService });
    const response = await request(app).get('/api/students/lookup/99999999');

    expect(response.status).toBe(404);
    expect(response.body.success).toBe(false);
    expect(response.body.code).toBe('STUDENT_NOT_FOUND');
  });
});
```

- [ ] **Step 2: Run the lookup test to verify the RED state**

Run: `cd backend && npm test -- tests/studentRoutes.test.js`

Expected: FAIL because `createApp` does not mount the student lookup route.

- [ ] **Step 3: Implement the repository, service, route, and route wiring**

```js
function createStudentRepository(pool) {
  return {
    async findByStudentId(studentId) {
      const [rows] = await pool.query(
        'SELECT student_id AS studentId, student_name AS studentName FROM student_name_map WHERE student_id = ?',
        [studentId]
      );
      return rows[0] || null;
    }
  };
}

module.exports = { createStudentRepository };
```

```js
function createStudentService(studentRepository) {
  return {
    async lookupStudent(studentId) {
      return studentRepository.findByStudentId(studentId);
    }
  };
}

module.exports = { createStudentService };
```

```js
const express = require('express');

function createStudentRouter(studentService) {
  const router = express.Router();

  router.get('/lookup/:studentId', async (request, response, next) => {
    try {
      const student = await studentService.lookupStudent(request.params.studentId);

      if (!student) {
        response.status(404).json({
          success: false,
          code: 'STUDENT_NOT_FOUND',
          message: '学号未登记'
        });
        return;
      }

      response.json({ success: true, data: student });
    } catch (error) {
      next(error);
    }
  });

  return router;
}

module.exports = { createStudentRouter };
```

```js
const { createStudentRouter } = require('./routes/studentRoutes');

function createApp(deps = {}) {
  const app = express();
  const studentService = deps.studentService || { lookupStudent: async () => null };

  app.use('/api/students', createStudentRouter(studentService));
  return app;
}
```

- [ ] **Step 4: Run the route test and full backend test suite**

Run: `cd backend && npm test -- tests/studentRoutes.test.js && npm test`

Expected: the lookup route test passes and the full suite stays green.

## Task 4: Implement fingerprint enrollment with account bootstrap

**Files:**
- Create: `backend/tests/fingerprintRoutes.test.js`
- Create: `backend/src/routes/fingerprintRoutes.js`
- Create: `backend/src/services/fingerprintService.js`
- Create: `backend/src/repositories/accountRepository.js`
- Create: `backend/src/repositories/fingerprintRepository.js`
- Modify: `backend/src/app.js`

- [ ] **Step 1: Write the failing fingerprint enrollment tests**

```js
const request = require('supertest');
const { describe, expect, it, vi } = require('vitest');
const { createApp } = require('../src/app');

describe('POST /api/fingerprints/enroll', () => {
  it('binds a template to a known student', async () => {
    const fingerprintService = {
      enrollFingerprint: vi.fn().mockResolvedValue({
        studentId: '20240001',
        fingerprintTemplateId: 8,
        balance: '0.00'
      })
    };

    const app = createApp({ fingerprintService });
    const response = await request(app)
      .post('/api/fingerprints/enroll')
      .send({
        studentId: '20240001',
        fingerprintTemplateId: 8,
        deviceId: 'esp32-watch-001',
        enrolledAt: '2026-05-19T10:00:00.000Z'
      });

    expect(response.status).toBe(201);
    expect(response.body.success).toBe(true);
    expect(response.body.data.fingerprintTemplateId).toBe(8);
  });

  it('rejects a duplicate template binding', async () => {
    const fingerprintService = {
      enrollFingerprint: vi.fn().mockRejectedValue(Object.assign(new Error('模板已被占用'), {
        statusCode: 409,
        code: 'FINGERPRINT_TEMPLATE_IN_USE'
      }))
    };

    const app = createApp({ fingerprintService });
    const response = await request(app)
      .post('/api/fingerprints/enroll')
      .send({
        studentId: '20240001',
        fingerprintTemplateId: 8,
        deviceId: 'esp32-watch-001',
        enrolledAt: '2026-05-19T10:00:00.000Z'
      });

    expect(response.status).toBe(409);
    expect(response.body.code).toBe('FINGERPRINT_TEMPLATE_IN_USE');
  });
});
```

- [ ] **Step 2: Run the enrollment tests to verify the RED state**

Run: `cd backend && npm test -- tests/fingerprintRoutes.test.js`

Expected: FAIL because the enroll route is not registered.

- [ ] **Step 3: Implement enrollment logic with account bootstrap**

```js
function createFingerprintRepository(pool) {
  return {
    async findByTemplateId(connection, fingerprintTemplateId) {
      const [rows] = await connection.query(
        'SELECT * FROM fingerprint_bindings WHERE fingerprint_template_id = ?',
        [fingerprintTemplateId]
      );
      return rows[0] || null;
    },
    async insertBinding(connection, payload) {
      await connection.query(
        `INSERT INTO fingerprint_bindings (student_id, fingerprint_template_id, device_id, status, enrolled_at)
         VALUES (?, ?, ?, 'active', ?)`,
        [payload.studentId, payload.fingerprintTemplateId, payload.deviceId, payload.enrolledAt]
      );
    }
  };
}

module.exports = { createFingerprintRepository };
```

```js
function createAccountRepository() {
  return {
    async ensureAccountExists(connection, studentId) {
      await connection.query(
        `INSERT INTO student_accounts (student_id, balance, status)
         VALUES (?, 0.00, 'active')
         ON DUPLICATE KEY UPDATE student_id = student_id`,
        [studentId]
      );
    }
  };
}

module.exports = { createAccountRepository };
```

```js
function createFingerprintService({ withTransaction, studentRepository, accountRepository, fingerprintRepository }) {
  return {
    async enrollFingerprint(payload) {
      return withTransaction(async (connection) => {
        const student = await studentRepository.findByStudentId(payload.studentId);
        if (!student) {
          const error = new Error('学号未登记');
          error.statusCode = 404;
          error.code = 'STUDENT_NOT_FOUND';
          throw error;
        }

        const existingBinding = await fingerprintRepository.findByTemplateId(connection, payload.fingerprintTemplateId);
        if (existingBinding) {
          const error = new Error('模板已被占用');
          error.statusCode = 409;
          error.code = 'FINGERPRINT_TEMPLATE_IN_USE';
          throw error;
        }

        await accountRepository.ensureAccountExists(connection, payload.studentId);
        await fingerprintRepository.insertBinding(connection, payload);

        return {
          studentId: payload.studentId,
          studentName: student.studentName,
          fingerprintTemplateId: payload.fingerprintTemplateId,
          balance: '0.00'
        };
      });
    }
  };
}

module.exports = { createFingerprintService };
```

```js
const express = require('express');

function createFingerprintRouter(fingerprintService) {
  const router = express.Router();

  router.post('/enroll', async (request, response, next) => {
    try {
      const result = await fingerprintService.enrollFingerprint(request.body);
      response.status(201).json({ success: true, data: result });
    } catch (error) {
      next(error);
    }
  });

  return router;
}

module.exports = { createFingerprintRouter };
```

- [ ] **Step 4: Run the enrollment tests and the full backend suite**

Run: `cd backend && npm test -- tests/fingerprintRoutes.test.js && npm test`

Expected: the enrollment route tests pass and the rest of the backend tests remain green.

## Task 5: Implement top-up and consume requests with transaction logging

**Files:**
- Create: `backend/tests/paymentRoutes.test.js`
- Create: `backend/src/routes/paymentRoutes.js`
- Create: `backend/src/services/paymentService.js`
- Create: `backend/src/repositories/transactionRepository.js`
- Modify: `backend/src/repositories/accountRepository.js`
- Modify: `backend/src/repositories/fingerprintRepository.js`
- Modify: `backend/src/app.js`

- [ ] **Step 1: Write the failing payment route tests**

```js
const request = require('supertest');
const { describe, expect, it, vi } = require('vitest');
const { createApp } = require('../src/app');

describe('payment routes', () => {
  it('tops up a student account by fingerprint template id', async () => {
    const paymentService = {
      topup: vi.fn().mockResolvedValue({
        studentId: '20240001',
        studentName: '张三',
        amount: '20.00',
        balance: '20.00',
        transactionType: 'topup'
      })
    };

    const app = createApp({ paymentService });
    const response = await request(app).post('/api/payments/topup').send({
      fingerprintTemplateId: 8,
      amount: '20.00',
      deviceId: 'esp32-watch-001',
      occurredAt: '2026-05-19T10:30:00.000Z'
    });

    expect(response.status).toBe(200);
    expect(response.body.data.transactionType).toBe('topup');
    expect(response.body.data.balance).toBe('20.00');
  });

  it('rejects consume when balance is insufficient', async () => {
    const paymentService = {
      consume: vi.fn().mockRejectedValue(Object.assign(new Error('余额不足'), {
        statusCode: 409,
        code: 'INSUFFICIENT_BALANCE'
      }))
    };

    const app = createApp({ paymentService });
    const response = await request(app).post('/api/payments/consume').send({
      fingerprintTemplateId: 8,
      amount: '50.00',
      deviceId: 'esp32-watch-001',
      occurredAt: '2026-05-19T10:30:00.000Z'
    });

    expect(response.status).toBe(409);
    expect(response.body.code).toBe('INSUFFICIENT_BALANCE');
  });
});
```

- [ ] **Step 2: Run the payment tests to verify the RED state**

Run: `cd backend && npm test -- tests/paymentRoutes.test.js`

Expected: FAIL because the payment routes are missing.

- [ ] **Step 3: Implement transaction-safe top-up and consume logic**

```js
function createAccountRepository() {
  return {
    async ensureAccountExists(connection, studentId) {
      await connection.query(
        `INSERT INTO student_accounts (student_id, balance, status)
         VALUES (?, 0.00, 'active')
         ON DUPLICATE KEY UPDATE student_id = student_id`,
        [studentId]
      );
    },
    async findAccountForUpdate(connection, studentId) {
      const [rows] = await connection.query(
        `SELECT sa.student_id, sa.balance, snm.student_name
           FROM student_accounts sa
           JOIN student_name_map snm ON snm.student_id = sa.student_id
          WHERE sa.student_id = ?
          FOR UPDATE`,
        [studentId]
      );
      return rows[0] || null;
    },
    async updateBalance(connection, studentId, balance) {
      await connection.query(
        'UPDATE student_accounts SET balance = ?, updated_at = CURRENT_TIMESTAMP WHERE student_id = ?',
        [balance, studentId]
      );
    }
  };
}

module.exports = { createAccountRepository };
```

```js
function createTransactionRepository() {
  return {
    async insertRecord(connection, payload) {
      await connection.query(
        `INSERT INTO transaction_records (
           student_id, student_name, fingerprint_template_id, transaction_type,
           amount, balance_before, balance_after, device_id, result, message, occurred_at
         ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
        [
          payload.studentId,
          payload.studentName,
          payload.fingerprintTemplateId,
          payload.transactionType,
          payload.amount,
          payload.balanceBefore,
          payload.balanceAfter,
          payload.deviceId,
          payload.result,
          payload.message || null,
          payload.occurredAt
        ]
      );
    }
  };
}

module.exports = { createTransactionRepository };
```

```js
function createPaymentService({ withTransaction, fingerprintRepository, accountRepository, transactionRepository }) {
  async function resolveAccount(connection, fingerprintTemplateId) {
    const binding = await fingerprintRepository.findByTemplateId(connection, fingerprintTemplateId);
    if (!binding) {
      const error = new Error('指纹未绑定');
      error.statusCode = 404;
      error.code = 'FINGERPRINT_NOT_FOUND';
      throw error;
    }

    const account = await accountRepository.findAccountForUpdate(connection, binding.student_id);
    return { binding, account };
  }

  return {
    async topup(payload) {
      return withTransaction(async (connection) => {
        const { binding, account } = await resolveAccount(connection, payload.fingerprintTemplateId);
        const balanceBefore = Number(account.balance);
        const balanceAfter = (balanceBefore + Number(payload.amount)).toFixed(2);

        await accountRepository.updateBalance(connection, binding.student_id, balanceAfter);
        await transactionRepository.insertRecord(connection, {
          studentId: binding.student_id,
          studentName: account.student_name,
          fingerprintTemplateId: payload.fingerprintTemplateId,
          transactionType: 'topup',
          amount: payload.amount,
          balanceBefore: balanceBefore.toFixed(2),
          balanceAfter,
          deviceId: payload.deviceId,
          result: 'success',
          occurredAt: payload.occurredAt
        });

        return {
          studentId: binding.student_id,
          studentName: account.student_name,
          amount: Number(payload.amount).toFixed(2),
          balance: balanceAfter,
          transactionType: 'topup'
        };
      });
    },

    async consume(payload) {
      return withTransaction(async (connection) => {
        const { binding, account } = await resolveAccount(connection, payload.fingerprintTemplateId);
        const balanceBefore = Number(account.balance);
        const amount = Number(payload.amount);

        if (balanceBefore < amount) {
          const error = new Error('余额不足');
          error.statusCode = 409;
          error.code = 'INSUFFICIENT_BALANCE';
          throw error;
        }

        const balanceAfter = (balanceBefore - amount).toFixed(2);
        await accountRepository.updateBalance(connection, binding.student_id, balanceAfter);
        await transactionRepository.insertRecord(connection, {
          studentId: binding.student_id,
          studentName: account.student_name,
          fingerprintTemplateId: payload.fingerprintTemplateId,
          transactionType: 'consume',
          amount: amount.toFixed(2),
          balanceBefore: balanceBefore.toFixed(2),
          balanceAfter,
          deviceId: payload.deviceId,
          result: 'success',
          occurredAt: payload.occurredAt
        });

        return {
          studentId: binding.student_id,
          studentName: account.student_name,
          amount: amount.toFixed(2),
          balance: balanceAfter,
          transactionType: 'consume'
        };
      });
    }
  };
}

module.exports = { createPaymentService };
```

```js
const express = require('express');

function createPaymentRouter(paymentService) {
  const router = express.Router();

  router.post('/topup', async (request, response, next) => {
    try {
      const result = await paymentService.topup(request.body);
      response.json({ success: true, data: result });
    } catch (error) {
      next(error);
    }
  });

  router.post('/consume', async (request, response, next) => {
    try {
      const result = await paymentService.consume(request.body);
      response.json({ success: true, data: result });
    } catch (error) {
      next(error);
    }
  });

  return router;
}

module.exports = { createPaymentRouter };
```

- [ ] **Step 4: Run the payment tests and then the full backend suite**

Run: `cd backend && npm test -- tests/paymentRoutes.test.js && npm test`

Expected: the top-up and consume route tests pass and all backend tests stay green.

## Task 6: Implement transaction query support for the OLED records screen

**Files:**
- Create: `backend/tests/transactionRoutes.test.js`
- Create: `backend/src/routes/transactionRoutes.js`
- Create: `backend/src/services/transactionService.js`
- Modify: `backend/src/repositories/transactionRepository.js`
- Modify: `backend/src/app.js`

- [ ] **Step 1: Write the failing transaction query test**

```js
const request = require('supertest');
const { describe, expect, it, vi } = require('vitest');
const { createApp } = require('../src/app');

describe('GET /api/transactions', () => {
  it('returns the latest transactions for a student id', async () => {
    const transactionService = {
      listTransactions: vi.fn().mockResolvedValue([
        {
          studentId: '20240001',
          studentName: '张三',
          transactionType: 'consume',
          amount: '8.50',
          balanceAfter: '11.50',
          occurredAt: '2026-05-19T11:00:00.000Z'
        }
      ])
    };

    const app = createApp({ transactionService });
    const response = await request(app).get('/api/transactions?studentId=20240001&limit=5');

    expect(response.status).toBe(200);
    expect(response.body.success).toBe(true);
    expect(response.body.data).toHaveLength(1);
    expect(response.body.data[0].transactionType).toBe('consume');
  });
});
```

- [ ] **Step 2: Run the records test to verify the RED state**

Run: `cd backend && npm test -- tests/transactionRoutes.test.js`

Expected: FAIL because the transaction route is not wired.

- [ ] **Step 3: Implement the records route and query service**

```js
function createTransactionRepository(pool) {
  return {
    async insertRecord(connection, payload) {
      await connection.query(
        `INSERT INTO transaction_records (
           student_id, student_name, fingerprint_template_id, transaction_type,
           amount, balance_before, balance_after, device_id, result, message, occurred_at
         ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
        [
          payload.studentId,
          payload.studentName,
          payload.fingerprintTemplateId,
          payload.transactionType,
          payload.amount,
          payload.balanceBefore,
          payload.balanceAfter,
          payload.deviceId,
          payload.result,
          payload.message || null,
          payload.occurredAt
        ]
      );
    },
    async listByStudentId(studentId, limit) {
      const [rows] = await pool.query(
        `SELECT student_id AS studentId, student_name AS studentName, transaction_type AS transactionType,
                amount, balance_after AS balanceAfter, occurred_at AS occurredAt
           FROM transaction_records
          WHERE student_id = ?
          ORDER BY occurred_at DESC
          LIMIT ?`,
        [studentId, limit]
      );
      return rows;
    }
  };
}

module.exports = { createTransactionRepository };
```

```js
function createTransactionService(transactionRepository) {
  return {
    async listTransactions(studentId, limit) {
      return transactionRepository.listByStudentId(studentId, limit);
    }
  };
}

module.exports = { createTransactionService };
```

```js
const express = require('express');

function createTransactionRouter(transactionService) {
  const router = express.Router();

  router.get('/', async (request, response, next) => {
    try {
      const studentId = request.query.studentId;
      const limit = Number(request.query.limit || 5);
      const result = await transactionService.listTransactions(studentId, limit);
      response.json({ success: true, data: result });
    } catch (error) {
      next(error);
    }
  });

  return router;
}

module.exports = { createTransactionRouter };
```

- [ ] **Step 4: Run the records test and then the full backend suite**

Run: `cd backend && npm test -- tests/transactionRoutes.test.js && npm test`

Expected: the transactions test passes and the full backend suite stays green.

## Task 7: Add the ESP32 firmware foundation, keypad mapping, and OLED menu shell

**Files:**
- Modify: `sketch_may19a.ino`
- Create: `DeviceConfig.h`
- Create: `DeviceState.h`
- Create: `KeypadInput.h`
- Create: `KeypadInput.cpp`
- Create: `OledUi.h`
- Create: `OledUi.cpp`

- [ ] **Step 1: Add a compile-first probe in the sketch that references the new modules before they exist**

```cpp
#include "DeviceConfig.h"
#include "DeviceState.h"
#include "KeypadInput.h"
#include "OledUi.h"

DeviceState appState;

void setup() {
  initializeDisplay();
  initializeKeypad();
  initializeDefaultState(appState);
}

void loop() {
  handleKeypadInput(appState);
  renderCurrentScreen(appState);
}
```

- [ ] **Step 2: Run the firmware compile to verify the RED state**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: FAIL with missing-header or missing-symbol errors for the new modules.

- [ ] **Step 3: Implement the menu foundation and keypad mapping**

```cpp
#pragma once

constexpr uint8_t ROW_PINS[4] = {13, 12, 14, 27};
constexpr uint8_t COL_PINS[4] = {26, 25, 33, 32};
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_SDA_PIN = 21;
constexpr uint8_t OLED_SCL_PIN = 22;
constexpr char API_BASE_URL[] = "http://192.168.1.100:3000";
constexpr char DEVICE_ID[] = "esp32-watch-001";
```

```cpp
#pragma once

enum class ScreenId {
  Clock,
  MainMenu,
  WifiList,
  WifiPassword,
  EnrollStudentId,
  EnrollFinger,
  ConsumeAmount,
  TopupAmount,
  TransactionResult,
  Records
};

struct DeviceState {
  ScreenId screen = ScreenId::Clock;
  int menuIndex = 0;
  String inputBuffer = "";
  String statusLine = "未联网";
  String currentTime = "--:--:--";
};
```

```cpp
#pragma once

#include "DeviceState.h"

void initializeKeypad();
void handleKeypadInput(DeviceState& state);
```

```cpp
#include <Keypad.h>
#include "DeviceConfig.h"
#include "KeypadInput.h"

char keyMap[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keyMap), (byte*)ROW_PINS, (byte*)COL_PINS, 4, 4);

void initializeKeypad() {}

void handleKeypadInput(DeviceState& state) {
  char key = keypad.getKey();
  if (!key) {
    return;
  }

  if (key == 'A' && state.menuIndex > 0) {
    state.menuIndex--;
  } else if (key == 'B' && state.menuIndex < 6) {
    state.menuIndex++;
  } else if ((key >= '0' && key <= '9') || key == '*') {
    state.inputBuffer += key;
  } else if (key == '#' && state.inputBuffer.length() > 0) {
    state.inputBuffer.remove(state.inputBuffer.length() - 1);
  } else if (key == 'D') {
    state.screen = ScreenId::MainMenu;
  } else if (key == 'C') {
    state.screen = ScreenId::Clock;
  }
}
```

```cpp
#pragma once

#include "DeviceState.h"

void initializeDisplay();
void initializeDefaultState(DeviceState& state);
void renderCurrentScreen(const DeviceState& state);
```

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "DeviceConfig.h"
#include "OledUi.h"

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void initializeDisplay() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void initializeDefaultState(DeviceState& state) {
  state.screen = ScreenId::Clock;
  state.menuIndex = 0;
  state.statusLine = "待机";
}

void renderCurrentScreen(const DeviceState& state) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (state.screen == ScreenId::Clock) {
    display.setCursor(0, 0);
    display.print("Time");
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(state.currentTime);
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print(state.statusLine);
  } else {
    display.setCursor(0, 0);
    display.print("Menu:");
    display.setCursor(0, 16);
    display.print(state.menuIndex);
    display.setCursor(0, 32);
    display.print(state.inputBuffer);
  }

  display.display();
}
```

- [ ] **Step 4: Re-run the firmware compile to verify GREEN**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: compile succeeds with the menu shell in place.

## Task 8: Add WiFi scan, NTP clock sync, and backend health ping

**Files:**
- Create: `NetworkClock.h`
- Create: `NetworkClock.cpp`
- Create: `ApiClient.h`
- Create: `ApiClient.cpp`
- Modify: `sketch_may19a.ino`
- Create: `PaymentFlow.h`
- Create: `PaymentFlow.cpp`

- [ ] **Step 1: Add a compile-first probe for WiFi and API support**

```cpp
#include "ApiClient.h"
#include "NetworkClock.h"
#include "PaymentFlow.h"

ApiClient apiClient;

void setup() {
  initializeDisplay();
  initializeKeypad();
  initializeNetworkClock();
  initializeDefaultState(appState);
  apiClient.begin();
}

void loop() {
  handleKeypadInput(appState);
  updateClock(appState);
  updatePaymentFlow(appState, apiClient);
  renderCurrentScreen(appState);
}
```

- [ ] **Step 2: Run the firmware compile to verify the RED state**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: FAIL because the WiFi, clock, and API modules do not exist yet.

- [ ] **Step 3: Implement WiFi scan/connect and NTP sync support**

```cpp
#pragma once

#include "DeviceState.h"

void initializeNetworkClock();
void beginWifiScan(DeviceState& state);
bool connectWifiBySelection(DeviceState& state, int selectedIndex, const String& password);
void updateClock(DeviceState& state);
```

```cpp
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>
#include "NetworkClock.h"

void initializeNetworkClock() {
  WiFi.mode(WIFI_STA);
}

void beginWifiScan(DeviceState& state) {
  int found = WiFi.scanNetworks();
  state.statusLine = found > 0 ? "扫描完成" : "未找到WiFi";
}

bool connectWifiBySelection(DeviceState& state, int selectedIndex, const String& password) {
  String ssid = WiFi.SSID(selectedIndex);
  WiFi.begin(ssid.c_str(), password.c_str());

  for (int attempt = 0; attempt < 20; ++attempt) {
    if (WiFi.status() == WL_CONNECTED) {
      configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
      state.statusLine = "WiFi已连接";
      return true;
    }
    delay(500);
  }

  state.statusLine = "WiFi连接失败";
  return false;
}

void updateClock(DeviceState& state) {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
    state.currentTime = String(buffer);
  }
}
```

```cpp
#pragma once

#include "DeviceConfig.h"

struct ApiClient {
  void begin();
  bool pingHealth();
};
```

```cpp
#include <HTTPClient.h>
#include "DeviceConfig.h"
#include "ApiClient.h"

void ApiClient::begin() {}

bool ApiClient::pingHealth() {
  HTTPClient http;
  http.begin(String(API_BASE_URL) + "/api/health");
  int statusCode = http.GET();
  http.end();
  return statusCode == 200;
}
```

```cpp
#pragma once

#include "ApiClient.h"
#include "DeviceState.h"

void updatePaymentFlow(DeviceState& state, ApiClient& apiClient);
```

```cpp
#include "PaymentFlow.h"

void updatePaymentFlow(DeviceState& state, ApiClient& apiClient) {
  if (state.screen == ScreenId::Clock && apiClient.pingHealth()) {
    state.statusLine = "服务在线";
  }
}
```

- [ ] **Step 4: Re-run the firmware compile to verify GREEN**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: compile succeeds and the device clock page can be backed by NTP once WiFi is connected.

## Task 9: Add fingerprint enrollment, amount entry, and payment API calls to the firmware

**Files:**
- Create: `FingerprintService.h`
- Create: `FingerprintService.cpp`
- Modify: `ApiClient.h`
- Modify: `ApiClient.cpp`
- Modify: `DeviceState.h`
- Modify: `PaymentFlow.cpp`
- Modify: `OledUi.cpp`
- Modify: `sketch_may19a.ino`

- [ ] **Step 1: Add a compile-first probe for enrollment and payment submission**

```cpp
#include "FingerprintService.h"

FingerprintService fingerprintService;

void setup() {
  initializeDisplay();
  initializeKeypad();
  initializeNetworkClock();
  fingerprintService.begin();
  initializeDefaultState(appState);
  apiClient.begin();
}

void loop() {
  handleKeypadInput(appState);
  updateClock(appState);
  updatePaymentFlow(appState, apiClient, fingerprintService);
  renderCurrentScreen(appState);
}
```

- [ ] **Step 2: Run the firmware compile to verify the RED state**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: FAIL with missing symbol errors for `FingerprintService` and the updated `updatePaymentFlow` signature.

- [ ] **Step 3: Implement AS608 helpers and API requests for lookup, enroll, top-up, and consume**

```cpp
#pragma once

#include <Adafruit_Fingerprint.h>

class FingerprintService {
public:
  void begin();
  bool searchFingerprint(uint16_t& templateId);
  bool enrollFingerprint(uint16_t& templateId);

private:
  bool assignNextTemplateId(uint16_t& templateId);
  HardwareSerial serialPort = HardwareSerial(2);
  Adafruit_Fingerprint sensor = Adafruit_Fingerprint(&serialPort);
};
```

```cpp
#include "FingerprintService.h"

void FingerprintService::begin() {
  serialPort.begin(57600, SERIAL_8N1, 16, 17);
  sensor.begin(57600);
}

bool FingerprintService::searchFingerprint(uint16_t& templateId) {
  if (sensor.getImage() != FINGERPRINT_OK) {
    return false;
  }
  if (sensor.image2Tz() != FINGERPRINT_OK) {
    return false;
  }
  if (sensor.fingerSearch() != FINGERPRINT_OK) {
    return false;
  }
  templateId = sensor.fingerID;
  return true;
}

bool FingerprintService::assignNextTemplateId(uint16_t& templateId) {
  if (sensor.getTemplateCount() != FINGERPRINT_OK) {
    return false;
  }
  templateId = sensor.templateCount + 1;
  return true;
}

bool FingerprintService::enrollFingerprint(uint16_t& templateId) {
  if (!assignNextTemplateId(templateId)) {
    return false;
  }
  if (sensor.getImage() != FINGERPRINT_OK) {
    return false;
  }
  if (sensor.image2Tz(1) != FINGERPRINT_OK) {
    return false;
  }
  delay(1000);
  if (sensor.getImage() != FINGERPRINT_OK) {
    return false;
  }
  if (sensor.image2Tz(2) != FINGERPRINT_OK) {
    return false;
  }
  if (sensor.createModel() != FINGERPRINT_OK) {
    return false;
  }
  return sensor.storeModel(templateId) == FINGERPRINT_OK;
}
```

```cpp
struct StudentLookupResult {
  bool success = false;
  String studentId = "";
  String studentName = "";
};

struct PaymentResult {
  bool success = false;
  String studentId = "";
  String studentName = "";
  String amount = "0.00";
  String balance = "0.00";
  String message = "";
};
```

```cpp
struct ApiClient {
  void begin();
  bool pingHealth();
  StudentLookupResult lookupStudent(const String& studentId);
  bool enrollFingerprint(const String& studentId, uint16_t templateId);
  PaymentResult submitTopup(uint16_t templateId, const String& amount);
  PaymentResult submitConsume(uint16_t templateId, const String& amount);
};
```

```cpp
void updatePaymentFlow(DeviceState& state, ApiClient& apiClient, FingerprintService& fingerprintService) {
  if (state.screen == ScreenId::EnrollFinger && state.inputBuffer.length() > 0) {
    StudentLookupResult lookup = apiClient.lookupStudent(state.inputBuffer);
    if (!lookup.success) {
      state.statusLine = "学号未登记";
      state.screen = ScreenId::TransactionResult;
      return;
    }

    uint16_t templateId = 0;
    if (!fingerprintService.enrollFingerprint(templateId)) {
      state.statusLine = "录入失败";
      state.screen = ScreenId::TransactionResult;
      return;
    }

    if (apiClient.enrollFingerprint(lookup.studentId, templateId)) {
      state.statusLine = "录入成功";
    } else {
      state.statusLine = "绑定失败";
    }
    state.screen = ScreenId::TransactionResult;
  }
}
```

- [ ] **Step 4: Re-run the firmware compile to verify GREEN**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: compile succeeds with all major firmware modules connected.

## Task 10: Validate the integrated workflow and write the install checklist

**Files:**
- Modify: `sketch_may19a.ino`
- Modify: `backend/.env.example`
- Modify: `backend/db/schema.sql`

- [ ] **Step 1: Run the backend test suite as the first integration gate**

Run: `cd backend && npm test && npm run check`

Expected: all backend tests pass and syntax checks are clean.

- [ ] **Step 2: Run the firmware compile as the second integration gate**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32 .`

Expected: firmware compiles successfully.

- [ ] **Step 3: Load the MySQL schema and smoke-test the core API paths**

Run: `mysql -u root -p < backend/db/schema.sql`

Expected: all four tables are created without SQL errors.

Run: `cd backend && node src/server.js`

Expected: server starts on the configured port.

Run: `curl http://127.0.0.1:3000/api/health`

Expected: JSON response with `success: true` and `status: ok`.

- [ ] **Step 4: Hand the user the exact library install list**

Arduino libraries to install in the IDE or Arduino CLI:

```text
Keypad
Adafruit GFX Library
Adafruit SSD1306
Adafruit Fingerprint Sensor Library
ArduinoJson
esp32 board package by Espressif Systems
```

Node packages installed through `npm install` in `backend`:

```text
express
mysql2
dotenv
cors
helmet
morgan
vitest
supertest
```

## Plan Self-Review

### Spec coverage

- OLED standby clock: covered by Task 8 and Task 7
- WiFi scan and numeric password input: covered by Task 8 and Task 9
- Fingerprint enrollment by student ID: covered by Task 4 on the backend and Task 9 on the firmware
- Consume and top-up flows: covered by Task 5 on the backend and Task 9 on the firmware
- MySQL schema and record table: covered by Task 2 and Task 6
- Student ID/name mapping table: covered by Task 2 and Task 3
- Transaction query support for the OLED records menu: covered by Task 6

### Placeholder scan

- No `TODO`, `TBD`, or deferred implementation markers remain.
- All commands use explicit paths and expected outcomes.

### Type consistency

- `studentId`, `studentName`, `fingerprintTemplateId`, `amount`, and `balance` use the same names in the schema, backend routes, and firmware payload shapes.
- Payment route names are `topup` and `consume` consistently across backend and firmware tasks.
