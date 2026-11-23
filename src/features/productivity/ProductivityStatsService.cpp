#include "ProductivityStatsService.h"
#include "core/database/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QUrl>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>
#include <QDebug>
#include <atomic>

namespace {
QString extractDomainForReport(const QString &url)
{
    if (url.isEmpty()) return QString();
    QString cleanUrl = url;
    if (!cleanUrl.startsWith(QStringLiteral("http://")) && !cleanUrl.startsWith(QStringLiteral("https://"))) {
        cleanUrl = QStringLiteral("https://") + cleanUrl;
    }
    QUrl qurl(cleanUrl);
    QString host = qurl.host();
    if (host.isEmpty()) {
        QString temp = url;
        static const QRegularExpression protoRe(QStringLiteral("^https?://"));
        temp.remove(protoRe);
        int slashPos = temp.indexOf(QLatin1Char('/'));
        if (slashPos != -1) temp = temp.left(slashPos);
        int queryPos = temp.indexOf(QLatin1Char('?'));
        if (queryPos != -1) temp = temp.left(queryPos);
        int fragmentPos = temp.indexOf(QLatin1Char('#'));
        if (fragmentPos != -1) temp = temp.left(fragmentPos);
        host = temp;
    }
    if (host.startsWith(QStringLiteral("www."))) host = host.mid(4);
    return host.toLower();
}

QByteArray buildDailyUsageReportPayload(QSqlDatabase db,
                                        int userId,
                                        const QString &today,
                                        const QHash<QString, int> &appTypes,
                                        const QHash<QString, int> &domainTypes)
{
    if (!db.isOpen() || userId == -1) return QByteArray();

    qint64 startEpoch = 0;
    qint64 endExclusiveEpoch = 0;
    bool hasStart = false;
    bool hasEnd = false;
    WorkLogRepository::dateRangeToEpoch(today, today, startEpoch, hasStart, endExclusiveEpoch, hasEnd);

    QHash<QString, qint64> appDurations;
    QHash<QString, int> appProductivity;
    QHash<QPair<QString, QString>, qint64> browserUsage;
    QHash<QString, int> domainProductivity;

    QSqlQuery logQuery(db);
    logQuery.prepare(QStringLiteral(R"(
        SELECT app_name, title, url, start_time, end_time
        FROM activity_logs
        WHERE user_id = :user_id
          AND start_time >= :startEpoch
          AND start_time < :endEpoch
          AND app_name != 'Idle'
    )"));
    logQuery.bindValue(":user_id", userId);
    logQuery.bindValue(":startEpoch", startEpoch);
    logQuery.bindValue(":endEpoch", endExclusiveEpoch);

    if (!logQuery.exec()) return QByteArray();

    while (logQuery.next()) {
        const QString appName = logQuery.value(0).toString();
        const QString urlString = logQuery.value(2).toString();
        const qint64 start = logQuery.value(3).toLongLong();
        const qint64 end = logQuery.value(4).toLongLong();
        const qint64 duration = end - start;
        if (duration <= 0) continue;

        if (!urlString.isEmpty()) {
            const QString domain = extractDomainForReport(urlString);
            const QPair<QString, QString> key(appName, domain);
            browserUsage[key] += duration;
            if (!domainProductivity.contains(domain)) {
                domainProductivity[domain] = ProductivityAppRepository::lookupProductivityType(appName, urlString, appTypes, domainTypes);
            }
        } else {
            appDurations[appName] += duration;
            if (!appProductivity.contains(appName)) {
                appProductivity[appName] = ProductivityAppRepository::lookupProductivityType(appName, QString(), appTypes, domainTypes);
            }
        }
    }

    QJsonArray dataArray;
    for (auto it = appDurations.constBegin(); it != appDurations.constEnd(); ++it) {
        const int prodType = appProductivity.value(it.key(), 0);
        QString statusString = QStringLiteral("neutral");
        if (prodType == 1) statusString = QStringLiteral("productive");
        else if (prodType == 2) statusString = QStringLiteral("non-productive");

        QJsonObject appObject;
        appObject["user_id"] = userId;
        appObject["app_name"] = it.key();
        appObject["duration"] = it.value();
        appObject["url"] = QJsonValue();
        appObject["status"] = statusString;
        dataArray.append(appObject);
    }

    for (auto it = browserUsage.constBegin(); it != browserUsage.constEnd(); ++it) {
        const QString &appName = it.key().first;
        const QString &domain = it.key().second;
        const int prodType = domainProductivity.value(domain, 0);
        QString statusString = QStringLiteral("neutral");
        if (prodType == 1) statusString = QStringLiteral("productive");
        else if (prodType == 2) statusString = QStringLiteral("non-productive");

        QJsonObject browserObject;
        browserObject["user_id"] = userId;
        browserObject["app_name"] = appName;
        browserObject["duration"] = it.value();
        browserObject["url"] = domain;
        browserObject["status"] = statusString;
        dataArray.append(browserObject);
    }

    if (dataArray.isEmpty()) return QByteArray();

    QJsonObject payload;
    payload["data"] = dataArray;
    return QJsonDocument(payload).toJson();
}
} // namespace

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
    m_productivePingTimer.setInterval(180000); // 180 detik (3 menit)
    connect(&m_productivePingTimer, &QTimer::timeout, this, &ProductivityStatsService::sendProductiveTimeToAPI);

