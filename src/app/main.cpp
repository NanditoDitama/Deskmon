#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QQuickWindow>
#include <QIcon>
#include <QQmlContext>
#include <QDebug>
#include <QSharedMemory>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QMutex>
#include <QQuickStyle>
#include <cstdio>
#include "AppController.h"
#include "features/tracking/IdleChecker.h"

// ===================================================================
// Sistem Logging ke File dan Terminal
// ===================================================================
static QFile *g_logFile = nullptr;
static QMutex g_logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&g_logMutex);

    QString levelStr;
    switch (type) {
    case QtDebugMsg:    levelStr = "[DEBUG]"; break;
    case QtInfoMsg:     levelStr = "[INFO ]"; break;
    case QtWarningMsg:  levelStr = "[WARN ]"; break;
    case QtCriticalMsg: levelStr = "[ERROR]"; break;
    case QtFatalMsg:    levelStr = "[FATAL]"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logLine = QString("%1 %2 %3\n").arg(timestamp, levelStr, msg);

    // Tampilkan di terminal (stdout/stderr)
    fprintf(stderr, "%s", logLine.toLocal8Bit().constData());
    fflush(stderr);

    // Simpan ke file log
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream stream(g_logFile);
        stream << logLine;
        stream.flush();
    }
}

void initLogging()
{
    QString logsDirPath = QDir::current().filePath("logs");
    QDir logsDir(logsDirPath);
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString logFilePath = logsDir.filePath(QString("deskmon_%1.log").arg(timestamp));

    g_logFile = new QFile(logFilePath);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(customMessageHandler);
        qInfo() << "=== Deskmon Started - Log File:" << logFilePath << "===";
    } else {
        qWarning() << "Failed to create log file at:" << logFilePath;
    }
}

