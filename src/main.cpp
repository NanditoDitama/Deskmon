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
#include "deskmon.h"
#include "idlechecker.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/icon.ico"));
    app.setQuitOnLastWindowClosed(false);

    QSharedMemory sharedMemory("DeskmonAppInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, "Warning", "Application is already running!");
        return 1;
    }

    Deskmon logger;
    logger.checkAndCreateNewDayRecord();
    logger.loadWorkTimeData();
    IdleChecker idleChecker(&logger);
    QObject::connect(&idleChecker, &IdleChecker::idleDetected, &logger, &Deskmon::logIdle);
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
    QSystemTrayIcon trayIcon(QIcon(":/assets/icon.ico"));
    trayIcon.setToolTip("Deskmon");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show");
    QAction *pauseAction = trayMenu.addAction("Pause");
    QAction *quitAction = trayMenu.addAction("Quit");

    QObject::connect(showAction, &QAction::triggered, &app, showQmlWindow);

    // 1. Menangani klik ganda (double click) pada tray icon untuk menampilkan window
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showQmlWindow(); // Panggil fungsi untuk menampilkan window
        }
    });

    auto updateTrayIcon = [&]() {
        if (logger.isTaskPaused()) {
            trayIcon.setIcon(QIcon(":/assets/play_icon_app.png"));
            pauseAction->setText("Resume");
        } else {
            trayIcon.setIcon(QIcon(":/assets/pause_icon_app.png"));
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

        // --- PERUBAHAN LOGIKA ADA DI SINI ---
        // Cek waktu kerja HANYA SETELAH dialog detail tugas selesai.
        if (logger.workTimeElapsedSeconds() < 32400) {
            // Waktu BELUM cukup -> Tampilkan EarlyLeaveDialog.
            qDebug() << "Work time is less than required. Showing reason dialog.";
            showEarlyLeaveDialog();
        } else {
            // Waktu SUDAH cukup -> Langsung Quit.
            qDebug() << "Work time is sufficient. Quitting directly.";
            app.quit();
        }
        // --- BATAS PERUBAHAN ---
    };

    // Hubungkan sinyal dari logger ke lambda di atas.
    // Ini akan dipanggil SETELAH dialog detail tugas selesai (baik di-OK maupun di-Cancel).
    QObject::connect(&logger, &Deskmon::readyToProceedWithQuit, &app, proceedWithEarlyLeaveCheck);

    // 'quitApplication' sekarang menjadi pemicu utama
    auto quitApplication = [&]() {
        // --- PERUBAHAN LOGIKA ADA DI SINI ---
        // 1. Cek JIKA ADA task aktif.
        if (logger.activeTaskId() != -1 && !logger.isTaskPaused()) {
            // Ada task aktif, tampilkan dialog detail dan TUNGGU sinyal.
            qDebug() << "Active task detected. Requesting details before proceeding with quit checks.";
            QMetaObject::invokeMethod(qmlWindow, "showTaskDetailsDialog",
                                      Q_ARG(QVariant, logger.activeTaskId()),
                                      Q_ARG(QVariant, "quit"), // Aksi "quit"
                                      Q_ARG(QVariant, -1));
            return; // Berhenti di sini, tunggu sinyal 'readyToProceedWithQuit'
        }

        // 2. TIDAK ADA task aktif.
        //    Langsung lanjutkan ke pemeriksaan 'Early Leave' (yang akan mengecek waktu).
        qDebug() << "No active task. Proceeding to early leave check.";
        proceedWithEarlyLeaveCheck();
        // --- BATAS PERUBAHAN ---
    };

    QObject::connect(quitAction, &QAction::triggered, &app, [&]() {
        // Panggil logika quit yang sudah ada
        showQmlWindow();
        quitApplication();
    });
    QObject::connect(&logger, &Deskmon::earlyLeaveReasonSubmitted, &app, &QCoreApplication::quit);

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

#ifdef Q_OS_MACOS
    trayIcon.setVisible(true);
#endif

    // ===================================================================
    // 5. Hubungkan sinyal notifikasi dan lainnya
    // ===================================================================
    QObject::connect(&logger, &Deskmon::taskPausedChanged, &app, updateTrayIcon);
    QObject::connect(&idleChecker, &IdleChecker::showIdleNotification, &app, [&](const QString &message) {
        trayIcon.showMessage("Deskmon", message, QSystemTrayIcon::Information, 15000);
    });
    QObject::connect(&trayIcon, &QSystemTrayIcon::messageClicked, &app, showQmlWindow);
    QObject::connect(&logger, &Deskmon::taskReviewNotification, &app, [&](const QString &message) {
        trayIcon.showMessage("Task Review", message, QSystemTrayIcon::Information, 10000);
        QMetaObject::invokeMethod(qmlWindow, "showReviewNotification", Q_ARG(QVariant, message));
    });

    // ===================================================================
    // 6. Mulai Timer dan tampilkan window
    // ===================================================================
    updateTrayIcon();
    showQmlWindow(); // Tampilkan window utama saat start

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
