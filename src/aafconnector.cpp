/*
 * Copyright 2026 NXP
 *
 * NXP Proprietary. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and that
 * you agree to comply with and are bound by, such license terms.  If you do
 * not agree to be bound by the applicable license terms, then you may not
 * retain, install, activate or otherwise use the software.
 *
 */

#include "aafconnector.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QNetworkRequest>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(lcConnector, "aaf.connector")

namespace {
// Configuration constants
constexpr int MODEL_STATUS_POLL_INTERVAL_MS = 2000;
constexpr int METRICS_REQUEST_DELAY_MS = 500;
constexpr int DEFAULT_MAX_TOKENS = 1000;
constexpr double DEFAULT_TEMPERATURE = 0.7;
constexpr int DEFAULT_MAX_PROMPT_SIZE = 2048;

// API endpoints
constexpr const char *ENDPOINT_CHAT_COMPLETIONS = "/v1/chat/completions";
constexpr const char *ENDPOINT_MODELS = "/v1/models";
constexpr const char *ENDPOINT_METRICS = "/metrics/";

// JSON keys
constexpr const char *JSON_KEY_MODEL = "model";
constexpr const char *JSON_KEY_STREAM = "stream";
constexpr const char *JSON_KEY_MESSAGES = "messages";
constexpr const char *JSON_KEY_ROLE = "role";
constexpr const char *JSON_KEY_CONTENT = "content";
constexpr const char *JSON_KEY_CHOICES = "choices";
constexpr const char *JSON_KEY_DELTA = "delta";
constexpr const char *JSON_KEY_TYPE = "type";
constexpr const char *JSON_KEY_NAME = "name";
constexpr const char *JSON_KEY_TOOL_CALLING = "tool_calling";

// Stream markers
constexpr const char *STREAM_DATA_PREFIX = "data: ";
constexpr const char *STREAM_DONE_MARKER = "[DONE]";

// Model file
constexpr const char *MODEL_DVM_FILE = "model.dvm";
}  // namespace

