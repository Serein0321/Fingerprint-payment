const express = require('express');

const { asyncHandler } = require('../utils/asyncHandler');

function createTransactionRouter({ transactionService }) {
  const router = express.Router();

  router.get(
    '/',
    asyncHandler(async (request, response) => {
      const result = await transactionService.listRecent({
        studentId: request.query.studentId,
        limit: request.query.limit
      });

      response.json({
        success: true,
        data: result
      });
    })
  );

  return router;
}

module.exports = {
  createTransactionRouter
};