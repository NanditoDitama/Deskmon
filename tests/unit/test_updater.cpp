#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include "MockHttpServer.h"
#include "core/network/ApiClient.h"
#include "features/updater/UpdateManager.h"

namespace {

QByteArray createTestZipBytes(const QTemporaryDir &workDir, const QString &innerRelPath, const QByteArray &content)
{
    const QString srcRoot = workDir.filePath(QStringLiteral("zip_src"));
    const QString payloadFile = QDir(srcRoot).filePath(innerRelPath);
    QDir().mkpath(QFileInfo(payloadFile).absolutePath());

    QFile f(payloadFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    f.write(content);
    f.close();

    const QString zipOut = workDir.filePath(QStringLiteral("test_payload.zip"));
    QFile::remove(zipOut);

    QProcess proc;
#ifdef Q_OS_WIN
    proc.start(QStringLiteral("tar"), {QStringLiteral("-a"), QStringLiteral("-cf"), QDir::toNativeSeparators(zipOut), QStringLiteral("-C"), QDir::toNativeSeparators(srcRoot), innerRelPath});
#else
    proc.start(QStringLiteral("zip"), {QStringLiteral("-q"), QStringLiteral("-r"), zipOut, innerRelPath}, QIODevice::ReadWrite);
    proc.setWorkingDirectory(srcRoot);
#endif
    if (!proc.waitForFinished(10000) || proc.exitCode() != 0) {
#ifdef Q_OS_WIN
        QProcess ps;
        const QString cmd = QStringLiteral("Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
            .arg(QDir::toNativeSeparators(srcRoot), QDir::toNativeSeparators(zipOut));
        ps.start(QStringLiteral("powershell.exe"), {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-Command"), cmd});
        ps.waitForFinished(10000);
#endif
    }

    QFile zf(zipOut);
    if (!zf.open(QIODevice::ReadOnly)) {
        return {};
    }
    return zf.readAll();
}

} // namespace

class TestUpdater : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());

        m_updatesDir = m_tempDir->filePath(QStringLiteral("updates"));
        m_server = std::make_unique<MockHttpServer>();
        QVERIFY(m_server->start());
    }

    void cleanup()
    {
        m_server.reset();
        m_tempDir.reset();
    }

    void testUpToDateVersionRemainsIdle()
    {
        m_server->setHandler([](const RecordedHttpRequest &req) {
            Q_UNUSED(req);
            QJsonObject obj;
            obj.insert(QStringLiteral("version"), QStringLiteral("2.0.0.0"));
            obj.insert(QStringLiteral("releaseNotes"), QStringLiteral("No changes"));
            return MockHttpResponse{200, "OK", QJsonDocument(obj).toJson(), "application/json", false};
        });

        ApiClient api;
        UpdateManager mgr(&api, QStringLiteral("2.0.0.0"));
        mgr.setUpdatesRootDir(m_updatesDir);
        mgr.setManifestUrlOverride(m_server->baseUrl() + QStringLiteral("/version.json"));

        QSignalSpy notifSpy(&mgr, &UpdateManager::showNotification);
        mgr.checkForUpdates(true);
        QTRY_COMPARE_WITH_TIMEOUT(mgr.updateState(), QStringLiteral("idle"), 5000);
        QCOMPARE(m_server->requests().size(), 1);
        QCOMPARE(notifSpy.count(), 0);

        mgr.checkForUpdates(false);
        QTRY_COMPARE_WITH_TIMEOUT(notifSpy.count(), 1, 5000);
        QCOMPARE(mgr.updateState(), QStringLiteral("idle"));
    }

    void testAutoDownloadVerifyAndStageRealZip()
    {
        const QByteArray expectedBytes = QByteArray("Deskmon v2.1.0.0 binary payload");
        const QByteArray zipBytes = createTestZipBytes(*m_tempDir, QStringLiteral("Deskmon/version.txt"), expectedBytes);
        QVERIFY(!zipBytes.isEmpty());

        const QString sha256Hex = QString::fromLatin1(QCryptographicHash::hash(zipBytes, QCryptographicHash::Sha256).toHex());
        const QString platformKey = UpdateManager::candidatePlatformKeys().first();
        const QString assetUrl = m_server->baseUrl() + QStringLiteral("/Deskmon-2.1.0.0.zip");

        m_server->setHandler([zipBytes, assetUrl, sha256Hex, platformKey](const RecordedHttpRequest &req) {
            if (req.path == QStringLiteral("/version.json")) {
                QJsonObject asset;
                asset.insert(QStringLiteral("url"), assetUrl);
                asset.insert(QStringLiteral("sha256"), sha256Hex);
                asset.insert(QStringLiteral("type"), QStringLiteral("zip"));

                QJsonObject platforms;
                platforms.insert(platformKey, asset);

                QJsonObject manifest;
                manifest.insert(QStringLiteral("version"), QStringLiteral("2.1.0.0"));
                manifest.insert(QStringLiteral("releaseNotes"), QStringLiteral("New auto-update engine"));
                manifest.insert(QStringLiteral("platforms"), platforms);
                return MockHttpResponse{200, "OK", QJsonDocument(manifest).toJson(), "application/json", false};
            }
            if (req.path == QStringLiteral("/Deskmon-2.1.0.0.zip")) {
                return MockHttpResponse{200, "OK", zipBytes, "application/zip", false};
            }
            return MockHttpResponse{404, "Not Found", "missing", "text/plain", false};
        });

        ApiClient api;
        UpdateManager mgr(&api, QStringLiteral("2.0.0.0"));
        mgr.setUpdatesRootDir(m_updatesDir);
        mgr.setManifestUrlOverride(m_server->baseUrl() + QStringLiteral("/version.json"));

        QSignalSpy readySpy(&mgr, &UpdateManager::updateReadyToRestart);
        mgr.checkForUpdates(true);

        QTRY_COMPARE_WITH_TIMEOUT(mgr.updateState(), QStringLiteral("ready"), 15000);
        QCOMPARE(mgr.updateProgress(), 100);
        QVERIFY(mgr.isReadyToRestart());
        QCOMPARE(mgr.availableVersion(), QStringLiteral("2.1.0.0"));
        QCOMPARE(mgr.releaseNotes(), QStringLiteral("New auto-update engine"));
        QCOMPARE(readySpy.count(), 1);

        const StagedUpdateInfo staged = UpdateManager::loadReadyMetadata(m_updatesDir);
        QVERIFY(staged.valid);
        QCOMPARE(staged.version, QStringLiteral("2.1.0.0"));
        QVERIFY(QFile::exists(QDir(staged.payloadDir).filePath(QStringLiteral("version.txt"))));

        // Re-running startup bootstrapper with SAME version cleans up staging instead of looping
        const bool launchedOnSameVersion = UpdateManager::tryApplyStagedUpdateOnStartup(
            QStringLiteral("2.1.0.0"),
            m_tempDir->path(),
            m_tempDir->filePath(QStringLiteral("Deskmon.exe")),
            1234,
            m_updatesDir);
        QVERIFY(!launchedOnSameVersion);
        QVERIFY(!UpdateManager::loadReadyMetadata(m_updatesDir).valid);
    }

    void testSha256MismatchRejectsStagedUpdate()
    {
        const QByteArray zipBytes = createTestZipBytes(*m_tempDir, QStringLiteral("version.txt"), QByteArray("tampered"));
        QVERIFY(!zipBytes.isEmpty());

        const QString platformKey = UpdateManager::candidatePlatformKeys().first();
        const QString assetUrl = m_server->baseUrl() + QStringLiteral("/bad.zip");

        m_server->setHandler([zipBytes, assetUrl, platformKey](const RecordedHttpRequest &req) {
            if (req.path == QStringLiteral("/version.json")) {
                QJsonObject asset;
                asset.insert(QStringLiteral("url"), assetUrl);
                asset.insert(QStringLiteral("sha256"), QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000"));
                asset.insert(QStringLiteral("type"), QStringLiteral("zip"));

                QJsonObject platforms;
                platforms.insert(platformKey, asset);

                QJsonObject manifest;
                manifest.insert(QStringLiteral("version"), QStringLiteral("2.2.0.0"));
                manifest.insert(QStringLiteral("releaseNotes"), QStringLiteral("Broken hash"));
                manifest.insert(QStringLiteral("platforms"), platforms);
                return MockHttpResponse{200, "OK", QJsonDocument(manifest).toJson(), "application/json", false};
            }
            return MockHttpResponse{200, "OK", zipBytes, "application/zip", false};
        });

        ApiClient api;
        UpdateManager mgr(&api, QStringLiteral("2.0.0.0"));
        mgr.setUpdatesRootDir(m_updatesDir);
        mgr.setManifestUrlOverride(m_server->baseUrl() + QStringLiteral("/version.json"));

        mgr.checkForUpdates(false);
        QTRY_COMPARE_WITH_TIMEOUT(mgr.updateState(), QStringLiteral("error"), 10000);
        QVERIFY(!mgr.isReadyToRestart());
        QVERIFY(!mgr.updateErrorMessage().isEmpty());
        QVERIFY(!UpdateManager::loadReadyMetadata(m_updatesDir).valid);
    }

    void testManualFallbackWhenPlatformMissing()
    {
        m_server->setHandler([](const RecordedHttpRequest &req) {
            Q_UNUSED(req);
            QJsonObject manifest;
            manifest.insert(QStringLiteral("version"), QStringLiteral("2.3.0.0"));
            manifest.insert(QStringLiteral("releaseNotes"), QStringLiteral("Manual download only"));
            manifest.insert(QStringLiteral("releasePageUrl"), QStringLiteral("https://gitlab.example.com/deskmon/-/releases"));
            return MockHttpResponse{200, "OK", QJsonDocument(manifest).toJson(), "application/json", false};
        });

        ApiClient api;
        UpdateManager mgr(&api, QStringLiteral("2.0.0.0"));
        mgr.setUpdatesRootDir(m_updatesDir);
        mgr.setManifestUrlOverride(m_server->baseUrl() + QStringLiteral("/version.json"));

        QSignalSpy availableSpy(&mgr, &UpdateManager::updateAvailable);
        mgr.checkForUpdates(true);

        QTRY_COMPARE_WITH_TIMEOUT(mgr.updateState(), QStringLiteral("manual"), 5000);
        QCOMPARE(availableSpy.count(), 1);
        QCOMPARE(mgr.releasePageUrl(), QStringLiteral("https://gitlab.example.com/deskmon/-/releases"));
    }

    void testBuildSwapScriptContentPerOs()
    {
        const QString winRestart = UpdateManager::buildSwapScriptContent(
            QStringLiteral("windows"), 4242, QStringLiteral("C:/upd/payload"),
            QStringLiteral("C:/Apps/Deskmon"), QStringLiteral("C:/upd/staged"),
            QStringLiteral("C:/Apps/Deskmon/Deskmon.exe"), true);
        QVERIFY(winRestart.contains(QStringLiteral("$targetPid = 4242")));
        QVERIFY(winRestart.contains(QStringLiteral("$relaunch = $true")));
        QVERIFY(winRestart.contains(QStringLiteral("robocopy")));

        const QString macQuit = UpdateManager::buildSwapScriptContent(
            QStringLiteral("macos"), 777, QStringLiteral("/tmp/upd/payload"),
            QStringLiteral("/Applications/Deskmon.app/Contents/MacOS"), QStringLiteral("/tmp/upd/staged"),
            QStringLiteral("/Applications/Deskmon.app/Contents/MacOS/Deskmon"), false);
        QVERIFY(macQuit.contains(QStringLiteral("RELAUNCH=0")));
        QVERIFY(macQuit.contains(QStringLiteral("TARGET_BUNDLE='/Applications/Deskmon.app'")));
        QVERIFY(macQuit.contains(QStringLiteral("ditto")));

        const QString linuxRestart = UpdateManager::buildSwapScriptContent(
            QStringLiteral("linux"), 999, QStringLiteral("/tmp/upd/payload"),
            QStringLiteral("/opt/deskmon"), QStringLiteral("/tmp/upd/staged"),
            QStringLiteral("/opt/deskmon/Deskmon"), true);
        QVERIFY(linuxRestart.contains(QStringLiteral("RELAUNCH=1")));
        QVERIFY(linuxRestart.contains(QStringLiteral("pkexec")));
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_updatesDir;
    std::unique_ptr<MockHttpServer> m_server;
};

QTEST_MAIN(TestUpdater)
#include "test_updater.moc"
