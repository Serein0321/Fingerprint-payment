const { HttpError } = require('./httpError');

function requireNonEmptyString(value, fieldName) {
  if (typeof value !== 'string' || value.trim() === '') {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName}不能为空`);
  }

  return value.trim();
}

function requirePositiveInteger(value, fieldName) {
  const parsed = Number(value);

  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName}必须是大于0的整数`);
  }

  return parsed;
}

function parseOptionalTimestamp(value, fieldName) {
  if (value === undefined || value === null || value === '') {
    return new Date();
  }

  const parsed = new Date(value);

  if (Number.isNaN(parsed.getTime())) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName}必须是有效时间`);
  }

  return parsed;
}

function requireLimit(value, fallback, max) {
  if (value === undefined || value === null || value === '') {
    return fallback;
  }

  const parsed = Number.parseInt(value, 10);

  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', 'limit必须是大于0的整数');
  }

  return Math.min(parsed, max);
}

module.exports = {
  parseOptionalTimestamp,
  requireLimit,
  requireNonEmptyString,
  requirePositiveInteger
};