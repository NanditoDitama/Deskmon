#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QScopeGuard>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>

#include "core/database/DatabaseManager.h"
#include "core/database/ProductivityAppRepository.h"
#include "core/database/UserRepository.h"
#include "core/database/WorkLogRepository.h"
#include "core/database/WorkTimeRepository.h"

class TestDatabaseAndRepos : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());
        m_dbPath = m_tempDir->filePath("test_deskmon.db");
        qputenv("DESKMON_DB_PATH", m_dbPath.toUtf8());
    }

    void cleanup()
    {
        DatabaseManager::instance().close();
        qunsetenv("DESKMON_DB_PATH");
        m_tempDir.reset();
    }

    void testFreshInitializeAndSchema()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        QCOMPARE(DatabaseManager::instance().databasePath(), m_dbPath);
        QVERIFY(QFileInfo::exists(m_dbPath));

        QSqlDatabase db = DatabaseManager::instance().database();
        QStringList tables = db.tables(QSql::Tables);
        QStringList views = db.tables(QSql::Views);

        for (const QString &t : {"users", "activity_logs", "productivity_apps", "idle_settings",
                                 "tasks", "completed_tasks", "task_pause_logs", "work_time_records"}) {
            QVERIFY2(tables.contains(t), qPrintable("Missing table: " + t));
        }
        for (const QString &v : {"log", "aplikasi", "task", "work_time"}) {
            QVERIFY2(views.contains(v), qPrintable("Missing view: " + v));
        }

        QSqlQuery q(db);
        QVERIFY(q.exec("PRAGMA table_info(productivity_apps)"));
        bool hasProductivityCol = false;
        while (q.next()) {
            if (q.value(1).toString() == "productivity") {
                hasProductivityCol = true;
            }
        }
        QVERIFY(hasProductivityCol);
    }

    void testUpgradeMigrationAddsProductivityColumnAndUpdatesAplikasiView()
    {
        // Pre-create an older V2 DB without 'productivity' column and with stale 0 AS productivity view
        {
            QSqlDatabase legacy = QSqlDatabase::addDatabase("QSQLITE", "pre_upgrade_conn");
            legacy.setDatabaseName(m_dbPath);
            QVERIFY(legacy.open());
            QSqlQuery q(legacy);
            QVERIFY(q.exec(R"(
                CREATE TABLE productivity_apps (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    app_name TEXT NOT NULL,
                    window_title TEXT,
                    url TEXT,
                    productivity_type INTEGER NOT NULL,
                    for_user TEXT NOT NULL DEFAULT '0'
                )
            )"));
            QVERIFY(q.exec(R"(
                CREATE VIEW aplikasi AS
                SELECT id, app_name AS aplikasi, window_title, url, productivity_type AS jenis, 0 AS productivity, for_user
                FROM productivity_apps
            )"));
            QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, productivity_type, for_user) VALUES ('OldApp', 1, '0')"));
            legacy.close();
        }
        QSqlDatabase::removeDatabase("pre_upgrade_conn");

        QVERIFY(DatabaseManager::instance().initialize());
        QSqlDatabase db = DatabaseManager::instance().database();

        QSqlQuery q(db);
        QVERIFY(q.exec("UPDATE productivity_apps SET productivity = 2 WHERE app_name = 'OldApp'"));
        QVERIFY(q.exec("SELECT aplikasi, jenis, productivity FROM aplikasi WHERE aplikasi = 'OldApp'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("OldApp"));
        QCOMPARE(q.value(1).toInt(), 1);
        QCOMPARE(q.value(2).toInt(), 2);
    }

    void testProductivityAppRepoPendingAndPresetAndSync()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        ProductivityAppRepository repo;
        QVERIFY(repo.initialize());

        // 1. Preset 48 apps from 1.0.3.3
        QVariantList available = repo.getAvailableApps();
        QCOMPARE(available.size(), 48);
        QCOMPARE(available.first().toString(), QStringLiteral("Other"));
        QVERIFY(available.contains(QStringLiteral("Chrome")));
        QVERIFY(available.contains(QStringLiteral("Thunderbird")));

        // 2. addProductivityApp inserts productivity_type=0 and productivity=requestedType
        QVERIFY(repo.addProductivityApp("Figma", "", "", 1));
        QVERIFY(repo.addProductivityApp("Chrome", "", "https://example.com", 2));

        QVariantList pending = repo.getPendingApplicationRequests();
        QCOMPARE(pending.size(), 2);
        QCOMPARE(pending[0].toMap()["app_name"].toString(), QStringLiteral("Figma"));
        QCOMPARE(pending[0].toMap()["productivity"].toInt(), 1);
        QCOMPARE(pending[0].toMap()["productivity_text"].toString(), QStringLiteral("Productive"));
        QCOMPARE(pending[0].toMap()["for_users"].toString(), QStringLiteral("All Users"));

        QCOMPARE(pending[1].toMap()["app_name"].toString(), QStringLiteral("Chrome"));
        QCOMPARE(pending[1].toMap()["productivity"].toInt(), 2);
        QCOMPARE(pending[1].toMap()["productivity_text"].toString(), QStringLiteral("Non-Productive"));
        QCOMPARE(pending[1].toMap()["url"].toString(), QStringLiteral("https://example.com"));

        // 3. storeProductivityAppsFromApi maps user_id -> for_user, promotes pending rows, and deduplicates
        QJsonArray apiApps;
        apiApps.append(QJsonObject{
            {"application_name", "Figma"},
            {"process_name", "Figma.exe"},
            {"url", "N/A"},
            {"productivity_status", "productive"},
            {"user_id", 10}
        });
        apiApps.append(QJsonObject{
            {"application_name", "YouTube"},
            {"process_name", ""},
            {"url", "https://www.youtube.com"},
            {"productivity_status", "non-productive"},
            {"user_id", 1}
        });

        QVERIFY(repo.storeProductivityAppsFromApi(apiApps, 1));
        // Pending Figma should have been promoted from productivity_type=0 -> 1
        QCOMPARE(repo.getPendingApplicationRequests().size(), 1);

        // Re-syncing identical payload must NOT create duplicate rows
        QVERIFY(repo.storeProductivityAppsFromApi(apiApps, 1));
        QSqlQuery countQ(DatabaseManager::instance().database());
        QVERIFY(countQ.exec("SELECT COUNT(*) FROM productivity_apps WHERE app_name = 'Figma'"));
        QVERIFY(countQ.next());
        QCOMPARE(countQ.value(0).toInt(), 1);

        // 4. Tokenized user matching: user 1 must NOT match for_user='10'
        QCOMPARE(repo.getProductivityApps(-1).size(), 0);
        QVariantList user1Apps = repo.getProductivityApps(1);
        bool user1HasFigma = false;
        bool user1HasYouTube = false;
        for (const QVariant &v : user1Apps) {
            QString name = v.toMap()["appName"].toString();
            if (name == "Figma") user1HasFigma = true;
            if (name == "YouTube") user1HasYouTube = true;
        }
        QVERIFY(!user1HasFigma);
        QVERIFY(user1HasYouTube);

        // 5. Cache & productivity lookup guards
        QCOMPARE(repo.getAppProductivityType("YouTube", "https://youtube.com", -1), 0);
        QCOMPARE(repo.getAppProductivityType("Chrome", "https://m.youtube.com/watch?v=1", 1), 2);
        QCOMPARE(repo.getAppProductivityType("Chrome", "https://notyoutube.com", 1), 0);
        // Should not fall through to appName when URL is provided
        QVERIFY(repo.addProductivityApp("Chrome", "", "", 1));
        QSqlQuery approveChrome(DatabaseManager::instance().database());
        QVERIFY(approveChrome.exec("UPDATE productivity_apps SET productivity_type = 1 WHERE app_name = 'Chrome' AND url IS NULL"));
        repo.updateProductivityCache(1);
        QCOMPARE(repo.getAppProductivityType("Chrome", "https://unknown-site.org", 1), 0);
        QCOMPARE(repo.getAppProductivityType("Chrome", "", 1), 1);
    }

    void testUserRepositoryGetDepartmentReturnsRole()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(5, "alice", "secret", "alice@example.com", "Engineering", "Senior Developer", "tok123"));
        QCOMPARE(userRepo.getDepartment("alice"), QStringLiteral("Senior Developer"));
    }

    void testWorkLogAndWorkTimeRepositories()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(7, "bob", "pw", "bob@example.com", "QA", "Tester", "tok"));

        WorkLogRepository logRepo;
        QVERIFY(logRepo.initialize());

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        WindowInfo searchWin{"Chrome", "Google Search", "how to write qt tests"};
        logRepo.logWindowChange(searchWin, now - 100, now - 50, 7);

        WindowInfo validWin{"Chrome", "GitHub", "https://github.com/org/repo"};
        logRepo.logWindowChange(validWin, now - 50, now, 7);

        QCOMPARE(logRepo.logCount(7), 2);
        QString csv = logRepo.logContent(7);
        // Search query with spaces should be stripped from URL
        QVERIFY(csv.contains(",Chrome,Google Search,\n"));
        QVERIFY(csv.contains(",Chrome,GitHub,https://github.com/org/repo\n"));

        const QString today = QDate::currentDate().toString("yyyy-MM-dd");
        logRepo.setLogFilter(today, today, 7);
        QCOMPARE(logRepo.logCount(7), 2);

        const QString tomorrow = QDate::currentDate().addDays(1).toString("yyyy-MM-dd");
        logRepo.setLogFilter(tomorrow, tomorrow, 7);
        QCOMPARE(logRepo.logCount(7), 0);

        // showLogs must not silently reset filters
        logRepo.showLogs(7);
        QCOMPARE(logRepo.logCount(7), 0);

        WorkTimeRepository wtRepo;
        QVERIFY(!wtRepo.hasRecordForDate(7, today));
        QVERIFY(wtRepo.saveElapsedSeconds(7, today, 1234));
        QVERIFY(wtRepo.hasRecordForDate(7, today));
        QCOMPARE(wtRepo.getElapsedSeconds(7, today), 1234);
    }

    void testMigrateLegacyDatabasesOrphanedFksIdleSettingsAndQuotedPath()
    {
        QVERIFY(DatabaseManager::instance().initialize());

        const QString quotedDirPath = m_tempDir->filePath("O'Reilly_legacy");
        QVERIFY(QDir().mkpath(quotedDirPath));
        const QString legacyActPath = quotedDirPath + "/activity_logs.db";
        const QString legacyProdPath = quotedDirPath + "/produktif_app_db.db";

        {
            QSqlDatabase actDb = QSqlDatabase::addDatabase("QSQLITE", "legacy_act_seed");
            actDb.setDatabaseName(legacyActPath);
            QVERIFY(actDb.open());
            QSqlQuery q(actDb);
            QVERIFY(q.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, username TEXT, password TEXT, department TEXT, profile_image TEXT, email TEXT, role TEXT, token TEXT)"));
            QVERIFY(q.exec("CREATE TABLE log (id INTEGER PRIMARY KEY, id_user INTEGER, start_time INTEGER, end_time INTEGER, app_name TEXT, title TEXT, url TEXT)"));
            QVERIFY(q.exec("INSERT INTO users (id, username, password, department, profile_image, email, role, token) VALUES (1, 'legacyuser', 'pw', 'Eng', NULL, 'l@example.com', 'Dev', 'tok')"));
            QVERIFY(q.exec("INSERT INTO log (id, id_user, start_time, end_time, app_name, title, url) VALUES (100, 1, 1000, 1060, 'Code', 'main.cpp', '')"));
            QVERIFY(q.exec("INSERT INTO log (id, id_user, start_time, end_time, app_name, title, url) VALUES (101, 999, 1060, 1120, 'Code', 'orphan.cpp', '')"));
            actDb.close();
        }
        QSqlDatabase::removeDatabase("legacy_act_seed");

        {
            QSqlDatabase prodDb = QSqlDatabase::addDatabase("QSQLITE", "legacy_prod_seed");
            prodDb.setDatabaseName(legacyProdPath);
            QVERIFY(prodDb.open());
            QSqlQuery q(prodDb);
            QVERIFY(q.exec("CREATE TABLE aplikasi (id INTEGER PRIMARY KEY, aplikasi TEXT, window_title TEXT, url TEXT, jenis INTEGER, productivity INTEGER, for_user TEXT)"));
            QVERIFY(q.exec("CREATE TABLE task (id INTEGER PRIMARY KEY, project_name TEXT, task TEXT, max_time INTEGER, time_usage INTEGER, active INTEGER, status TEXT, paused INTEGER, user_id INTEGER, created_at TEXT)"));
            QVERIFY(q.exec("CREATE TABLE completed_tasks (id INTEGER PRIMARY KEY, user_id INTEGER, project_name TEXT, task TEXT, max_time INTEGER, time_usage INTEGER, completed_time INTEGER)"));
            QVERIFY(q.exec("CREATE TABLE log_paused (id INTEGER PRIMARY KEY, task_id INTEGER, start_reality TEXT, end_reality TEXT, current_status TEXT)"));
            QVERIFY(q.exec("CREATE TABLE work_time (id INTEGER PRIMARY KEY, user_id INTEGER, date TEXT, elapsed_seconds INTEGER)"));
            QVERIFY(q.exec("CREATE TABLE idle_settings (id INTEGER PRIMARY KEY, threshold_seconds INTEGER)"));
            QVERIFY(q.exec("INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id, created_at) VALUES (10, 'Proj', 'ValidTask', 3600, 0, 0, 'Pending', 0, 1, '2026-11-21T00:00:00Z')"));
            QVERIFY(q.exec("INSERT INTO log_paused (id, task_id, start_reality, end_reality, current_status) VALUES (1, 10, '2026-11-21T00:00:00', NULL, 'play')"));
            QVERIFY(q.exec("INSERT INTO log_paused (id, task_id, start_reality, end_reality, current_status) VALUES (2, 9999, '2026-11-21T00:01:00', NULL, 'pause')"));
            QVERIFY(q.exec("INSERT INTO idle_settings (id, threshold_seconds) VALUES (2, 300)"));
            prodDb.close();
        }
        QSqlDatabase::removeDatabase("legacy_prod_seed");

        const QString prevCwd = QDir::currentPath();
        auto restoreCwd = qScopeGuard([&prevCwd]() { QDir::setCurrent(prevCwd); });
        QVERIFY(QDir::setCurrent(quotedDirPath));

        DatabaseManager::instance().migrateLegacyDatabases();

        QSqlQuery checkQ(DatabaseManager::instance().database());
        QVERIFY(checkQ.exec("SELECT COUNT(*) FROM activity_logs"));
        QVERIFY(checkQ.next());
        QCOMPARE(checkQ.value(0).toInt(), 1);

        QVERIFY(checkQ.exec("SELECT COUNT(*) FROM task_pause_logs"));
        QVERIFY(checkQ.next());
        QCOMPARE(checkQ.value(0).toInt(), 1);

        ProductivityAppRepository repo;
        QCOMPARE(repo.getIdleThreshold(), 300);
    }

    void testProductivityAppRepoStringUserIdNAUrlAndUserOverrideOrder()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        ProductivityAppRepository repo;
        QVERIFY(repo.initialize());

        // String user_id, untrimmed URLs, and " N/A " / " n/a "
        QJsonArray apiApps;
        apiApps.append(QJsonObject{
            {"application_name", "Discord"},
            {"process_name", " N/A "},
            {"url", " N/A "},
            {"productivity_status", "non-productive"},
            {"user_id", "10"}
        });
        apiApps.append(QJsonObject{
            {"application_name", "Notion"},
            {"process_name", ""},
            {"url", " n/a "},
            {"productivity_status", "productive"},
            {"user_id", "0"}
        });
        apiApps.append(QJsonObject{
            {"application_name", "Chrome"},
            {"process_name", ""},
            {"url", " https://youtube.com "},
            {"productivity_status", "non-productive"},
            {"user_id", 1}
        });

        QVERIFY(repo.storeProductivityAppsFromApi(apiApps, 1));

        QSqlQuery q(DatabaseManager::instance().database());
        QVERIFY(q.exec("SELECT for_user, url FROM productivity_apps WHERE app_name = 'Discord'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("10"));
        QVERIFY(q.value(1).isNull() || q.value(1).toString().isEmpty());

        QCOMPARE(repo.getAppProductivityType("Discord", "", 10), 2);
        QCOMPARE(repo.getAppProductivityType("Discord", "", 1), 0);
        QCOMPARE(repo.getAppProductivityType("Notion", "", 1), 1);
        QCOMPARE(repo.getAppProductivityType("Notion", "N/A", 1), 1);
        QCOMPARE(repo.getAppProductivityType("Chrome", "https://youtube.com", 1), 2);
        QCOMPARE(repo.getAppProductivityType("Chrome", " https://youtube.com/watch ", 1), 2);

        // Legacy/raw DB row with url='N/A' and user-specific override inserted BEFORE global rule
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('VSCode', 'N/A', 1, '0')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Slack', NULL, 2, '5')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Slack', NULL, 1, '0')"));

        repo.updateProductivityCache(5);
        QCOMPARE(repo.getAppProductivityType("VSCode", "", 5), 1);
        QCOMPARE(repo.getAppProductivityType("VSCode", "N/A", 5), 1);
        QCOMPARE(repo.getAppProductivityType("Slack", "", 5), 2);

        repo.updateProductivityCache(6);
        QCOMPARE(repo.getAppProductivityType("Slack", "", 6), 1);
    }

    void testBulkSyncAndIndexedLogFilterPerformance()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(42, "bulkuser", "pw", "bulk@example.com", "Eng", "Dev", "tok42"));

        QJsonArray appsArray;
        for (int i = 0; i < 500; ++i) {
            appsArray.append(QJsonObject{
                {"application_name", QString("App_%1").arg(i)},
                {"process_name", ""},
                {"url", (i % 2 == 0) ? QString("https://site%1.example.com").arg(i) : QString("N/A")},
                {"productivity_status", (i % 3 == 0) ? QString("non-productive") : QString("productive")},
                {"user_id", QString("42")}
            });
        }

        QSqlDatabase db = DatabaseManager::instance().database();

        const QDate today = QDate::currentDate();
        const qint64 todayBase = QDateTime(today, QTime(8, 0, 0)).toSecsSinceEpoch();
        const qint64 twoDaysAgoBase = QDateTime(today.addDays(-2), QTime(8, 0, 0)).toSecsSinceEpoch();

        QVERIFY(db.transaction());
        QSqlQuery ins(db);
        QVERIFY(ins.prepare("INSERT INTO activity_logs (user_id, start_time, end_time, app_name, title, url) "
                            "VALUES (42, :s, :e, :app, :title, :url)"));
        for (int i = 0; i < 1000; ++i) {
            ins.bindValue(":s", todayBase + i * 10);
            ins.bindValue(":e", todayBase + i * 10 + 5);
            ins.bindValue(":app", QString("App_%1").arg(i % 20));
            ins.bindValue(":title", QString("Today Title %1").arg(i));
            ins.bindValue(":url", (i % 2 == 0) ? QString("https://site%1.example.com/p").arg(i % 20) : QString());
            QVERIFY(ins.exec());
        }
        for (int i = 0; i < 1000; ++i) {
            ins.bindValue(":s", twoDaysAgoBase + i * 10);
            ins.bindValue(":e", twoDaysAgoBase + i * 10 + 5);
            ins.bindValue(":app", QString("OldApp_%1").arg(i % 20));
            ins.bindValue(":title", QString("Old Title %1").arg(i));
            ins.bindValue(":url", QString());
            QVERIFY(ins.exec());
        }
        QVERIFY(db.commit());

        QElapsedTimer timer;
        timer.start();

        // First sync + idempotent re-sync of 500 apps in O(N)
        QVERIFY(ProductivityAppRepository::syncAppsToDatabase(db, appsArray));
        QVERIFY(ProductivityAppRepository::syncAppsToDatabase(db, appsArray));

        QSqlQuery countAppsQ(db);
        QVERIFY(countAppsQ.exec("SELECT COUNT(*) FROM productivity_apps WHERE for_user = '42'"));
        QVERIFY(countAppsQ.next());
        QCOMPARE(countAppsQ.value(0).toInt(), 500);

        ProductivityAppRepository appRepo;
        QVERIFY(appRepo.initialize());
        appRepo.updateProductivityCache(42);
        QCOMPARE(appRepo.getAppProductivityType("App_0", "https://site0.example.com", 42), 2);
        QCOMPARE(appRepo.getAppProductivityType("App_1", "", 42), 1);

        // Indexed epoch range filtering & in-memory cache invalidation over 2000 logs
        WorkLogRepository logRepo;
        QVERIFY(logRepo.initialize());

        const QString todayStr = today.toString("yyyy-MM-dd");
        logRepo.setLogFilter(todayStr, todayStr, 42);
        QCOMPARE(logRepo.logCount(42), 1000);
        const QString cachedCsv = logRepo.logContent(42);
        QVERIFY(!cachedCsv.isEmpty());
        QCOMPARE(logRepo.logCount(42), 1000);
        QCOMPARE(logRepo.logContent(42), cachedCsv);

        // New log invalidates cache cleanly
        WindowInfo extra{"Editor", "Bulk Verification", ""};
        logRepo.logWindowChange(extra, todayBase + 20000, todayBase + 20010, 42);
        QCOMPARE(logRepo.logCount(42), 1001);
        QVERIFY(logRepo.logContent(42).contains(",Editor,Bulk Verification,"));

        const QString twoDaysAgoStr = today.addDays(-2).toString("yyyy-MM-dd");
        logRepo.setLogFilter(twoDaysAgoStr, twoDaysAgoStr, 42);
        QCOMPARE(logRepo.logCount(42), 1000);

        logRepo.clearLogFilter(42);
        QCOMPARE(logRepo.logCount(42), 2001);

        const qint64 elapsedMs = timer.elapsed();
        QVERIFY2(elapsedMs < 1000, qPrintable(QString("Bulk sync + indexed filter took %1 ms").arg(elapsedMs)));
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
};

QTEST_GUILESS_MAIN(TestDatabaseAndRepos)
#include "test_database_and_repos.moc"
