#include "apimanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>

ApiManager::ApiManager(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void ApiManager::setAuthToken(const QString &token)
{
    m_authToken = token;
}

QNetworkReply* ApiManager::post(const QString &url, const QJsonObject &payload)
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    return m_networkManager->post(request, QJsonDocument(payload).toJson());
}

QNetworkReply* ApiManager::get(const QString &url)
{
    QNetworkRequest request{QUrl(url)};
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    return m_networkManager->get(request);
}

void ApiManager::fetchTasks(int userId)
{
    if (userId == -1) return;
    QString url = QString("https://deskmon.pranala-dt.co.id/api/get-task/%1").arg(userId);
    QNetworkReply *reply = get(url);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonArray tasks = doc.object().value("data").toArray();
                emit tasksFetched(tasks);
            }
        }
        reply->deleteLater();
    });
}

void ApiManager::fetchProductivityApps(int userId)
{
    if (userId == -1) return;
    QString url = QString("https://deskmon.pranala-dt.co.id/api/get-app/%1").arg(userId);
    QNetworkReply *reply = get(url);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonArray apps = doc.object().value("data").toArray();
                emit productivityAppsFetched(apps);
            }
        }
        reply->deleteLater();
    });
}

void ApiManager::submitWorkTime(int userId, int elapsedSeconds)
{
    if (userId == -1) return;
    QJsonObject payload;
    payload["user_id"] = userId;
    payload["time_at_work"] = elapsedSeconds;
    QNetworkReply *reply = post("https://deskmon.pranala-dt.co.id/api/send-time-at-work", payload);
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void ApiManager::logout()
{
    QNetworkReply *reply = post("https://deskmon.pranala-dt.co.id/api/logout", QJsonObject());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        emit logoutFinished(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

void ApiManager::fetchWorkTime(int userId)
{
    if (userId == -1) return;
    QString url = QString("https://deskmon.pranala-dt.co.id/api/get-time-at-work/%1").arg(userId);
    QNetworkReply *reply = get(url);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonObject data = doc.object().value("data").toObject();
                if (data.contains("raw")) {
                    emit workTimeFetched(data["raw"].toInt());
                }
            }
        }
        reply->deleteLater();
    });
}

void ApiManager::submitEarlyLeaveReason(const QString &email, const QString &reason)
{
    QJsonObject payload;
    payload["email"] = email;
    payload["alasan"] = reason;
    QNetworkReply *reply = post("https://deskmon.pranala-dt.co.id/api/send-alasan", payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool success = (reply->error() == QNetworkReply::NoError);
        QString message;
        if (!success) message = reply->errorString();
        emit earlyLeaveSubmitted(success, message);
        reply->deleteLater();
    });
}

void ApiManager::submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId)
{
    QJsonObject payload;
    payload["task_id"] = taskId;
    payload["details"] = details;
    payload["action"] = action;
    if (nextTaskId != -1) payload["next_task_id"] = nextTaskId;

    QNetworkReply *reply = post("https://deskmon.pranala-dt.co.id/api/task-details", payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool success = (reply->error() == QNetworkReply::NoError);
        QString message;
        if (!success) message = reply->errorString();
        emit taskDetailsSubmitted(success, message);
        reply->deleteLater();
    });
}

void ApiManager::sendLogout(const QString &token)
{
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/logout"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    m_networkManager->post(request, QJsonDocument().toJson());
}

void ApiManager::submitEarlyLeaveReason(const QString &email, const QString &reason, const QString &token)
{
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/early-leave/store"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    QJsonObject payload;
    payload["email"] = email;
    payload["alasan"] = reason;

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit earlyLeaveReasonSubmitted();
        }
        reply->deleteLater();
    });
}

void ApiManager::sendDailyUsageReport(const QJsonArray &data, const QString &token)
{
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/productivity-app"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    QJsonObject payload;
    payload["data"] = data;

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Daily usage report sent successfully.";
        } else {
            qWarning() << "Failed to send daily usage report:" << reply->errorString();
        }
        reply->deleteLater();
    });
}
