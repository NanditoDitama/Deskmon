#include "AuthManager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>
#endif

namespace {

QString normalizeMacAddress(const QString &raw)
{
    const QString cleaned = raw.trimmed().toLower().replace(':', '-');
    const QStringList parts = cleaned.split('-');
    if (parts.size() != 6) return QString();

    QStringList padded;
    padded.reserve(6);
    for (const QString &p : parts) {
        bool ok = false;
        const int val = p.toInt(&ok, 16);
        if (!ok || val < 0 || val > 255) return QString();
        padded << QStringLiteral("%1").arg(val, 2, 16, QLatin1Char('0'));
    }

    const QString mac = padded.join('-');
    if (mac == QStringLiteral("00-00-00-00-00-00") || mac == QStringLiteral("ff-ff-ff-ff-ff-ff")) {
        return QString();
    }
    return mac;
}

void appendUniqueMac(QStringList &out, const QString &mac)
{
    const QString norm = normalizeMacAddress(mac);
    if (!norm.isEmpty() && !out.contains(norm)) {
        out.append(norm);
    }
}

} // namespace

AuthManager::AuthManager(ApiClient *apiClient, UserRepository *userRepo, QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_userRepo(userRepo)
{
}

void AuthManager::setGatewayMacProvider(GatewayMacProvider provider)
{
    m_gatewayMacProvider = std::move(provider);
}

QStringList AuthManager::collectGatewayMacs()
{
    QStringList macs;

#ifdef Q_OS_WIN
    ULONG tableSize = 0;
    if (GetIpForwardTable(nullptr, &tableSize, FALSE) == ERROR_INSUFFICIENT_BUFFER && tableSize > 0) {
        std::vector<BYTE> buffer(tableSize);
        auto *fwdTable = reinterpret_cast<PMIB_IPFORWARDTABLE>(buffer.data());
        if (GetIpForwardTable(fwdTable, &tableSize, FALSE) == NO_ERROR) {
            QList<DWORD> gatewayIps;
            for (DWORD i = 0; i < fwdTable->dwNumEntries; ++i) {
                const MIB_IPFORWARDROW &row = fwdTable->table[i];
                if (row.dwForwardDest == 0 && row.dwForwardNextHop != 0) {
                    if (!gatewayIps.contains(row.dwForwardNextHop)) {
                        gatewayIps.append(row.dwForwardNextHop);
                    }
                }
            }

            for (DWORD gwIp : gatewayIps) {
                ULONG macAddr[2] = {0, 0};
                ULONG macLen = 6;
                if (SendARP(gwIp, 0, macAddr, &macLen) == NO_ERROR && macLen == 6) {
                    const auto *b = reinterpret_cast<const unsigned char *>(macAddr);
                    const QString formatted = QString::asprintf("%02x-%02x-%02x-%02x-%02x-%02x",
                                                                b[0], b[1], b[2], b[3], b[4], b[5]);
                    appendUniqueMac(macs, formatted);
                }
            }

            if (macs.isEmpty() && !gatewayIps.isEmpty()) {
                ULONG netSize = 0;
                if (GetIpNetTable(nullptr, &netSize, FALSE) == ERROR_INSUFFICIENT_BUFFER && netSize > 0) {
                    std::vector<BYTE> netBuf(netSize);
                    auto *netTable = reinterpret_cast<PMIB_IPNETTABLE>(netBuf.data());
                    if (GetIpNetTable(netTable, &netSize, FALSE) == NO_ERROR) {
                        for (DWORD i = 0; i < netTable->dwNumEntries; ++i) {
                            const MIB_IPNETROW &row = netTable->table[i];
                            if (gatewayIps.contains(row.dwAddr) && row.dwPhysAddrLen == 6) {
                                const auto *b = row.bPhysAddr;
                                const QString formatted = QString::asprintf("%02x-%02x-%02x-%02x-%02x-%02x",
                                                                            b[0], b[1], b[2], b[3], b[4], b[5]);
                                appendUniqueMac(macs, formatted);
                            }
                        }
                    }
                }
            }
        }
    }
#elif defined(Q_OS_MACOS)
    QStringList gatewayIps;
    QProcess routeProc;
    routeProc.start(QStringLiteral("route"), {QStringLiteral("-n"), QStringLiteral("get"), QStringLiteral("default")});
    if (routeProc.waitForFinished(500) && routeProc.exitCode() == 0) {
        const QString out = QString::fromUtf8(routeProc.readAllStandardOutput());
        static const QRegularExpression gwRe(QStringLiteral(R"(gateway:\s*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+))"));
        const QRegularExpressionMatch m = gwRe.match(out);
        if (m.hasMatch()) {
            gatewayIps << m.captured(1);
        }
    }

    static const QRegularExpression macRe(QStringLiteral(R"(([0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}))"));
    for (const QString &gw : gatewayIps) {
        QProcess arpProc;
        arpProc.start(QStringLiteral("arp"), {QStringLiteral("-n"), gw});
        if (arpProc.waitForFinished(500) && arpProc.exitCode() == 0) {
            const QString arpOut = QString::fromUtf8(arpProc.readAllStandardOutput());
            const QRegularExpressionMatch m = macRe.match(arpOut);
            if (m.hasMatch()) {
                appendUniqueMac(macs, m.captured(1));
            }
        }
    }
#else
    QStringList gatewayIps;
    QFile routeFile(QStringLiteral("/proc/net/route"));
    if (routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bool firstLine = true;
        while (!routeFile.atEnd()) {
            const QString line = QString::fromUtf8(routeFile.readLine()).trimmed();
            if (firstLine) {
                firstLine = false;
                continue;
            }
            const QStringList cols = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (cols.size() >= 3 && cols[1] == QStringLiteral("00000000")) {
                bool ok = false;
                const quint32 gwVal = cols[2].toUInt(&ok, 16);
                if (ok && gwVal != 0) {
                    const QString ip = QStringLiteral("%1.%2.%3.%4")
                        .arg(gwVal & 0xFF)
                        .arg((gwVal >> 8) & 0xFF)
                        .arg((gwVal >> 16) & 0xFF)
                        .arg((gwVal >> 24) & 0xFF);
                    if (!gatewayIps.contains(ip)) gatewayIps << ip;
                }
            }
        }
    }

    if (!gatewayIps.isEmpty()) {
        QFile arpFile(QStringLiteral("/proc/net/arp"));
        if (arpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            bool firstLine = true;
            while (!arpFile.atEnd()) {
                const QString line = QString::fromUtf8(arpFile.readLine()).trimmed();
                if (firstLine) {
                    firstLine = false;
                    continue;
                }
                const QStringList cols = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                if (cols.size() >= 4 && gatewayIps.contains(cols[0])) {
                    appendUniqueMac(macs, cols[3]);
                }
            }
        }
    }

    if (macs.isEmpty()) {
        QProcess ipRoute;
        ipRoute.start(QStringLiteral("ip"), {QStringLiteral("route"), QStringLiteral("show"), QStringLiteral("default")});
        if (ipRoute.waitForFinished(400) && ipRoute.exitCode() == 0) {
            const QString routeOut = QString::fromUtf8(ipRoute.readAllStandardOutput());
            static const QRegularExpression viaRe(QStringLiteral(R"(via\s+([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+))"));
            auto it = viaRe.globalMatch(routeOut);
            while (it.hasNext()) {
                const QString gw = it.next().captured(1);
                QProcess neighProc;
                neighProc.start(QStringLiteral("ip"), {QStringLiteral("neigh"), QStringLiteral("show"), gw});
                if (neighProc.waitForFinished(400) && neighProc.exitCode() == 0) {
                    const QString neighOut = QString::fromUtf8(neighProc.readAllStandardOutput());
                    static const QRegularExpression macRe(QStringLiteral(R"(([0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5}))"));
                    const QRegularExpressionMatch mm = macRe.match(neighOut);
                    if (mm.hasMatch()) appendUniqueMac(macs, mm.captured(1));
                }
            }
        }
    }
#endif

    return macs;
}

bool AuthManager::tryAutoLogin()
{
    int userId = -1;
    QString username, email, token;
    if (m_userRepo && m_userRepo->findUserWithToken(userId, username, email, token)) {
        m_isTokenErrorVisible = false;
        if (!email.isEmpty()) {
            m_userRepo->saveLastLoginEmail(email);
        }
        setCurrentUserInfo(userId, username, email, token);
        qDebug() << "Auto login as user ID:" << userId << "Username:" << username;
        emit loggedIn();
        return true;
    }
    return false;
}

void AuthManager::setCurrentUserInfo(int userId, const QString &username, const QString &email, const QString &token)
{
    m_currentUserId = userId;
    m_currentUsername = username;
    m_currentUserEmail = email;
    m_userEmail = email;
    m_authToken = token;

    if (m_apiClient) {
        m_apiClient->setAuthToken(token);
    }

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();
    emit authTokenChanged();

    qDebug() << "Current user set - ID:" << userId << ", Username:" << username << ", Email:" << email;
}

QString AuthManager::authenticate(const QString &loginInput, const QString &password)
{
    bool isEmail = loginInput.contains("@");
    QString loginType = isEmail ? "email" : "username";
    qDebug() << "Attempting login with" << loginType << ":" << loginInput;

    QJsonObject jsonPayload;
    if (isEmail) {
        jsonPayload["email"] = loginInput;
    } else {
        QString email = m_userRepo ? m_userRepo->getEmail(loginInput) : "";
        jsonPayload["email"] = email.isEmpty() ? loginInput : email;
    }
    jsonPayload["password"] = password;
    const QStringList gatewayMacs = m_gatewayMacProvider ? m_gatewayMacProvider() : collectGatewayMacs();
    jsonPayload["gateway_macs"] = QJsonArray::fromStringList(gatewayMacs);
    QJsonDocument doc(jsonPayload);
    QByteArray data = doc.toJson();

    qDebug() << "Sending login request for" << loginType << ":" << jsonPayload["email"].toString()
             << "with" << gatewayMacs.size() << "gateway MAC(s)";

    QNetworkReply *reply = m_apiClient->post(m_apiClient->buildUrl("/login"), data, false);
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return QStringLiteral("Koneksi ke server gagal atau timeout. Periksa koneksi internet Anda.");
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response);

    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        qWarning() << "Invalid JSON response. Network error:" << reply->errorString();
        reply->deleteLater();
        return QStringLiteral("Koneksi ke server gagal. Periksa koneksi internet, username, dan password Anda.");
    }

    QJsonObject jsonObj = jsonResponse.object();
    qDebug() << "API login response received. success:" << jsonObj.value("success").toBool()
             << "message:" << jsonObj.value("message").toString();

    if (jsonObj.contains("success") && jsonObj["success"].toBool()) {
        qDebug() << "API login successful. Storing user data...";

        QJsonObject userData = jsonObj["user"].toObject();
        int userId = userData["id"].toInt();
        QString username = userData["name"].toString();
        QString userEmail = userData["email"].toString();
        QString role = userData["role"].toObject()["rolename"].toString();
        QString department = userData["department"].toObject()["rolename"].toString();
        QString token = jsonObj["token"].toString();

        if (m_userRepo) {
            m_userRepo->saveUser(userId, username, password, userEmail, department, role, token);
            const QString rememberedEmail = !userEmail.isEmpty() ? userEmail : loginInput.trimmed();
            if (!rememberedEmail.isEmpty()) {
                m_userRepo->saveLastLoginEmail(rememberedEmail);
            }
        }

        m_isTokenErrorVisible = false;
        setCurrentUserInfo(userId, username, userEmail, token);
        emit loggedIn();

        reply->deleteLater();
        return ""; // Success
    } else {
        QString serverMessage = jsonObj.value("message").toString();
        if (serverMessage == QStringLiteral("Login not allowed from this IP address")) {
            QString ipAddress = jsonObj.value("your_ip").toString();
            if (!ipAddress.isEmpty()) {
                serverMessage.append(". " + ipAddress);
            }
        }
        reply->deleteLater();
        return serverMessage.isEmpty() ? QStringLiteral("Username atau password salah.") : serverMessage;
    }
}

