const express = require('express');

const { asyncHandler } = require('../utils/asyncHandler');

function createStudentRouter({ studentService }) {
  const router = express.Router();

  router.get(
    '/lookup/:studentId',
    asyncHandler(async (request, response) => {
      const result = await studentService.lookupStudent(request.params.studentId);

      response.json({
        success: true,
        data: result
      });
    })
  );

  return router;
}

module.exports = {
  createStudentRouter
};