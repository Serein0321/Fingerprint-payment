const express = require('express');

const { asyncHandler } = require('../utils/asyncHandler');

function createPaymentRouter({ paymentService }) {
  const router = express.Router();

  router.post(
    '/topup',
    asyncHandler(async (request, response) => {
      const result = await paymentService.topup(request.body || {});

      response.json({
        success: true,
        data: result
      });
    })
  );

  router.post(
    '/consume',
    asyncHandler(async (request, response) => {
      const result = await paymentService.consume(request.body || {});

      response.json({
        success: true,
        data: result
      });
    })
  );

  return router;
}

module.exports = {
  createPaymentRouter
};