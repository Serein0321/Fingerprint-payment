const { HttpError } = require('../utils/httpError');
const { requireLimit, requireNonEmptyString } = require('../utils/validation');

function createTransactionService({ pool, studentRepository, transactionRepository }) {
  return {
    async listRecent({ studentId: studentIdInput, limit: limitInput }) {
      const studentId = requireNonEmptyString(studentIdInput, 'studentId');
      const limit = requireLimit(limitInput, 20, 100);
      const student = await studentRepository.findByStudentId(pool, studentId);

      if (!student) {
        throw new HttpError(404, 'STUDENT_NOT_FOUND', '学号不存在');
      }

      const transactions = await transactionRepository.listRecentByStudentId(pool, studentId, limit);

      return {
        studentId: student.studentId,
        studentName: student.studentName,
        limit,
        transactions
      };
    }
  };
}

module.exports = {
  createTransactionService
};