const { HttpError } = require('./httpError');

function decimalValueToCents(value) {
  const parsed = Number(value || 0);

  if (!Number.isFinite(parsed)) {
    return 0;
  }

  return Math.round((parsed + Number.EPSILON) * 100);
}

function parsePositiveAmountToCents(value, fieldName) {
  const parsed = Number(value);

  if (!Number.isFinite(parsed) || parsed <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName}必须是大于0的数字`);
  }

  const cents = Math.round((parsed + Number.EPSILON) * 100);

  if (cents <= 0) {
    throw new HttpError(400, 'VALIDATION_ERROR', `${fieldName}必须是大于0的数字`);
  }

  return cents;
}

function centsToAmountString(cents) {
  return (cents / 100).toFixed(2);
}

module.exports = {
  centsToAmountString,
  decimalValueToCents,
  parsePositiveAmountToCents
};