    m_usageReportTimer.setInterval(300000); // 5 menit
    connect(&m_usageReportTimer, &QTimer::timeout, this, [this]() {
        sendDailyUsageReport(true);
    });

    if (m_workLogRepo) {
        connect(m_workLogRepo, &WorkLogRepository::logsChanged, this, &ProductivityStatsService::invalidateStatsCache);
    }
    if (m_prodRepo) {
        connect(m_prodRepo, &ProductivityAppRepository::productivityAppsChanged, this, &ProductivityStatsService::invalidateStatsCache);
    }
    if (m_authManager) {
        connect(m_authManager, &AuthManager::currentUserIdChanged, this, &ProductivityStatsService::invalidateStatsCache);
        connect(m_authManager, &AuthManager::loggedIn, this, &ProductivityStatsService::startTimers);
        connect(m_authManager, &AuthManager::loggedOut, this, &ProductivityStatsService::invalidateStatsCache);
        connect(m_authManager, &AuthManager::loggedOut, this, &ProductivityStatsService::stopTimers);
    }
}

void ProductivityStatsService::invalidateStatsCache()
{
    m_statsCacheValid = false;
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

    m_prodRepo->updateProductivityCache(userId);

    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    qint64 startEpoch = 0;
    qint64 endExclusiveEpoch = 0;
    bool hasStart = false;
    bool hasEnd = false;
    WorkLogRepository::dateRangeToEpoch(today, today, startEpoch, hasStart, endExclusiveEpoch, hasEnd);

    qint64 totalProductiveSeconds = 0;

    QSqlQuery query(m_workLogRepo->database());
    query.prepare(R"(
        SELECT start_time, end_time, app_name, url
        FROM activity_logs
        WHERE user_id = :user_id
          AND app_name IS NOT NULL
          AND app_name != 'Idle'
          AND start_time >= :startEpoch
          AND start_time < :endEpoch
    )");

    query.bindValue(":user_id", userId);
    query.bindValue(":startEpoch", startEpoch);
    query.bindValue(":endEpoch", endExclusiveEpoch);

    if (query.exec()) {
        while (query.next()) {
            const qint64 start = query.value(0).toLongLong();
            const qint64 end = query.value(1).toLongLong();
            const qint64 duration = end - start;
            if (duration <= 0) continue;

            const QString appName = query.value(2).toString();
            const QString url = query.value(3).toString();
            if (m_prodRepo->getAppProductivityType(appName, url, userId) == 1) {
                totalProductiveSeconds += duration;
            }
        }
    } else {
        qWarning() << "Failed to calculate productive seconds:" << query.lastError().text();
    }

    return static_cast<int>(totalProductiveSeconds);
}

