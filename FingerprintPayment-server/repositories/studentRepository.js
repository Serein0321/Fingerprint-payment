function createStudentRepository() {
  return {
    async findByStudentId(executor, studentId) {
      const [rows] = await executor.query(
        `SELECT snm.student_id AS studentId,
                snm.student_name AS studentName,
                COALESCE(sa.balance, 0.00) AS balance
           FROM student_name_map snm
           LEFT JOIN student_accounts sa ON sa.student_id = snm.student_id
          WHERE snm.student_id = ?
          LIMIT 1`,
        [studentId]
      );

      return rows[0] || null;
    }
  };
}

module.exports = {
  createStudentRepository
};
