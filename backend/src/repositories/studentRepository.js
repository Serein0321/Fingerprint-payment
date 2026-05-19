function createStudentRepository() {
  return {
    async findByStudentId(executor, studentId) {
      const [rows] = await executor.execute(
        `
          SELECT
            student_id AS studentId,
            student_name AS studentName,
            created_at AS createdAt,
            updated_at AS updatedAt
          FROM student_name_map
          WHERE student_id = ?
          LIMIT 1
        `,
        [studentId]
      );

      return rows[0] || null;
    }
  };
}

module.exports = {
  createStudentRepository
};