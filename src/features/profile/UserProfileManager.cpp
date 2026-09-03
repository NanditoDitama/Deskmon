#include "UserProfileManager.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QUrl>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>

UserProfileManager::UserProfileManager(UserRepository *userRepo, AuthManager *authManager, QObject *parent)
    : QObject(parent)
    , m_userRepo(userRepo)
    , m_authManager(authManager)
{
}

QString UserProfileManager::hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool UserProfileManager::validateFilePath(const QString &filePath)
{
    QString localPath = filePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(filePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        return false;
    }

    QImage image(localPath);
    return !image.isNull();
}

QString UserProfileManager::updateUserProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword)
{
    if (!m_userRepo) {
        return "Database is not accessible";
    }

    if (currentUsername.isEmpty() || newUsername.isEmpty()) {
        return "Username cannot be empty";
    }

    if (newUsername != currentUsername && m_authManager->isUsernameTaken(newUsername)) {
        return "Username already taken";
    }

    QString hashedPassword = newPassword.isEmpty() ? "" : hashPassword(newPassword);
    if (!m_userRepo->updateProfile(currentUsername, newUsername, hashedPassword)) {
        return "Failed to update profile";
    }

    if (currentUsername == m_authManager->currentUsername()) {
        m_authManager->setCurrentUserInfo(m_authManager->currentUserId(), newUsername, m_authManager->currentUserEmail(), m_authManager->authToken());
    }

    return "";
}

QString UserProfileManager::cropProfileImage(const QString &imagePath, qreal x, qreal y,
                                             qreal imageWidth, qreal imageHeight,
                                             qreal cropWidth, qreal cropHeight)
{
    QString localPath = imagePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(imagePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        return "";
    }

    QImage image(localPath);
    if (image.isNull()) return "";

    qreal scaleX = image.width() / imageWidth;
    qreal scaleY = image.height() / imageHeight;

    int cropSize = qMin(cropWidth, cropHeight) * scaleX;
    int cropX = (-x) * scaleX;
    int cropY = (-y) * scaleY;

    cropX = qMax(0, qMin(cropX, image.width() - cropSize));
    cropY = qMax(0, qMin(cropY, image.height() - cropSize));

    QImage cropped = image.copy(cropX, cropY, cropSize, cropSize);
    if (cropped.isNull()) return "";

    QImage circularImage(cropped.size(), QImage::Format_ARGB32);
    circularImage.fill(Qt::transparent);

    QPainter painter(&circularImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(cropped));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, cropSize, cropSize);
    painter.end();

    QDir appDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QString username = m_authManager->getUsernameById(m_authManager->currentUserId());
    if (username.isEmpty()) return "";

    QString userSubDir = QString("profiles/%1").arg(username);
    if (!appDir.exists(userSubDir)) {
        appDir.mkpath(userSubDir);
    }

    QString outputPath = appDir.filePath(QString("%1/profile_%2_%3.png")
                                             .arg(userSubDir)
                                             .arg(m_authManager->currentUserId())
                                             .arg(QDateTime::currentMSecsSinceEpoch()));
    if (!circularImage.save(outputPath, "PNG")) {
        return "";
    }

    return QUrl::fromLocalFile(outputPath).toString() + "?t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
}

bool UserProfileManager::updateProfileImage(const QString &username, const QString &imagePath)
{
    if (!m_userRepo || username.isEmpty()) {
        return false;
    }

    // Hapus foto profil lama
    QString oldImagePath = getProfileImagePath(username);
    if (!oldImagePath.isEmpty()) {
        QString localOldPath = oldImagePath;
        if (localOldPath.startsWith("file:///")) {
            localOldPath = QUrl(localOldPath).toLocalFile();
        } else if (localOldPath.startsWith("file://")) {
            localOldPath = localOldPath.mid(7);
        }
        QFile oldFile(localOldPath);
        if (oldFile.exists()) oldFile.remove();
    }

    if (m_userRepo->updateProfileImage(username, imagePath)) {
        emit profileImageChanged(username, imagePath);
        return true;
    }
    return false;
}

QString UserProfileManager::getProfileImagePath(const QString &username)
{
    if (!m_userRepo || username.isEmpty()) {
        return "";
    }
    return m_userRepo->getProfileImagePath(username);
}
