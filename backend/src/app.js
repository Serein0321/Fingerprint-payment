const cors = require('cors');
const express = require('express');
const helmet = require('helmet');
const morgan = require('morgan');

const { errorHandler, notFoundHandler } = require('./middleware/errorHandler');
const { createFingerprintRouter } = require('./routes/fingerprintRoutes');
const { createHealthRouter } = require('./routes/healthRoutes');
const { createPaymentRouter } = require('./routes/paymentRoutes');
const { createStudentRouter } = require('./routes/studentRoutes');
const { createTransactionRouter } = require('./routes/transactionRoutes');

function createApp(services) {
  if (!services) {
    throw new Error('services are required to create the app');
  }

  const app = express();

  app.use(helmet());
  app.use(cors());
  app.use(express.json());
  app.use(morgan('dev'));

  app.use('/api/health', createHealthRouter({ healthService: services.healthService }));
  app.use('/api/students', createStudentRouter({ studentService: services.studentService }));
  app.use('/api/fingerprints', createFingerprintRouter({ fingerprintService: services.fingerprintService }));
  app.use('/api/payments', createPaymentRouter({ paymentService: services.paymentService }));
  app.use('/api/transactions', createTransactionRouter({ transactionService: services.transactionService }));

  app.use(notFoundHandler);
  app.use(errorHandler);

  return app;
}

module.exports = {
  createApp
};