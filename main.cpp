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
#include "logger.h"
#include "idlechecker.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.ico"));
    app.setQuitOnLastWindowClosed(true);

    // Initialize components
    Logger logger;
    logger.checkAndCreateNewDayRecord();
    logger.loadWorkTimeData();
    IdleChecker idleChecker(&logger);
    QObject::connect(&idleChecker, &IdleChecker::idleDetected, &logger, &Logger::logIdle);
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        qDebug() << "Application is about to quit, saving final data...";
        logger.saveWorkTimeData();
        logger.sendWorkTimeToAPI();
        logger.logout();
    });



    // Setup system tray
    QSystemTrayIcon trayIcon(QIcon(":/icon.ico"));
    trayIcon.setToolTip("Deskmon");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show");
    QAction *pauseAction = trayMenu.addAction("Pause");
    QAction *quitAction = trayMenu.addAction("Quit");

#ifdef Q_OS_MACOS
    // macOS memerlukan setVisible(true) untuk menampilkan tray icon
    trayIcon.setVisible(true);
#endif

    // Function to update tray icon based on pause state
    auto updateTrayIcon = [&]() {
        if (logger.isTaskPaused()) {
            trayIcon.setIcon(QIcon(":/play_icon_app.png"));
            pauseAction->setText("Resume");
        } else {
            trayIcon.setIcon(QIcon(":/pause_icon_app.png"));
            pauseAction->setText("Pause");
        }
    };

    // Connect pause action
    QObject::connect(pauseAction, &QAction::triggered, &app, [&]() {
        logger.toggleTaskPause();
        updateTrayIcon();
    });

    // Initialize QML engine and window
    QQmlApplicationEngine *engine = nullptr;
    QQuickWindow *qmlWindow = nullptr;

    // Fungsi untuk menampilkan dialog alasan keluar lebih awal
    auto showEarlyLeaveDialog = [&]() {
        if (qmlWindow) { // Pastikan window utama sudah ada
            QMetaObject::invokeMethod(qmlWindow, "showEarlyLeaveDialog");
        } else {
            qWarning() << "Main window is not available to show the dialog. Quitting as a fallback.";
            // Jika window tidak ada, kita tidak bisa menampilkan dialog, jadi langsung keluar.
            app.quit();
        }
    };
    // Function to show QML window
    auto showQmlWindow = [&]() {
        if (!engine) {
            engine = new QQmlApplicationEngine(&app);
            engine->rootContext()->setContextProperty("logger", &logger);
            engine->rootContext()->setContextProperty("idleChecker", &idleChecker);
            qDebug() << "Loading QML module: window_logger, Main";
            engine->loadFromModule("window_logger", "Main");

            if (engine->rootObjects().isEmpty()) {
                qWarning() << "Failed to load QML module: window_logger, Main";
                return;
            }

            const auto rootObjects = engine->rootObjects();
            if (!rootObjects.isEmpty()) {
                qmlWindow = qobject_cast<QQuickWindow*>(rootObjects.constFirst());
                // qmlWindow->setVisibility(QWindow::Maximized);
                if (!qmlWindow) {
                    qWarning() << "Failed to cast root object to QQuickWindow";
                    return;
                }
            }
        }

        if (qmlWindow) {
            // Pastikan jendela tidak minimized
            qmlWindow->showMaximized();
            qmlWindow->raise();
            qmlWindow->requestActivate(); // Perbaikan: Ganti setActiveWindow
            qDebug() << "QML window shown, state:" << qmlWindow->windowState();
        } else {
            qWarning() << "qmlWindow is null, cannot show window";
        }
    };

    // Connect show action
    QObject::connect(showAction, &QAction::triggered, &app, showQmlWindow);

    // Handle tray icon double-click to show the window
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showQmlWindow();
        }
    });


    // Di main.cpp, tambahkan koneksi untuk menangani notifikasi review
    QObject::connect(&logger, &Logger::taskReviewNotification, &app, [&](const QString &message) {
        // Tampilkan notifikasi system tray
        trayIcon.showMessage("Task Review", message, QSystemTrayIcon::Information, 10000);

        // Jika window QML terbuka, kirim sinyal untuk menampilkan notifikasi in-app
        if (qmlWindow) {
            QMetaObject::invokeMethod(qmlWindow, "showReviewNotification",
                                      Q_ARG(QVariant, message));
        }
    });

    // Connect notifikasi idle ke system tray
    QObject::connect(&idleChecker, &IdleChecker::showIdleNotification, &app, [&](const QString &message) {
        trayIcon.showMessage("Deskmon", message, QSystemTrayIcon::Information, 15000);
        qDebug() << "System tray notification shown:" << message;
    });

    // Connect klik notifikasi untuk membuka aplikasi
    QObject::connect(&trayIcon, &QSystemTrayIcon::messageClicked, &app, [&]() {
        qDebug() << "Notification clicked, attempting to show QML window";
        showQmlWindow();
    });

    auto quitApplication = [&]() {
        if (logger.workTimeElapsedSeconds() < logger.totalWorkSeconds()) {
            qDebug() << "Work time is less than required. Showing reason dialog.";
            showEarlyLeaveDialog();
        } else {
            qDebug() << "Work time is sufficient. Quitting directly.";
            app.quit();
        }
    };

    QObject::connect(quitAction, &QAction::triggered, &app, quitApplication);
    QObject::connect(&logger, &Logger::earlyLeaveReasonSubmitted, &app, &QCoreApplication::quit);
    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();



    // Connect to logger's pause state changed signal
    QObject::connect(&logger, &Logger::taskPausedChanged, &app, [&]() {
        updateTrayIcon();
    });

    // Initial icon update
    updateTrayIcon();
    showQmlWindow();

    // Start monitoring
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, [&]() {
        if (!idleChecker.isIdle()) {
            logger.logActiveWindow();
        }
    });
    timer.start(1000);

    // Di main()
    QTimer *dayChangeTimer = new QTimer(&app);
    QObject::connect(dayChangeTimer, &QTimer::timeout, [&]() {
        static QString lastDate = QDate::currentDate().toString("yyyy-MM-dd");
        QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
        if (currentDate != lastDate) {
            lastDate = currentDate;
            logger.checkAndCreateNewDayRecord();
            logger.loadWorkTimeData();
        }
    });
    dayChangeTimer->start(60000); // cek setiap 1 menit

    // Load QML UI if started with --show argument
    if (app.arguments().contains("--show")) {
        showQmlWindow();
    }

    return app.exec();
}