AAFConnector::AAFConnector(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_currentReply(nullptr),
      m_timeoutTimer(new QTimer(this)),
      m_modelStatusTimer(new QTimer(this)),
      m_serverUrl(AAFConfig::DEFAULT_SERVER_URL),
      m_timeout(AAFConfig::DEFAULT_TIMEOUT_MS),
      m_isConnected(false),
      m_isProcessing(false) {
  // Configure timeout timer
  m_timeoutTimer->setSingleShot(true);
  connect(m_timeoutTimer, &QTimer::timeout, this,
          &AAFConnector::onRequestTimeout);

  // Configure model status polling timer
  m_modelStatusTimer->setInterval(MODEL_STATUS_POLL_INTERVAL_MS);
  connect(m_modelStatusTimer, &QTimer::timeout, this, [this]() {
    QMutexLocker locker(&m_mutex);
    if (!m_loadingModelName.isEmpty()) {
      locker.unlock();
      checkModelStatus(m_loadingModelName);
    }
  });

  qCDebug(lcConnector) << "AAFConnector initialized with server:"
                       << m_serverUrl;
}

AAFConnector::~AAFConnector() {
  stopModelStatusPolling();
  cleanupCurrentReply();
  qCDebug(lcConnector) << "AAFConnector destroyed";
}

// ============================================================================
// Public API - Configuration
// ============================================================================

void AAFConnector::setServerUrl(const QString &url) {
  QMutexLocker locker(&m_mutex);
  if (url.isEmpty()) {
    qCWarning(lcConnector) << "Attempted to set empty server URL";
    return;
  }
  m_serverUrl = url;
  qCDebug(lcConnector) << "Server URL set to:" << m_serverUrl;
}

void AAFConnector::setTimeout(int timeoutMs) {
  QMutexLocker locker(&m_mutex);
  if (timeoutMs <= 0) {
    qCWarning(lcConnector) << "Invalid timeout value:" << timeoutMs;
    return;
  }
  m_timeout = timeoutMs;
  qCDebug(lcConnector) << "Timeout set to:" << m_timeout << "ms";
}

// ============================================================================
// Public API - Connection Management
// ============================================================================

void AAFConnector::connectToServer() {
  qCDebug(lcConnector) << "Connecting to server:" << getServerUrl();
  loadModelsFromConfig(AAFConfig::CONFIG_PATH);
}

void AAFConnector::disconnectFromServer() {
  cleanupCurrentReply();
  setConnectionState(false);
  qCDebug(lcConnector) << "Disconnected from server";
}

// ============================================================================
// Public API - Prompt Submission
// ============================================================================

void AAFConnector::sendTextPrompt(const QString &prompt) {
  if (!validatePromptRequest(prompt)) {
    return;
  }

  QJsonObject payload = createTextPayload(prompt);
  sendRequest(payload);

  qCDebug(lcConnector) << "Text prompt sent:" << prompt.left(50) + "...";
}

void AAFConnector::sendVisionPrompt(const QString &prompt,
                                    const QString &mediaPath) {
  if (!validateVisionRequest(prompt, mediaPath)) {
    return;
  }

  QJsonObject payload = createVideoPayload(prompt, mediaPath);
  sendRequest(payload);

  qCDebug(lcConnector) << "Vision prompt sent with media:" << mediaPath;
}

void AAFConnector::cancelRequest() {
  cleanupCurrentReply();
  setProcessingState(false);
  m_timeoutTimer->stop();
  qCDebug(lcConnector) << "Request cancelled";
}

// ============================================================================
// Public API - Model Management
// ============================================================================

void AAFConnector::loadModelsFromConfig(const QString &configPath) {
  qCDebug(lcConnector) << "Loading models from config:" << configPath;

  QList<ModelInfo> models;
  if (!parseServerConfig(configPath, models)) {
    qCWarning(lcConnector) << "Failed to parse server config";
    emit modelsListError("Failed to parse server configuration file");
    return;
  }

  QList<ModelInfo> enabledModels;
  enabledModels.reserve(models.size());  // Pre-allocate for efficiency

  for (const ModelInfo &model : qAsConst(models)) {
    if (validateModelInstallation(model.name)) {
      enabledModels.append(model);
    }
  }

  if (enabledModels.isEmpty()) {
    qCWarning(lcConnector) << "No enabled models found in config";
    emit modelsListError("No enabled models found in configuration");
  } else {
    qCDebug(lcConnector) << "Found" << enabledModels.size()
                         << "installed model(s)";
    emit modelsListReceived(enabledModels);
  }
}

void AAFConnector::setModelById(const QString &modelId) {
  if (modelId.isEmpty()) {
    qCWarning(lcConnector) << "Attempted to set empty model ID";
    emit modelOperationError("Model ID cannot be empty");
    return;
  }

  QMutexLocker locker(&m_mutex);
  if (m_currentModelId == modelId) {
    qCDebug(lcConnector) << "Model already set to:" << modelId;
    locker.unlock();
    emit modelLoadCompleted(modelId);
    return;
  }
  locker.unlock();

  qCDebug(lcConnector) << "Setting model to:" << modelId;
  loadModel(modelId);
}

void AAFConnector::loadModel(const QString &modelName,
                             const QString &toolCalling) {
  if (modelName.isEmpty()) {
    emit modelOperationError("Empty model name provided");
    return;
  }

  qCDebug(lcConnector) << "Loading model:" << modelName
                       << "with tool_calling:" << toolCalling;

  QUrl url(getServerUrl() + ENDPOINT_MODELS);
  QNetworkRequest request = createJsonRequest(url);

  QJsonObject payload;
  payload[JSON_KEY_NAME] = modelName;
  payload[JSON_KEY_TOOL_CALLING] = toolCalling;

  QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
  qCDebug(lcConnector) << "Model load request:" << jsonData;

  QNetworkReply *reply = m_networkManager->post(request, jsonData);
  connectReplySignals(reply, &AAFConnector::handleModelLoadReply);

  emit modelLoadStarted(modelName);
  startModelStatusPolling(modelName);
}

void AAFConnector::removeModel(const QString &modelName) {
  if (modelName.isEmpty()) {
    emit modelOperationError("Empty model name provided");
    return;
  }

  qCDebug(lcConnector) << "Removing model:" << modelName;

  QUrl url(getServerUrl() + ENDPOINT_MODELS + "/" + modelName);
  QNetworkRequest request = createJsonRequest(url);

  QNetworkReply *reply = m_networkManager->deleteResource(request);
  reply->setProperty("modelName", modelName);

  connectReplySignals(reply, &AAFConnector::handleModelRemoveReply);
}

// ============================================================================
// Public API - Metrics
// ============================================================================

void AAFConnector::requestMetrics(const QString &modelName) {
  QString model = modelName.isEmpty() ? getCurrentModelId() : modelName;
  qCDebug(lcConnector) << "Requesting metrics for model:" << model;

  QNetworkRequest request = createMetricsRequest(model);
  QNetworkReply *reply = m_networkManager->get(request);

  connectReplySignals(reply, &AAFConnector::handleMetricsReply);
  qCDebug(lcConnector) << "Metrics request sent to:" << request.url();
}

// ============================================================================
// Private - Configuration Parsing
// ============================================================================

bool AAFConnector::parseServerConfig(const QString &configPath,
                                     QList<ModelInfo> &models) const {
  QFile configFile(configPath);

  if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCWarning(lcConnector) << "Failed to open config file:" << configPath
                           << "Error:" << configFile.errorString();
    return false;
  }

  const QByteArray jsonData = configFile.readAll();
  configFile.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(lcConnector) << "JSON parse error:" << parseError.errorString()
                           << "at offset:" << parseError.offset;
    return false;
  }

  if (!doc.isObject()) {
    qCWarning(lcConnector) << "Config root is not a JSON object";
    return false;
  }

  const QJsonObject rootObj = doc.object();
  if (!rootObj.contains("available_models") ||
      !rootObj["available_models"].isArray()) {
    qCWarning(lcConnector) << "Config missing 'available_models' array";
    return false;
  }

  const QJsonArray modelsArray = rootObj["available_models"].toArray();
  models.reserve(modelsArray.size());  // Pre-allocate

  for (const QJsonValue &value : modelsArray) {
    if (!value.isObject()) {
      continue;
    }

    const QJsonObject modelObj = value.toObject();
    ModelInfo info = parseModelInfo(modelObj);

    // Only include text models
    if (info.type == "text") {
      qCDebug(lcConnector) << "Model:" << info.name << "Type:" << info.type;
      models.append(info);
    }
  }

  return true;
}

