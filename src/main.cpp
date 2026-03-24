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

#include <sys/socket.h>
#include <unistd.h>

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSocketNotifier>
#include <csignal>
#include <memory>

#include "submitprompt.h"
#include "version.h"

// Enable Qt logging categories for debugging
Q_LOGGING_CATEGORY(lcMain, "main")

namespace {
// Constants
constexpr int DEFAULT_ENDPOINT = 0;

// Valid group types
const QStringList VALID_GROUPS = {
    "all", "pcie"};  // const QStringList VALID_GROUPS = {"all", "usb", "pcie"};

// Socket pair for safe signal handling
int signalFd[2];

void unixSignalHandler(int) {
  char a = 1;
  [[maybe_unused]] ssize_t result = ::write(signalFd[0], &a, sizeof(a));
  // Note: Cannot handle errors in signal handler
}
}  // namespace

class SignalHandler : public QObject {
  Q_OBJECT
 public:
  explicit SignalHandler(QObject* parent = nullptr) : QObject(parent) {
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd)) {
      qCCritical(lcMain) << "Failed to create socket pair for signal handling";
      return;
    }

    m_notifier = std::make_unique<QSocketNotifier>(signalFd[1],
                                                   QSocketNotifier::Read, this);
    connect(m_notifier.get(), &QSocketNotifier::activated, this,
            &SignalHandler::handleSignal);

    std::signal(SIGINT, unixSignalHandler);
    std::signal(SIGTERM, unixSignalHandler);
  }

  ~SignalHandler() override {
    if (signalFd[0] != -1) {
      ::close(signalFd[0]);
      ::close(signalFd[1]);
    }
  }

 private slots:
  void handleSignal() {
    m_notifier->setEnabled(false);
    char tmp;
    ::read(signalFd[1], &tmp, sizeof(tmp));

    qCInfo(lcMain)
        << "Received termination signal, shutting down gracefully...";
    QGuiApplication::quit();

    m_notifier->setEnabled(true);
  }

 private:
  std::unique_ptr<QSocketNotifier> m_notifier;
};

struct CommandLineOptions {
  int endpoint = DEFAULT_ENDPOINT;
  QString group = QStringLiteral("all");

  bool isValid() const { return endpoint >= 0 && VALID_GROUPS.contains(group); }
};

bool parseCommandLine(const QGuiApplication& app, CommandLineOptions& options) {
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("LLM Edge Studio"));
  parser.addHelpOption();
  parser.addVersionOption();

  /*
  QCommandLineOption endpointOption(
      QStringList() << "e" << "endpoint",
      QStringLiteral("Endpoint ID (non-negative integer, default: %1). "
                     "Ignored when group=\"all\"")
          .arg(DEFAULT_ENDPOINT),
      QStringLiteral("endpoint"), QString::number(DEFAULT_ENDPOINT));
  parser.addOption(endpointOption);


  QCommandLineOption groupOption(QStringList() << "g" << "group",
                                 QStringLiteral("Group type: %1 (default: all)")
                                     .arg(VALID_GROUPS.join(", ")),
                                 QStringLiteral("group"),
                                 QStringLiteral("all"));
  parser.addOption(groupOption);

  parser.process(app);

  // Parse endpoint
  bool ok;
  options.endpoint = parser.value(endpointOption).toInt(&ok);
  if (!ok || options.endpoint < 0) {
    qCCritical(lcMain)
        << "Invalid --endpoint value. Must be a non-negative integer.";
    return false;
  }

  // Parse group
  options.group = parser.value(groupOption).toLower();
  if (!VALID_GROUPS.contains(options.group)) {
    qCCritical(lcMain) << "Invalid --group value. Must be one of:"
                       << VALID_GROUPS.join(", ");
    return false;
  }

  // Apply logic: if group is "all", reset endpoint
  if (options.group == QLatin1String("all")) {
    options.endpoint = 0;
  }
*/
  return true;
}

void logStartupInfo(const QGuiApplication& app,
                    const CommandLineOptions& options) {
  qCInfo(lcMain) << "=== Application Startup ===";
  qCInfo(lcMain) << "Application:" << app.applicationName();
  qCInfo(lcMain) << "Version:" << app.applicationVersion();
  // qCInfo(lcMain) << "Endpoint:" << options.endpoint;
  // qCInfo(lcMain) << "Group:" << options.group;
  qCInfo(lcMain) << "Qt Version:" << QT_VERSION_STR;
  qCInfo(lcMain) << "==========================";
}

int main(int argc, char* argv[]) {
  // Set application attributes before QGuiApplication construction
  QGuiApplication::setApplicationName(QStringLiteral("LLM Edge Studio"));
  QGuiApplication::setApplicationVersion(APP_VERSION);

  QGuiApplication app(argc, argv);

  // Setup signal handler
  SignalHandler signalHandler;

  // Parse command line
  CommandLineOptions options;
  if (!parseCommandLine(app, options)) {
    return EXIT_FAILURE;
  }

  logStartupInfo(app, options);

  // Create SubmitPrompt instance - owned by QML engine
  auto* prompt = new SubmitPrompt(
      0, QLatin1String("all"));  //(options.endpoint, options.group);
  if (!prompt) {
    qCCritical(lcMain) << "Failed to create SubmitPrompt instance";
    return EXIT_FAILURE;
  }

  // Register QML singleton from file
  qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/AppStyle.qml")),
                           "AppStyle", 1, 0, "AppStyle");

  QQmlApplicationEngine engine;

  // SubmitPrompt will be owned by the engine to prevent memory leak
  prompt->setParent(&engine);

  // Set context properties
  QQmlContext* rootContext = engine.rootContext();
  if (!rootContext) {
    qCCritical(lcMain) << "Failed to get root context";
    return EXIT_FAILURE;
  }

  rootContext->setContextProperty(QStringLiteral("mySubmitPrompt"), prompt);
  rootContext->setContextProperty(QStringLiteral("appVersion"), APP_VERSION);

  // Load QML file
  qCInfo(lcMain) << "Loading QML interface...";
  const QUrl qmlUrl(QStringLiteral("qrc:/Main.qml"));

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [qmlUrl](QObject* obj, const QUrl& objUrl) {
        if (!obj && qmlUrl == objUrl) {
          qCCritical(lcMain) << "Failed to load QML file:" << qmlUrl;
          QCoreApplication::exit(EXIT_FAILURE);
        } else if (obj && qmlUrl == objUrl) {
          qCInfo(lcMain) << "QML interface loaded successfully";
        }
      },
      Qt::QueuedConnection);

  engine.load(qmlUrl);

  // Verify engine loaded at least one object
  if (engine.rootObjects().isEmpty()) {
    qCCritical(lcMain) << "No root objects created, exiting";
    return EXIT_FAILURE;
  }

  qCInfo(lcMain) << "Application initialized successfully";

  // Run the application event loop
  const int result = app.exec();

  qCInfo(lcMain) << "Application exiting with code:" << result;
  return result;
}

#include "main.moc"
