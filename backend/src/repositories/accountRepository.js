function createAccountRepository() {
  return {
    async ensureAccount(executor, studentId) {
      await executor.execute(
        `
          INSERT INTO student_accounts (student_id, balance, status)
          VALUES (?, 0.00, 'active')
          ON DUPLICATE KEY UPDATE student_id = student_id
        `,
        [studentId]
      );
    },

    async findByStudentId(executor, studentId) {
      const [rows] = await executor.execute(
        `
          SELECT
            student_id AS studentId,
            balance,
            status,
            created_at AS createdAt,
            updated_at AS updatedAt
          FROM student_accounts
          WHERE student_id = ?
          LIMIT 1
        `,
        [studentId]
      );

      return rows[0] || null;
    },

    async findByStudentIdForUpdate(executor, studentId) {
      const [rows] = await executor.execute(
        `
          SELECT
            student_id AS studentId,
            balance,
            status,
            created_at AS createdAt,
            updated_at AS updatedAt
          FROM student_accounts
          WHERE student_id = ?
          LIMIT 1
          FOR UPDATE
        `,
        [studentId]
      );

      return rows[0] || null;
    },

    async updateBalance(executor, studentId, balance) {
      await executor.execute(
        `
          UPDATE student_accounts
          SET balance = ?, updated_at = CURRENT_TIMESTAMP
          WHERE student_id = ?
        `,
        [balance, studentId]
      );
    }
  };
}

module.exports = {
  createAccountRepository
};