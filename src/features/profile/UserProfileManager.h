#ifndef USERPROFILEMANAGER_H
#define USERPROFILEMANAGER_H

#include <QObject>
#include <QString>
#include "core/database/UserRepository.h"
#include "features/auth/AuthManager.h"

class UserProfileManager : public QObject
{
    Q_OBJECT
public:
    explicit UserProfileManager(UserRepository *userRepo, AuthManager *authManager, QObject *parent = nullptr);
    ~UserProfileManager() override = default;

    QString hashPassword(const QString &password);
    QString updateUserProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword);
    QString cropProfileImage(const QString &imagePath, qreal x, qreal y, qreal imageWidth, qreal imageHeight, qreal cropWidth, qreal cropHeight);
    bool updateProfileImage(const QString &username, const QString &imagePath);
    QString getProfileImagePath(const QString &username);
    bool validateFilePath(const QString &filePath);

signals:
    void profileImageChanged(const QString &username, const QString &newPath);

private:
    UserRepository *m_userRepo;
    AuthManager *m_authManager;
};

#endif // USERPROFILEMANAGER_H
