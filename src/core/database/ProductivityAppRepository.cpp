#include "ProductivityAppRepository.h"
#include "DatabaseManager.h"
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>
#include <QUrl>
#include <QJsonObject>

ProductivityAppRepository::ProductivityAppRepository(QObject *parent)
    : QObject(parent)
    , m_productiveAppsModel(new QSqlQueryModel(this))
    , m_nonProductiveAppsModel(new QSqlQueryModel(this))
{
}

bool ProductivityAppRepository::ensureDatabaseOpen() const
{
    return DatabaseManager::instance().ensureOpen();
}

QSqlDatabase ProductivityAppRepository::database() const
{
    return DatabaseManager::instance().database();
}

bool ProductivityAppRepository::initialize()
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    m_productiveAppsModel->setQuery(
        "SELECT app_name AS appName, window_title AS windowTitle, productivity_type AS type "
        "FROM productivity_apps WHERE productivity_type = 1",
        database());

    m_nonProductiveAppsModel->setQuery(
        "SELECT app_name AS appName, window_title AS windowTitle, productivity_type AS type "
        "FROM productivity_apps WHERE productivity_type = 2",
        database());

    return true;
}

void ProductivityAppRepository::refreshProductivityModels(int userId)
{
    if (!ensureDatabaseOpen()) return;

    QString productiveQuery = QString(
        "SELECT a.id, a.app_name AS appName, a.window_title AS windowTitle, a.url, a.productivity_type AS type "
        "FROM productivity_apps a "
        "WHERE a.productivity_type = 1 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
        "EXISTS (SELECT 1 FROM productivity_apps WHERE app_name = a.app_name AND "
        "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
        "GROUP BY a.app_name, COALESCE(a.url, '') "
        "ORDER BY a.for_user = '0' DESC").arg(userId);

    QString nonProductiveQuery = QString(
        "SELECT a.id, a.app_name AS appName, a.window_title AS windowTitle, a.url, a.productivity_type AS type "
        "FROM productivity_apps a "
        "WHERE a.productivity_type = 2 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
        "EXISTS (SELECT 1 FROM productivity_apps WHERE app_name = a.app_name AND "
        "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
        "GROUP BY a.app_name, COALESCE(a.url, '') "
        "ORDER BY a.for_user = '0' DESC").arg(userId);

    m_productiveAppsModel->setQuery(productiveQuery, database());
    m_nonProductiveAppsModel->setQuery(nonProductiveQuery, database());

    emit productivityAppsChanged();
}

void ProductivityAppRepository::updateProductivityCache(int userId)
{
    m_cachedAppTypes.clear();
    m_cachedDomainTypes.clear();

    if (!ensureDatabaseOpen() || userId == -1) return;

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT app_name, url, productivity_type, for_user
        FROM productivity_apps
        WHERE productivity_type IN (1, 2)
    )");

    if (!query.exec()) return;

    auto normalizeString = [](const QString &str) {
        return str.toLower().remove(' ').remove('-').remove('_').remove('.');
    };
    auto extractDomain = [](const QString &url) -> QString {
        if (url.isEmpty()) return "";
        QUrl qurl(url);
        QString domain = qurl.host();
        if (domain.isEmpty()) {
            QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
            QRegularExpressionMatch match = domainRegex.match(url);
            if (match.hasMatch()) {
                domain = match.captured(1);
            }
        }
        if (domain.startsWith("www.")) {
            domain = domain.mid(4);
        }
        return domain.toLower();
    };

    while (query.next()) {
        QString appName = query.value(0).toString();
        QString url = query.value(1).toString();
        int type = query.value(2).toInt();

        if (!url.isEmpty()) {
            QString domain = extractDomain(url);
            if (!domain.isEmpty()) {
                m_cachedDomainTypes[domain] = type;
            }
        } else if (!appName.isEmpty()) {
            m_cachedAppTypes[normalizeString(appName)] = type;
        }
    }
}

int ProductivityAppRepository::getAppProductivityType(const QString &appName, const QString &url, int userId) const
{
    if (m_cachedAppTypes.isEmpty() && m_cachedDomainTypes.isEmpty()) {
        const_cast<ProductivityAppRepository*>(this)->updateProductivityCache(userId);
    }

    auto normalizeString = [](const QString &str) {
        return str.toLower().remove(' ').remove('-').remove('_').remove('.');
    };
    auto extractDomain = [](const QString &url) -> QString {
        if (url.isEmpty()) return "";
        QUrl qurl(url);
        QString domain = qurl.host();
        if (domain.isEmpty()) {
            QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
            QRegularExpressionMatch match = domainRegex.match(url);
            if (match.hasMatch()) {
                domain = match.captured(1);
            }
        }
        if (domain.startsWith("www.")) {
            domain = domain.mid(4);
        }
        return domain.toLower();
    };

    if (!url.isEmpty()) {
        QString domain = extractDomain(url);
        if (!domain.isEmpty()) {
            if (m_cachedDomainTypes.contains(domain)) {
                return m_cachedDomainTypes[domain];
            }
            for (auto it = m_cachedDomainTypes.constBegin(); it != m_cachedDomainTypes.constEnd(); ++it) {
                if (domain.endsWith(it.key()) || domain.contains(it.key())) {
                    return it.value();
                }
            }
        }
    }

    if (!appName.isEmpty()) {
        QString normalizedInput = normalizeString(appName);
        if (m_cachedAppTypes.contains(normalizedInput)) {
            return m_cachedAppTypes[normalizedInput];
        }
        for (auto it = m_cachedAppTypes.constBegin(); it != m_cachedAppTypes.constEnd(); ++it) {
            if (normalizedInput.contains(it.key())) {
                return it.value();
            }
        }
    }

    return 0; // Neutral
}

