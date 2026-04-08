// deskmon_profile.cpp
// Menangani semua logika profil pengguna:
//   - Update username dan password
//   - Crop dan simpan foto profil
//   - Ambil path foto profil dari database
//   - Validasi file gambar
#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>

// ─────────────────────────────────────────────────
//  Validasi File Gambar
// ─────────────────────────────────────────────────

bool Deskmon::validateFilePath(const QString &filePath)
{
    QString localPath = filePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(filePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << localPath;
        return false;
    }
    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        qWarning() << "File is not a valid file or is not readable:" << localPath;
        return false;
    }

    QImage image(localPath);
    if (image.isNull()) {
        qWarning() << "File is not a valid image:" << localPath;
        return false;
    }

    qDebug() << "File validated successfully:" << localPath;
    return true;
}

// ─────────────────────────────────────────────────
//  Update Profil (Username & Password)
// ─────────────────────────────────────────────────

QString Deskmon::hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool Deskmon::isUsernameTaken(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot check username: Database is not open";
        return false;
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (!query.exec()) {
        qWarning() << "Failed to check username:" << query.lastError().text();
        return false;
    }
    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

QString Deskmon::updateUserProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword)
{
    if (!ensureDatabaseOpen()) return "Database is not accessible";
    if (currentUsername.isEmpty() || newUsername.isEmpty()) return "Username cannot be empty";

    if (newUsername != currentUsername && isUsernameTaken(newUsername))
        return "Username already taken";

    QSqlQuery query(m_dataManager->activityDb());
    if (newPassword.isEmpty()) {
        query.prepare("UPDATE users SET username = :newUsername WHERE username = :currentUsername");
    } else {
        query.prepare("UPDATE users SET username = :newUsername, password = :newPassword WHERE username = :currentUsername");
        query.bindValue(":newPassword", hashPassword(newPassword));
    }

    query.bindValue(":newUsername", newUsername);
    query.bindValue(":currentUsername", currentUsername);

    if (!query.exec()) {
        qWarning() << "Failed to update user profile:" << query.lastError().text();
        return "Database error: " + query.lastError().text();
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No user found with username:" << currentUsername;
        return "User not found";
    }

    qDebug() << "User profile updated successfully for:" << newUsername;
    return "";
}

// ─────────────────────────────────────────────────
//  Crop & Simpan Foto Profil
// ─────────────────────────────────────────────────

QString Deskmon::cropProfileImage(const QString &imagePath, qreal x, qreal y,
                                  qreal imageWidth, qreal imageHeight,
                                  qreal cropWidth, qreal cropHeight)
{
    QString localPath = imagePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(imagePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    qDebug() << "Cropping image from path:" << localPath;

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        qWarning() << "Image file is not valid or readable:" << localPath;
        return "";
    }

    QImage image(localPath);
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << localPath;
        return "";
    }

    qreal scaleX = image.width() / imageWidth;
    qreal scaleY = image.height() / imageHeight;
    int cropSize = qMin(cropWidth, cropHeight) * scaleX;
    int cropX = qMax(0, qMin((int)((-x) * scaleX), image.width() - cropSize));
    int cropY = qMax(0, qMin((int)((-y) * scaleY), image.height() - cropSize));

    qDebug() << "Crop parameters: x=" << cropX << ", y=" << cropY << ", size=" << cropSize;

    QImage cropped = image.copy(cropX, cropY, cropSize, cropSize);
    if (cropped.isNull()) {
        qWarning() << "Failed to crop image: Invalid crop parameters";
        return "";
    }

    // Buat gambar circular
    QImage circularImage(cropped.size(), QImage::Format_ARGB32);
    circularImage.fill(Qt::transparent);
    QPainter painter(&circularImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(cropped));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, cropSize, cropSize);
    painter.end();

    // Simpan ke direktori permanen
    QDir appDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QString userSubDir = QString("profiles/%1").arg(getUsernameById(m_currentUserId));
    if (!appDir.exists(userSubDir)) {
        appDir.mkpath(userSubDir);
    }

    QString username = getUsernameById(m_currentUserId);
    if (username.isEmpty()) {
        qWarning() << "Cannot save cropped image: No valid user logged in";
        return "";
    }

    QString outputPath = appDir.filePath(QString("%1/profile_%2_%3.png")
                                             .arg(userSubDir)
                                             .arg(m_currentUserId)
                                             .arg(QDateTime::currentMSecsSinceEpoch()));
    if (!circularImage.save(outputPath, "PNG")) {
        qWarning() << "Failed to save cropped image to:" << outputPath;
        return "";
    }

    qDebug() << "Cropped image saved for user" << username << "to:" << outputPath;
    return QUrl::fromLocalFile(outputPath).toString() + "?t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
}

// ─────────────────────────────────────────────────
//  Database — Update & Ambil Foto Profil
// ─────────────────────────────────────────────────

bool Deskmon::updateProfileImage(const QString &username, const QString &imagePath)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot update profile image: Database is not open";
        return false;
    }
    if (username.isEmpty()) {
        qWarning() << "Cannot update profile image: Username is empty";
        return false;
    }

    QSqlQuery checkQuery(m_dataManager->activityDb());
    checkQuery.prepare("SELECT id FROM users WHERE username = :username");
    checkQuery.bindValue(":username", username);
    if (!checkQuery.exec() || !checkQuery.next()) {
        qWarning() << "No user found with username:" << username;
        return false;
    }

    // Hapus gambar lama
    QString oldImagePath = getProfileImagePath(username);
    if (!oldImagePath.isEmpty()) {
        QString localOldPath = oldImagePath;
        if (localOldPath.startsWith("file:///"))
            localOldPath = QUrl(localOldPath).toLocalFile();
        else if (localOldPath.startsWith("file://"))
            localOldPath = localOldPath.mid(7);

        QFile oldFile(localOldPath);
        if (oldFile.exists() && !oldFile.remove())
            qWarning() << "Failed to delete old profile image:" << localOldPath;
    }

    // Simpan path gambar baru
    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("UPDATE users SET profile_image = :imagePath WHERE username = :username");
    query.bindValue(":imagePath", imagePath);
    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "Failed to update profile image:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        qWarning() << "No rows affected for username:" << username;
        return false;
    }

    qDebug() << "Profile image updated for" << username << "to" << imagePath;
    emit profileImageChanged(username, imagePath);
    return true;
}

QString Deskmon::getProfileImagePath(const QString &username)
{
    if (!ensureDatabaseOpen() || username.isEmpty()) {
        qWarning() << "Cannot retrieve profile image path";
        return "";
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT profile_image FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "Failed to retrieve profile image path:" << query.lastError().text();
        return "";
    }

    if (query.next()) {
        return query.value(0).toString();
    }

    qWarning() << "No user found with username:" << username;
    return "";
}

// ─────────────────────────────────────────────────
//  Helper — Ambil Username berdasarkan ID
// ─────────────────────────────────────────────────

QString Deskmon::getUsernameById(int userId) const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot retrieve username: Database is not open";
        return "";
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT username FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to retrieve username:" << query.lastError().text();
        return "";
    }

    return query.value(0).toString();
}