ModelInfo AAFConnector::parseModelInfo(const QJsonObject &modelObj) const {
  ModelInfo info;
  info.name = modelObj["name"].toString();
  info.description = modelObj["description"].toString();
  info.type = modelObj[JSON_KEY_TYPE].toString();
  info.toolCalling = modelObj[JSON_KEY_TOOL_CALLING].toString();
  info.maxPromptSize =
      modelObj.value("max_prompt_size").toInt(DEFAULT_MAX_PROMPT_SIZE);
  info.enabled = modelObj["enabled"].toBool(false);

  // Determine vision capabilities based on type
  if (info.type == "qwen_vl") {
    info.supportsVideo = true;
    info.supportsImage = true;
  } else {
    info.supportsVideo = false;
    info.supportsImage = false;
  }

  return info;
}

bool AAFConnector::validateModelInstallation(const QString &modelName) const {
  if (modelName.isEmpty()) {
    return false;
  }

  const QString modelPath =
      QStringLiteral("%1/%2").arg(AAFConfig::MODEL_PATH, modelName);
  const QDir modelDir(modelPath);

  if (!modelDir.exists()) {
    qCDebug(lcConnector) << "Model directory not found:" << modelPath;
    return false;
  }

  // Search for model.dvm file
  QDirIterator it(modelPath, QStringList() << MODEL_DVM_FILE, QDir::Files,
                  QDirIterator::Subdirectories);

  if (it.hasNext()) {
    qCDebug(lcConnector) << "Found" << MODEL_DVM_FILE << "at:" << it.next();
    return true;
  }

  qCDebug(lcConnector) << MODEL_DVM_FILE << "not found in:" << modelPath;
  return false;
}