QVariantList ProductivityAppRepository::getAvailableApps() const
{
    QVariantList apps;
    if (!ensureDatabaseOpen()) return apps;

    QSqlQuery query(database());
    query.prepare("SELECT app_name, url, productivity_type, for_user FROM productivity_apps");

    if (query.exec()) {
        while (query.next()) {
            apps.append(query.value(0).toString());
        }
    }
    return apps;
}

QVariantList ProductivityAppRepository::getProductivityApps(int userId) const
{
    QVariantList apps;
    if (!ensureDatabaseOpen()) return apps;

    QSqlQuery query(database());
    query.prepare(QString(
        "SELECT a.id, a.app_name, a.window_title, a.url, a.productivity_type "
        "FROM productivity_apps a "
        "WHERE (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
        "EXISTS (SELECT 1 FROM productivity_apps WHERE app_name = a.app_name AND "
        "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
        "GROUP BY a.app_name, COALESCE(a.url, '') "
        "ORDER BY a.for_user = '0' DESC").arg(userId));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap app;
            app["id"] = query.value(0).toInt();
            app["appName"] = query.value(1).toString();
            app["windowTitle"] = query.value(2).toString();
            app["url"] = query.value(3).toString();
            app["type"] = query.value(4).toInt();
            apps.append(app);
        }
    }
    return apps;
}

QVariantList ProductivityAppRepository::getPendingApplicationRequests() const
{
    QVariantList requests;
    if (!ensureDatabaseOpen()) return requests;

    QSqlQuery query(database());
    query.prepare("SELECT id, app_name, window_title, url, productivity_type, for_user FROM productivity_apps WHERE productivity_type = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap req;
            req["id"] = query.value(0).toInt();
            req["appName"] = query.value(1).toString();
            req["windowTitle"] = query.value(2).toString();
            req["url"] = query.value(3).toString();
            req["type"] = query.value(4).toInt();
            req["forUser"] = query.value(5).toString();
            requests.append(req);
        }
    }
    return requests;
}

bool ProductivityAppRepository::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (!ensureDatabaseOpen()) return false;

    QSqlQuery query(database());
    query.prepare("INSERT INTO productivity_apps (app_name, window_title, url, productivity_type) "
                  "VALUES (:app, :window, :url, :type)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":url", url.isEmpty() ? QVariant() : url);
    query.bindValue(":type", productivityType);

    return query.exec();
}

bool ProductivityAppRepository::storeProductivityAppsFromApi(const QJsonArray &appsArray, int userId)
{
    if (!ensureDatabaseOpen()) return false;

    DatabaseManager::instance().transaction();
    QSqlQuery query(database());

    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) continue;
        QJsonObject appObj = appValue.toObject();
        QString appName = appObj["application_name"].toString();
        QString processName = appObj["process_name"].toString();
        QString url = appObj["url"].toString();
        QString status = appObj["productivity_status"].toString().toLower();
        int type = (status == "productive") ? 1 : (status == "non-productive" ? 2 : 0);
        QString forUsers = appObj.contains("for_user") ? appObj["for_user"].toString() : "0";

        query.prepare("INSERT INTO productivity_apps (app_name, window_title, url, productivity_type, for_user) "
                      "VALUES (:app, :window, :url, :type, :forUsers)");
        query.bindValue(":app", appName);
        query.bindValue(":window", (processName.isEmpty() || processName == "N/A") ? QVariant() : processName);
        query.bindValue(":url", (url.isEmpty() || url == "N/A") ? QVariant() : url);
        query.bindValue(":type", type);
        query.bindValue(":forUsers", forUsers);
        query.exec();
    }

    DatabaseManager::instance().commit();
    refreshProductivityModels(userId);
    updateProductivityCache(userId);
    return true;
}

int ProductivityAppRepository::getIdleThreshold() const
{
    if (!ensureDatabaseOpen()) return 180;

    QSqlQuery query(database());
    query.prepare("SELECT threshold_seconds FROM idle_settings LIMIT 1");
    if (query.exec() && query.next()) {
        int val = query.value(0).toInt();
        return val > 0 ? val : 180;
    }
    return 180;
}

void ProductivityAppRepository::setIdleThreshold(int seconds)
{
    if (!ensureDatabaseOpen() || seconds <= 0) return;

    QSqlQuery query(database());
    query.prepare("UPDATE idle_settings SET threshold_seconds = :seconds WHERE id = (SELECT id FROM idle_settings LIMIT 1)");
    query.bindValue(":seconds", seconds);
    if (!query.exec() || query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO idle_settings (threshold_seconds) VALUES (:seconds)");
        query.bindValue(":seconds", seconds);
        query.exec();
    }
    emit idleThresholdChanged();
}
