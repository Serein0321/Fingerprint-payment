const { runInTransaction } = require('../config/db');
const { centsToAmountString, decimalValueToCents } = require('../utils/amount');
const { HttpError } = require('../utils/httpError');
const {
  parseOptionalTimestamp,
  requireNonEmptyString,
  requirePositiveInteger
} = require('../utils/validation');

function createFingerprintService({ pool, studentRepository, accountRepository, fingerprintRepository }) {
  return {
    async enroll(payload) {
      const studentId = requireNonEmptyString(payload.studentId, 'studentId');
      const fingerprintTemplateId = requirePositiveInteger(
        payload.fingerprintTemplateId,
        'fingerprintTemplateId'
      );
      const deviceId = requireNonEmptyString(payload.deviceId, 'deviceId');
      const enrolledAt = parseOptionalTimestamp(payload.enrolledAt, 'enrolledAt');

      return runInTransaction(pool, async (connection) => {
        const student = await studentRepository.findByStudentId(connection, studentId);
        if (!student) {
          throw new HttpError(404, 'STUDENT_NOT_FOUND', '学号不存在');
        }

        const existingStudentBinding = await fingerprintRepository.findByStudentId(connection, studentId);
        if (existingStudentBinding) {
          throw new HttpError(409, 'STUDENT_ALREADY_BOUND', '该学号已经绑定指纹');
        }

        const existingTemplateBinding = await fingerprintRepository.findByTemplateId(
          connection,
          fingerprintTemplateId
        );
        if (existingTemplateBinding) {
          throw new HttpError(409, 'FINGERPRINT_TEMPLATE_IN_USE', '该指纹模板已被占用');
        }

        await accountRepository.ensureAccount(connection, studentId);
        await fingerprintRepository.create(connection, {
          studentId,
          fingerprintTemplateId,
          deviceId,
          enrolledAt
        });

        const account = await accountRepository.findByStudentIdForUpdate(connection, studentId);

        return {
          studentId: student.studentId,
          studentName: student.studentName,
          fingerprintTemplateId,
          balance: centsToAmountString(decimalValueToCents(account ? account.balance : 0)),
          message: '指纹录入成功'
        };
      });
    }
  };
}

module.exports = {
  createFingerprintService
};
