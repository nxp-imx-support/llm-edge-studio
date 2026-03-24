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

#ifndef AAFCONNECTOR_H
#define AAFCONNECTOR_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QTimer>

#include "aafconfig.h"

/**
 * @brief Model information structure
 */
struct ModelInfo {
  QString name;
  QString description;
  QString type;
  QString toolCalling;
  int maxPromptSize{2048};
  bool enabled{false};
  bool supportsVideo{false};
  bool supportsImage{false};
};

/**
 * @brief Model status structure
 */
struct ModelStatus {
  QString name;
  QString id;
  QString description;
  bool ready{false};
};

/**
 * @brief AAF Connector - Manages communication with AI Accelerator Framework
 *
 * This class provides thread-safe access to LLM models running on the
 * AI Accelerator Framework backend. It handles model loading, prompt
 * submission, streaming responses, and metrics collection.
 *
 * Thread Safety: All public methods are thread-safe and can be called
 * from different threads. Internal state is protected by mutex.
 *
 * Memory Management: Uses Qt parent-child ownership model for automatic
 * cleanup. Network replies are properly managed and deleted.
 */
class AAFConnector : public QObject {
  Q_OBJECT

 public:
  explicit AAFConnector(QObject *parent = nullptr);
  ~AAFConnector() override;

  // Prevent copying
  AAFConnector(const AAFConnector &) = delete;
  AAFConnector &operator=(const AAFConnector &) = delete;

  // Configuration
  void setServerUrl(const QString &url);
  void setTimeout(int timeoutMs);

  // Connection management
  void connectToServer();
  void disconnectFromServer();

  // Prompt submission
  void sendTextPrompt(const QString &prompt);
  void sendVisionPrompt(const QString &prompt, const QString &mediaPath);
  void cancelRequest();

  // Model management
  void loadModelsFromConfig(const QString &configPath);
  void setModelById(const QString &modelId);
  void loadModel(const QString &modelName, const QString &toolCalling = "");
  void removeModel(const QString &modelName);

  // Metrics
  void requestMetrics(const QString &modelName = QString());

  // Getters (thread-safe)
  QString getServerUrl() const;
  QString getCurrentModelId() const;
  int getTimeout() const;
  bool isProcessing() const;
  bool isConnected() const;
  QString formatModelName(const QString &modelId) const;

 signals:
  // Response signals
  void responseReceived(const QString &response);
  void tokenReceived(const QString &token);
  void errorOccurred(const QString &error) const;

  // Request state signals
  void requestStarted();
  void requestFinished();

  // Connection signals
  void connectionStatusChanged();
  void connectionEstablished();
  void connectionLost();

  // Model management signals
  void modelsListReceived(const QList<ModelInfo> &models);
  void modelsListError(const QString &error);
  void modelLoadStarted(const QString &modelName);
  void modelLoadCompleted(const QString &modelName);
  void modelStillLoading(const QString &modelName, int progress);
  void modelReady(const QString &modelName);
  void modelRemoved(const QString &modelName);
  void modelOperationError(const QString &error) const;
  void modelStatusReceived(const ModelStatus &status);

  // Metrics signals
  void metricsReceived(const QString &metrics);

 private slots:
  // Network handlers
  void handleNetworkReply();
  void handleStreamingData();
  void handleNetworkError(QNetworkReply::NetworkError error);
  void onRequestTimeout();

  // Model management handlers
  void handleModelLoadReply();
  void handleModelRemoveReply();
  void handleModelStatusReply();

  // Metrics handlers
  void handleMetricsReply();

 private:
  // Configuration parsing
  bool parseServerConfig(const QString &configPath,
                         QList<ModelInfo> &models) const;
  ModelInfo parseModelInfo(const QJsonObject &modelObj) const;
  bool validateModelInstallation(const QString &modelName) const;

  // Request creation
  QNetworkRequest createJsonRequest(const QUrl &url) const;
  QNetworkRequest createMetricsRequest(const QString &modelName) const;
  QNetworkRequest createModelStatusRequest(const QString &modelName) const;

  // Payload creation
  QJsonObject createTextPayload(const QString &prompt) const;
  QJsonObject createVideoPayload(const QString &prompt,
                                 const QString &videoPath) const;

  // Request handling
  void sendRequest(const QJsonObject &payload);
  void processStreamingChunk(const QString &jsonStr);
  QString processCompleteResponse(const QByteArray &data) const;

  // Metrics formatting
  QString formatMetrics(const QJsonObject &metrics) const;

  // Model status polling
  void startModelStatusPolling(const QString &modelName);
  void stopModelStatusPolling();
  void checkModelStatus(const QString &modelName);

  // State management
  void setProcessingState(bool processing);
  void setConnectionState(bool connected);

  // Utility methods
  void cleanupCurrentReply();
  void connectReplySignals(QNetworkReply *reply,
                           void (AAFConnector::*finishedSlot)());
  bool validatePromptRequest(const QString &prompt) const;
  bool validateVisionRequest(const QString &prompt,
                             const QString &mediaPath) const;

  // Member variables
  QNetworkAccessManager *m_networkManager;
  QNetworkReply *m_currentReply;
  QTimer *m_timeoutTimer;
  QTimer *m_modelStatusTimer;

  mutable QMutex m_mutex;  // Protects shared state

  QString m_serverUrl;
  QString m_currentModelId;
  QString m_apiKey;
  QString m_loadingModelName;
  int m_timeout;
  bool m_isConnected;
  bool m_isProcessing;
};

#endif  // AAFCONNECTOR_H
