#include "ApiClient.h"

namespace {
QString normalizeBaseUrl(const QString &url)
{
    QString trimmed = url.trimmed();
    while (trimmed.endsWith('/')) {
        trimmed.chop(1);
    }
    return trimmed;
}
} // namespace

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    QString envUrl = normalizeBaseUrl(qEnvironmentVariable("DESKMON_API_BASE_URL"));
    m_baseUrl = envUrl.isEmpty() ? defaultBaseUrl() : envUrl;
}

QString ApiClient::defaultBaseUrl()
{
    return QStringLiteral("https://deskmon.pranala-dt.co.id/api");
}

void ApiClient::setBaseUrl(const QString &baseUrl)
{
    QString normalized = normalizeBaseUrl(baseUrl);
    m_baseUrl = normalized.isEmpty() ? defaultBaseUrl() : normalized;
}

QUrl ApiClient::buildUrl(const QString &endpointPath) const
{
    if (endpointPath.isEmpty()) {
        return QUrl(m_baseUrl);
    }
    QString path = endpointPath.startsWith('/') ? endpointPath : ('/' + endpointPath);
    return QUrl(m_baseUrl + path);
}

QNetworkRequest ApiClient::createRequest(const QUrl &url, bool withAuth) const
{
    QUrl targetUrl = url;
    const QString defaultPrefix = defaultBaseUrl();
    const QString urlStr = url.toString();
    if (m_baseUrl != defaultPrefix && urlStr.startsWith(defaultPrefix)) {
        targetUrl = QUrl(m_baseUrl + urlStr.mid(defaultPrefix.length()));
    }

    QNetworkRequest request(targetUrl);
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
