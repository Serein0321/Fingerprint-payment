const { centsToAmountString, decimalValueToCents } = require('../utils/amount');
const { HttpError } = require('../utils/httpError');
const { requireNonEmptyString } = require('../utils/validation');

function createStudentService({ pool, studentRepository, accountRepository }) {
  return {
    async lookupStudent(studentIdInput) {
      const studentId = requireNonEmptyString(studentIdInput, 'studentId');
      const student = await studentRepository.findByStudentId(pool, studentId);

      if (!student) {
        throw new HttpError(404, 'STUDENT_NOT_FOUND', '学号不存在');
      }

      await accountRepository.ensureAccount(pool, studentId);
      const refreshedStudent = await studentRepository.findByStudentId(pool, studentId);

      return {
        studentId: refreshedStudent.studentId,
        studentName: refreshedStudent.studentName,
        balance: centsToAmountString(decimalValueToCents(refreshedStudent.balance))
      };
    }
  };
}

module.exports = {
  createStudentService
};
