const mysql = require('mysql2/promise');

function createPool(dbConfig) {
  return mysql.createPool({
    host: dbConfig.host,
    port: dbConfig.port,
    database: dbConfig.database,
    user: dbConfig.user,
    password: dbConfig.password,
    waitForConnections: true,
    connectionLimit: dbConfig.connectionLimit,
    queueLimit: 0,
    timezone: 'Z',
    dateStrings: true
  });
}

async function runInTransaction(pool, handler) {
  const connection = await pool.getConnection();

  try {
    await connection.beginTransaction();
    const result = await handler(connection);
    await connection.commit();
    return result;
  } catch (error) {
    await connection.rollback();
    throw error;
  } finally {
    connection.release();
  }
}

async function pingDatabase(pool) {
  try {
    await pool.query('SELECT 1');

    return {
      status: 'up',
      message: null
    };
  } catch (error) {
    return {
      status: 'down',
      message: error.message
    };
  }
}

module.exports = {
  createPool,
  pingDatabase,
  runInTransaction
};
