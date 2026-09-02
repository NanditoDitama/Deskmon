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
#include "AppController.h"
#include "features/tracking/IdleChecker.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.ico"));
    app.setQuitOnLastWindowClosed(false);

    QSharedMemory sharedMemory("DeskmonAppInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, "Warning", "Application is already running!");
        return 1;
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
    });

    bool isEarlyLeaveDialogShown = false;

    // ===================================================================
    // 1. Inisialisasi QML Engine dan Window di awal
    // ===================================================================
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("logger", &logger);
    engine.rootContext()->setContextProperty("idleChecker", &idleChecker);
    engine.loadFromModule("window_logger", "Main");

    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML module: window_logger, Main. Aborting.";
        return -1;
    }

    QQuickWindow *qmlWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (!qmlWindow) {
        qWarning() << "Failed to cast root object to QQuickWindow. Aborting.";
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

    return app.exec();
}