// ============================================================================
// Private - Request Creation
// ============================================================================

QNetworkRequest AAFConnector::createJsonRequest(const QUrl &url) const {
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setHeader(QNetworkRequest::UserAgentHeader, "AAFConnector/1.0");
  request.setRawHeader("Accept", "application/json");
  return request;
}

QNetworkRequest AAFConnector::createMetricsRequest(
    const QString &modelName) const {
  QUrl url(getServerUrl() + ENDPOINT_METRICS);
  QUrlQuery query;
  query.addQueryItem("model_name", modelName);
  url.setQuery(query);

  return createJsonRequest(url);
}

QNetworkRequest AAFConnector::createModelStatusRequest(
    const QString &modelName) const {
  QUrl url(getServerUrl() + ENDPOINT_MODELS + "/" + modelName);
  return createJsonRequest(url);
}

// ============================================================================
// Private - Payload Creation
// ============================================================================

QJsonObject AAFConnector::createTextPayload(const QString &prompt) const {
  QJsonObject payload;
  payload[JSON_KEY_MODEL] = getCurrentModelId();
  payload[JSON_KEY_STREAM] = true;

  QJsonArray messages;
  QJsonObject userMsg;
  userMsg[JSON_KEY_ROLE] = "user";
  userMsg[JSON_KEY_CONTENT] = prompt;
  messages.append(userMsg);

  payload[JSON_KEY_MESSAGES] = messages;
  return payload;
}

QJsonObject AAFConnector::createVideoPayload(const QString &prompt,
                                             const QString &videoPath) const {
  QJsonObject payload;
  payload[JSON_KEY_MODEL] = getCurrentModelId();
  payload[JSON_KEY_STREAM] = true;

  QJsonArray messages;
  QJsonObject userMsg;
  userMsg[JSON_KEY_ROLE] = "user";

  // Create content array with text and video
  QJsonArray content;

  QJsonObject textPart;
  textPart[JSON_KEY_TYPE] = "text";
  textPart["text"] = prompt;
  content.append(textPart);

  QJsonObject videoPart;
  videoPart[JSON_KEY_TYPE] = "video_url";
  QJsonObject videoUrl;
  videoUrl["url"] = videoPath;
  videoPart["video_url"] = videoUrl;
  content.append(videoPart);

  userMsg[JSON_KEY_CONTENT] = content;
  messages.append(userMsg);
  payload[JSON_KEY_MESSAGES] = messages;

  return payload;
}

// ============================================================================
// Private - Request Handling
// ============================================================================

void AAFConnector::sendRequest(const QJsonObject &payload) {
  if (isProcessing()) {
    emit errorOccurred("Request already in progress");
    return;
  }

  QUrl url(getServerUrl() + ENDPOINT_CHAT_COMPLETIONS);
  QNetworkRequest request = createJsonRequest(url);
  const QByteArray jsonData =
      QJsonDocument(payload).toJson(QJsonDocument::Compact);

  qCDebug(lcConnector) << "Sending request to:" << url.toString();

  {
    QMutexLocker locker(&m_mutex);
    m_currentReply = m_networkManager->post(request, jsonData);
  }

  // Connect signals for streaming
  connect(m_currentReply, &QNetworkReply::readyRead, this,
          &AAFConnector::handleStreamingData);
  connect(m_currentReply, &QNetworkReply::finished, this,
          &AAFConnector::handleNetworkReply);
  connect(
      m_currentReply,
      QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
      this, &AAFConnector::handleNetworkError);

  setProcessingState(true);
  m_timeoutTimer->start(getTimeout());
}

