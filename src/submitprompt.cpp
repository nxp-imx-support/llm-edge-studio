/*
 * Copyright 2024-2026 NXP
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

#include "submitprompt.h"

#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSubmitPrompt, "submit.prompt")

namespace {
// Default values - tuned for embedded systems
constexpr const char *DEFAULT_PROMPT =
    "Describe the steps involved in machine learning model training.";
constexpr int DEFAULT_TTFT_MS = 2100;
constexpr int DEFAULT_MODEL_LOAD_TIME_MS = 111000;  // ~1.85 minutes
constexpr int PROGRESS_MIN = 0;
constexpr int PROGRESS_MAX = 100;
}  // namespace

SubmitPrompt::SubmitPrompt(int endpoint, const QString &group, QObject *parent)
    : QObject(parent),
      m_promptText(DEFAULT_PROMPT),
      m_initialEndpoint(endpoint),
      m_initialGroup(group),
      m_currentModelIndex(-1),
      m_modelLoadTime(DEFAULT_MODEL_LOAD_TIME_MS),
      m_modelLoadProgress(0),
      m_TTFT(DEFAULT_TTFT_MS),
      m_processingLLM(false),
      m_progressTTFT(false),
      m_loadModel(false),
      m_modelLoaded(false),
      m_modelsLoading(true),
      m_submitButtonEnabled(true),
      m_isDownloadingModels(false),
      m_downloadProgress(0),
      m_hasDownloadError(false),
      m_downloadProcess(nullptr),
      m_currentDownloadIndex(0),
      m_aafConnector(nullptr) {

  m_modelsToDownload = QStringList()
      << "nxp/Qwen2.5-7B-Instruct-Ara240";

  initializeDefaults();
  initializeEndpoint(endpoint, group);
  connectSignals();

  qCDebug(lcSubmitPrompt) << "SubmitPrompt initialized with endpoint:"
                          << endpoint << "group:" << group;
}

SubmitPrompt::~SubmitPrompt() {
  // Cancel any ongoing requests before destruction
  if (m_aafConnector && m_processingLLM) {
    m_aafConnector->cancelRequest();
  }
  // m_aafConnector is deleted automatically by Qt parent-child relationship
  qCDebug(lcSubmitPrompt) << "SubmitPrompt destroyed";
}

// ============================================================================
// Initialization
// ============================================================================

void SubmitPrompt::initializeDefaults() {
  m_aafConnector = new AAFConnector(this);
  if (!m_aafConnector) {
    qCCritical(lcSubmitPrompt) << "Failed to create AAFConnector";
    return;
  }
}

void SubmitPrompt::initializeEndpoint(int endpoint, const QString &group) {
  // Format and set the initial endpoint display string
  m_currentEndpoint = formatEndpointName(endpoint, group);
  qCDebug(lcSubmitPrompt) << "Initial endpoint set to:" << m_currentEndpoint;

  // Note: If AAFConnector needs endpoint/group configuration, pass it here
  // For example: m_aafConnector->setEndpoint(endpoint, group);
}

void SubmitPrompt::connectSignals() {
  if (!m_aafConnector) {
    qCCritical(lcSubmitPrompt)
        << "Cannot connect signals: AAFConnector is null";
    return;
  }

  // Model list signals
  connect(m_aafConnector, &AAFConnector::modelsListReceived, this,
          &SubmitPrompt::onModelsListReceived);
  connect(m_aafConnector, &AAFConnector::modelsListError, this,
          &SubmitPrompt::onModelsListError);

  // Response signals
  connect(m_aafConnector, &AAFConnector::responseReceived, this,
          &SubmitPrompt::onAgentResponseReceived);
  connect(m_aafConnector, &AAFConnector::errorOccurred, this,
          &SubmitPrompt::onAgentError);

  // Request state signals
  connect(m_aafConnector, &AAFConnector::requestStarted, this,
          [this]() { setProcessingLLM(true); });
  connect(m_aafConnector, &AAFConnector::requestFinished, this,
          [this]() { setProcessingLLM(false); });

  // Token streaming
  connect(m_aafConnector, &AAFConnector::tokenReceived, this,
          &SubmitPrompt::appendOutputToken);

  // Metrics
  connect(m_aafConnector, &AAFConnector::metricsReceived, this,
          &SubmitPrompt::setOutputStats);

  // Model loading signals
  connect(m_aafConnector, &AAFConnector::modelLoadStarted, this,
          [this](const QString &modelName) {
            qCDebug(lcSubmitPrompt) << "Model load started:" << modelName;
            setLoadModel(true);
            setModelLoadProgress(PROGRESS_MIN);
          });

  connect(m_aafConnector, &AAFConnector::modelLoadCompleted, this,
          [this](const QString &modelName) {
            qCDebug(lcSubmitPrompt) << "Model load completed:" << modelName;
            setModelLoaded(true);
            setLoadModel(false);
            setModelLoadProgress(PROGRESS_MAX);
          });

  connect(m_aafConnector, &AAFConnector::modelReady, this,
          [this](const QString &modelName) {
            qCDebug(lcSubmitPrompt) << "Model ready:" << modelName;
            setModelLoaded(true);
            setLoadModel(false);
            setModelLoadProgress(PROGRESS_MAX);
          });

  connect(m_aafConnector, &AAFConnector::modelStillLoading, this,
          [this](const QString &modelName, int progress) {
            Q_UNUSED(modelName)
            qCDebug(lcSubmitPrompt)
                << "Model still loading, progress:" << progress << "%";
            setModelLoadProgress(progress);
          });

  connect(m_aafConnector, &AAFConnector::modelOperationError, this,
          [this](const QString &error) {
            qCWarning(lcSubmitPrompt) << "Model operation error:" << error;
            setModelLoadProgress(PROGRESS_MIN);
            setLoadModel(false);
            setModelLoaded(false);
          });

  // Start connection
  m_aafConnector->connectToServer();
}

// ============================================================================
// Public Invokable Methods
// ============================================================================

void SubmitPrompt::stopLlm() {
  if (!m_aafConnector) {
    qCWarning(lcSubmitPrompt) << "Cannot stop LLM: AAFConnector is null";
    return;
  }

  if (!m_processingLLM) {
    qCDebug(lcSubmitPrompt) << "No active LLM processing to stop";
    return;
  }

  m_aafConnector->cancelRequest();
  emit triggerStopLlm();
  qCDebug(lcSubmitPrompt) << "LLM processing stopped";
}

void SubmitPrompt::ejectModel() {
  if (!m_aafConnector) {
    qCWarning(lcSubmitPrompt) << "Cannot eject: AAFConnector is null";
    return;
  }

  if (!m_modelLoaded) {
    qCDebug(lcSubmitPrompt) << "No model loaded to eject";
    return;
  }

  if (!validateModelIndex(m_currentModelIndex)) {
    qCWarning(lcSubmitPrompt) << "Cannot eject: invalid model index";
    return;
  }

  const ModelInfo &selectedModel = m_availableModels[m_currentModelIndex];
  qCDebug(lcSubmitPrompt) << "Ejecting model:" << selectedModel.name;

  m_aafConnector->removeModel(selectedModel.name);
  setModelLoaded(false);
  setModelLoadProgress(PROGRESS_MIN);
  cleanGUI();
}

QStringList SubmitPrompt::getAvailableModels() const {
  return m_availableModelNames;
}

void SubmitPrompt::refreshModelsList() {
  if (!m_aafConnector) {
    qCWarning(lcSubmitPrompt) << "Cannot refresh: AAFConnector is null";
    return;
  }

  qCDebug(lcSubmitPrompt) << "Refreshing models list from config...";
  m_modelsLoading = true;
  emit modelsLoadingChanged();
  m_aafConnector->loadModelsFromConfig(AAFConfig::CONFIG_PATH);
}

void SubmitPrompt::killConnectorProcess() {
  QProcess::execute("pkill", QStringList() << "connector");
}

void SubmitPrompt::submitPrompt() {
  if (!m_aafConnector) {
    qCWarning(lcSubmitPrompt) << "Cannot submit: AAFConnector is null";
    return;
  }

  if (m_promptText.isEmpty()) {
    qCWarning(lcSubmitPrompt) << "Cannot submit: empty prompt";
    return;
  }

  if (m_processingLLM) {
    qCWarning(lcSubmitPrompt) << "Cannot submit: already processing";
    return;
  }

  if (!m_modelLoaded) {
    qCWarning(lcSubmitPrompt) << "Cannot submit: no model loaded";
    return;
  }

  qCDebug(lcSubmitPrompt) << "Submitting prompt, length:"
                          << m_promptText.length();
  cleanGUI();
  m_aafConnector->sendTextPrompt(m_promptText);
}

// ============================================================================
// Getters
// ============================================================================

QString SubmitPrompt::promptText() const { return m_promptText; }
bool SubmitPrompt::processingLLM() const { return m_processingLLM; }
QString SubmitPrompt::outputLlm() const { return m_outputLlm; }
QString SubmitPrompt::outputStats() const { return m_outputStats; }
bool SubmitPrompt::progressTTFT() const { return m_progressTTFT; }
int SubmitPrompt::TTFT() const { return m_TTFT; }
int SubmitPrompt::modelLoadTime() const { return m_modelLoadTime; }
bool SubmitPrompt::loadModel() const { return m_loadModel; }
bool SubmitPrompt::modelLoaded() const { return m_modelLoaded; }
bool SubmitPrompt::submitButtonEnabled() const { return m_submitButtonEnabled; }
int SubmitPrompt::currentModelIndex() const { return m_currentModelIndex; }
bool SubmitPrompt::modelsLoading() const { return m_modelsLoading; }
QString SubmitPrompt::modelsLoadingError() const {
  return m_modelsLoadingError;
}
int SubmitPrompt::modelLoadProgress() const { return m_modelLoadProgress; }
QString SubmitPrompt::currentEndpoint() const { return m_currentEndpoint; }

QString SubmitPrompt::currentModelName() const {
  if (validateModelIndex(m_currentModelIndex)) {
    return m_availableModelNames[m_currentModelIndex];
  }
  return QStringLiteral("Unknown");
}

bool SubmitPrompt::isDownloadingModels() const { return m_isDownloadingModels; }
int SubmitPrompt::downloadProgress() const { return m_downloadProgress; }
QString SubmitPrompt::downloadStatus() const { return m_downloadStatus; }
bool SubmitPrompt::hasDownloadError() const {return m_hasDownloadError; }

// ============================================================================
// Setters
// ============================================================================

void SubmitPrompt::setPromptText(const QString &inputPrompt) {
  if (m_promptText == inputPrompt) {
    return;
  }

  m_promptText = inputPrompt;
  emit promptTextChanged();
}

void SubmitPrompt::setProcessingLLM(bool status) {
  if (m_processingLLM == status) {
    return;
  }
  m_processingLLM = status;
  emit processingLLMChanged();

  // Update submit button state based on processing status
  setSubmitButtonEnabled(!status && m_modelLoaded);
}

void SubmitPrompt::setOutputLlm(const QString &newOutputLlm) {
  if (m_outputLlm == newOutputLlm) {
    return;
  }
  m_outputLlm = newOutputLlm;
  emit outputLlmChanged();
}

void SubmitPrompt::setOutputStats(const QString &newOutputStats) {
  if (m_outputStats == newOutputStats) {
    return;
  }
  m_outputStats = newOutputStats;
  emit outputStatsChanged();
}

void SubmitPrompt::setProgressTTFT(bool newProgressTTFT) {
  if (m_progressTTFT == newProgressTTFT) {
    return;
  }
  m_progressTTFT = newProgressTTFT;
  emit progressTTFTChanged();
}

void SubmitPrompt::setTTFT(int newTTFT) {
  if (m_TTFT == newTTFT || newTTFT < 0) {
    return;
  }
  m_TTFT = newTTFT;
  emit TTFTChanged();
}

void SubmitPrompt::setModelLoadTime(int newModelLoadTime) {
  if (m_modelLoadTime == newModelLoadTime || newModelLoadTime < 0) {
    return;
  }
  m_modelLoadTime = newModelLoadTime;
  emit modelLoadTimeChanged();
}

void SubmitPrompt::setLoadModel(bool newLoadModel) {
  if (m_loadModel == newLoadModel) {
    return;
  }

  if (!m_aafConnector) {
    qCWarning(lcSubmitPrompt) << "Cannot load model: AAFConnector is null";
    return;
  }

  m_loadModel = newLoadModel;

  if (m_loadModel && !m_modelLoaded &&
      validateModelIndex(m_currentModelIndex)) {
    cleanGUI();
    const ModelInfo &selectedModel = m_availableModels[m_currentModelIndex];
    qCDebug(lcSubmitPrompt) << "Loading model:" << selectedModel.name
                            << "Type:" << selectedModel.type;
    m_aafConnector->setModelById(selectedModel.name);
  }

  emit loadModelChanged();
}

void SubmitPrompt::setModelLoaded(bool newModelLoaded) {
  if (m_modelLoaded == newModelLoaded) {
    return;
  }
  m_modelLoaded = newModelLoaded;
  emit modelLoadedChanged();

  // Update submit button state based on model loaded status
  setSubmitButtonEnabled(newModelLoaded && !m_processingLLM);
}

void SubmitPrompt::setModelLoadProgress(int progress) {
  const int clampedProgress = qBound(PROGRESS_MIN, progress, PROGRESS_MAX);
  if (m_modelLoadProgress == clampedProgress) {
    return;
  }
  m_modelLoadProgress = clampedProgress;
  qCDebug(lcSubmitPrompt) << "Model load progress:" << m_modelLoadProgress
                          << "%";
  emit modelLoadProgressChanged();
}

void SubmitPrompt::setSubmitButtonEnabled(bool enabled) {
  if (m_submitButtonEnabled == enabled) {
    return;
  }
  m_submitButtonEnabled = enabled;
  emit submitButtonEnabledChanged();
}

void SubmitPrompt::setCurrentModelIndex(int index) {
  if (m_currentModelIndex == index) {
    return;
  }

  if (!validateModelIndex(index)) {
    qCWarning(lcSubmitPrompt)
        << "Invalid model index:" << index << "Valid range: 0 -"
        << m_availableModels.size() - 1;
    return;
  }

  if (m_processingLLM) {
    qCWarning(lcSubmitPrompt) << "Cannot change model while processing";
    return;
  }

  m_currentModelIndex = index;
  cleanGUI();

  emit currentModelIndexChanged();
  emit currentModelNameChanged();

  qCDebug(lcSubmitPrompt) << "Model index changed to:" << index << "("
                          << currentModelName() << ")";
}

void SubmitPrompt::setCurrentEndpoint(const QString &endpoint) {
  if (m_currentEndpoint == endpoint) {
    return;
  }
  qCDebug(lcSubmitPrompt) << "Endpoint changed from" << m_currentEndpoint
                          << "to" << endpoint;
  m_currentEndpoint = endpoint;
  emit currentEndpointChanged();

  // Note: If AAFConnector needs to be notified of endpoint changes, do it here
  // For example: m_aafConnector->setEndpoint(endpoint);
}

// ============================================================================
// Response Handlers
// ============================================================================

void SubmitPrompt::appendOutputToken(const QString &token) {
  if (token.isEmpty()) {
    return;
  }
  m_outputLlm.append(token);
  emit outputLlmChanged();
}

void SubmitPrompt::onAgentResponseReceived(const QString &response) {
  qCDebug(lcSubmitPrompt) << "Received complete response, length:"
                          << response.length();
  setOutputLlm(response);
  setProcessingLLM(false);
}

void SubmitPrompt::onAgentError(const QString &error) {
  qCWarning(lcSubmitPrompt) << "AAFConnector error:" << error;
  setOutputLlm(QStringLiteral("Error: %1").arg(error));
  setProcessingLLM(false);
}

void SubmitPrompt::onModelsListReceived(const QList<ModelInfo> &models) {
  if (models.isEmpty()) {
    qCWarning(lcSubmitPrompt) << "Received empty models list";
    onModelsListError(QStringLiteral("No models available"));
    return;
  }
  qCDebug(lcSubmitPrompt) << "Received" << models.size() << "models";

  m_availableModels = models;
  m_availableModelNames.clear();
  m_availableModelNames.reserve(models.size());

  for (const ModelInfo &model : models) {
    const QString displayName = formatModelName(model.name);
    m_availableModelNames.append(displayName);

    qCDebug(lcSubmitPrompt)
        << "Model:" << model.name << "Display:" << displayName
        << "Type:" << model.type << "Enabled:" << model.enabled;
  }

  m_modelsLoading = false;
  m_modelsLoadingError.clear();

  // Set default model if not already set
  if (m_currentModelIndex < 0) {
    setCurrentModelIndex(0);
  }

  emit modelsLoadingChanged();
  emit modelsLoadingErrorChanged();
  emit availableModelsChanged();

}

void SubmitPrompt::onModelsListError(const QString &error) {
  qCWarning(lcSubmitPrompt) << "Failed to load models:" << error;

  m_modelsLoading = false;
  m_modelsLoadingError = error;

  emit modelsLoadingChanged();
  emit modelsLoadingErrorChanged();

  // Create fallback model only if no models are available try to fetch a fallback model
  if (m_availableModels.isEmpty()) {
    qCDebug(lcSubmitPrompt) << "No models available - user can download manually";
    m_availableModelNames.clear();
    m_availableModels.clear();
    emit availableModelsChanged();
  }
}

// ============================================================================
// Utility Methods
// ============================================================================

void SubmitPrompt::cleanGUI() {
  if (!m_outputLlm.isEmpty()) {
    m_outputLlm.clear();
    emit outputLlmChanged();
  }

  if (!m_outputStats.isEmpty()) {
    m_outputStats.clear();
    emit outputStatsChanged();
  }
}

QString SubmitPrompt::formatModelName(const QString &modelName) const {
  if (modelName.isEmpty()) {
    return QStringLiteral("Unknown Model");
  }

  QString name = modelName;
  name.replace(QLatin1Char('-'), QLatin1Char(' '));
  name.replace(QLatin1Char('_'), QLatin1Char('.'));

  QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  for (QString &word : words) {
    if (!word.isEmpty() && !word[0].isDigit()) {
      word[0] = word[0].toUpper();
    }
  }

  return words.join(QLatin1Char(' '));
}

QString SubmitPrompt::formatEndpointName(int endpoint,
                                         const QString &group) const {
  // Format endpoint based on group type
  if (group == QLatin1String("all")) {
    return QStringLiteral("All");
  } else if (group == QLatin1String("usb")) {
    return QStringLiteral("USB");
  } else if (group == QLatin1String("pcie")) {
    return QStringLiteral("PCIe%1").arg(endpoint);
  }

  // Default formatting
  return QStringLiteral("PCIe%1").arg(endpoint);
}

bool SubmitPrompt::validateModelIndex(int index) const {
  return index >= 0 && index < m_availableModels.size();
}

// ============================================================================
// Fetch Models Methods
// ============================================================================

void SubmitPrompt::downloadMissingModels() {
  if (m_isDownloadingModels) {
    qCWarning(lcSubmitPrompt) << "Download already in progress";
    return;
  }

  qCDebug(lcSubmitPrompt) << "Starting manual model download...";

  // Reset state
  m_currentDownloadIndex = 0;
  setHasDownloadError(false);
  setIsDownloadingModels(true);
  setDownloadProgress(0);

  // Start downloading the first model
  downloadNextModel();
}

void SubmitPrompt::downloadNextModel() {
  // Check if all models are downloaded
  if (m_currentDownloadIndex >= m_modelsToDownload.size()) {
    qCDebug(lcSubmitPrompt) << "All models downloaded successfully";
    setDownloadStatus("All models downloaded successfully!");
    setIsDownloadingModels(false);
    setDownloadProgress(100);

    // Refresh the models list to pick up newly downloaded models
    QTimer::singleShot(500, this, &SubmitPrompt::refreshModelsList);
    emit modelDownloadCompleted();
    return;
  }

  const QString &modelId = m_modelsToDownload[m_currentDownloadIndex];
  qCDebug(lcSubmitPrompt) << "Downloading model"
                          << (m_currentDownloadIndex + 1)
                          << "of" << m_modelsToDownload.size()
                          << ":" << modelId;

  setDownloadStatus(QString("Downloading %1 (%2/%3)...")
                        .arg(modelId)
                        .arg(m_currentDownloadIndex + 1)
                        .arg(m_modelsToDownload.size()));

  // Calculate progress based on model index
  int progressPerModel = 100 / m_modelsToDownload.size();
  int baseProgress = m_currentDownloadIndex * progressPerModel;
  setDownloadProgress(baseProgress);

  // Clean up previous process if exists
  if (m_downloadProcess) {
    m_downloadProcess->disconnect();
    m_downloadProcess->deleteLater();
  }

  // Create new process
  m_downloadProcess = new QProcess(this);

  // Connect signals
  connect(m_downloadProcess, &QProcess::readyReadStandardOutput,
          this, &SubmitPrompt::handleDownloadOutput);
  connect(m_downloadProcess, &QProcess::readyReadStandardError,
          this, &SubmitPrompt::handleDownloadOutput);
  connect(m_downloadProcess, 
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &SubmitPrompt::handleDownloadFinished);
  connect(m_downloadProcess, &QProcess::errorOccurred,
          this, &SubmitPrompt::handleDownloadError);

  // Prepare command: uvx --from /usr/share/python-wheels/fetch_models-1.0.0-py3-none-any.whl fetch_models
  QString program = "uvx";
  QStringList arguments;
  arguments << "--from"
            << "/usr/share/python-wheels/fetch_models-1.0.0-py3-none-any.whl"
            << "fetch_models"
            << "--repo-id" << modelId;

  qCDebug(lcSubmitPrompt) << "Running command:" << program << arguments.join(" ");

  // Start download process
  m_downloadProcess->start(program, arguments);
  if (!m_downloadProcess->waitForStarted(5000)) {
    qCWarning(lcSubmitPrompt) << "Failed to start download process";
    setDownloadStatus("Failed to start download process");
    setHasDownloadError(true);
    setIsDownloadingModels(false);
  }
}

void SubmitPrompt::handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  const QString &modelId = m_modelsToDownload[m_currentDownloadIndex];

  if (exitStatus == QProcess::NormalExit && exitCode == 0) {
    qCDebug(lcSubmitPrompt) << "Successfully downloaded:" << modelId;

    // Calculate progress
    int progressPerModel = 100 / m_modelsToDownload.size();
    int newProgress = (m_currentDownloadIndex + 1) * progressPerModel;
    setDownloadProgress(newProgress);

    // Move to next model
    m_currentDownloadIndex++;
    downloadNextModel();
  } else {
    qCWarning(lcSubmitPrompt) << "Failed to download model:" << modelId
                              << "Exit code:" << exitCode;

    QString errorOutput = m_downloadProcess->readAllStandardError();
    setDownloadStatus(QString("Failed to download %1: %2")
                          .arg(modelId)
                          .arg(errorOutput.isEmpty() ? "Unknown error" : errorOutput));
    setHasDownloadError(true);

    // Continue with next model (don't stop on error, like postinst)
    m_currentDownloadIndex++;
    downloadNextModel();
  }
}

void SubmitPrompt::handleDownloadError(QProcess::ProcessError error) {
  qCWarning(lcSubmitPrompt) << "Download process error:" << error;

  QString errorMsg;
  switch (error) {
    case QProcess::FailedToStart:
      errorMsg = "Failed to start download. Is 'uvx' installed?";
      break;
    case QProcess::Crashed:
      errorMsg = "Download process crashed";
      break;
    case QProcess::Timedout:
      errorMsg = "Download process timed out";
      break;
    default:
      errorMsg = "Download process error occurred";
      break;
  }

  setDownloadStatus(errorMsg);
  setHasDownloadError(true);
  setIsDownloadingModels(false);
}

void SubmitPrompt::handleDownloadOutput() {
  if (!m_downloadProcess) {
    return;
  }

  QRegularExpression re(R"(\b(\d{1,3})%\|)");
  QString output = m_downloadProcess->readAllStandardOutput();
  QString errorOutput = m_downloadProcess->readAllStandardError();

  if (!output.isEmpty()) {
    qCDebug(lcSubmitPrompt) << "Download output:" << output.trimmed();
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
      int percent = match.captured(1).toInt();
      setDownloadProgress(percent);
    }
  }

  if (!errorOutput.isEmpty()) {
    qCDebug(lcSubmitPrompt) << "Download error output:" << errorOutput.trimmed();
    QRegularExpressionMatch match = re.match(errorOutput);
    if (match.hasMatch()) {
      int percent = match.captured(1).toInt();
      setDownloadProgress(percent);
    }
  }
}

void SubmitPrompt::cancelDownload() {
  if (!m_isDownloadingModels || !m_downloadProcess) {
    return;
  }

  qCDebug(lcSubmitPrompt) << "Cancelling download...";

  m_downloadProcess->kill();
  m_downloadProcess->waitForFinished(3000);

  setDownloadStatus("Download cancelled by user");
  setIsDownloadingModels(false);
  setDownloadProgress(0);
}

void SubmitPrompt::setIsDownloadingModels(const bool downloading){
  if (m_isDownloadingModels == downloading) {
    return;
  }
  m_isDownloadingModels = downloading;
  emit isDownloadingModelsChanged();
}

void SubmitPrompt::setDownloadProgress(const int progress){
  if (m_downloadProgress == progress) {
    return;
  }
  m_downloadProgress = progress;
  emit downloadProgressChanged();
}

void SubmitPrompt::setDownloadStatus(const QString &status){
  if (m_downloadStatus == status) {
    return;
  }
  m_downloadStatus = status;
  emit downloadStatusChanged();
}

void SubmitPrompt::setHasDownloadError(const bool &error){
  if (m_hasDownloadError == error) {
    return;
  }
  m_hasDownloadError = error;
  emit hasDownloadErrorChanged();
}