#include "ProductivityAppRepository.h"
#include <QSqlError>
#include <QDebug>
#include <QUrl>
#include <QRegularExpression>
#include <QVariant>

ProductivityAppRepository::ProductivityAppRepository(QObject *parent)
    : QObject(parent)
    , m_productiveAppsModel(new QSqlQueryModel(this))
    , m_nonProductiveAppsModel(new QSqlQueryModel(this))
{
}

bool ProductivityAppRepository::ensureDatabaseOpen() const
{
    if (!m_productivityDb.isValid()) {
        m_productivityDb = QSqlDatabase::database("productivity_db");
    }
    if (!m_productivityDb.isOpen()) {
        if (!m_productivityDb.open()) {
            qWarning() << "Failed to open productivity database:" << m_productivityDb.lastError().text();
            return false;
        }
    }
    return true;
}

bool ProductivityAppRepository::initialize()
{
    m_productivityDb = QSqlDatabase::addDatabase("QSQLITE", "productivity_db");
    m_productivityDb.setDatabaseName("produktif_app_db.db");

    if (!m_productivityDb.open()) {
        qWarning() << "Failed to open productivity database:" << m_productivityDb.lastError().text();
        return false;
    }

    migrateDatabase();

    QSqlQuery query(m_productivityDb);
    if (!query.exec("CREATE TABLE IF NOT EXISTS aplikasi ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "aplikasi TEXT NOT NULL, "
                    "window_title TEXT, "
                    "url TEXT, "
                    "jenis INTEGER NOT NULL, "
                    "productivity INTEGER NOT NULL DEFAULT 0, "
                    "for_user TEXT NOT NULL DEFAULT '0')")) {
        qWarning() << "Failed to create aplikasi table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS task ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "project_name TEXT NOT NULL, "
                    "task TEXT, "
                    "max_time INTEGER NOT NULL, "
                    "time_usage INTEGER NOT NULL, "
                    "active BOOLEAN NOT NULL, "
                    "status TEXT NOT NULL, "
                    "paused BOOLEAN NOT NULL DEFAULT 0,"
                    "user_id INTEGER NOT NULL,"
                    "created_at TEXT)")) {
        qWarning() << "Failed to create task table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS completed_tasks ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "project_name TEXT, "
                    "task TEXT, "
                    "max_time INTEGER, "
                    "time_usage INTEGER, "
                    "completed_time INTEGER, "
                    "user_id INTEGER)")) {
        qWarning() << "Failed to create completed_tasks table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS idle_settings ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "threshold_seconds INTEGER)")) {
        qWarning() << "Failed to create idle_settings table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS log_paused ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "task_id INTEGER NOT NULL, "
                    "start_reality TEXT NOT NULL, "
                    "end_reality TEXT, "
                    "current_status TEXT NOT NULL, "
                    "FOREIGN KEY(task_id) REFERENCES task(id))")) {
        qWarning() << "Failed to create log_paused table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS work_time ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "user_id INTEGER NOT NULL, "
                    "date TEXT NOT NULL, "
                    "elapsed_seconds INTEGER NOT NULL DEFAULT 0, "
                    "UNIQUE(user_id, date))")) {
        qWarning() << "Failed to create work_time table:" << query.lastError().text();
    }

    m_productiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1", m_productivityDb);
    m_nonProductiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2", m_productivityDb);

    return true;
}

void ProductivityAppRepository::migrateDatabase()
{
    if (!ensureDatabaseOpen()) return;

    QSqlQuery query(m_productivityDb);

    query.exec("PRAGMA table_info(task)");
    bool hasUserId = false;
    bool hasCreatedAt = false;
    while (query.next()) {
        QString name = query.value("name").toString();
        if (name == "user_id") hasUserId = true;
        if (name == "created_at") hasCreatedAt = true;
    }
    if (!hasUserId) {
        query.exec("ALTER TABLE task ADD COLUMN user_id INTEGER NOT NULL DEFAULT 0");
    }
    if (!hasCreatedAt) {
        query.exec("ALTER TABLE task ADD COLUMN created_at TEXT");
    }

    query.exec("PRAGMA table_info(completed_tasks)");
    hasUserId = false;
    while (query.next()) {
        if (query.value("name").toString() == "user_id") {
            hasUserId = true;
            break;
        }
    }
    if (!hasUserId) {
        query.exec("ALTER TABLE completed_tasks ADD COLUMN user_id INTEGER NOT NULL DEFAULT 0");
    }
}