void AuthManager::logout()
{
    if (m_currentUserId == -1 && m_authToken.isEmpty()) {
        return;
    }

    sendLogoutToAPI();

    if (!m_currentUserEmail.isEmpty() && m_userRepo) {
        m_userRepo->saveLastLoginEmail(m_currentUserEmail);
    }

    if (m_currentUserId != -1 && m_userRepo) {
        m_userRepo->clearToken(m_currentUserId);
    }

    m_currentUserId = -1;
    m_currentUsername.clear();
    m_currentUserEmail.clear();
    m_userEmail.clear();
    m_authToken.clear();
    if (m_apiClient) m_apiClient->clearAuthToken();

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();
    emit authTokenChanged();
    emit loggedOut();

    qDebug() << "User logged out";
}

void AuthManager::sendLogoutToAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) return;

    QEventLoop loop;
    QNetworkReply *reply = m_apiClient->post(m_apiClient->buildUrl("/logout"), QJsonDocument(QJsonObject()).toJson(), true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Logout API success:" << reply->readAll();
    } else {
        qWarning() << "Logout API failed:" << reply->errorString();
    }
    reply->deleteLater();
}

QString AuthManager::savedUsername() const
{
    if (!m_userRepo) return "";
    int userId = -1;
    QString username, email, token;
    if (m_userRepo->findUserWithToken(userId, username, email, token) && !email.isEmpty()) {
        return email;
    }
    return m_userRepo->lastLoginEmail();
}

