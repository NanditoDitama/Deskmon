#include "ApiClient.h"

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

QNetworkRequest ApiClient::createRequest(const QUrl &url, bool withAuth) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (withAuth && !m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }
    return request;
}

QNetworkReply* ApiClient::get(const QUrl &url, bool withAuth)
{
    QNetworkRequest request = createRequest(url, withAuth);
    return m_networkManager->get(request);
}

QNetworkReply* ApiClient::post(const QUrl &url, const QByteArray &data, bool withAuth)
{
    QNetworkRequest request = createRequest(url, withAuth);
    return m_networkManager->post(request, data);
}

QNetworkReply* ApiClient::put(const QUrl &url, const QByteArray &data, bool withAuth)
{
    QNetworkRequest request = createRequest(url, withAuth);
    return m_networkManager->put(request, data);
}