void AAFConnector::handleStreamingData() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  const QByteArray data = reply->readAll();
  const QString dataStr = QString::fromUtf8(data);
  const QStringList lines = dataStr.split('\n', Qt::SkipEmptyParts);

  for (const QString &line : lines) {
    if (!line.startsWith(STREAM_DATA_PREFIX)) {
      continue;
    }

    const QString jsonStr = line.mid(strlen(STREAM_DATA_PREFIX)).trimmed();
    if (jsonStr == STREAM_DONE_MARKER) {
      qCDebug(lcConnector) << "Stream done marker received";
      continue;
    }

    processStreamingChunk(jsonStr);
  }
}

void AAFConnector::processStreamingChunk(const QString &jsonStr) {
  QJsonParseError parseError;
  const QJsonDocument doc =
      QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(lcConnector) << "Failed to parse streaming JSON:"
                           << parseError.errorString();
    return;
  }

  const QJsonObject obj = doc.object();
  if (!obj.contains(JSON_KEY_CHOICES)) {
    return;
  }

  const QJsonArray choices = obj[JSON_KEY_CHOICES].toArray();
  if (choices.isEmpty()) {
    return;
  }

  const QJsonObject firstChoice = choices[0].toObject();
  const QJsonObject delta = firstChoice[JSON_KEY_DELTA].toObject();

  if (delta.contains(JSON_KEY_CONTENT)) {
    const QString token = delta[JSON_KEY_CONTENT].toString();
    emit tokenReceived(token);
  }
}

void AAFConnector::handleNetworkReply() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  m_timeoutTimer->stop();

  if (reply->error() == QNetworkReply::NoError) {
    const QByteArray data = reply->readAll();
    const QString fullResponse = processCompleteResponse(data);

    if (!fullResponse.isEmpty()) {
      emit responseReceived(fullResponse);
    }

    // Always request metrics after completion, not just when fullResponse is
    // non-empty
    QTimer::singleShot(METRICS_REQUEST_DELAY_MS, this,
                       [this]() { requestMetrics(getCurrentModelId()); });
  }

  setProcessingState(false);

  {
    QMutexLocker locker(&m_mutex);
    if (reply == m_currentReply) {
      m_currentReply = nullptr;
    }
  }

  reply->deleteLater();
}

QString AAFConnector::processCompleteResponse(const QByteArray &data) const {
  const QString dataStr = QString::fromUtf8(data);
  const QStringList lines = dataStr.split('\n', Qt::SkipEmptyParts);

  QString fullResponse;
  fullResponse.reserve(data.size());  // Pre-allocate

  for (const QString &line : lines) {
    if (!line.startsWith(STREAM_DATA_PREFIX)) {
      continue;
    }

    const QString jsonStr = line.mid(strlen(STREAM_DATA_PREFIX)).trimmed();
    if (jsonStr == STREAM_DONE_MARKER) {
      continue;
    }

    QJsonParseError parseError;
    const QJsonDocument doc =
        QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
      continue;
    }

    const QJsonObject obj = doc.object();
    if (!obj.contains(JSON_KEY_CHOICES)) {
      continue;
    }

    const QJsonArray choices = obj[JSON_KEY_CHOICES].toArray();
    if (choices.isEmpty()) {
      continue;
    }

    const QJsonObject firstChoice = choices[0].toObject();
    const QJsonObject delta = firstChoice[JSON_KEY_DELTA].toObject();

    if (delta.contains(JSON_KEY_CONTENT)) {
      fullResponse += delta[JSON_KEY_CONTENT].toString();
    }
  }

  return fullResponse;
}

