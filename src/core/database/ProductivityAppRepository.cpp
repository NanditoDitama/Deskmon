#include "ProductivityAppRepository.h"
#include "DatabaseManager.h"
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>
#include <QUrl>
#include <QJsonObject>
#include <QPair>
#include <QVector>

namespace {
QString normalizeAppString(const QString &str)
{
    return str.toLower().remove(' ').remove('-').remove('_').remove('.');
}

QString normalizeAppUrl(const QString &rawUrl)
{
    QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty() || trimmed.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0) {
        return QString();
    }
    return trimmed;
}

QString extractAppDomain(const QString &url)
{
    if (url.isEmpty()) return QString();
    QUrl qurl(url);
    QString domain = qurl.host();
    if (domain.isEmpty()) {
        static const QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
        QRegularExpressionMatch match = domainRegex.match(url);
        if (match.hasMatch()) {
            domain = match.captured(1);
        }
    }
    if (domain.startsWith(QStringLiteral("www."))) {
        domain = domain.mid(4);
    }
    return domain.toLower();
}

struct ExistingAppRow {
    int id = 0;
    QString windowTitle;
    int type = 0;
    QString forUser;
};
} // namespace

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
        "SELECT id, app_name AS appName, window_title AS windowTitle, url, productivity_type AS type "
        "FROM productivity_apps WHERE productivity_type = 1",
        database());

    m_nonProductiveAppsModel->setQuery(
        "SELECT id, app_name AS appName, window_title AS windowTitle, url, productivity_type AS type "
        "FROM productivity_apps WHERE productivity_type = 2",
        database());

    return true;
}

void ProductivityAppRepository::refreshProductivityModels(int userId)
{
    if (!ensureDatabaseOpen()) return;

    if (userId == -1) {
        m_productiveAppsModel->clear();
        m_nonProductiveAppsModel->clear();
        emit productivityAppsChanged();
        return;
    }

    const QString userPattern = "%," + QString::number(userId) + ",%";

    QSqlQuery productiveQuery(database());
    productiveQuery.prepare(R"(
        SELECT a.id, a.app_name AS appName, a.window_title AS windowTitle, a.url, a.productivity_type AS type
        FROM productivity_apps a
        WHERE a.productivity_type = 1
          AND (a.for_user = '0' OR ',' || a.for_user || ',' LIKE :userPattern)
        GROUP BY a.app_name, COALESCE(a.url, '')
        ORDER BY a.for_user = '0' DESC
    )");
    productiveQuery.bindValue(":userPattern", userPattern);
    if (productiveQuery.exec()) {
        m_productiveAppsModel->setQuery(std::move(productiveQuery));
    }

    QSqlQuery nonProductiveQuery(database());
    nonProductiveQuery.prepare(R"(
        SELECT a.id, a.app_name AS appName, a.window_title AS windowTitle, a.url, a.productivity_type AS type
        FROM productivity_apps a
        WHERE a.productivity_type = 2
          AND (a.for_user = '0' OR ',' || a.for_user || ',' LIKE :userPattern)
        GROUP BY a.app_name, COALESCE(a.url, '')
        ORDER BY a.for_user = '0' DESC
    )");
    nonProductiveQuery.bindValue(":userPattern", userPattern);
    if (nonProductiveQuery.exec()) {
        m_nonProductiveAppsModel->setQuery(std::move(nonProductiveQuery));
    }

    emit productivityAppsChanged();
}

