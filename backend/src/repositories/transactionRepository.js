function createTransactionRepository() {
  return {
    async create(executor, transaction) {
      await executor.execute(
        `
          INSERT INTO transaction_records (
            student_id,
            student_name,
            fingerprint_template_id,
            transaction_type,
            amount,
            balance_before,
            balance_after,
            device_id,
            result,
            message,
            occurred_at
          )
          VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        `,
        [
          transaction.studentId,
          transaction.studentName,
          transaction.fingerprintTemplateId,
          transaction.transactionType,
          transaction.amount,
          transaction.balanceBefore,
          transaction.balanceAfter,
          transaction.deviceId,
          transaction.result,
          transaction.message,
          transaction.occurredAt
        ]
      );
    },

    async listRecentByStudentId(executor, studentId, limit) {
      const [rows] = await executor.execute(
        `
          SELECT
            id,
            student_id AS studentId,
            student_name AS studentName,
            fingerprint_template_id AS fingerprintTemplateId,
            transaction_type AS transactionType,
            amount,
            balance_before AS balanceBefore,
            balance_after AS balanceAfter,
            device_id AS deviceId,
            result,
            message,
            occurred_at AS occurredAt,
            created_at AS createdAt
          FROM transaction_records
          WHERE student_id = ?
          ORDER BY occurred_at DESC, id DESC
          LIMIT ?
        `,
        [studentId, limit]
      );

      return rows;
    }
  };
}

module.exports = {
  createTransactionRepository
};