QVariantMap ProductivityStatsService::productivityStats(const QString &startDateFilter, const QString &endDateFilter) const
{
    int userId = m_authManager->currentUserId();
    if (!m_workLogRepo->ensureDatabaseOpen() || !m_prodRepo->ensureDatabaseOpen() || userId == -1) {
        return QVariantMap();
    }

    if (m_statsCacheValid &&
        m_cachedStatsUserId == userId &&
        m_cachedStartFilter == startDateFilter &&
        m_cachedEndFilter == endDateFilter) {
        return m_cachedStats;
    }

    m_prodRepo->updateProductivityCache(userId);

    qint64 startEpoch = 0;
    qint64 endExclusiveEpoch = 0;
    bool hasStartEpoch = false;
    bool hasEndEpoch = false;
    WorkLogRepository::dateRangeToEpoch(startDateFilter, endDateFilter, startEpoch, hasStartEpoch, endExclusiveEpoch, hasEndEpoch);

    QVariantMap stats;
    double productiveTime = 0;
    double nonProductiveTime = 0;
    double neutralTime = 0;
    double idleTime = 0;
    double totalTime = 0;

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs "
                       "WHERE app_name IS NOT NULL AND user_id = :user_id ";

    if (!startDateFilter.isEmpty()) {
        queryStr += hasStartEpoch ? "AND start_time >= :startEpoch "
                                  : "AND date(start_time, 'unixepoch', 'localtime') >= date(:startDate) ";
    }
    if (!endDateFilter.isEmpty()) {
        queryStr += hasEndEpoch ? "AND start_time < :endEpoch "
                                : "AND date(start_time, 'unixepoch', 'localtime') <= date(:endDate) ";
    }

    QSqlQuery query(m_workLogRepo->database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
    if (!startDateFilter.isEmpty()) {
        if (hasStartEpoch) query.bindValue(":startEpoch", startEpoch);
        else query.bindValue(":startDate", startDateFilter);
    }
    if (!endDateFilter.isEmpty()) {
        if (hasEndEpoch) query.bindValue(":endEpoch", endExclusiveEpoch);
        else query.bindValue(":endDate", endDateFilter);
    }

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

    m_cachedStats = stats;
    m_cachedStatsUserId = userId;
    m_cachedStartFilter = startDateFilter;
    m_cachedEndFilter = endDateFilter;
    m_statsCacheValid = true;
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

    QNetworkReply *reply = m_apiClient->post(m_apiClient->buildUrl("/send-productive-time"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    emit refreshRequested();
}

void ProductivityStatsService::sendDailyUsageReport(bool async)
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) return;
    if (!m_workLogRepo->ensureDatabaseOpen() || !m_prodRepo->ensureDatabaseOpen()) return;

    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QHash<QString, int> appTypes;
    QHash<QString, int> domainTypes;
    m_prodRepo->copyCachesForUser(userId, appTypes, domainTypes);

    const QString dbPath = DatabaseManager::instance().databasePath();
    if (!async || dbPath.isEmpty()) {
        const QByteArray payloadBytes = buildDailyUsageReportPayload(m_workLogRepo->database(), userId, today, appTypes, domainTypes);
        if (payloadBytes.isEmpty()) return;
        QNetworkReply *reply = m_apiClient->post(m_apiClient->buildUrl("/productivity-app"), payloadBytes, true);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        return;
    }

    static std::atomic<quint64> s_reportSeq{0};
    const quint64 seq = ++s_reportSeq;
    const QString connName = QStringLiteral("worker_usage_report_%1").arg(seq);
    QPointer<ProductivityStatsService> self(this);

    QThreadPool::globalInstance()->start([self, dbPath, connName, userId, today, appTypes, domainTypes]() {
        QByteArray payloadBytes;
        {
            QSqlDatabase workerDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
            workerDb.setDatabaseName(dbPath);
            if (workerDb.open()) {
                QSqlQuery pragmaQ(workerDb);
                pragmaQ.exec(QStringLiteral("PRAGMA busy_timeout = 5000;"));
                payloadBytes = buildDailyUsageReportPayload(workerDb, userId, today, appTypes, domainTypes);
                workerDb.close();
            }
        }
        QSqlDatabase::removeDatabase(connName);

        if (payloadBytes.isEmpty() || !self) return;

        QMetaObject::invokeMethod(self, [self, userId, payloadBytes]() {
            if (!self || !self->m_authManager || !self->m_apiClient) return;
            if (self->m_authManager->currentUserId() != userId || self->m_authManager->authToken().isEmpty()) return;

            QNetworkReply *reply = self->m_apiClient->post(self->m_apiClient->buildUrl("/productivity-app"), payloadBytes, true);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }, Qt::QueuedConnection);
    });
}