void AAFConnector::handleNetworkError(QNetworkReply::NetworkError error) {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  m_timeoutTimer->stop();
  setProcessingState(false);
  setConnectionState(false);

  const QString errorString = reply->errorString();
  qCWarning(lcConnector) << "Network error:" << error << errorString;

  emit errorOccurred("Network error: " + errorString);
}

void AAFConnector::onRequestTimeout() {
  cleanupCurrentReply();
  setProcessingState(false);

  const QString errorMsg = QStringLiteral("Request timed out after %1 seconds")
                               .arg(getTimeout() / 1000);
  emit errorOccurred(errorMsg);
}

// ============================================================================
// Private - Metrics Handling
// ============================================================================

void AAFConnector::handleMetricsReply() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  if (reply->error() != QNetworkReply::NoError) {
    qCWarning(lcConnector) << "Metrics request failed:" << reply->errorString();
    emit errorOccurred("Metrics request failed: " + reply->errorString());
    reply->deleteLater();
    return;
  }

  const QByteArray data = reply->readAll();
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(lcConnector) << "Failed to parse metrics:"
                           << parseError.errorString();
    emit errorOccurred("Failed to parse metrics: " + parseError.errorString());
    reply->deleteLater();
    return;
  }

  const QString metricsText = formatMetrics(doc.object());
  emit metricsReceived(metricsText);

  reply->deleteLater();
}
QString AAFConnector::formatMetrics(const QJsonObject &metrics) const {
  if (metrics.isEmpty()) {
    return QString();
  }

  // Metrics are nested under model name: {"model-name": {...}}
  const QString modelKey = metrics.keys().first();
  const QJsonObject modelMetrics = metrics[modelKey].toObject();

  if (modelMetrics.isEmpty()) {
    return QString();
  }

  QStringList lines;
  lines.reserve(6);

  // Time to first token (TTFT)
  if (modelMetrics.contains("llm_first_infer_duration")) {
    const double ttft = modelMetrics["llm_first_infer_duration"]
                            .toDouble();  // Already in seconds
    lines << QStringLiteral("TTFT: %1 s").arg(ttft, 0, 'f', 2);
  }

  // Tokens per second
  if (modelMetrics.contains("llm_average_token_per_second")) {
    const double tps = modelMetrics["llm_average_token_per_second"].toDouble();
    lines << QStringLiteral("TPS: %1 tok/s").arg(tps, 0, 'f', 1);
  }

  // Total generation time
  if (modelMetrics.contains("llm_token_generation_time")) {
    const double genTime = modelMetrics["llm_token_generation_time"].toDouble();
    lines << QStringLiteral("Inf. Time: %1 s").arg(genTime, 0, 'f', 1);
  }

  // Input tokens
  if (modelMetrics.contains("input_token_num")) {
    lines << QStringLiteral("Input: %1 tokens")
                 .arg(modelMetrics["input_token_num"].toInt());
  }

  // Generated tokens
  if (modelMetrics.contains("generated_token_num")) {
    lines << QStringLiteral("Output: %1 tokens")
                 .arg(modelMetrics["generated_token_num"].toInt());
  }

  return lines.join(" | ");
}
// ============================================================================
// Private - Model Management Handlers
// ============================================================================