void ProductivityAppRepository::updateProductivityCache(int userId)
{
    m_cachedAppTypes.clear();
    m_cachedDomainTypes.clear();
    m_cachedUserId = userId;

    if (!ensureDatabaseOpen() || userId == -1) return;

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT app_name, url, productivity_type, for_user
        FROM productivity_apps
        WHERE productivity_type IN (1, 2)
          AND (for_user = '0' OR ',' || for_user || ',' LIKE :userPattern)
        ORDER BY (for_user = '0') DESC, id ASC
    )");
    query.bindValue(":userPattern", "%," + QString::number(userId) + ",%");

    if (!query.exec()) return;

    while (query.next()) {
        QString appName = query.value(0).toString().trimmed();
        QString url = normalizeAppUrl(query.value(1).toString());
        int type = query.value(2).toInt();

        if (!url.isEmpty()) {
            QString domain = extractAppDomain(url);
            if (!domain.isEmpty()) {
                m_cachedDomainTypes[domain] = type;
            }
        } else if (!appName.isEmpty()) {
            m_cachedAppTypes[normalizeAppString(appName)] = type;
        }
    }
}

void ProductivityAppRepository::copyCachesForUser(int userId, QHash<QString, int> &outAppTypes, QHash<QString, int> &outDomainTypes) const
{
    if (userId == -1) {
        outAppTypes.clear();
        outDomainTypes.clear();
        return;
    }

    if ((m_cachedAppTypes.isEmpty() && m_cachedDomainTypes.isEmpty()) || m_cachedUserId != userId) {
        const_cast<ProductivityAppRepository*>(this)->updateProductivityCache(userId);
    }

    outAppTypes = m_cachedAppTypes;
    outDomainTypes = m_cachedDomainTypes;
}

int ProductivityAppRepository::lookupProductivityType(const QString &appName, const QString &url,
                                                      const QHash<QString, int> &appTypes,
                                                      const QHash<QString, int> &domainTypes)
{
    QString trimmedUrl = normalizeAppUrl(url);

    if (!trimmedUrl.isEmpty()) {
        QString domain = extractAppDomain(trimmedUrl);
        if (!domain.isEmpty()) {
            if (domainTypes.contains(domain)) {
                return domainTypes[domain];
            }
            for (auto it = domainTypes.constBegin(); it != domainTypes.constEnd(); ++it) {
                if (domain.endsWith("." + it.key())) {
                    return it.value();
                }
            }
        }
        return 0;
    }

    QString trimmedAppName = appName.trimmed();
    if (!trimmedAppName.isEmpty()) {
        QString normalizedInput = normalizeAppString(trimmedAppName);
        if (appTypes.contains(normalizedInput)) {
            return appTypes[normalizedInput];
        }
        for (auto it = appTypes.constBegin(); it != appTypes.constEnd(); ++it) {
            if (normalizedInput.contains(it.key())) {
                return it.value();
            }
        }
    }

    return 0; // Neutral
}

int ProductivityAppRepository::getAppProductivityType(const QString &appName, const QString &url, int userId) const
{
    if (userId == -1) {
        return 0;
    }

    if ((m_cachedAppTypes.isEmpty() && m_cachedDomainTypes.isEmpty()) || m_cachedUserId != userId) {
        const_cast<ProductivityAppRepository*>(this)->updateProductivityCache(userId);
    }

    return lookupProductivityType(appName, url, m_cachedAppTypes, m_cachedDomainTypes);
}

QVariantList ProductivityAppRepository::getAvailableApps() const
{
    return QVariantList {
        "Other", "Chrome", "Firefox", "Edge", "Safari", "Opera",
        "Visual Studio Code", "Qt Creator", "Android Studio", "IntelliJ IDEA", "PyCharm", "Xcode",
        "Microsoft Word", "Excel", "PowerPoint", "WPS Office", "LibreOffice", "OneNote", "Obsidian",
        "Photoshop", "GIMP", "Figma", "Canva", "Blender", "Premiere Pro", "After Effects",
        "Slack", "Microsoft Teams", "Zoom", "Google Meet", "Discord", "Skype",
        "Notion", "Trello", "Jira", "Asana", "ClickUp",
        "Postman", "FileZilla", "Docker", "GitHub Desktop", "GitKraken", "Terminal",
        "Deskmon", "Desklog-Client", "Explorer", "Outlook", "Thunderbird"
    };
}

