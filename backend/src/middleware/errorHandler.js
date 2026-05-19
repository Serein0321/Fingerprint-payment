function buildErrorPayload(code, message, details) {
  const payload = {
    success: false,
    error: {
      code,
      message
    }
  };

  if (details !== undefined) {
    payload.error.details = details;
  }

  return payload;
}

function notFoundHandler(_request, response) {
  response.status(404).json(buildErrorPayload('NOT_FOUND', '请求的资源不存在'));
}

function errorHandler(error, _request, response, _next) {
  const statusCode = Number.isInteger(error.statusCode)
    ? error.statusCode
    : Number.isInteger(error.status)
      ? error.status
      : 500;
  const code =
    error.code || (error.type === 'entity.parse.failed' ? 'INVALID_JSON' : 'INTERNAL_SERVER_ERROR');
  const clientMessage = error.type === 'entity.parse.failed' ? '请求体不是合法JSON' : error.message;
  const message = statusCode >= 500 ? '服务器内部错误' : clientMessage;

  response.status(statusCode).json(buildErrorPayload(code, message, error.details));
}

module.exports = {
  errorHandler,
  notFoundHandler
};