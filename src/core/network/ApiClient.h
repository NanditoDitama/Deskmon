#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QByteArray>
#include <QString>

class ApiClient : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient() override = default;

    QNetworkAccessManager* networkManager() const { return m_networkManager; }

    void setAuthToken(const QString &token) { m_authToken = token; }
    QString authToken() const { return m_authToken; }
    void clearAuthToken() { m_authToken.clear(); }

    QNetworkRequest createRequest(const QUrl &url, bool withAuth = true) const;
    QNetworkReply* get(const QUrl &url, bool withAuth = true);
    QNetworkReply* post(const QUrl &url, const QByteArray &data, bool withAuth = true);
    QNetworkReply* put(const QUrl &url, const QByteArray &data, bool withAuth = true);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_authToken;
};

#endif // APICLIENT_H
