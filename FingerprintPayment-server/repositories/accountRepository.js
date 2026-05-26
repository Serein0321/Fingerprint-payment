function createAccountRepository() {
  return {
    async ensureAccount(executor, studentId) {
      await executor.query(
        `INSERT INTO student_accounts (student_id, balance, status)
         VALUES (?, 0.00, 'active')
         ON DUPLICATE KEY UPDATE student_id = VALUES(student_id)`,
        [studentId]
      );
    },

    async findByStudentIdForUpdate(connection, studentId) {
      const [rows] = await connection.query(
        `SELECT student_id AS studentId, balance, status
           FROM student_accounts
          WHERE student_id = ?
          FOR UPDATE`,
        [studentId]
      );

      return rows[0] || null;
    },

    async updateBalance(connection, studentId, balance) {
      await connection.query(
        `UPDATE student_accounts
            SET balance = ?, updated_at = CURRENT_TIMESTAMP
          WHERE student_id = ?`,
        [balance, studentId]
      );
    }
  };
}

module.exports = {
  createAccountRepository
};