QVariantList ProductivityAppRepository::getProductivityApps(int userId) const
{
    QVariantList apps;
    if (!ensureDatabaseOpen() || userId == -1) return apps;

    const QString userPattern = "%," + QString::number(userId) + ",%";

    QSqlQuery query(database());
    query.prepare(R"(
        SELECT a.id, a.app_name, a.window_title, a.url, a.productivity_type
        FROM productivity_apps a
        WHERE (a.for_user = '0' OR ',' || a.for_user || ',' LIKE :userPattern)
        GROUP BY a.app_name, COALESCE(a.url, '')
        ORDER BY a.for_user = '0' DESC
    )");
    query.bindValue(":userPattern", userPattern);

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
    query.prepare("SELECT id, app_name, window_title, url, productivity, for_user "
                  "FROM productivity_apps WHERE productivity_type = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap req;
            req["id"] = query.value(0).toInt();
            req["app_name"] = query.value(1).toString();
            req["window_title"] = query.value(2).toString();
            req["url"] = query.value(3).toString();
            int productivity = query.value(4).toInt();
            req["productivity"] = productivity;
            switch (productivity) {
            case 1: req["productivity_text"] = QStringLiteral("Productive"); break;
            case 2: req["productivity_text"] = QStringLiteral("Non-Productive"); break;
            default: req["productivity_text"] = QStringLiteral("Neutral"); break;
            }
            QString forUsers = query.value(5).toString();
            if (forUsers == QStringLiteral("0")) {
                req["for_users"] = QStringLiteral("All Users");
            } else {
                req["for_users"] = forUsers.split(',', Qt::SkipEmptyParts).join(", ");
            }
            requests.append(req);
        }
    }
    return requests;
}

bool ProductivityAppRepository::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (!ensureDatabaseOpen()) return false;

    QSqlQuery query(database());
    query.prepare("INSERT INTO productivity_apps (app_name, window_title, url, productivity_type, productivity) "
                  "VALUES (:app, :window, :url, 0, :prod)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":url", url.isEmpty() ? QVariant() : url);
    query.bindValue(":prod", productivityType);

    return query.exec();
}

