const { runInTransaction } = require('../config/db');
const { HttpError } = require('../utils/httpError');
const { centsToAmountString, decimalValueToCents } = require('../utils/amount');
const {
  parseOptionalTimestamp,
  requireNonEmptyString,
  requirePositiveInteger
} = require('../utils/validation');

function createFingerprintService({
  pool,
  studentRepository,
  accountRepository,
  fingerprintRepository
}) {
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

        const occupiedTemplate = await fingerprintRepository.findByTemplateId(
          connection,
          fingerprintTemplateId
        );

        if (occupiedTemplate) {
          throw new HttpError(409, 'FINGERPRINT_TEMPLATE_IN_USE', '指纹模板已被占用');
        }

        const existingBinding = await fingerprintRepository.findByStudentId(connection, studentId);

        if (existingBinding) {
          throw new HttpError(409, 'STUDENT_ALREADY_BOUND', '该学号已绑定指纹');
        }

        await accountRepository.ensureAccount(connection, studentId);
        await fingerprintRepository.create(connection, {
          studentId,
          fingerprintTemplateId,
          deviceId,
          enrolledAt
        });

        const account = await accountRepository.findByStudentId(connection, studentId);

        if (!account) {
          throw new HttpError(500, 'ACCOUNT_NOT_FOUND', '学生账户初始化失败');
        }

        return {
          studentId,
          studentName: student.studentName,
          fingerprintTemplateId,
          deviceId,
          enrolledAt: enrolledAt.toISOString(),
          balance: centsToAmountString(decimalValueToCents(account.balance))
        };
      });
    }
  };
}

module.exports = {
  createFingerprintService
};