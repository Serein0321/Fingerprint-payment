const { createApp } = require('./app');
const { loadEnv } = require('./config/env');
const { createPool, pingDatabase } = require('./config/db');
const { createAccountRepository } = require('./repositories/accountRepository');
const { createFingerprintRepository } = require('./repositories/fingerprintRepository');
const { createStudentRepository } = require('./repositories/studentRepository');
const { createTransactionRepository } = require('./repositories/transactionRepository');
const { createFingerprintService } = require('./services/fingerprintService');
const { createPaymentService } = require('./services/paymentService');
const { createStudentService } = require('./services/studentService');
const { createTransactionService } = require('./services/transactionService');

async function startServer() {
  const env = loadEnv();
  const pool = createPool(env.db);

  const studentRepository = createStudentRepository();
  const accountRepository = createAccountRepository();
  const fingerprintRepository = createFingerprintRepository();
  const transactionRepository = createTransactionRepository();

  const services = {
    healthService: {
      async getStatus() {
        const database = await pingDatabase(pool);

        return {
          serviceStatus: database.status === 'up' ? 'ok' : 'degraded',
          databaseStatus: database.status,
          time: new Date().toISOString(),
          message: database.message || null
        };
      }
    },
    studentService: createStudentService({
      pool,
      studentRepository,
      accountRepository
    }),
    fingerprintService: createFingerprintService({
      pool,
      studentRepository,
      accountRepository,
      fingerprintRepository
    }),
    paymentService: createPaymentService({
      pool,
      studentRepository,
      accountRepository,
      fingerprintRepository,
      transactionRepository
    }),
    transactionService: createTransactionService({
      pool,
      studentRepository,
      transactionRepository
    })
  };

  const app = createApp(services);
  const server = app.listen(env.port, () => {
    console.log(`ESP32 fingerprint payment backend listening on port ${env.port}`);
  });

  const shutdown = async (signal) => {
    console.log(`${signal} received, shutting down backend server`);

    server.close(async () => {
      await pool.end();
      process.exit(0);
    });
  };

  process.on('SIGINT', () => {
    void shutdown('SIGINT');
  });

  process.on('SIGTERM', () => {
    void shutdown('SIGTERM');
  });
}

startServer().catch((error) => {
  console.error('Failed to start backend server', error);
  process.exit(1);
});