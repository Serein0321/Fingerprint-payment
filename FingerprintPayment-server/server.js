const { createApp } = require('./app');
const { loadEnv } = require('./config/env');
const { createPool } = require('./config/db');
const { createAccountRepository } = require('./repositories/accountRepository');
const { createFingerprintRepository } = require('./repositories/fingerprintRepository');
const { createStudentRepository } = require('./repositories/studentRepository');
const { createTransactionRepository } = require('./repositories/transactionRepository');
const { createFingerprintService } = require('./services/fingerprintService');
const { createHealthService } = require('./services/healthService');
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
    healthService: createHealthService({ pool }),
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
    console.log(`Fingerprint payment server listening on port ${env.port}`);
  });

  const shutdown = async (signal) => {
    console.log(`${signal} received, shutting down server`);

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
  console.error('Failed to start server', error);
  process.exit(1);
});
