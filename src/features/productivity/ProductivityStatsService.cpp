#include "ProductivityStatsService.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ProductivityStatsService::ProductivityStatsService(ApiClient *apiClient,
                                                   WorkLogRepository *workLogRepo,
                                                   ProductivityAppRepository *prodRepo,
                                                   AuthManager *authManager,
                                                   QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_workLogRepo(workLogRepo)
    , m_prodRepo(prodRepo)
    , m_authManager(authManager)
{
    m_productivePingTimer.setInterval(30000); // 30 detik
    connect(&m_productivePingTimer, &QTimer::timeout, this, &ProductivityStatsService::sendProductiveTimeToAPI);

    m_usageReportTimer.setInterval(300000); // 5 menit
    connect(&m_usageReportTimer, &QTimer::timeout, this, &ProductivityStatsService::sendDailyUsageReport);

    connect(m_authManager, &AuthManager::loggedIn, this, &ProductivityStatsService::startTimers);
    connect(m_authManager, &AuthManager::loggedOut, this, &ProductivityStatsService::stopTimers);
}

void ProductivityStatsService::startTimers()
{
    m_productivePingTimer.start();
    m_usageReportTimer.start();
}

void ProductivityStatsService::stopTimers()
{
    m_productivePingTimer.stop();
    m_usageReportTimer.stop();
}

int ProductivityStatsService::calculateTodayProductiveSeconds() const
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_workLogRepo->ensureDatabaseOpen() || !m_prodRepo->ensureDatabaseOpen()) {
        return 0;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    int totalProductiveSeconds = 0;

    QSqlQuery query(m_workLogRepo->database());
    query.prepare(R"(
        SELECT IFNULL(SUM(a.end_time - a.start_time), 0) AS productive_seconds
        FROM activity_logs a
        JOIN productivity_apps p
        ON (
            (a.url IS NOT NULL AND a.url != '' AND p.url IS NOT NULL AND p.url != ''
             AND LOWER(a.url) LIKE '%' || LOWER(p.url) || '%')
            OR
            (a.app_name IS NOT NULL AND a.app_name != '' AND p.app_name IS NOT NULL AND p.app_name != ''
             AND LOWER(a.app_name) LIKE '%' || LOWER(p.app_name) || '%')
        )
        WHERE a.user_id = :user_id
          AND date(a.start_time, 'unixepoch', 'localtime') = :today
          AND p.productivity_type = 1
    )");

    query.bindValue(":user_id", userId);
    query.bindValue(":today", today);

    if (query.exec() && query.next()) {
        totalProductiveSeconds = query.value(0).toInt();
    } else {
        qWarning() << "Failed to calculate productive seconds:" << query.lastError().text();
    }

    return totalProductiveSeconds;
}

QVariantMap ProductivityStatsService::productivityStats(const QString &startDateFilter, const QString &endDateFilter) const
{
    int userId = m_authManager->currentUserId();
    if (!m_workLogRepo->ensureDatabaseOpen() || !m_prodRepo->ensureDatabaseOpen() || userId == -1) {
        return QVariantMap();
    }

    QVariantMap stats;
    double productiveTime = 0;
    double nonProductiveTime = 0;
    double neutralTime = 0;
    double idleTime = 0;
    double totalTime = 0;

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs "
                       "WHERE app_name IS NOT NULL AND user_id = :user_id ";

    if (!startDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ").arg(startDateFilter);
    }
    if (!endDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ").arg(endDateFilter);
    }

    QSqlQuery query(m_workLogRepo->database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);

    if (!query.exec()) {
        return stats;
    }

    while (query.next()) {
        qint64 start = query.value(0).toLongLong();
        qint64 end = query.value(1).toLongLong();
        QString appName = query.value(2).toString();
        QString url = query.value(4).toString();

        double duration = end - start;
        if (duration <= 0) continue;

        if (appName == "Idle") {
            idleTime += duration;
            totalTime += duration;
            continue;
        }

        int type = m_prodRepo->getAppProductivityType(appName, url, userId);
        switch (type) {
        case 1: productiveTime += duration; break;
        case 2: nonProductiveTime += duration; break;
        default: neutralTime += duration; break;
        }
        totalTime += duration;
    }

    double total = totalTime > 0 ? totalTime : 1;
    stats["productive"] = (productiveTime / total) * 100;
    stats["nonProductive"] = (nonProductiveTime / total) * 100;
    stats["neutral"] = (neutralTime / total) * 100;
    stats["idle"] = (idleTime / total) * 100;
    return stats;
}