void closeLogging()
{
    if (g_logFile) {
        qInfo() << "=== Deskmon Closed ===";
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Pranala");
    app.setApplicationName("Deskmon");
    QQuickStyle::setStyle("Material");
    app.setWindowIcon(QIcon(":/icon.ico"));
    app.setQuitOnLastWindowClosed(false);

    // Inisialisasi logging ke folder logs/
    initLogging();

    QSharedMemory sharedMemory("DeskmonAppInstance");
    if (!sharedMemory.create(1)) {
        // Coba detach jika ada stale shared memory dari proses yang crash
        sharedMemory.attach();
        sharedMemory.detach();
        if (!sharedMemory.create(1)) {
            qWarning() << "Application is already running (single instance lock active).";
            QMessageBox::warning(nullptr, "Warning", "Application is already running!");
            closeLogging();
            return 1;
        }
    }

    AppController logger;
    logger.checkAndCreateNewDayRecord();
    logger.loadWorkTimeData();
    IdleChecker idleChecker(&logger);
    QObject::connect(&idleChecker, &IdleChecker::idleDetected, &logger, &AppController::logIdle);
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        qDebug() << "Application is about to quit, saving final data...";
        logger.saveWorkTimeData();
        logger.sendWorkTimeToAPI();
        logger.logout();
        closeLogging();
    });

    bool isEarlyLeaveDialogShown = false;

    // ===================================================================
    // 1. Inisialisasi QML Engine dan Window di awal
    // ===================================================================
    qmlRegisterSingletonType(QUrl("qrc:/qt/qml/window_logger/qml/theme/Theme.qml"), "theme", 1, 0, "Theme");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("logger", &logger);
    engine.rootContext()->setContextProperty("idleChecker", &idleChecker);
    engine.loadFromModule("window_logger", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML module: window_logger, Main. Aborting.";
        closeLogging();
        return -1;
    }

    QQuickWindow *qmlWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (!qmlWindow) {
        qCritical() << "Failed to cast root object to QQuickWindow. Aborting.";
        closeLogging();
        return -1;
    }

    // ===================================================================
    // 2. Sekarang qmlWindow dijamin tidak null, kita bisa definisikan fungsi
    // ===================================================================
    auto showQmlWindow = [&]() {
        if (qmlWindow->visibility() == QWindow::Hidden) {
            qmlWindow->show();
        }
        qmlWindow->showMaximized();
        qmlWindow->raise();
        qmlWindow->requestActivate();
        qDebug() << "QML window shown, state:" << qmlWindow->windowState();
    };

    auto showEarlyLeaveDialog = [&]() {
        showQmlWindow();
        QMetaObject::invokeMethod(qmlWindow, "showEarlyLeaveDialog");
    };

    // ===================================================================
    // 3. Setup System Tray dan hubungkan dengan fungsi yang sudah didefinisikan
    // ===================================================================
    QSystemTrayIcon trayIcon(QIcon(":/icon.ico"));
    trayIcon.setToolTip("Deskmon");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show");
    QAction *pauseAction = trayMenu.addAction("Pause");
    QAction *quitAction = trayMenu.addAction("Quit");

    QObject::connect(showAction, &QAction::triggered, &app, showQmlWindow);

    // 1. Menangani klik ganda (double click) pada tray icon untuk menampilkan window
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showQmlWindow();
        }
    });

    auto updateTrayIcon = [&]() {
        if (logger.isTaskPaused()) {
            trayIcon.setIcon(QIcon(":/play_icon_app.png"));
            pauseAction->setText("Resume");
        } else {
            trayIcon.setIcon(QIcon(":/pause_icon_app.png"));
            pauseAction->setText("Pause");
        }
    };

    QObject::connect(pauseAction, &QAction::triggered, &app, [&]() {
        logger.toggleTaskPause();
        updateTrayIcon();
    });

    // ===================================================================
    // 4. Definisikan dan hubungkan quitApplication SETELAH semuanya siap
    // ===================================================================
    auto proceedWithEarlyLeaveCheck = [&]() {
        if (isEarlyLeaveDialogShown) {
            qDebug() << "Early leave dialog already shown, opening application";
            showQmlWindow();
            return;
        }

        if (logger.workTimeElapsedSeconds() < 32400) {
            qDebug() << "Work time is less than required. Showing reason dialog.";
            showEarlyLeaveDialog();
        } else {
            qDebug() << "Work time is sufficient. Quitting directly.";
            app.quit();
        }
    };

    QObject::connect(&logger, &AppController::readyToProceedWithQuit, &app, proceedWithEarlyLeaveCheck);

    auto quitApplication = [&]() {
        if (logger.activeTaskId() != -1 && !logger.isTaskPaused()) {
            qDebug() << "Active task detected. Requesting details before proceeding with quit checks.";
            QMetaObject::invokeMethod(qmlWindow, "showTaskDetailsDialog",
                                      Q_ARG(QVariant, logger.activeTaskId()),
                                      Q_ARG(QVariant, "quit"),
                                      Q_ARG(QVariant, -1));
            return;
        }

        qDebug() << "No active task. Proceeding to early leave check.";
        proceedWithEarlyLeaveCheck();
    };

    QObject::connect(quitAction, &QAction::triggered, &app, [&]() {
        showQmlWindow();
        quitApplication();
    });
    QObject::connect(&logger, &AppController::earlyLeaveReasonSubmitted, &app, &QCoreApplication::quit);

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

#ifdef Q_OS_MACOS
    trayIcon.setVisible(true);
#endif

    // ===================================================================
    // 5. Hubungkan sinyal notifikasi dan lainnya
    // ===================================================================
    QObject::connect(&logger, &AppController::taskPausedChanged, &app, updateTrayIcon);
    QObject::connect(&idleChecker, &IdleChecker::showIdleNotification, &app, [&](const QString &message) {
        trayIcon.showMessage("Deskmon", message, QSystemTrayIcon::Information, 15000);
    });
    QObject::connect(&trayIcon, &QSystemTrayIcon::messageClicked, &app, showQmlWindow);
    QObject::connect(&logger, &AppController::taskReviewNotification, &app, [&](const QString &message) {
        trayIcon.showMessage("Task Review", message, QSystemTrayIcon::Information, 10000);
        QMetaObject::invokeMethod(qmlWindow, "showReviewNotification", Q_ARG(QVariant, message));
    });

    // ===================================================================
    // 6. Mulai Timer dan tampilkan window
    // ===================================================================
    updateTrayIcon();
    showQmlWindow();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, [&]() {
        if (!idleChecker.isIdle()) {
            logger.logActiveWindow();
        }
    });
    timer.start(1000);

    QTimer dayChangeTimer;
    QObject::connect(&dayChangeTimer, &QTimer::timeout, [&]() {
        static QString lastDate = QDate::currentDate().toString("yyyy-MM-dd");
        QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
        if (currentDate != lastDate) {
            lastDate = currentDate;
            logger.checkAndCreateNewDayRecord();
            logger.loadWorkTimeData();
        }
    });
    dayChangeTimer.start(60000);

    int result = app.exec();
    closeLogging();
    return result;
}
