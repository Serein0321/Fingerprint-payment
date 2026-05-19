const { HttpError } = require('../utils/httpError');
const { centsToAmountString, decimalValueToCents } = require('../utils/amount');
const { requireNonEmptyString } = require('../utils/validation');

function createStudentService({ pool, studentRepository, accountRepository }) {
  return {
    async lookupStudent(studentIdInput) {
      const studentId = requireNonEmptyString(studentIdInput, 'studentId');
      const student = await studentRepository.findByStudentId(pool, studentId);

      if (!student) {
        throw new HttpError(404, 'STUDENT_NOT_FOUND', '学号不存在');
      }

      const account = await accountRepository.findByStudentId(pool, studentId);

      return {
        studentId: student.studentId,
        studentName: student.studentName,
        hasAccount: Boolean(account),
        balance: centsToAmountString(account ? decimalValueToCents(account.balance) : 0)
      };
    }
  };
}

module.exports = {
  createStudentService
};