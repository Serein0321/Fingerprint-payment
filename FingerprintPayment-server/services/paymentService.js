const { runInTransaction } = require('../config/db');
const {
  centsToAmountString,
  decimalValueToCents,
  parsePositiveAmountToCents
} = require('../utils/amount');
const { HttpError } = require('../utils/httpError');
const {
  parseOptionalTimestamp,
  requireNonEmptyString,
  requirePositiveInteger
} = require('../utils/validation');

async function resolvePaymentContext({
  connection,
  fingerprintTemplateId,
  studentRepository,
  accountRepository,
  fingerprintRepository
}) {
  const binding = await fingerprintRepository.findByTemplateId(connection, fingerprintTemplateId);

  if (!binding) {
    throw new HttpError(404, 'FINGERPRINT_NOT_FOUND', '未找到对应的指纹绑定');
  }

  const student = await studentRepository.findByStudentId(connection, binding.studentId);
  if (!student) {
    throw new HttpError(404, 'STUDENT_NOT_FOUND', '该指纹绑定的学号不存在');
  }

  await accountRepository.ensureAccount(connection, student.studentId);
  const account = await accountRepository.findByStudentIdForUpdate(connection, student.studentId);

  if (!account) {
    throw new HttpError(500, 'ACCOUNT_NOT_FOUND', '账户初始化失败');
  }

  return {
    binding,
    student,
    account
  };
}

function createPaymentService({
  pool,
  studentRepository,
  accountRepository,
  fingerprintRepository,
  transactionRepository
}) {
  async function topup(payload) {
    const fingerprintTemplateId = requirePositiveInteger(
      payload.fingerprintTemplateId,
      'fingerprintTemplateId'
    );
    const amountCents = parsePositiveAmountToCents(payload.amount, 'amount');
    const deviceId = requireNonEmptyString(payload.deviceId, 'deviceId');
    const occurredAt = parseOptionalTimestamp(payload.occurredAt, 'occurredAt');

    return runInTransaction(pool, async (connection) => {
      const { student, account } = await resolvePaymentContext({
        connection,
        fingerprintTemplateId,
        studentRepository,
        accountRepository,
        fingerprintRepository
      });

      const balanceBeforeCents = decimalValueToCents(account.balance);
      const balanceAfterCents = balanceBeforeCents + amountCents;

      await accountRepository.updateBalance(
        connection,
        student.studentId,
        centsToAmountString(balanceAfterCents)
      );
      await transactionRepository.create(connection, {
        studentId: student.studentId,
        studentName: student.studentName,
        fingerprintTemplateId,
        transactionType: 'topup',
        amount: centsToAmountString(amountCents),
        balanceBefore: centsToAmountString(balanceBeforeCents),
        balanceAfter: centsToAmountString(balanceAfterCents),
        deviceId,
        result: 'success',
        message: '充值成功',
        occurredAt
      });

      return {
        studentId: student.studentId,
        studentName: student.studentName,
        fingerprintTemplateId,
        transactionType: 'topup',
        amount: centsToAmountString(amountCents),
        balance: centsToAmountString(balanceAfterCents),
        message: '充值成功',
        occurredAt: occurredAt.toISOString()
      };
    });
  }

  async function consume(payload) {
    const fingerprintTemplateId = requirePositiveInteger(
      payload.fingerprintTemplateId,
      'fingerprintTemplateId'
    );
    const amountCents = parsePositiveAmountToCents(payload.amount, 'amount');
    const deviceId = requireNonEmptyString(payload.deviceId, 'deviceId');
    const occurredAt = parseOptionalTimestamp(payload.occurredAt, 'occurredAt');

    const result = await runInTransaction(pool, async (connection) => {
      const { student, account } = await resolvePaymentContext({
        connection,
        fingerprintTemplateId,
        studentRepository,
        accountRepository,
        fingerprintRepository
      });

      const balanceBeforeCents = decimalValueToCents(account.balance);
      if (balanceBeforeCents < amountCents) {
        await transactionRepository.create(connection, {
          studentId: student.studentId,
          studentName: student.studentName,
          fingerprintTemplateId,
          transactionType: 'consume',
          amount: centsToAmountString(amountCents),
          balanceBefore: centsToAmountString(balanceBeforeCents),
          balanceAfter: centsToAmountString(balanceBeforeCents),
          deviceId,
          result: 'failed',
          message: '余额不足',
          occurredAt
        });

        return {
          ok: false,
          studentId: student.studentId,
          studentName: student.studentName,
          fingerprintTemplateId,
          amount: centsToAmountString(amountCents),
          balance: centsToAmountString(balanceBeforeCents)
        };
      }

      const balanceAfterCents = balanceBeforeCents - amountCents;

      await accountRepository.updateBalance(
        connection,
        student.studentId,
        centsToAmountString(balanceAfterCents)
      );
      await transactionRepository.create(connection, {
        studentId: student.studentId,
        studentName: student.studentName,
        fingerprintTemplateId,
        transactionType: 'consume',
        amount: centsToAmountString(amountCents),
        balanceBefore: centsToAmountString(balanceBeforeCents),
        balanceAfter: centsToAmountString(balanceAfterCents),
        deviceId,
        result: 'success',
        message: '消费成功',
        occurredAt
      });

      return {
        ok: true,
        studentId: student.studentId,
        studentName: student.studentName,
        fingerprintTemplateId,
        amount: centsToAmountString(amountCents),
        balance: centsToAmountString(balanceAfterCents),
        occurredAt: occurredAt.toISOString()
      };
    });

    if (!result.ok) {
      throw new HttpError(409, 'INSUFFICIENT_BALANCE', '余额不足', {
        studentId: result.studentId,
        studentName: result.studentName,
        currentBalance: result.balance,
        amount: result.amount,
        fingerprintTemplateId: result.fingerprintTemplateId
      });
    }

    return {
      studentId: result.studentId,
      studentName: result.studentName,
      fingerprintTemplateId: result.fingerprintTemplateId,
      transactionType: 'consume',
      amount: result.amount,
      balance: result.balance,
      message: '消费成功',
      occurredAt: result.occurredAt
    };
  }

  return {
    topup,
    consume
  };
}

module.exports = {
  createPaymentService
};