void ProductivityAppRepository::refreshProductivityModels(int userId)
{
    if (!ensureDatabaseOpen()) return;

    QString productiveQuery = QString(
        "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
        "FROM aplikasi a "
        "WHERE a.jenis = 1 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
        "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
        "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
        "GROUP BY a.aplikasi, COALESCE(a.url, '') "
        "ORDER BY a.for_user = '0' DESC").arg(userId);

    QString nonProductiveQuery = QString(
        "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
        "FROM aplikasi a "
        "WHERE a.jenis = 2 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
        "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
        "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
        "GROUP BY a.aplikasi, COALESCE(a.url, '') "
        "ORDER BY a.for_user = '0' DESC").arg(userId);

    m_productiveAppsModel->setQuery(productiveQuery, m_productivityDb);
    m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_productivityDb);

    emit productivityAppsChanged();
}

void ProductivityAppRepository::updateProductivityCache(int userId)
{
    m_cachedAppTypes.clear();
    m_cachedDomainTypes.clear();

    if (!ensureDatabaseOpen() || userId == -1) return;

    QSqlQuery query(m_productivityDb);
    query.prepare(R"(
        SELECT aplikasi, url, jenis, for_user
        FROM aplikasi
        WHERE jenis IN (1, 2)
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
        int jenis = query.value(2).toInt();

        if (!url.isEmpty()) {
            QString domain = extractDomain(url);
            if (!domain.isEmpty()) {
                m_cachedDomainTypes[domain] = jenis;
            }
        } else if (!appName.isEmpty()) {
            m_cachedAppTypes[normalizeString(appName)] = jenis;
        }
    }
}

int ProductivityAppRepository::getAppProductivityType(const QString &appName, const QString &url, int userId) const
{
    if (userId == -1) return 0;

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
        if (m_cachedDomainTypes.contains(domain)) {
            return m_cachedDomainTypes.value(domain);
        }
    } else {
        QString normApp = normalizeString(appName);
        if (m_cachedAppTypes.contains(normApp)) {
            return m_cachedAppTypes.value(normApp);
        }
    }

    return 0;
}

QVariantList ProductivityAppRepository::getAvailableApps() const
{
    return {
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
    if (!ensureDatabaseOpen() || userId == -1) {
        return QVariantList();
    }

    QVariantList apps;
    QSqlQuery query(m_productivityDb);
    query.prepare(R"(
        SELECT aplikasi, jenis, url, for_user
        FROM aplikasi
        WHERE for_user = '0'
           OR ',' || for_user || ',' LIKE :userPattern
    )");
    query.bindValue(":userPattern", "%," + QString::number(userId) + ",%");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap app;
            app["appName"] = query.value(0).toString();
            app["type"] = query.value(1).toInt();
            app["url"] = query.value(2).toString();
            apps.append(app);
        }
    }
    return apps;
}

QVariantList ProductivityAppRepository::getPendingApplicationRequests() const
{
    QVariantList requests;
    if (!ensureDatabaseOpen()) return requests;

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id, aplikasi, window_title, url, productivity, for_user FROM aplikasi WHERE jenis = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap request;
            request["id"] = query.value(0);
            request["app_name"] = query.value(1);
            request["window_title"] = query.value(2);
            request["url"] = query.value(3);
            request["productivity"] = query.value(4);

            int productivity = query.value(4).toInt();
            switch(productivity) {
            case 1: request["productivity_text"] = "Productive"; break;
            case 2: request["productivity_text"] = "Non-Productive"; break;
            default: request["productivity_text"] = "Neutral"; break;
            }

            QString forUsers = query.value(5).toString();
            if (forUsers == "0") {
                request["for_users"] = "All Users";
            } else {
                QStringList userIds = forUsers.split(',', Qt::SkipEmptyParts);
                request["for_users"] = userIds.join(", ");
            }

            requests.append(request);
        }
    }
    return requests;
}

bool ProductivityAppRepository::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (!ensureDatabaseOpen()) return false;

    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT INTO aplikasi (aplikasi, window_title, url, jenis, productivity) "
                  "VALUES (:app, :window, :url, :type, :prod)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":url", url.isEmpty() ? QVariant() : url);
    query.bindValue(":type", 0);
    query.bindValue(":prod", productivityType);

    return query.exec();
}

int ProductivityAppRepository::getIdleThreshold() const
{
    if (!ensureDatabaseOpen()) return 180;

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT threshold_seconds FROM idle_settings LIMIT 1");
    if (query.exec() && query.next()) {
        int threshold = query.value(0).toInt();
        if (threshold > 0) return threshold;
    }
    return 180;
}

void ProductivityAppRepository::setIdleThreshold(int seconds)
{
    if (!ensureDatabaseOpen() || seconds <= 0) return;

    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT OR REPLACE INTO idle_settings (id, threshold_seconds) VALUES (1, :threshold)");
    query.bindValue(":threshold", seconds);
    if (query.exec()) {
        emit idleThresholdChanged();
    }
}
