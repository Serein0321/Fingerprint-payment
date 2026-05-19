function createFingerprintRepository() {
  return {
    async findByTemplateId(executor, fingerprintTemplateId) {
      const [rows] = await executor.execute(
        `
          SELECT
            student_id AS studentId,
            fingerprint_template_id AS fingerprintTemplateId,
            device_id AS deviceId,
            status,
            enrolled_at AS enrolledAt,
            created_at AS createdAt,
            updated_at AS updatedAt
          FROM fingerprint_bindings
          WHERE fingerprint_template_id = ? AND status = 'active'
          LIMIT 1
        `,
        [fingerprintTemplateId]
      );

      return rows[0] || null;
    },

    async findByStudentId(executor, studentId) {
      const [rows] = await executor.execute(
        `
          SELECT
            student_id AS studentId,
            fingerprint_template_id AS fingerprintTemplateId,
            device_id AS deviceId,
            status,
            enrolled_at AS enrolledAt,
            created_at AS createdAt,
            updated_at AS updatedAt
          FROM fingerprint_bindings
          WHERE student_id = ? AND status = 'active'
          LIMIT 1
        `,
        [studentId]
      );

      return rows[0] || null;
    },

    async create(executor, binding) {
      await executor.execute(
        `
          INSERT INTO fingerprint_bindings (
            student_id,
            fingerprint_template_id,
            device_id,
            status,
            enrolled_at
          )
          VALUES (?, ?, ?, 'active', ?)
        `,
        [
          binding.studentId,
          binding.fingerprintTemplateId,
          binding.deviceId,
          binding.enrolledAt
        ]
      );
    }
  };
}

module.exports = {
  createFingerprintRepository
};