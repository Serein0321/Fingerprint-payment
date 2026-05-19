const dotenv = require('dotenv');

let cachedEnv;

function requireEnv(name) {
  const value = process.env[name];

  if (value === undefined || value === null || value === '') {
    throw new Error(`Missing required environment variable: ${name}`);
  }

  return value;
}

function parseInteger(value, fallback) {
  const parsed = Number.parseInt(value, 10);

  if (!Number.isInteger(parsed) || parsed <= 0) {
    return fallback;
  }

  return parsed;
}

function loadEnv() {
  if (cachedEnv) {
    return cachedEnv;
  }

  dotenv.config();

  cachedEnv = {
    port: parseInteger(process.env.PORT, 3000),
    db: {
      host: requireEnv('DB_HOST'),
      port: parseInteger(requireEnv('DB_PORT'), 3306),
      database: requireEnv('DB_NAME'),
      user: requireEnv('DB_USER'),
      password: requireEnv('DB_PASSWORD'),
      connectionLimit: parseInteger(process.env.DB_CONNECTION_LIMIT, 10)
    }
  };

  return cachedEnv;
}

module.exports = {
  loadEnv
};