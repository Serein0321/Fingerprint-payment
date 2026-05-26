const { HttpError } = require('./httpError');

function requireNonEmptyString(value, fieldName) {
  const text = String(value === undefined || value === null ? '' : value).trim();

  if (!text) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName} 不能为空`);
  }

  return text;
}

function requirePositiveInteger(value, fieldName) {
  const parsed = Number.parseInt(String(value), 10);

  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName} 必须是正整数`);
  }

  return parsed;
}

function parseOptionalTimestamp(value, fieldName) {
  if (value === undefined || value === null || value === '') {
    return new Date();
  }

  const parsed = new Date(value);
  if (Number.isNaN(parsed.getTime())) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName} 不是合法时间`);
  }

  return parsed;
}

function requireLimit(value, defaultValue, maxValue) {
  if (value === undefined || value === null || value === '') {
    return defaultValue;
  }

  const parsed = Number.parseInt(String(value), 10);
  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', 'limit 必须是正整数');
  }

  return Math.min(parsed, maxValue);
}

module.exports = {
  parseOptionalTimestamp,
  requireLimit,
  requireNonEmptyString,
  requirePositiveInteger
};
