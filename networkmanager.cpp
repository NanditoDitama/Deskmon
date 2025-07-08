#include "networkmanager.h"
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void NetworkManager::login(const QString &username, const QString &password)
{
    // Ganti URL ini dengan URL server Anda
    QUrl url("https://task-planner.bayueka.com/api/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Membuat data JSON untuk dikirim
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    // Mengirim POST request
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLoginReply(reply);
    });
}

void NetworkManager::onLoginReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        // Membaca respons dari server
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject json = doc.object();

        if (json["success"].toBool()) {
            QString token = json["token"].toString();
            QJsonObject user = json["user"].toObject();
            emit loginSuccess(token, user);
        } else {
            emit loginFailed(json["message"].toString());
        }
    } else {
        emit loginFailed(reply->errorString());
    }
    reply->deleteLater();
}
