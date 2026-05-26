const express = require('express');

const { asyncHandler } = require('../utils/asyncHandler');

function createFingerprintRouter({ fingerprintService }) {
  const router = express.Router();

  router.post(
    '/enroll',
    asyncHandler(async (request, response) => {
      const result = await fingerprintService.enroll(request.body || {});

      response.status(201).json({
        success: true,
        data: result
      });
    })
  );

  return router;
}

module.exports = {
  createFingerprintRouter
};
