#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>
#include <QSqlQuery>

#include "MockHttpServer.h"
#include "app/AppController.h"
#include "core/database/DatabaseManager.h"
#include "core/database/ProductivityAppRepository.h"
#include "core/database/TaskRepository.h"
#include "core/database/UserRepository.h"
#include "core/database/WorkLogRepository.h"
#include "core/database/WorkTimeRepository.h"
#include "core/network/ApiClient.h"
#include "features/auth/AuthManager.h"
#include "features/productivity/ProductivityStatsService.h"
#include "features/tracking/ActivityTracker.h"
#include "features/tracking/IdleChecker.h"
#include "features/worktime/WorkTimeTracker.h"

class TestServicesIdleAuth : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());
        qputenv("DESKMON_DB_PATH", m_tempDir->filePath("services_test.db").toUtf8());

        m_server = std::make_unique<MockHttpServer>();
        QVERIFY(m_server->start());
        qputenv("DESKMON_API_BASE_URL", m_server->baseUrl().toUtf8());
    }

    void cleanup()
    {
        DatabaseManager::instance().close();
        m_server.reset();
        qunsetenv("DESKMON_API_BASE_URL");
        qunsetenv("DESKMON_UPDATE_URL");
        qunsetenv("DESKMON_INSTANCE_KEY");
        qunsetenv("DESKMON_DB_PATH");
        m_tempDir.reset();
    }

    void testApiClientAndInstanceKeyAndUpdateUrlHooks()
    {
        // Default ApiClient URL when env unset
        qunsetenv("DESKMON_API_BASE_URL");
        ApiClient defaultClient;
        QCOMPARE(defaultClient.baseUrl(), QStringLiteral("https://deskmon.pranala-dt.co.id/api"));
        QCOMPARE(defaultClient.buildUrl("/login").toString(), QStringLiteral("https://deskmon.pranala-dt.co.id/api/login"));

        // Env override
        qputenv("DESKMON_API_BASE_URL", (m_server->baseUrl() + "/").toUtf8());
        ApiClient overrideClient;
        QCOMPARE(overrideClient.baseUrl(), m_server->baseUrl());
        QCOMPARE(overrideClient.buildUrl("/login").toString(), m_server->baseUrl() + "/login");
        QNetworkRequest rewritten = overrideClient.createRequest(QUrl("https://deskmon.pranala-dt.co.id/api/ping"));
        QCOMPARE(rewritten.url().toString(), m_server->baseUrl() + "/ping");

        // DESKMON_INSTANCE_KEY resolution
        qunsetenv("DESKMON_INSTANCE_KEY");
        QCOMPARE(AppController::resolveInstanceKey(), QStringLiteral("DeskmonAppInstance"));
        qputenv("DESKMON_INSTANCE_KEY", "  DeskmonE2ETest_42  ");
        QCOMPARE(AppController::resolveInstanceKey(), QStringLiteral("DeskmonE2ETest_42"));

        // Update URL override via DESKMON_UPDATE_URL
        qputenv("DESKMON_UPDATE_URL", (m_server->baseUrl() + "/version.json").toUtf8());
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/version.json") {
                return {200, "OK", R"({"version":"9.9.9.9","releaseNotes":"Test release"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        AppController controller;
        QCOMPARE(controller.apiBaseUrl(), m_server->baseUrl());
        QSignalSpy updateSpy(&controller, &AppController::updateAvailable);
        controller.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(updateSpy.count(), 1, 3000);
        QCOMPARE(updateSpy.first().at(0).toString(), QStringLiteral("9.9.9.9"));
        QCOMPARE(updateSpy.first().at(1).toString(), QStringLiteral("Test release"));
    }

    void testAuthLoginIpAppendTokenResetAndLogoutOrdering()
    {
        // Verify login payload always sends gateway_macs array
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/login") {
                QJsonObject body = QJsonDocument::fromJson(req.body).object();
                if (!body.contains("gateway_macs") || !body["gateway_macs"].isArray()) {
                    return {403, "Forbidden", R"({"success":false,"message":"Office network verification required. Please update your Deskmon desktop app or connect to the office network.","reason":"gateway_mac_missing","your_ip":"127.0.0.1"})"};
                }
                if (body["password"].toString() == "blocked") {
                    return {200, "OK", R"({"success":false,"message":"Login not allowed from this IP address","your_ip":"10.20.30.40"})"};
                }
                if (body["password"].toString() == "mac_missing") {
                    return {403, "Forbidden", R"({"success":false,"message":"Office network verification required. Please update your Deskmon desktop app or connect to the office network.","reason":"gateway_mac_missing","your_ip":"127.0.0.1"})"};
                }
                return {200, "OK", R"({"success":true,"token":"fresh-tok","user":{"id":42,"name":"tester","email":"t@example.com","role":{"rolename":"QA"},"department":{"rolename":"Engineering"}},"verification_method":"gateway_mac_matched"})"};
            }
            if (req.path == "/task-by-user/42") {
                return {200, "OK", R"({"success":true,"data":[{"id":501,"title":"Proj","description":"Active Task","user_id":42,"duration":1,"total_duration":0,"created_at":"2026-11-21T00:00:00Z","status":"pending"}]})"};
            }
            if (req.path == "/app-request/all") {
                return {200, "OK", R"({"success":true,"data":[]})"};
            }
            if (req.path == "/get-time-at-work/42") {
                return {200, "OK", R"({"success":true,"data":{"raw":100}})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        AppController controller;
        // Startup must NOT auto-login
        QCOMPARE(controller.currentUserId(), -1);

        // 403 gateway MAC missing error returns server message cleanly
        QString macErr = controller.authenticate("t@example.com", "mac_missing");
        QCOMPARE(macErr, QStringLiteral("Office network verification required. Please update your Deskmon desktop app or connect to the office network."));

        // IP restriction error appends your_ip
        QString err = controller.authenticate("t@example.com", "blocked");
        QCOMPARE(err, QStringLiteral("Login not allowed from this IP address. 10.20.30.40"));

        // Verify collected gateway MACs (if any present on host) are formatted aa-bb-cc-dd-ee-ff
        const QStringList hostMacs = AuthManager::collectGatewayMacs();
        const QRegularExpression macFormat(QStringLiteral(R"(^[0-9a-f]{2}(-[0-9a-f]{2}){5}$)"));
        for (const QString &mac : hostMacs) {
            QVERIFY2(macFormat.match(mac).hasMatch(), qPrintable("Invalid MAC format: " + mac));
        }

        // Login succeeds and sends gateway_macs in payload
        QCOMPARE(controller.authenticate("t@example.com", "secret"), QString());
        QCOMPARE(controller.currentUserId(), 42);
        QCOMPARE(controller.getUserDepartment("tester"), QStringLiteral("QA"));

        // Assert sent /login body actually included gateway_macs array
        QJsonObject lastLoginBody;
        for (const auto &req : m_server->requests()) {
            if (req.path == "/login") lastLoginBody = QJsonDocument::fromJson(req.body).object();
        }
        QVERIFY(lastLoginBody.contains("gateway_macs"));
        QVERIFY(lastLoginBody["gateway_macs"].isArray());

        // Wait for login task fetch to populate task 501, then activate it
        QTRY_VERIFY_WITH_TIMEOUT(!controller.taskList().isEmpty(), 2000);
        controller.setActiveTask(501);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.isTaskPaused(), 4000);

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        WorkLogRepository logRepo;
        logRepo.logWindowChange({"Code", "main.cpp", ""}, now - 60, now - 10, 42);

        m_server->clearRequests();
        controller.logout();

        // Find request indices: end-implementation & productivity-app MUST appear with Bearer token BEFORE /logout
        int stopIdx = -1, usageIdx = -1, logoutIdx = -1;
        const auto &reqs = m_server->requests();
        for (int i = 0; i < reqs.size(); ++i) {
            if (reqs[i].path == "/end-implementation/501") {
                stopIdx = i;
                QCOMPARE(reqs[i].headers.value("authorization"), QStringLiteral("Bearer fresh-tok"));
            } else if (reqs[i].path == "/productivity-app") {
                usageIdx = i;
                QCOMPARE(reqs[i].headers.value("authorization"), QStringLiteral("Bearer fresh-tok"));
            } else if (reqs[i].path == "/logout") {
                logoutIdx = i;
                QCOMPARE(reqs[i].headers.value("authorization"), QStringLiteral("Bearer fresh-tok"));
            }
        }
        QVERIFY(stopIdx != -1);
        QVERIFY(usageIdx != -1);
        QVERIFY(logoutIdx != -1);
        QVERIFY(stopIdx < logoutIdx);
        QVERIFY(usageIdx < logoutIdx);
        QCOMPARE(controller.currentUserId(), -1);
        QCOMPARE(controller.savedUsername(), QStringLiteral("t@example.com"));
        QCOMPARE(controller.savedPassword(), QString());

        // Second logout() is a safe no-op
        m_server->clearRequests();
        controller.logout();
        QCOMPARE(m_server->requests().size(), 0);

        // Token error visibility resets on login: trigger 401 -> login -> trigger 401 again
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/login") {
                return {200, "OK", R"({"success":true,"token":"tok2","user":{"id":42,"name":"tester","email":"t@example.com","role":{"rolename":"QA"},"department":{"rolename":"Engineering"}}})"};
            }
            if (req.path == "/task-by-user/42") {
                return {401, "Unauthorized", R"({"message":"Unauthenticated"})"};
            }
            return {200, "OK", R"({"success":true,"data":[]})"};
        });
        QSignalSpy tokenErrSpy(&controller, &AppController::showAuthTokenErrorWindow);
        QCOMPARE(controller.authenticate("t@example.com", "secret"), QString());
        QTRY_COMPARE_WITH_TIMEOUT(tokenErrSpy.count(), 1, 3000);
        // Second login resets m_isTokenErrorVisible so another 401 fires again
        QCOMPARE(controller.authenticate("t@example.com", "secret"), QString());
        QTRY_COMPARE_WITH_TIMEOUT(tokenErrSpy.count(), 2, 3000);
    }

    void testIdleCheckerGuardsAndIntervals()
    {
        AppController controller;
        IdleChecker checker(&controller);
        qint64 fakeIdleSecs = 200;
        checker.setIdleTimeProvider([&fakeIdleSecs]() { return fakeIdleSecs; });
        checker.setIdleThreshold(60);

        QSignalSpy idleNotifSpy(&checker, &IdleChecker::showIdleNotification);
        QSignalSpy idleDetectedSpy(&checker, &IdleChecker::idleDetected);
        QSignalSpy hideNotifSpy(&checker, &IdleChecker::hideIdleNotification);

        // 1. Logged out -> checkIdleTimeAt is ignored
        checker.checkIdleTimeAt(1000);
        QVERIFY(!checker.isIdle());
        QCOMPARE(idleNotifSpy.count(), 0);

        // 2. Log in & activate a task
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(8, "idleuser", "pw", "i@example.com", "Dept", "Role", "tok-8"));
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/login") {
                return {200, "OK", R"({"success":true,"token":"tok-8","user":{"id":8,"name":"idleuser","email":"i@example.com","role":{"rolename":"Role"},"department":{"rolename":"Dept"}}})"};
            }
            if (req.path == "/task-by-user/8") {
                return {200, "OK", R"({"success":true,"data":[{"id":801,"title":"P","description":"Task","user_id":8,"duration":1,"total_duration":0,"created_at":"2026-11-21T00:00:00Z","status":"pending"}]})"};
            }
            return {200, "OK", R"({"success":true,"data":[]})"};
        });
        QCOMPARE(controller.authenticate("i@example.com", "pw"), QString());

        QTRY_VERIFY_WITH_TIMEOUT(!controller.taskList().isEmpty(), 2000);
        controller.setActiveTask(801);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.isTaskPaused(), 4000);

        // 3. Negative idleSeconds is skipped
        fakeIdleSecs = -1;
        checker.checkIdleTimeAt(2000);
        QVERIFY(!checker.isIdle());

        // 4. Entering idle emits "Idle Terdeteksi", auto-pauses task, and does NOT immediately emit idleDetected
        fakeIdleSecs = 100;
        checker.checkIdleTimeAt(2000);
        QVERIFY(checker.isIdle());
        QCOMPARE(idleNotifSpy.count(), 1);
        QCOMPARE(idleNotifSpy.first().at(0).toString(), QStringLiteral("Idle Terdeteksi"));
        QVERIFY(controller.isTaskPaused());
        QCOMPARE(idleDetectedSpy.count(), 0);

        // 5. At +10s and +60s, still no idleDetected (consolidated into single log on return)
        checker.checkIdleTimeAt(2010);
        QCOMPARE(idleDetectedSpy.count(), 0);

        checker.checkIdleTimeAt(2060);
        QCOMPARE(idleDetectedSpy.count(), 0);

        // 6. Return from idle -> emits a single consolidated idleDetected(2000, 2075), hides notification, auto-resumes task
        fakeIdleSecs = 0;
        checker.checkIdleTimeAt(2075);
        QVERIFY(!checker.isIdle());
        QCOMPARE(idleDetectedSpy.count(), 1);
        QCOMPARE(idleDetectedSpy.first().at(0).toLongLong(), 2000);
        QCOMPARE(idleDetectedSpy.first().at(1).toLongLong(), 2075);
        QCOMPARE(hideNotifSpy.count(), 1);
        QVERIFY(!controller.isTaskPaused());

        // 8. Manually paused task skips idle detection
        controller.toggleTaskPause();
        QVERIFY(controller.isTaskPaused());
        fakeIdleSecs = 200;
        checker.checkIdleTimeAt(3000);
        QVERIFY(!checker.isIdle());
    }

    void testProductivityStatsWorkTimeAndActivityTracker()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(15, "statsuser", "pw", "s@example.com", "D", "R", "tok-15"));

        ApiClient apiClient;
        AuthManager auth(&apiClient, &userRepo);
        auth.setCurrentUserInfo(15, "statsuser", "s@example.com", "tok-15");

        WorkLogRepository logRepo;
        ProductivityAppRepository prodRepo;
        WorkTimeRepository wtRepo;
        QVERIFY(logRepo.initialize());
        QVERIFY(prodRepo.initialize());

        // Seed 2 productive Chrome URL rules to verify calculateTodayProductiveSeconds does NOT double-count
        QSqlQuery q(DatabaseManager::instance().database());
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Chrome', 'github.com', 1, '0')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Chrome', 'docs.qt.io', 1, '0')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('YouTube', 'youtube.com', 2, '0')"));
        prodRepo.updateProductivityCache(15);

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        logRepo.logWindowChange({"Chrome", "GitHub", "https://github.com/org/repo"}, now - 300, now - 200, 15); // 100s productive
        logRepo.logWindowChange({"Chrome", "YouTube", "https://youtube.com/watch"}, now - 200, now - 150, 15); // 50s non-productive
        logRepo.logWindowChange({"Notepad", "notes.txt", ""}, now - 150, now - 100, 15); // 50s neutral

        ActivityTracker tracker(&logRepo, &auth);
        QSignalSpy trackerRefreshSpy(&tracker, &ActivityTracker::refreshRequested);
        tracker.logIdle(now - 100, now - 50); // 50s idle
        QCOMPARE(trackerRefreshSpy.count(), 1);

        // Verify ActivityTracker resets on loggedOut / loggedIn
        emit auth.loggedOut();
        QVERIFY(!tracker.isTrackingActive());
        emit auth.loggedIn();
        QVERIFY(tracker.isTrackingActive());

        ProductivityStatsService statsSvc(&apiClient, &logRepo, &prodRepo, &auth);
        QCOMPARE(statsSvc.productivePingIntervalMs(), 180000);
        QCOMPARE(statsSvc.calculateTodayProductiveSeconds(), 100);

        QVariantMap stats = statsSvc.productivityStats();
        QVERIFY(qAbs(stats["productive"].toDouble() - 40.0) < 0.01);
        QVERIFY(qAbs(stats["nonProductive"].toDouble() - 20.0) < 0.01);
        QVERIFY(qAbs(stats["neutral"].toDouble() - 20.0) < 0.01);
        QVERIFY(qAbs(stats["idle"].toDouble() - 20.0) < 0.01);

        QSignalSpy refreshSpy(&statsSvc, &ProductivityStatsService::refreshRequested);
        statsSvc.sendProductiveTimeToAPI();
        QCOMPARE(refreshSpy.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(!m_server->requests().isEmpty(), 2000);
        QCOMPARE(m_server->requests().last().path, QStringLiteral("/send-productive-time"));
        QCOMPARE(QJsonDocument::fromJson(m_server->requests().last().body).object()["productive_time"].toInt(), 100);

        // WorkTimeTracker earlyLeaveSubmitFailed & earlyLeaveReasonSubmitted
        WorkTimeTracker wtTracker(&apiClient, &wtRepo, &auth);
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/send-alasan") {
                QJsonObject body = QJsonDocument::fromJson(req.body).object();
                if (body["alasan"].toString() == "fail") {
                    return {200, "OK", R"({"success":false,"message":"Reason rejected"})"};
                }
                return {200, "OK", R"({"success":true})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        QSignalSpy failSpy(&wtTracker, &WorkTimeTracker::earlyLeaveSubmitFailed);
        QSignalSpy okSpy(&wtTracker, &WorkTimeTracker::earlyLeaveReasonSubmitted);
        wtTracker.submitEarlyLeaveReason("fail");
        QTRY_COMPARE_WITH_TIMEOUT(failSpy.count(), 1, 3000);
        QCOMPARE(okSpy.count(), 0);

        wtTracker.submitEarlyLeaveReason("family emergency");
        QTRY_COMPARE_WITH_TIMEOUT(okSpy.count(), 1, 3000);
    }

    void testCalculateTodayProductiveSecondsMatchesLookupAndTryAutoLoginResetsTokenError()
    {
        QVERIFY(DatabaseManager::instance().initialize());
        UserRepository userRepo;
        QVERIFY(userRepo.saveUser(16, "consuser", "pw", "c@example.com", "D", "R", "tok-16"));

        ApiClient apiClient;
        AuthManager auth(&apiClient, &userRepo);

        // tryAutoLogin() must reset m_isTokenErrorVisible
        QSignalSpy tokenErrSpy(&auth, &AuthManager::showAuthTokenErrorWindow);
        auth.showAuthTokenErrorMessage();
        auth.showAuthTokenErrorMessage();
        QCOMPARE(tokenErrSpy.count(), 1);
        QVERIFY(auth.tryAutoLogin());
        QCOMPARE(auth.currentUserId(), 16);
        auth.showAuthTokenErrorMessage();
        QCOMPARE(tokenErrSpy.count(), 2);

        WorkLogRepository logRepo;
        ProductivityAppRepository prodRepo;
        QVERIFY(logRepo.initialize());
        QVERIFY(prodRepo.initialize());

        QSqlQuery q(DatabaseManager::instance().database());
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('YouTube', 'youtube.com', 1, '0')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('VSCode', 'N/A', 1, '0')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Slack', NULL, 2, '16')"));
        QVERIFY(q.exec("INSERT INTO productivity_apps (app_name, url, productivity_type, for_user) VALUES ('Slack', NULL, 1, '0')"));

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        logRepo.logWindowChange({"Chrome", "Not YouTube", "https://notyoutube.com/watch"}, now - 200, now - 100, 16); // 100s neutral
        logRepo.logWindowChange({"VSCode", "main.cpp", ""}, now - 100, now - 60, 16); // 40s productive via N/A rule
        logRepo.logWindowChange({"Slack", "General", ""}, now - 60, now, 16); // 60s non-productive via user-16 override

        ProductivityStatsService statsSvc(&apiClient, &logRepo, &prodRepo, &auth);
        QCOMPARE(statsSvc.calculateTodayProductiveSeconds(), 40);

        QVariantMap stats = statsSvc.productivityStats();
        QVERIFY(qAbs(stats["productive"].toDouble() - 20.0) < 0.01);
        QVERIFY(qAbs(stats["nonProductive"].toDouble() - 30.0) < 0.01);
        QVERIFY(qAbs(stats["neutral"].toDouble() - 50.0) < 0.01);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<MockHttpServer> m_server;
};

QTEST_MAIN(TestServicesIdleAuth)
#include "test_services_idle_auth.moc"