QString AuthManager::savedPassword() const
{
    int userId = -1;
    QString username, email, token;
    if (m_userRepo && m_userRepo->findUserWithToken(userId, username, email, token)) {
        return m_userRepo->getPassword(username);
    }
    return "";
}

QString AuthManager::getUserEmail(const QString &username) const
{
    return m_userRepo ? m_userRepo->getEmail(username) : "";
}

QString AuthManager::getUserDepartment(const QString &username) const
{
    return m_userRepo ? m_userRepo->getDepartment(username) : "";
}

QString AuthManager::getUsernameById(int userId) const
{
    return m_userRepo ? m_userRepo->getUsernameById(userId) : "";
}

QString AuthManager::getUserPassword(const QString &username) const
{
    return m_userRepo ? m_userRepo->getPassword(username) : "";
}

bool AuthManager::isUsernameTaken(const QString &username) const
{
    return m_userRepo ? m_userRepo->isUsernameTaken(username) : false;
}

void AuthManager::showAuthTokenErrorMessage()
{
    if (m_isTokenErrorVisible) return;
    m_isTokenErrorVisible = true;
    emit showAuthTokenErrorWindow("Sesi Anda telah berakhir atau tidak valid.\nSilakan login ulang untuk melanjutkan.");
}

void AuthManager::clearToken()
{
    m_authToken.clear();
    if (m_apiClient) m_apiClient->clearAuthToken();
    if (m_currentUserId > 0 && m_userRepo) {
        m_userRepo->clearToken(m_currentUserId);
    }
}
