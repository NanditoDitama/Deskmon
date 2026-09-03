#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include <QObject>
#include <QString>

class UserRepository : public QObject
{
    Q_OBJECT
public:
    explicit UserRepository(QObject *parent = nullptr);
    ~UserRepository() override = default;

    bool findUserWithToken(int &userId, QString &username, QString &email, QString &token) const;
    bool saveUser(int userId, const QString &username, const QString &password,
                  const QString &email, const QString &department, const QString &role,
                  const QString &token);
    bool clearToken(int userId);

    QString getPassword(const QString &username) const;
    QString getEmail(const QString &username) const;
    QString getDepartment(const QString &username) const;
    QString getUsernameById(int userId) const;
    bool isUsernameTaken(const QString &username) const;

    bool updatePassword(const QString &username, const QString &newPassword);
    bool updateProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword);
    bool updateProfileImage(const QString &username, const QString &imagePath);
    QString getProfileImagePath(const QString &username) const;
};

#endif // USERREPOSITORY_H
