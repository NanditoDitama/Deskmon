#include "UserRepository.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

UserRepository::UserRepository(QObject *parent)
    : QObject(parent)
{
}

bool UserRepository::findUserWithToken(int &userId, QString &username, QString &email, QString &token) const
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, username, email, token FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1");
    if (query.exec() && query.next()) {
        userId = query.value(0).toInt();
        username = query.value(1).toString();
        email = query.value(2).toString();
        token = query.value(3).toString();
        return true;
    }
    return false;
}

bool UserRepository::saveUser(int userId, const QString &username, const QString &password,
                              const QString &email, const QString &department, const QString &role,
                              const QString &token)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT OR REPLACE INTO users (id, username, password, email, department, role, token) "
                  "VALUES (:id, :username, :password, :email, :department, :role, :token)");
    query.bindValue(":id", userId);
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":email", email);
    query.bindValue(":department", department);
    query.bindValue(":role", role);
    query.bindValue(":token", token);

    if (!query.exec()) {
        qWarning() << "Failed to save user:" << query.lastError().text();
        return false;
    }
    return true;
}

bool UserRepository::clearToken(int userId)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE users SET token = '' WHERE id = :id");
    query.bindValue(":id", userId);
    return query.exec();
}

QString UserRepository::getPassword(const QString &username) const
{
    if (!DatabaseManager::instance().ensureOpen()) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString UserRepository::getEmail(const QString &username) const
{
    if (!DatabaseManager::instance().ensureOpen()) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT email FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString UserRepository::getDepartment(const QString &username) const
{
    if (!DatabaseManager::instance().ensureOpen()) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT department FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString UserRepository::getUsernameById(int userId) const
{
    if (!DatabaseManager::instance().ensureOpen()) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT username FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

bool UserRepository::isUsernameTaken(const QString &username) const
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool UserRepository::updatePassword(const QString &username, const QString &newPassword)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE users SET password = :password WHERE username = :username");
    query.bindValue(":password", newPassword);
    query.bindValue(":username", username);
    return query.exec();
}

bool UserRepository::updateProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    if (newPassword.isEmpty()) {
        query.prepare("UPDATE users SET username = :newUsername WHERE username = :currentUsername");
    } else {
        query.prepare("UPDATE users SET username = :newUsername, password = :newPassword WHERE username = :currentUsername");
        query.bindValue(":newPassword", newPassword);
    }
    query.bindValue(":newUsername", newUsername);
    query.bindValue(":currentUsername", currentUsername);
    return query.exec();
}

bool UserRepository::updateProfileImage(const QString &username, const QString &imagePath)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE users SET profile_image = :imagePath WHERE username = :username");
    query.bindValue(":imagePath", imagePath);
    query.bindValue(":username", username);
    return query.exec();
}

QString UserRepository::getProfileImagePath(const QString &username) const
{
    if (!DatabaseManager::instance().ensureOpen()) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT profile_image FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}
