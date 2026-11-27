#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QSettings>
#include <QSqlQuery>
#include <algorithm>

#include "MockHttpServer.h"
#include "core/database/DatabaseManager.h"
#include "core/database/TaskRepository.h"
#include "core/database/UserRepository.h"
#include "core/network/ApiClient.h"
#include "features/auth/AuthManager.h"
#include "features/tasks/TaskManager.h"

class TestTaskManagement : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());
        qputenv("DESKMON_DB_PATH", m_tempDir->filePath("task_test.db").toUtf8());
        QVERIFY(DatabaseManager::instance().initialize());

        m_server = std::make_unique<MockHttpServer>();
        QVERIFY(m_server->start());
        qputenv("DESKMON_API_BASE_URL", m_server->baseUrl().toUtf8());

        m_apiClient = std::make_unique<ApiClient>();
        m_userRepo = std::make_unique<UserRepository>();
        m_taskRepo = std::make_unique<TaskRepository>();
        m_authManager = std::make_unique<AuthManager>(m_apiClient.get(), m_userRepo.get());
        m_taskManager = std::make_unique<TaskManager>(m_apiClient.get(), m_taskRepo.get(), m_authManager.get());

        QVERIFY(m_userRepo->saveUser(10, "dev", "pw", "dev@example.com", "Eng", "Dev", "tok-10"));
        m_authManager->setCurrentUserInfo(10, "dev", "dev@example.com", "tok-10");
    }

    void cleanup()
    {
        QSettings().remove(QStringLiteral("need_review_queue"));
        m_taskManager.reset();
        m_authManager.reset();
        m_taskRepo.reset();
        m_userRepo.reset();
        m_apiClient.reset();
        m_server.reset();
        DatabaseManager::instance().close();
        qunsetenv("DESKMON_API_BASE_URL");
        qunsetenv("DESKMON_DB_PATH");
        m_tempDir.reset();
    }

    void testTaskRepositoryDefaultsExpiryPendingCountAndLabels()
    {
        const QDate today = QDate::currentDate();
        const QString sameMonthIso = QDateTime(QDate(today.year(), today.month(), 1), QTime(9, 0)).toString(Qt::ISODate);
        const QDate prevMonthDate = today.addMonths(-1);
        const QString prevMonthIso = QDateTime(QDate(prevMonthDate.year(), prevMonthDate.month(), 1), QTime(9, 0)).toString(Qt::ISODate);

        QVERIFY(m_taskRepo->upsertTask(1, 10, "Proj", "Current Month", 28800, 0, sameMonthIso));
        QVERIFY(m_taskRepo->upsertTask(2, 10, "Proj", "Expired Task", 28800, 120, prevMonthIso));
        QVERIFY(m_taskRepo->upsertTask(3, 10, "Proj", "Long Cross-Month", 800 * 3600, 0, prevMonthIso));
        QVERIFY(m_taskRepo->upsertTask(4, 10, "Proj", "Review Task", 28800, 50, sameMonthIso, "review"));

        // Default status must be 'Pending'
        int maxT = 0, usage = 0;
        QString status;
        QVERIFY(m_taskRepo->getTaskDetails(1, maxT, usage, status));
        QCOMPARE(status, QStringLiteral("Pending"));

        // Expiry check
        QVERIFY(!m_taskRepo->isTaskExpired(1));
        QVERIFY(m_taskRepo->isTaskExpired(2));
        QVERIFY(!m_taskRepo->isTaskExpired(3));
        QVERIFY(!m_taskManager->isTaskExpired(1));
        QVERIFY(m_taskManager->isTaskExpired(2));

        // getPendingStartedTaskCount counts only Pending with time_usage > 0
        QCOMPARE(m_taskRepo->getPendingStartedTaskCount(10), 1);
        QCOMPARE(m_taskManager->getPendingStartedTaskCount(), 1);

        // Status label mapping
        QVERIFY(m_taskRepo->updateTaskTiming(1, 0, true, false));
        QVariantList pausedList = m_taskRepo->getTasksForUser(10, true);
        QVariantList runningList = m_taskRepo->getTasksForUser(10, false);

        auto findStatus = [](const QVariantList &list, int id) -> QString {
            for (const QVariant &v : list) {
                if (v.toMap()["id"].toInt() == id) return v.toMap()["status"].toString();
            }
            return QString();
        };

        QCOMPARE(findStatus(pausedList, 1), QStringLiteral("Paused"));
        QCOMPARE(findStatus(runningList, 1), QStringLiteral("Is Running"));
        QCOMPARE(findStatus(runningList, 4), QStringLiteral("Review"));
        QCOMPARE(findStatus(runningList, 2), QStringLiteral("Pending"));
    }

    void testSetActiveTaskSwitchDeactivateAnd2sDelay()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(101, 10, "P1", "Task A", 3600, 500, created));
        QVERIFY(m_taskRepo->upsertTask(102, 10, "P2", "Task B", 0, 250, created));

        QSignalSpy detailsSpy(m_taskManager.get(), &TaskManager::requestTaskDetails);
        QSignalSpy saveWorkTimeSpy(m_taskManager.get(), &TaskManager::saveWorkTimeRequested);

        // Activate Task A -> initially paused, max_time unchanged (3600)
        m_taskManager->setActiveTask(101);
        QCOMPARE(m_taskManager->activeTaskId(), 101);
        QVERIFY(m_taskManager->isTaskPaused());

        // Wait >2s for automatic transition to on-progress + ping
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);
        QTRY_COMPARE_WITH_TIMEOUT(m_server->requests().size(), 1, 2000);
        QCOMPARE(m_server->requests().last().path, QStringLiteral("/ping"));
        QJsonObject pingBody = QJsonDocument::fromJson(m_server->requests().last().body).object();
        QCOMPARE(pingBody["task_id"].toString(), QStringLiteral("101"));

        m_server->clearRequests();

        // Switch from running Task A to Task B
        m_taskManager->setActiveTask(102);

        // Emits requestTaskDetails(101, "switch", 102) without aborting switch
        QCOMPARE(detailsSpy.count(), 1);
        QCOMPARE(detailsSpy.first().at(0).toInt(), 101);
        QCOMPARE(detailsSpy.first().at(1).toString(), QStringLiteral("switch"));
        QCOMPARE(detailsSpy.first().at(2).toInt(), 102);
        QCOMPARE(saveWorkTimeSpy.count(), 1);

        // Task B keeps its own time_usage (250), NOT overwritten by Task A's 500, and max_time defaults to 28800
        int maxA = 0, usageA = 0, maxB = 0, usageB = 0;
        QString statusA, statusB;
        QVERIFY(m_taskRepo->getTaskDetails(101, maxA, usageA, statusA));
        QVERIFY(m_taskRepo->getTaskDetails(102, maxB, usageB, statusB));
        QVERIFY(usageA >= 500);
        QCOMPARE(statusA, QStringLiteral("Pending"));
        QCOMPARE(usageB, 250);
        QCOMPARE(maxB, 28800);
        QCOMPARE(statusB, QStringLiteral("Paused"));

        // Verify PUT /end-implementation/101 {"status":"stop"} was sent
        QTRY_VERIFY_WITH_TIMEOUT(!m_server->requests().isEmpty(), 2000);
        bool sawStopA = false;
        for (const auto &req : m_server->requests()) {
            if (req.method == "PUT" && req.path == "/end-implementation/101") {
                sawStopA = QJsonDocument::fromJson(req.body).object()["status"].toString() == "stop";
            }
        }
        QVERIFY(sawStopA);

        // Wait for Task B 2s delayed start
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);
        QVERIFY(m_taskRepo->getTaskDetails(102, maxB, usageB, statusB));
        QCOMPARE(statusB, QStringLiteral("on-progress"));

        // Deactivate via setActiveTask(-1) cleanly without FK errors
        m_taskManager->setActiveTask(-1);
        QCOMPARE(m_taskManager->activeTaskId(), -1);
        QVERIFY(!m_taskManager->isTaskPaused());
    }

    void testPingTimerAndStatusRepliesAndDetailsRetry()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(201, 10, "P1", "Task One", 3600, 100, created));
        QVERIFY(m_taskRepo->upsertTask(202, 10, "P2", "Task Two", 3600, 0, created));

        // Activate 201 and let it start
        m_taskManager->setActiveTask(201);
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);

        // Pause 201 and verify startPingTimer sends user_id (not task_id) while paused
        m_taskManager->toggleTaskPause();
        QVERIFY(m_taskManager->isTaskPaused());
        QTRY_VERIFY_WITH_TIMEOUT(std::any_of(m_server->requests().cbegin(), m_server->requests().cend(),
            [](const RecordedHttpRequest &r) { return r.path == "/end-implementation/201"; }), 2000);
        m_server->clearRequests();
        m_taskManager->startPingTimer();
        QTRY_VERIFY_WITH_TIMEOUT(!m_server->requests().isEmpty(), 2000);
        QJsonObject pausedPing = QJsonDocument::fromJson(m_server->requests().last().body).object();
        QVERIFY(pausedPing.contains("user_id"));
        QVERIFY(!pausedPing.contains("task_id"));
        m_taskManager->stopPingTimer();

        // Resume 201 and test handleTaskStatusReply with object {"status": "need-revise"} -> auto-switch to 202
        m_taskManager->toggleTaskPause();
        QVERIFY(!m_taskManager->isTaskPaused());

        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/get-current-task-status/201") {
                return {200, "OK", R"({"success":true,"data":{"status":"need-revise"}})"};
            }
            if (req.path == "/send-detail-pekerjaan") {
                return {500, "Internal Server Error", R"({"success":false,"message":"Server down"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        m_taskManager->updateTaskStatus(201);
        QTRY_COMPARE_WITH_TIMEOUT(m_taskManager->activeTaskId(), 202, 3000);
        // Wait for 202's 2s timer to settle or cancel by stopping
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);

        int maxT = 0, usage = 0;
        QString status201;
        QVERIFY(m_taskRepo->getTaskDetails(201, maxT, usage, status201));
        QCOMPARE(status201, QStringLiteral("Need Revise"));

        // String data "on-review" on 202 -> Review + review notification
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/get-current-task-status/202") {
                return {200, "OK", R"({"success":true,"data":"on-review"})"};
            }
            if (req.path == "/send-detail-pekerjaan") {
                return {500, "Internal Server Error", R"({"success":false,"message":"Server down"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });
        QSignalSpy reviewSpy(m_taskManager.get(), &TaskManager::taskReviewNotification);
        m_taskManager->updateTaskStatus(202);
        QTRY_COMPARE_WITH_TIMEOUT(reviewSpy.count(), 1, 3000);
        QCOMPARE(m_taskManager->activeTaskId(), -1);

        // sendTaskDetailsToAPI emits taskDetailsSubmissionFailedWithRetry (5 args) and does NOT emit readyToProceedWithQuit
        QSignalSpy retrySpy(m_taskManager.get(), &TaskManager::taskDetailsSubmissionFailedWithRetry);
        QSignalSpy quitSpy(m_taskManager.get(), &TaskManager::readyToProceedWithQuit);
        m_taskManager->submitTaskDetails(201, "worked on bug", "quit", -1);
        QTRY_COMPARE_WITH_TIMEOUT(retrySpy.count(), 1, 3000);
        QCOMPARE(quitSpy.count(), 0);
        QCOMPARE(retrySpy.first().at(1).toInt(), 201);
        QCOMPARE(retrySpy.first().at(2).toString(), QStringLiteral("worked on bug"));
        QCOMPARE(retrySpy.first().at(3).toString(), QStringLiteral("quit"));
        QCOMPARE(retrySpy.first().at(4).toInt(), -1);
    }

    void testCheckTaskStatusBeforeStartAndHttp401Handling()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(301, 10, "P1", "Startup Task", 3600, 420, created, "on-progress"));
        QVERIFY(m_taskRepo->updateTaskTiming(301, 420, true, true));

        m_taskManager->checkTaskStatusBeforeStart();
        QCOMPARE(m_taskManager->activeTaskId(), 301);
        QVERIFY(!m_taskManager->isTaskPaused());

        // HTTP 401 on task-by-user triggers showAuthTokenErrorWindow
        m_server->setHandler([](const RecordedHttpRequest &) -> MockHttpResponse {
            return {401, "Unauthorized", R"({"message":"Unauthenticated"})"};
        });
        QSignalSpy authErrSpy(m_authManager.get(), &AuthManager::showAuthTokenErrorWindow);
        m_taskManager->fetchAndStoreTasks();
        QTRY_COMPARE_WITH_TIMEOUT(authErrSpy.count(), 1, 3000);
    }

    void testSetActiveTaskVersioned2sTimerRaces()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(401, 10, "P1", "Early Resume Task", 3600, 0, created));
        QVERIFY(m_taskRepo->upsertTask(402, 10, "P2", "Deactivate Reactivate Task", 3600, 0, created));

        // 3A: Early toggleTaskPause before 2s timer fires must NOT double-log play or double-ping
        m_taskManager->setActiveTask(401);
        QVERIFY(m_taskManager->isTaskPaused());
        QTest::qWait(200);
        m_taskManager->toggleTaskPause();
        QVERIFY(!m_taskManager->isTaskPaused());
        QTRY_VERIFY_WITH_TIMEOUT(std::any_of(m_server->requests().cbegin(), m_server->requests().cend(),
            [](const RecordedHttpRequest &r) { return r.path == QStringLiteral("/ping"); }), 2000);

        m_server->clearRequests();
        QTest::qWait(2100);

        QSqlQuery playCountQ(DatabaseManager::instance().database());
        playCountQ.prepare("SELECT COUNT(*) FROM task_pause_logs WHERE task_id = 401 AND status = 'play'");
        QVERIFY(playCountQ.exec() && playCountQ.next());
        QCOMPARE(playCountQ.value(0).toInt(), 1);

        int pingCount = 0;
        for (const auto &req : m_server->requests()) {
            if (req.path == QStringLiteral("/ping")) pingCount++;
        }
        QCOMPARE(pingCount, 0);

        // 3B: Deactivate + reactivate same task within 2s must NOT prematurely unpause 2nd activation
        m_taskManager->setActiveTask(402);
        QVERIFY(m_taskManager->isTaskPaused());
        QTest::qWait(200);
        m_taskManager->setActiveTask(-1);
        QCOMPARE(m_taskManager->activeTaskId(), -1);
        QTest::qWait(1300);
        m_taskManager->setActiveTask(402);
        QCOMPARE(m_taskManager->activeTaskId(), 402);
        QVERIFY(m_taskManager->isTaskPaused());

        QTest::qWait(700);
        QVERIFY(m_taskManager->isTaskPaused());
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 3000);
    }

    void testOnProgressStatusSyncAndHttp200SuccessFalse()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(403, 10, "P1", "On Progress Sync Task", 3600, 100, created));

        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/get-current-task-status/403") {
                return {200, "OK", R"({"success":true,"data":"on-progress"})"};
            }
            if (req.path == "/send-detail-pekerjaan") {
                return {200, "OK", R"({"success":false,"message":"Task details rejected"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        m_taskManager->updateTaskStatus(403);
        QTRY_COMPARE_WITH_TIMEOUT(m_taskManager->activeTaskId(), 403, 3000);
        QVERIFY(!m_taskManager->isTaskPaused());

        QSqlQuery q(DatabaseManager::instance().database());
        QVERIFY(q.exec("SELECT active, paused, status FROM tasks WHERE id = 403"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        QCOMPARE(q.value(1).toInt(), 0);
        QCOMPARE(q.value(2).toString(), QStringLiteral("on-progress"));

        // Ensure canceled 2s timer from setActiveTask does not fire later and double-log play
        QTest::qWait(2200);
        QVERIFY(q.exec("SELECT COUNT(*) FROM task_pause_logs WHERE task_id = 403 AND status = 'play'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);

        // HTTP 200 {"success": false} on sendTaskDetailsToAPI must trigger retry signal, NOT success
        QSignalSpy okSpy(m_taskManager.get(), &TaskManager::taskDetailsSubmissionSuccess);
        QSignalSpy retrySpy(m_taskManager.get(), &TaskManager::taskDetailsSubmissionFailedWithRetry);
        QSignalSpy quitSpy(m_taskManager.get(), &TaskManager::readyToProceedWithQuit);

        m_taskManager->submitTaskDetails(403, "details", "quit", -1);
        QTRY_COMPARE_WITH_TIMEOUT(retrySpy.count(), 1, 3000);
        QCOMPARE(okSpy.count(), 0);
        QCOMPARE(quitSpy.count(), 0);
        QCOMPARE(retrySpy.first().at(0).toString(), QStringLiteral("Task details rejected"));
    }

    void testBulkAsyncTaskFetchSync()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(999, 10, "StaleProj", "To Be Removed", 3600, 0, created));
        QVERIFY(m_taskRepo->upsertTask(1000, 10, "Project_0", "Bulk Task 0", 3600, 0, created, "Review"));

        QJsonArray tasksArray;
        for (int i = 0; i < 300; ++i) {
            tasksArray.append(QJsonObject{
                {"id", 1000 + i},
                {"title", QString("Project_%1").arg(i % 5)},
                {"description", QString("Bulk Task %1").arg(i)},
                {"user_id", 10},
                {"status", "pending"},
                {"duration", 4.0},
                {"total_duration", 0.25},
                {"created_at", created}
            });
        }
        const QByteArray payload = QJsonDocument(QJsonObject{
            {"success", true},
            {"data", tasksArray}
        }).toJson(QJsonDocument::Compact);

        m_server->setHandler([payload](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path.startsWith("/task-by-user/")) {
                return {200, "OK", payload};
            }
            return {200, "OK", R"({"success":true})"};
        });

        m_taskManager->fetchAndStoreTasks();
        QTRY_COMPARE_WITH_TIMEOUT(m_taskManager->taskList().size(), 300, 3000);

        QSqlQuery q(DatabaseManager::instance().database());
        QVERIFY(q.exec("SELECT COUNT(*) FROM tasks WHERE user_id = 10"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 300);

        int maxT = 0, usage = 0;
        QString status;
        QVERIFY(!m_taskRepo->getTaskDetails(999, maxT, usage, status));
        QVERIFY(m_taskRepo->getTaskDetails(1000, maxT, usage, status));
        QCOMPARE(maxT, 14400);
        QCOMPARE(usage, 900);
        QCOMPARE(status, QStringLiteral("Pending"));
    }

    void testNeedReviewDoesNotSwitchAndRetriesFallbackUntilConfirmed()
    {
        const QString created = QDateTime::currentDateTime().toString(Qt::ISODate);
        QVERIFY(m_taskRepo->upsertTask(501, 10, "P1", "Active Task One", 3600, 100, created));
        QVERIFY(m_taskRepo->upsertTask(502, 10, "P2", "Other Eligible Task", 3600, 0, created));

        // 1. Activate 501; when server returns "need-review" on status check, must NOT auto-switch to 502
        m_taskManager->setActiveTask(501);
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);

        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/get-current-task-status/501") {
                return {200, "OK", R"({"success":true,"data":"need-review"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        m_taskManager->updateTaskStatus(501);
        QTRY_COMPARE_WITH_TIMEOUT(m_taskManager->activeTaskId(), -1, 3000);
        QCOMPARE(m_taskManager->activeTaskId(), -1);
        int maxT = 0, usage = 0;
        QString status501;
        QVERIFY(m_taskRepo->getTaskDetails(501, maxT, usage, status501));
        QCOMPARE(status501, QStringLiteral("Need Review"));

        // 2. Activate 502 and submit Need Review while server fails (HTTP 500) -> fallback + locked + no other task selected
        m_taskManager->setActiveTask(502);
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPaused(), 4000);

        int postAttempts = 0;
        m_server->setHandler([&postAttempts](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/update-status-task/502/10") {
                ++postAttempts;
                return {500, "Internal Server Error", R"({"success":false})"};
            }
            return {200, "OK", R"({"success":true})"};
        });

        QSignalSpy pendingSpy(m_taskManager.get(), &TaskManager::pendingNeedReviewTasksChanged);
        m_taskManager->submitNeedReview(502, "Need supervisor review");

        QCOMPARE(m_taskManager->activeTaskId(), -1);
        QVERIFY(m_taskManager->isTaskPendingNeedReview(502));
        QVERIFY(m_taskManager->pendingNeedReviewTaskIds().contains(QVariant(502)));
        QTRY_VERIFY_WITH_TIMEOUT(postAttempts >= 1, 2000);

        // Selecting 502 while awaiting Need Review confirmation must be blocked
        m_taskManager->setActiveTask(502);
        QCOMPARE(m_taskManager->activeTaskId(), -1);

        // 3. Recover POST endpoint -> returns 200 OK, but /task-by-user and /get-current-task-status still show "pending" briefly
        m_server->setHandler([&postAttempts, created](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/update-status-task/502/10") {
                ++postAttempts;
                return {200, "OK", R"({"success":true})"};
            }
            if (req.path == "/get-current-task-status/502") {
                return {200, "OK", R"({"success":true,"data":"pending"})"};
            }
            if (req.path == "/task-by-user/10") {
                QJsonArray arr;
                arr.append(QJsonObject{{"id", 502}, {"title", "P2"}, {"description", "Other Eligible Task"}, {"user_id", 10}, {"status", "pending"}, {"duration", 1.0}, {"total_duration", 0.0}, {"created_at", created}});
                return {200, "OK", QJsonDocument(QJsonObject{{"success", true}, {"data", arr}}).toJson(QJsonDocument::Compact)};
            }
            return {200, "OK", R"({"success":true})"};
        });

        QMetaObject::invokeMethod(m_taskManager.get(), "processPendingNeedReviewQueue");
        QTRY_VERIFY_WITH_TIMEOUT(postAttempts >= 2, 3000);
        QTest::qWait(300);
        // Must remain locked (grey + loading) until server confirms "need-review"
        QVERIFY(m_taskManager->isTaskPendingNeedReview(502));

        // 4. Server finally reports "need-review" -> clears pending lock and marks status as "Need Review"
        m_server->setHandler([](const RecordedHttpRequest &req) -> MockHttpResponse {
            if (req.path == "/get-current-task-status/502") {
                return {200, "OK", R"({"success":true,"data":"need-review"})"};
            }
            return {200, "OK", R"({"success":true})"};
        });
        m_taskManager->updateTaskStatus(502);
        QTRY_VERIFY_WITH_TIMEOUT(!m_taskManager->isTaskPendingNeedReview(502), 3000);
        QCOMPARE(m_taskManager->pendingNeedReviewTaskIds().size(), 0);
        QString status502;
        QVERIFY(m_taskRepo->getTaskDetails(502, maxT, usage, status502));
        QCOMPARE(status502, QStringLiteral("Need Review"));
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<MockHttpServer> m_server;
    std::unique_ptr<ApiClient> m_apiClient;
    std::unique_ptr<UserRepository> m_userRepo;
    std::unique_ptr<TaskRepository> m_taskRepo;
    std::unique_ptr<AuthManager> m_authManager;
    std::unique_ptr<TaskManager> m_taskManager;
};

QTEST_GUILESS_MAIN(TestTaskManagement)
#include "test_task_management.moc"
