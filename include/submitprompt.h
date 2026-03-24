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

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "aafconnector.h"

/**
 * @brief SubmitPrompt - Backend interface for LLM prompt submission
 *
 * This class serves as the bridge between the QML UI and the AAFConnector.
 * It manages model selection, prompt submission, and response handling.
 *
 * Memory Management:
 * - AAFConnector is owned by this object via Qt parent-child relationship
 * - All Qt objects are automatically cleaned up when parent is destroyed
 *
 * Thread Safety:
 * - This class must be used from the main Qt thread only
 * - All slots are executed in the main thread context
 *
 * @note This class is designed for embedded systems with limited resources
 */
class SubmitPrompt : public QObject {
  Q_OBJECT

  // Properties exposed to QML
  Q_PROPERTY(QString promptText READ promptText WRITE setPromptText NOTIFY
                 promptTextChanged FINAL)
  Q_PROPERTY(
      bool processingLLM READ processingLLM NOTIFY processingLLMChanged FINAL)
  Q_PROPERTY(QString outputLlm READ outputLlm NOTIFY outputLlmChanged FINAL)
  Q_PROPERTY(
      QString outputStats READ outputStats NOTIFY outputStatsChanged FINAL)
  Q_PROPERTY(
      bool progressTTFT READ progressTTFT NOTIFY progressTTFTChanged FINAL)
  Q_PROPERTY(int TTFT READ TTFT NOTIFY TTFTChanged FINAL)
  Q_PROPERTY(bool submitButtonEnabled READ submitButtonEnabled NOTIFY
                 submitButtonEnabledChanged FINAL)
  Q_PROPERTY(
      int modelLoadTime READ modelLoadTime NOTIFY modelLoadTimeChanged FINAL)
  Q_PROPERTY(bool loadModel READ loadModel WRITE setLoadModel NOTIFY
                 loadModelChanged FINAL)
  Q_PROPERTY(bool modelLoaded READ modelLoaded NOTIFY modelLoadedChanged FINAL)
  Q_PROPERTY(int modelLoadProgress READ modelLoadProgress NOTIFY
                 modelLoadProgressChanged FINAL)
  Q_PROPERTY(int currentModelIndex READ currentModelIndex WRITE
                 setCurrentModelIndex NOTIFY currentModelIndexChanged FINAL)
  Q_PROPERTY(QString currentModelName READ currentModelName NOTIFY
                 currentModelNameChanged FINAL)
  Q_PROPERTY(
      bool modelsLoading READ modelsLoading NOTIFY modelsLoadingChanged FINAL)
  Q_PROPERTY(QString modelsLoadingError READ modelsLoadingError NOTIFY
                 modelsLoadingErrorChanged FINAL)
  Q_PROPERTY(QString currentEndpoint READ currentEndpoint NOTIFY
                 currentEndpointChanged FINAL)

 public:
  /**
   * @brief Construct a new SubmitPrompt object
   * @param endpoint Initial endpoint ID (used for multi-endpoint
   * configurations)
   * @param group Initial endpoint group ("all", "usb", "pcie")
   * @param parent Qt parent object for memory management
   */
  explicit SubmitPrompt(int endpoint = 0, const QString &group = QString(),
                        QObject *parent = nullptr);
  ~SubmitPrompt() override;

  // Prevent copying and moving (Qt objects should not be copied)
  SubmitPrompt(const SubmitPrompt &) = delete;
  SubmitPrompt &operator=(const SubmitPrompt &) = delete;
  SubmitPrompt(SubmitPrompt &&) = delete;
  SubmitPrompt &operator=(SubmitPrompt &&) = delete;

  // Invokable methods for QML
  Q_INVOKABLE void stopLlm();
  Q_INVOKABLE void ejectModel();
  Q_INVOKABLE void submitPrompt();
  Q_INVOKABLE QStringList getAvailableModels() const;
  Q_INVOKABLE void refreshModelsList();
  Q_INVOKABLE void killConnectorProcess();

  // Getters (const correctness)
  QString promptText() const;
  bool processingLLM() const;
  QString outputLlm() const;
  QString outputStats() const;
  bool progressTTFT() const;
  int TTFT() const;
  int modelLoadTime() const;
  bool loadModel() const;
  bool modelLoaded() const;
  bool submitButtonEnabled() const;
  int currentModelIndex() const;
  QString currentModelName() const;
  bool modelsLoading() const;
  QString modelsLoadingError() const;
  int modelLoadProgress() const;
  QString currentEndpoint() const;

 signals:
  // Property change notifications
  void promptTextChanged();
  void outputLlmChanged();
  void processingLLMChanged();
  void outputStatsChanged();
  void progressTTFTChanged();
  void TTFTChanged();
  void modelLoadTimeChanged();
  void loadModelChanged();
  void modelLoadedChanged();
  void modelLoadProgressChanged();
  void currentModelIndexChanged();
  void currentModelNameChanged();
  void modelsLoadingChanged();
  void modelsLoadingErrorChanged();
  void availableModelsChanged();
  void currentEndpointChanged();
  void submitButtonEnabledChanged();

  // Internal signal for stopping LLM processing
  void triggerStopLlm();

 public slots:
  // Setters
  void setPromptText(const QString &inputPrompt);
  void setLoadModel(bool newLoadModel);
  void setCurrentModelIndex(int index);

 private slots:
  // Internal setters (not exposed to QML)
  void setProcessingLLM(bool status);
  void setOutputLlm(const QString &newOutputLlm);
  void setOutputStats(const QString &newOutputStats);
  void setProgressTTFT(bool newProgressTTFT);
  void setTTFT(int newTTFT);
  void setModelLoadTime(int newModelLoadTime);
  void setModelLoaded(bool newModelLoaded);
  void setModelLoadProgress(int progress);
  void setSubmitButtonEnabled(bool enabled);
  void setCurrentEndpoint(const QString &endpoint);

  // Token streaming
  void appendOutputToken(const QString &token);

  // AAF Connector response handlers
  void onAgentResponseReceived(const QString &response);
  void onAgentError(const QString &error);
  void onModelsListReceived(const QList<ModelInfo> &models);
  void onModelsListError(const QString &error);

 private:
  // Helper methods
  void connectSignals();
  void initializeDefaults();
  void initializeEndpoint(int endpoint, const QString &group);
  bool validateModelIndex(int index) const;
  void cleanGUI();
  QString formatModelName(const QString &modelName) const;
  QString formatEndpointName(int endpoint, const QString &group) const;

  // UI state
  QString m_promptText;
  QString m_outputLlm;
  QString m_outputStats;
  QString m_modelsLoadingError;
  QString m_currentEndpoint;

  // Configuration
  int m_initialEndpoint{0};
  QString m_initialGroup;

  // Model state
  QStringList m_availableModelNames;
  QList<ModelInfo> m_availableModels;
  int m_currentModelIndex{-1};
  int m_modelLoadTime{0};
  int m_modelLoadProgress{0};
  int m_TTFT{0};

  // Flags
  bool m_processingLLM{false};
  bool m_progressTTFT{false};
  bool m_loadModel{false};
  bool m_modelLoaded{false};
  bool m_modelsLoading{true};
  bool m_submitButtonEnabled{true};

  // AAF Connector - owned by this object via Qt parent-child relationship
  AAFConnector *m_aafConnector{nullptr};
};