void AAFConnector::handleModelLoadReply() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  const int statusCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  qCDebug(lcConnector) << "Model load reply, HTTP status:" << statusCode;

  if (reply->error() != QNetworkReply::NoError) {
    qCWarning(lcConnector) << "Model load failed:" << reply->errorString();
    stopModelStatusPolling();
    emit modelOperationError("Model load failed: " + reply->errorString());
    reply->deleteLater();
    return;
  }

  const QByteArray data = reply->readAll();
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(lcConnector) << "Failed to parse model load response:"
                           << parseError.errorString();
    stopModelStatusPolling();
    emit modelOperationError("Failed to parse response: " +
                             parseError.errorString());
    reply->deleteLater();
    return;
  }

  if (statusCode == 201) {
    // Model already loaded
    qCDebug(lcConnector) << "Model already loaded";
    stopModelStatusPolling();
    emit modelLoadCompleted(getCurrentModelId());
  } else if (statusCode == 202) {
    // Model loading in progress
    qCDebug(lcConnector) << "Model loading in progress";
  }

  reply->deleteLater();
}

void AAFConnector::handleModelRemoveReply() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  const QString modelName = reply->property("modelName").toString();
  const int statusCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

  qCDebug(lcConnector) << "Model remove reply for:" << modelName
                       << "status:" << statusCode;

  if (reply->error() == QNetworkReply::NoError && statusCode == 200) {
    qCDebug(lcConnector) << "Model successfully removed:" << modelName;
    {
      QMutexLocker locker(&m_mutex);
      m_currentModelId.clear();
    }
    emit modelRemoved(modelName);
  } else if (reply->error() == QNetworkReply::ContentNotFoundError) {
    qCWarning(lcConnector) << "Model not found:" << modelName;
    emit modelOperationError("Model not found: " + modelName);
  } else {
    qCWarning(lcConnector) << "Model remove failed:" << reply->errorString();
    emit modelOperationError("Model remove failed: " + reply->errorString());
  }

  reply->deleteLater();
}

// ============================================================================
// Private - Model Status Polling
// ============================================================================

void AAFConnector::startModelStatusPolling(const QString &modelName) {
  qCDebug(lcConnector) << "Starting model status polling for:" << modelName;

  {
    QMutexLocker locker(&m_mutex);
    m_loadingModelName = modelName;
  }

  m_modelStatusTimer->start();
  checkModelStatus(modelName);
}

void AAFConnector::stopModelStatusPolling() {
  qCDebug(lcConnector) << "Stopping model status polling";

  m_modelStatusTimer->stop();

  QMutexLocker locker(&m_mutex);
  m_loadingModelName.clear();
}

void AAFConnector::checkModelStatus(const QString &modelName) {
  if (modelName.isEmpty()) {
    qCWarning(lcConnector) << "Cannot check status: empty model name";
    return;
  }

  qCDebug(lcConnector) << "Checking model status for:" << modelName;

  QNetworkRequest request = createModelStatusRequest(modelName);
  QNetworkReply *reply = m_networkManager->get(request);
  reply->setProperty("modelName", modelName);

  connect(reply, &QNetworkReply::finished, this,
          &AAFConnector::handleModelStatusReply);
  connect(
      reply,
      QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
      this, [reply](QNetworkReply::NetworkError error) {
        // Ignore 404 during polling (model may not exist yet)
        if (error != QNetworkReply::ContentNotFoundError) {
          qCWarning(lcConnector) << "Model status check error:" << error;
        }
        reply->deleteLater();
      });
}

void AAFConnector::handleModelStatusReply() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    return;
  }

  const QString modelName = reply->property("modelName").toString();
  const int statusCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

  qCDebug(lcConnector) << "Model status reply for:" << modelName
                       << "HTTP status:" << statusCode;

  if (reply->error() == QNetworkReply::ContentNotFoundError) {
    qCDebug(lcConnector) << "Model not found yet (still loading):" << modelName;
    emit modelStillLoading(modelName, 0);
    reply->deleteLater();
    return;
  }

  if (reply->error() != QNetworkReply::NoError) {
    qCWarning(lcConnector) << "Model status request failed:"
                           << reply->errorString();
    reply->deleteLater();
    return;
  }

  const QByteArray data = reply->readAll();
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(lcConnector) << "Failed to parse model status:"
                           << parseError.errorString();
    reply->deleteLater();
    return;
  }

  const QJsonObject response = doc.object().value("data").toObject();
  ModelStatus status;
  status.name = modelName;
  status.id = response["id"].toString("unknown");
  status.description = response["description"].toString("unknown");
  status.ready = response["ready"].toBool(false);

  qCDebug(lcConnector) << "Model:" << status.name << "Ready:" << status.ready;

  emit modelStatusReceived(status);

  if (status.ready) {
    {
      QMutexLocker locker(&m_mutex);
      m_currentModelId = modelName;
    }
    stopModelStatusPolling();
    emit modelReady(modelName);
    emit modelLoadCompleted(modelName);
  } else {
    emit modelStillLoading(modelName, 0);
  }

  reply->deleteLater();
}

