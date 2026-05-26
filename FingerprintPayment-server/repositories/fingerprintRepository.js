function createFingerprintRepository() {
  return {
    async findByStudentId(executor, studentId) {
      const [rows] = await executor.query(
        `SELECT student_id AS studentId,
                fingerprint_template_id AS fingerprintTemplateId,
                device_id AS deviceId,
                status,
                enrolled_at AS enrolledAt
           FROM fingerprint_bindings
          WHERE student_id = ?
          LIMIT 1`,
        [studentId]
      );

      return rows[0] || null;
    },

    async findByTemplateId(executor, fingerprintTemplateId) {
      const [rows] = await executor.query(
        `SELECT student_id AS studentId,
                fingerprint_template_id AS fingerprintTemplateId,
                device_id AS deviceId,
                status,
                enrolled_at AS enrolledAt
           FROM fingerprint_bindings
          WHERE fingerprint_template_id = ?
          LIMIT 1`,
        [fingerprintTemplateId]
      );

      return rows[0] || null;
    },

    async create(connection, payload) {
      await connection.query(
        `INSERT INTO fingerprint_bindings (
           student_id,
           fingerprint_template_id,
           device_id,
           status,
           enrolled_at
         ) VALUES (?, ?, ?, 'active', ?)`,
        [payload.studentId, payload.fingerprintTemplateId, payload.deviceId, payload.enrolledAt]
      );
    }
  };
}

module.exports = {
  createFingerprintRepository
};
