#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &username, const QString &password);

signals:
    void loginSuccess(const QString &token, const QJsonObject &user);
    void loginFailed(const QString &errorMessage);

private slots:
    void onLoginReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
};

#endif // NETWORKMANAGER_H
