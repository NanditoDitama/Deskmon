#include "httpclient.h"
#include <QUrlQuery>

HttpClient::HttpClient(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

QNetworkReply* HttpClient::login(const QString &email, const QString &password)
{
    QNetworkRequest request(QUrl("YOUR_API_ENDPOINT_HERE/login")); // Ganti dengan endpoint API Anda
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObject;
    jsonObject["email"] = email;
    jsonObject["password"] = password;
    QJsonDocument doc(jsonObject);
    QByteArray data = doc.toJson();

    return manager->post(request, data);
}