bool ProductivityAppRepository::syncAppsToDatabase(QSqlDatabase &db, const QJsonArray &appsArray)
{
    if (!db.isOpen()) return false;

    const bool inTx = db.transaction();
    if (!inTx) {
        qWarning() << "Failed to start transaction in syncAppsToDatabase";
        return false;
    }

    QHash<QPair<QString, QString>, QVector<ExistingAppRow>> existingByKey;
    {
        QSqlQuery loadQ(db);
        if (!loadQ.exec(QStringLiteral("SELECT id, app_name, window_title, url, productivity_type, for_user FROM productivity_apps"))) {
            qWarning() << "Failed to load existing productivity apps:" << loadQ.lastError().text();
            db.rollback();
            return false;
        }
        while (loadQ.next()) {
            const QString appName = loadQ.value(1).toString();
            const QString normalizedUrl = normalizeAppUrl(loadQ.value(3).toString());
            ExistingAppRow row;
            row.id = loadQ.value(0).toInt();
            row.windowTitle = loadQ.value(2).toString();
            row.type = loadQ.value(4).toInt();
            row.forUser = loadQ.value(5).toString();
            existingByKey[qMakePair(appName, normalizedUrl)].append(row);
        }
    }

    QSqlQuery updateExactQ(db);
    updateExactQ.prepare(QStringLiteral("UPDATE productivity_apps SET productivity_type = :type, window_title = :windowTitle WHERE id = :id"));

    QSqlQuery promotePendingQ(db);
    promotePendingQ.prepare(QStringLiteral("UPDATE productivity_apps SET productivity_type = :type, window_title = :windowTitle, for_user = :forUsers WHERE id = :id"));

    QSqlQuery insertQ(db);
    insertQ.prepare(QStringLiteral("INSERT INTO productivity_apps (app_name, window_title, url, productivity_type, for_user) "
                                   "VALUES (:app, :window, :url, :type, :forUsers)"));

    bool success = true;

    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) continue;
        QJsonObject appObj = appValue.toObject();
        QString appName = appObj["application_name"].toString().trimmed();
        QString processName = appObj["process_name"].toString().trimmed();
        QString rawUrl = appObj["url"].toString().trimmed();
        QString status = appObj["productivity_status"].toString().trimmed().toLower();
        int type = (status == "productive") ? 1 : (status == "non-productive" ? 2 : 0);

        QString forUsers = QStringLiteral("0");
        const QJsonValue uv = appObj.value("user_id");
        if (uv.isString() && !uv.toString().trimmed().isEmpty()) {
            forUsers = uv.toString().trimmed();
        } else if (uv.isDouble()) {
            forUsers = QString::number(uv.toInt());
        } else if (appObj.contains("for_user") && !appObj["for_user"].toString().trimmed().isEmpty()) {
            forUsers = appObj["for_user"].toString().trimmed();
        }

        QString expectedWindowTitle = (processName.isEmpty() || processName.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0)
                                          ? QString()
                                          : processName;
        QString expectedUrl = normalizeAppUrl(rawUrl);

        QVector<ExistingAppRow> &rows = existingByKey[qMakePair(appName, expectedUrl)];
        bool foundExactMatch = false;
        int zeroTypeIndex = -1;

        for (int i = 0; i < rows.size(); ++i) {
            ExistingAppRow &row = rows[i];
            if (row.forUser == forUsers) {
                foundExactMatch = true;
                if (row.type != type || row.windowTitle != expectedWindowTitle) {
                    updateExactQ.bindValue(":type", type);
                    updateExactQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                    updateExactQ.bindValue(":id", row.id);
                    if (!updateExactQ.exec()) {
                        qWarning() << "Failed to update exact match app:" << updateExactQ.lastError().text();
                        success = false;
                        break;
                    }
                    row.type = type;
                    row.windowTitle = expectedWindowTitle;
                }
                break;
            } else if (row.type == 0) {
                zeroTypeIndex = i;
            }
        }

        if (!success) break;

        if (!foundExactMatch) {
            if (zeroTypeIndex >= 0) {
                ExistingAppRow &pendingRow = rows[zeroTypeIndex];
                promotePendingQ.bindValue(":type", type);
                promotePendingQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                promotePendingQ.bindValue(":forUsers", forUsers);
                promotePendingQ.bindValue(":id", pendingRow.id);
                if (!promotePendingQ.exec()) {
                    qWarning() << "Failed to promote pending app:" << promotePendingQ.lastError().text();
                    success = false;
                    break;
                }
                pendingRow.type = type;
                pendingRow.windowTitle = expectedWindowTitle;
                pendingRow.forUser = forUsers;
            } else {
                insertQ.bindValue(":app", appName);
                insertQ.bindValue(":window", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                insertQ.bindValue(":url", expectedUrl.isEmpty() ? QVariant() : expectedUrl);
                insertQ.bindValue(":type", type);
                insertQ.bindValue(":forUsers", forUsers);
                if (!insertQ.exec()) {
                    qWarning() << "Failed to insert productivity app:" << insertQ.lastError().text();
                    success = false;
                    break;
                }
                ExistingAppRow inserted;
                inserted.id = insertQ.lastInsertId().toInt();
                inserted.windowTitle = expectedWindowTitle;
                inserted.type = type;
                inserted.forUser = forUsers;
                rows.append(inserted);
            }
        }
    }

    if (!success) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        qWarning() << "Failed to commit syncAppsToDatabase transaction";
        db.rollback();
        return false;
    }

    return true;
}

bool ProductivityAppRepository::storeProductivityAppsFromApi(const QJsonArray &appsArray, int userId)
{
    if (!ensureDatabaseOpen()) return false;

    QSqlDatabase db = database();
    if (!syncAppsToDatabase(db, appsArray)) {
        return false;
    }

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
