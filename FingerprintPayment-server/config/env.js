const fs = require('fs');
const path = require('path');
const dotenv = require('dotenv');

let cachedEnv;
let loadedEnvPath = null;

function loadDotenvConfig() {
  const projectRoot = path.resolve(__dirname, '..');
  const envPath = path.join(projectRoot, '.env');
  const exampleEnvPath = path.join(projectRoot, '.env.example');

  if (fs.existsSync(envPath)) {
    loadedEnvPath = envPath;
    dotenv.config({ path: envPath });
    return;
  }

  if (fs.existsSync(exampleEnvPath)) {
    loadedEnvPath = exampleEnvPath;
    dotenv.config({ path: exampleEnvPath });
    return;
  }

  loadedEnvPath = null;
  dotenv.config();
}

function requireEnv(name) {
  const value = process.env[name];

  if (value === undefined || value === null || value === '') {
    const sourceHint = loadedEnvPath
      ? `Loaded config file: ${loadedEnvPath}`
      : 'No .env or .env.example file was found';
    throw new Error(`Missing required environment variable: ${name}. ${sourceHint}`);
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

  loadDotenvConfig();

  cachedEnv = {
    port: parseInteger(process.env.PORT, 3004),
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
