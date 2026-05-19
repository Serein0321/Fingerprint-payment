const express = require('express');

const { asyncHandler } = require('../utils/asyncHandler');

function createHealthRouter({ healthService }) {
  const router = express.Router();

  router.get(
    '/',
    asyncHandler(async (_request, response) => {
      const status = await healthService.getStatus();

      response.json({
        success: true,
        data: status
      });
    })
  );

  return router;
}

module.exports = {
  createHealthRouter
};