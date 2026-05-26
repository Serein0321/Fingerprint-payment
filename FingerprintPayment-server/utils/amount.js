const { HttpError } = require('./httpError');

function parsePositiveAmountToCents(value, fieldName) {
  const text = String(value === undefined || value === null ? '' : value).trim();

  if (!/^\d+(\.\d{1,2})?$/.test(text)) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName} 必须是最多两位小数的正数`);
  }

  const numericValue = Number(text);
  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName} 必须大于 0`);
  }

  return Math.round(numericValue * 100);
}

function centsToAmountString(cents) {
  return (cents / 100).toFixed(2);
}

function decimalValueToCents(value) {
  return Math.round(Number(value || 0) * 100);
}

module.exports = {
  centsToAmountString,
  decimalValueToCents,
  parsePositiveAmountToCents
};
