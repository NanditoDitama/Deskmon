#include "sessionmanager.h"
#include "datamanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

SessionManager::SessionManager(QObject *parent) : QObject(parent)
{
}

void SessionManager::login(const QString &username, const QString &password)
{
    // Implementation logic from Logger::authenticate
    // 1. Try API first
    // 2. If API fails, check local DB
    // For now, I'll provide a cleaner implementation that emits loginResult

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["email"] = username;
    payload["password"] = password;

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, username, password]() {
        bool success = false;
        QString message;
        int userId = -1;
        QString email;
        QString token;

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root["success"].toBool()) {
                    QJsonObject data = root["data"].toObject();
                    userId = data["id"].toInt();
                    email = data["email"].toString();
                    token = root["token"].toString();
                    success = true;
                    message = "Login successful";

                    m_userId = userId;
                    m_username = data["name"].toString();
                    m_email = email;
                    m_token = token;
                } else {
                    message = root["message"].toString();
                }
            }
        }

        if (!success) {
            // Fallback to local DB
            QSqlDatabase db = QSqlDatabase::database("activity_db");
            if (db.isOpen()) {
                QSqlQuery query(db);
                query.prepare("SELECT id, email, password FROM users WHERE email = :email");
                query.bindValue(":email", username);
                if (query.exec() && query.next()) {
                    QString hashedInput = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
                    if (query.value(2).toString() == hashedInput) {
                        userId = query.value(0).toInt();
                        email = query.value(1).toString();
                        success = true;
                        message = "Login successful (Offline mode)";
                        
                        m_userId = userId;
                        m_username = username;
                        m_email = email;
                    } else {
                        message = "Invalid password (Offline)";
                    }
                } else {
                    message = "User not found (Offline)";
                }
            }
        }

        emit loginResult(success, message, userId, m_username, email, token);
        reply->deleteLater();
    });
}

void SessionManager::logout()
{
    m_userId = -1;
    m_username.clear();
    m_email.clear();
    m_token.clear();
    emit logoutFinished();
}

bool SessionManager::updateProfileImage(const QString &username, const QString &imagePath)
{
    QSqlDatabase db = QSqlDatabase::database("activity_db");
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE users SET profile_image = :image WHERE email = :username OR username = :username");
    query.bindValue(":image", imagePath);
    query.bindValue(":username", username);

    if (query.exec()) {
        emit profileImageChanged(username, imagePath);
        return true;
    }
    return false;
}

QString SessionManager::getProfileImagePath(const QString &username)
{
    QSqlDatabase db = QSqlDatabase::database("activity_db");
    if (!db.isOpen()) return "";

    QSqlQuery query(db);
    query.prepare("SELECT profile_image FROM users WHERE email = :username OR username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString SessionManager::getSavedUsername()
{
    if (!m_dataManager) return "";
    QSqlQuery query(m_dataManager->activityDb());
    if (query.exec("SELECT email FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString SessionManager::getSavedPassword()
{
    if (!m_dataManager) return "";
    QSqlQuery query(m_dataManager->activityDb());
    if (query.exec("SELECT password FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return "";
}