void ProductivityStatsService::sendProductiveTimeToAPI()
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) return;

    int productiveSeconds = calculateTodayProductiveSeconds();

    QJsonObject payload;
    payload["user_id"] = userId;
    payload["productive_time"] = productiveSeconds;

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/send-productive-time"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void ProductivityStatsService::sendDailyUsageReport()
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) return;
    if (!m_workLogRepo->ensureDatabaseOpen() || !m_prodRepo->ensureDatabaseOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    QHash<QString, qint64> appDurations;
    QHash<QString, int> appProductivity;
    QHash<QPair<QString, QString>, qint64> browserUsage;
    QHash<QString, int> domainProductivity;

    QSqlQuery logQuery(m_workLogRepo->database());
    logQuery.prepare(R"(
        SELECT app_name, title, url, start_time, end_time
        FROM activity_logs
        WHERE user_id = :user_id
        AND date(start_time, 'unixepoch', 'localtime') = :today
        AND app_name != 'Idle'
    )");
    logQuery.bindValue(":user_id", userId);
    logQuery.bindValue(":today", today);

    if (!logQuery.exec()) return;

    auto extractDomain = [](const QString& url) -> QString {
        if (url.isEmpty()) return "";
        QString cleanUrl = url;
        if (!cleanUrl.startsWith("http://") && !cleanUrl.startsWith("https://")) {
            cleanUrl = "https://" + cleanUrl;
        }
        QUrl qurl(cleanUrl);
        QString host = qurl.host();
        if (host.startsWith("www.")) host = host.mid(4);
        return host.toLower();
    };

    while (logQuery.next()) {
        QString appName = logQuery.value(0).toString();
        QString urlString = logQuery.value(2).toString();
        qint64 start = logQuery.value(3).toLongLong();
        qint64 end = logQuery.value(4).toLongLong();
        qint64 duration = end - start;
        if (duration <= 0) continue;

        bool isBrowserApp = !urlString.isEmpty();
        if (isBrowserApp) {
            QString domain = extractDomain(urlString);
            QPair<QString, QString> key(appName, domain);
            browserUsage[key] += duration;
            if (!domainProductivity.contains(domain)) {
                domainProductivity[domain] = m_prodRepo->getAppProductivityType(appName, urlString, userId);
            }
        } else {
            appDurations[appName] += duration;
            if (!appProductivity.contains(appName)) {
                appProductivity[appName] = m_prodRepo->getAppProductivityType(appName, "", userId);
            }
        }
    }

    QJsonArray dataArray;
    for (auto it = appDurations.constBegin(); it != appDurations.constEnd(); ++it) {
        int prodType = appProductivity.value(it.key(), 0);
        QString statusString = "neutral";
        if (prodType == 1) statusString = "productive";
        else if (prodType == 2) statusString = "non-productive";

        QJsonObject appObject;
        appObject["user_id"] = userId;
        appObject["app_name"] = it.key();
        appObject["duration"] = it.value();
        appObject["url"] = QJsonValue();
        appObject["status"] = statusString;
        dataArray.append(appObject);
    }

    for (auto it = browserUsage.constBegin(); it != browserUsage.constEnd(); ++it) {
        QString appName = it.key().first;
        QString domain = it.key().second;
        int prodType = domainProductivity.value(domain, 0);
        QString statusString = "neutral";
        if (prodType == 1) statusString = "productive";
        else if (prodType == 2) statusString = "non-productive";

        QJsonObject browserObject;
        browserObject["user_id"] = userId;
        browserObject["app_name"] = appName;
        browserObject["duration"] = it.value();
        browserObject["url"] = domain;
        browserObject["status"] = statusString;
        dataArray.append(browserObject);
    }

    if (dataArray.isEmpty()) return;

    QJsonObject payload;
    payload["data"] = dataArray;

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/productivity-app"), QJsonDocument(payload).toJson(), true);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}
