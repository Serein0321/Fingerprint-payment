const { pingDatabase } = require('../config/db');

function createHealthService({ pool }) {
  return {
    async getStatus() {
      const database = await pingDatabase(pool);

      return {
        serviceStatus: database.status === 'up' ? 'ok' : 'degraded',
        databaseStatus: database.status,
        time: new Date().toISOString(),
        message: database.message || null
      };
    }
  };
}

module.exports = {
  createHealthService
};
