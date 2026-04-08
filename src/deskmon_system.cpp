// deskmon_system.cpp
// Menangani pengecekan update aplikasi dan tool maintenance.
#include "deskmon.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>

// ─────────────────────────────────────────────────
//  Update Checker
// ─────────────────────────────────────────────────

void Deskmon::checkForUpdates()
{
    const QString currentVersion = "1.0.3.3";
    QUrl url("https://raw.githubusercontent.com/NanditoDitama/DeskmonUpdateRepo/main/version.json");

    qDebug() << "Mengecek update dari:" << url.toString();

    QNetworkRequest request(url);
    QNetworkReply *reply = m_apiManager->networkManager()->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit showNotification("error", "Gagal mengecek update: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            qWarning() << "Format JSON update tidak valid.";
            emit showNotification("warning", "Format file update di server tidak valid.");
            reply->deleteLater();
            return;
        }

        QJsonObject obj = doc.object();
        QString serverVersion = obj.value("version").toString();

        if (serverVersion > currentVersion) {
            QString notes = obj.value("releaseNotes").toString();
#ifdef Q_OS_MAC
            qDebug() << "Update tersedia (macOS, suppressed):" << serverVersion;
            emit showStatusMessage(QStringLiteral("Update %1 tersedia.").arg(serverVersion));
#else
            emit updateAvailable(serverVersion, notes);
#endif
        } else {
            qDebug() << "Aplikasi sudah versi terbaru.";
            emit showNotification("success", "Aplikasi Anda sudah versi terbaru.");
        }

        reply->deleteLater();
    });
}

// ─────────────────────────────────────────────────
//  Maintenance Tool Launcher
// ─────────────────────────────────────────────────

void Deskmon::launchMaintenanceTool()
{
    QString appDir = QApplication::applicationDirPath();
    QString maintenanceToolPath = QDir(appDir).filePath("DeskmonTool.exe");

    qDebug() << "Mencari Maintenance Tool di:" << maintenanceToolPath;

    if (!QFile::exists(maintenanceToolPath)) {
        qWarning() << "DeskmonTool.exe tidak ditemukan!";
        QMessageBox::critical(nullptr, "Error", "File update (DeskmonTool.exe) tidak ditemukan.");
        return;
    }

    qDebug() << "Menjalankan Maintenance Tool dan menutup aplikasi...";
    QProcess::startDetached(maintenanceToolPath);
    QApplication::quit();
}