// ============================================================================
// Private - Utility Methods
// ============================================================================

void AAFConnector::setProcessingState(bool processing) {
  QMutexLocker locker(&m_mutex);
  if (m_isProcessing != processing) {
    m_isProcessing = processing;
    locker.unlock();

    if (processing) {
      emit requestStarted();
    } else {
      emit requestFinished();
    }
  }
}

void AAFConnector::setConnectionState(bool connected) {
  QMutexLocker locker(&m_mutex);
  if (m_isConnected != connected) {
    m_isConnected = connected;
    locker.unlock();

    emit connectionStatusChanged();
    if (connected) {
      emit connectionEstablished();
    } else {
      emit connectionLost();
    }
  }
}

void AAFConnector::cleanupCurrentReply() {
  QMutexLocker locker(&m_mutex);
  if (m_currentReply) {
    m_currentReply->abort();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
  }
}

void AAFConnector::connectReplySignals(QNetworkReply *reply,
                                       void (AAFConnector::*finishedSlot)()) {
  connect(reply, &QNetworkReply::finished, this, finishedSlot);
  connect(
      reply,
      QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
      this, &AAFConnector::handleNetworkError);
}

bool AAFConnector::validatePromptRequest(const QString &prompt) const {
  if (isProcessing()) {
    emit errorOccurred("Request already in progress");
    return false;
  }

  if (prompt.isEmpty()) {
    emit errorOccurred("Empty prompt provided");
    return false;
  }

  return true;
}

bool AAFConnector::validateVisionRequest(const QString &prompt,
                                         const QString &mediaPath) const {
  if (isProcessing()) {
    emit errorOccurred("Request already in progress");
    return false;
  }

  if (prompt.isEmpty() || mediaPath.isEmpty()) {
    emit errorOccurred("Empty prompt or media path provided");
    return false;
  }

  if (!QFileInfo::exists(mediaPath)) {
    emit errorOccurred("Media file does not exist: " + mediaPath);
    return false;
  }

  return true;
}

QString AAFConnector::formatModelName(const QString &modelId) const {
  QString name = modelId;
  name.replace('-', ' ');
  name.replace('_', '.');

  QStringList words = name.split(' ', Qt::SkipEmptyParts);
  for (QString &word : words) {
    if (!word.isEmpty()) {
      if (word[0].isDigit()) {
        word = word.toUpper();
      } else {
        word[0] = word[0].toUpper();
      }
    }
  }

  return words.join(' ');
}

// ============================================================================
// Getters (Thread-Safe)
// ============================================================================

QString AAFConnector::getServerUrl() const {
  QMutexLocker locker(&m_mutex);
  return m_serverUrl;
}

QString AAFConnector::getCurrentModelId() const {
  QMutexLocker locker(&m_mutex);
  return m_currentModelId;
}

int AAFConnector::getTimeout() const {
  QMutexLocker locker(&m_mutex);
  return m_timeout;
}

bool AAFConnector::isProcessing() const {
  QMutexLocker locker(&m_mutex);
  return m_isProcessing;
}

bool AAFConnector::isConnected() const {
  QMutexLocker locker(&m_mutex);
  return m_isConnected;
}

#include "aafconnector.moc"
