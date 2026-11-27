#include "UpdateManager.h"
#include "core/network/ApiClient.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QVersionNumber>

#ifndef DESKMON_UPDATE_MANIFEST_URL
#define DESKMON_UPDATE_MANIFEST_URL "https://raw.githubusercontent.com/NanditoDitama/DeskmonUpdater/main/version.json"
#endif

#ifndef DESKMON_UPDATE_RELEASE_URL
#define DESKMON_UPDATE_RELEASE_URL "https://github.com/NanditoDitama/DeskmonUpdater/releases"
#endif

namespace {

QString stagedRootPath(const QString &updatesRoot)
{
    return QDir(updatesRoot).filePath(QStringLiteral("staged"));
}

QString readyMetadataFilePath(const QString &updatesRoot)
{
    return QDir(stagedRootPath(updatesRoot)).filePath(QStringLiteral("ready.json"));
}

QString downloadsRootPath(const QString &updatesRoot)
{
    return QDir(updatesRoot).filePath(QStringLiteral("downloads"));
}

QString psSingleQuote(QString value)
{
    return QStringLiteral("'") + value.replace(QStringLiteral("'"), QStringLiteral("''")) + QStringLiteral("'");
}

QString shSingleQuote(QString value)
{
    return QStringLiteral("'") + value.replace(QStringLiteral("'"), QStringLiteral("'\"'\"'")) + QStringLiteral("'");
}

QString macAppBundleRootFromDir(const QString &appDirPath)
{
    QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(appDirPath));
    while (normalized.endsWith('/') && normalized.size() > 1) {
        normalized.chop(1);
    }
    const QString suffix = QStringLiteral("/Contents/MacOS");
    if (normalized.endsWith(suffix)) {
        const QString candidate = normalized.left(normalized.size() - suffix.size());
        if (candidate.endsWith(QStringLiteral(".app"), Qt::CaseInsensitive)) {
            return candidate;
        }
    }
    return appDirPath;
}

} // namespace

UpdateManager::UpdateManager(ApiClient *apiClient,
                             const QString &currentVersion,
                             QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_currentVersion(currentVersion)
    , m_releasePageUrl(defaultReleasePageUrl())
{
    restoreStagedStateIfPending();
}

UpdateManager::~UpdateManager()
{
    if (m_activeDownloadReply) {
        m_activeDownloadReply->abort();
    }
    if (m_activeCheckReply) {
        m_activeCheckReply->abort();
    }
    if (m_downloadPartFile && m_downloadPartFile->isOpen()) {
        m_downloadPartFile->close();
    }
}

QString UpdateManager::updateState() const
{
    return stateToString(m_state);
}

bool UpdateManager::isUpdateBusy() const
{
    return m_state == State::Checking
        || m_state == State::Downloading
        || m_state == State::Verifying
        || m_state == State::Extracting;
}

void UpdateManager::setManifestUrlOverride(const QString &url)
{
    m_manifestUrlOverride = url.trimmed();
}

void UpdateManager::setUpdatesRootDir(const QString &dirPath)
{
    m_updatesRootOverride = dirPath.trimmed();
    restoreStagedStateIfPending();
}

QString UpdateManager::updatesRootDir() const
{
    if (!m_updatesRootOverride.isEmpty()) {
        return m_updatesRootOverride;
    }
    return defaultUpdatesRootDir();
}

QString UpdateManager::defaultManifestUrl()
{
    const QString envUrl = qEnvironmentVariable("DESKMON_UPDATE_URL").trimmed();
    if (!envUrl.isEmpty()) {
        return envUrl;
    }
    return QStringLiteral(DESKMON_UPDATE_MANIFEST_URL);
}

QString UpdateManager::defaultReleasePageUrl()
{
    const QString envUrl = qEnvironmentVariable("DESKMON_RELEASE_URL").trimmed();
    if (!envUrl.isEmpty()) {
        return envUrl;
    }
    return QStringLiteral(DESKMON_UPDATE_RELEASE_URL);
}

QString UpdateManager::defaultUpdatesRootDir()
{
    const QString envDir = qEnvironmentVariable("DESKMON_UPDATES_DIR").trimmed();
    if (!envDir.isEmpty()) {
        return envDir;
    }
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        base = QDir::tempPath() + QStringLiteral("/Deskmon");
    }
    return QDir(base).filePath(QStringLiteral("updates"));
}

QStringList UpdateManager::candidatePlatformKeys()
{
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    const bool isArm64 = arch.contains(QStringLiteral("arm64")) || arch.contains(QStringLiteral("aarch64"));

#if defined(Q_OS_WIN)
    return isArm64
        ? QStringList{QStringLiteral("windows-arm64"), QStringLiteral("windows-x64"), QStringLiteral("windows")}
        : QStringList{QStringLiteral("windows-x64"), QStringLiteral("windows")};
#elif defined(Q_OS_MACOS)
    return isArm64
        ? QStringList{QStringLiteral("macos-arm64"), QStringLiteral("macos-universal"), QStringLiteral("macos-x64"), QStringLiteral("macos")}
        : QStringList{QStringLiteral("macos-x64"), QStringLiteral("macos-universal"), QStringLiteral("macos")};
#else
    return isArm64
        ? QStringList{QStringLiteral("linux-arm64"), QStringLiteral("linux")}
        : QStringList{QStringLiteral("linux-x64"), QStringLiteral("linux")};
#endif
}

QUrl UpdateManager::effectiveManifestUrl() const
{
    if (!m_manifestUrlOverride.isEmpty()) {
        return QUrl(m_manifestUrlOverride);
    }
    return QUrl(defaultManifestUrl());
}

void UpdateManager::setState(State newState)
{
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit updateStateChanged();
}

void UpdateManager::setProgress(int progress)
{
    const int bounded = qBound(0, progress, 100);
    if (m_progress == bounded) {
        return;
    }
    m_progress = bounded;
    emit updateProgressChanged();
}

void UpdateManager::setError(const QString &sanitizedDetail, const QString &toastSummary, bool notifyUser)
{
    m_errorMessage = sanitizedDetail;
    emit updateErrorMessageChanged();
    setProgress(0);
    setState(State::Error);
    if (notifyUser) {
        emit updateFailed(toastSummary);
    }
}

void UpdateManager::restoreStagedStateIfPending()
{
    const StagedUpdateInfo info = loadReadyMetadata(updatesRootDir());
    if (!info.valid) {
        if (m_state == State::ReadyToRestart) {
            setProgress(0);
            setState(State::Idle);
        }
        return;
    }

    const QVersionNumber stagedVer = QVersionNumber::fromString(info.version);
    const QVersionNumber currentVer = QVersionNumber::fromString(m_currentVersion);
    if (stagedVer <= currentVer) {
        clearStagedUpdate(updatesRootDir());
        return;
    }

    m_availableVersion = info.version;
    m_releaseNotes = info.releaseNotes;
    emit availableVersionChanged();
    emit releaseNotesChanged();
    setProgress(100);
    setState(State::ReadyToRestart);
}

void UpdateManager::checkForUpdates(bool silent)
{
    if (!m_apiClient || !m_apiClient->networkManager()) {
        return;
    }

    if (isUpdateBusy()) {
        return;
    }

    restoreStagedStateIfPending();
    if (m_state == State::ReadyToRestart) {
        if (!silent) {
            emit updateReadyToRestart(m_availableVersion);
        }
        return;
    }

    m_errorMessage.clear();
    emit updateErrorMessageChanged();
    setProgress(0);
    setState(State::Checking);

    const QUrl url = effectiveManifestUrl();
    qInfo() << "[UpdateManager] Checking updates from:" << url.toString() << "current:" << m_currentVersion;

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Deskmon-Updater/%1").arg(m_currentVersion));

    m_activeCheckReply = m_apiClient->networkManager()->get(request);
    connect(m_activeCheckReply, &QNetworkReply::finished, this, [this, reply = m_activeCheckReply, silent]() {
        if (!reply) {
            return;
        }
        reply->deleteLater();
        m_activeCheckReply.clear();

        if (reply->error() != QNetworkReply::NoError) {
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qWarning() << "[UpdateManager] Check failed. HTTP:" << statusCode
                       << "URL:" << reply->url().toString()
                       << "Detail:" << reply->errorString();
            const QString detail = (statusCode >= 400)
                ? QStringLiteral("Tidak dapat memeriksa pembaruan dari server (Kode HTTP %1). Silakan coba beberapa saat lagi atau hubungi administrator.").arg(statusCode)
                : QStringLiteral("Tidak dapat memeriksa pembaruan (Kode ERR-UPD-NET). Periksa koneksi internet Anda lalu coba lagi.");
            if (silent) {
                setProgress(0);
                setState(State::Idle);
            } else {
                setError(detail, QStringLiteral("Gagal memeriksa pembaruan."), true);
            }
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            qWarning() << "[UpdateManager] Invalid manifest JSON from:" << reply->url().toString();
            const QString detail = QStringLiteral("Data pembaruan tidak dapat dibaca (Kode ERR-UPD-MANIFEST). Silakan coba lagi atau hubungi administrator.");
            if (silent) {
                setProgress(0);
                setState(State::Idle);
            } else {
                setError(detail, QStringLiteral("Gagal memeriksa pembaruan."), true);
            }
            return;
        }

        const QJsonObject obj = doc.object();
        const QString serverVersion = obj.value(QStringLiteral("version")).toString().trimmed();
        const QString notes = obj.value(QStringLiteral("releaseNotes")).toString();
        const QString manifestReleaseUrl = obj.value(QStringLiteral("releasePageUrl")).toString().trimmed();
        if (!manifestReleaseUrl.isEmpty() && manifestReleaseUrl != m_releasePageUrl) {
            m_releasePageUrl = manifestReleaseUrl;
            emit releasePageUrlChanged();
        }

        const QVersionNumber serverVer = QVersionNumber::fromString(serverVersion);
        const QVersionNumber currentVer = QVersionNumber::fromString(m_currentVersion);

        if (serverVersion.isEmpty() || serverVer <= currentVer) {
            setProgress(0);
            setState(State::Idle);
            if (!silent) {
                emit showNotification(QStringLiteral("success"), QStringLiteral("Aplikasi Anda sudah versi terbaru."));
            }
            return;
        }

        if (m_availableVersion != serverVersion) {
            m_availableVersion = serverVersion;
            emit availableVersionChanged();
        }
        if (m_releaseNotes != notes) {
            m_releaseNotes = notes;
            emit releaseNotesChanged();
        }

        // Check if matching version was already staged earlier
        const StagedUpdateInfo existingStaged = loadReadyMetadata(updatesRootDir());
        if (existingStaged.valid && QVersionNumber::fromString(existingStaged.version) == serverVer) {
            setProgress(100);
            setState(State::ReadyToRestart);
            emit updateReadyToRestart(m_availableVersion);
            return;
        }

        // Locate OS package entry inside "platforms"
        const QJsonObject platforms = obj.value(QStringLiteral("platforms")).toObject();
        QJsonObject matchedAsset;
        for (const QString &key : candidatePlatformKeys()) {
            if (platforms.contains(key) && platforms.value(key).isObject()) {
                matchedAsset = platforms.value(key).toObject();
                break;
            }
        }

        const QString assetUrlStr = matchedAsset.value(QStringLiteral("url")).toString().trimmed();
        const QString sha256 = matchedAsset.value(QStringLiteral("sha256")).toString().trimmed().toLower();
        QString archiveType = matchedAsset.value(QStringLiteral("type")).toString().trimmed().toLower();
        if (archiveType.isEmpty()) {
            archiveType = assetUrlStr.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive)
                ? QStringLiteral("tar.gz")
                : QStringLiteral("zip");
        }

        if (assetUrlStr.isEmpty()) {
            setProgress(0);
            setState(State::ManualFallback);
            emit updateAvailable(m_availableVersion, m_releaseNotes);
            return;
        }

        startDownload(QUrl(assetUrlStr), sha256, archiveType, silent);
    });
}

void UpdateManager::startDownload(const QUrl &assetUrl,
                                  const QString &expectedSha256,
                                  const QString &archiveType,
                                  bool silent)
{
    const QString dlDirPath = downloadsRootPath(updatesRootDir());
    QDir dlDir(dlDirPath);
    if (!dlDir.exists() && !dlDir.mkpath(QStringLiteral("."))) {
        qWarning() << "[UpdateManager] Failed creating download dir:" << dlDirPath;
        setError(QStringLiteral("Gagal menyiapkan penyimpanan sementara untuk pembaruan (Kode ERR-UPD-DIR)."),
                 QStringLiteral("Gagal mengunduh pembaruan."), true);
        return;
    }

    const QString ext = (archiveType == QStringLiteral("tar.gz")) ? QStringLiteral("tar.gz") : QStringLiteral("zip");
    const QString finalPath = dlDir.filePath(QStringLiteral("Deskmon-%1.%2").arg(m_availableVersion, ext));
    const QString partPath = finalPath + QStringLiteral(".part");

    QFile::remove(partPath);
    QFile::remove(finalPath);

    m_downloadPartFile = std::make_unique<QFile>(partPath);
    if (!m_downloadPartFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[UpdateManager] Failed opening download file:" << partPath;
        m_downloadPartFile.reset();
        setError(QStringLiteral("Gagal menulis berkas pembaruan di perangkat (Kode ERR-UPD-FILE)."),
                 QStringLiteral("Gagal mengunduh pembaruan."), true);
        return;
    }

    setProgress(1);
    setState(State::Downloading);
    qInfo() << "[UpdateManager] Downloading update" << m_availableVersion << "from" << assetUrl.toString();

    QNetworkRequest request(assetUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Deskmon-Updater/%1").arg(m_currentVersion));

    m_activeDownloadReply = m_apiClient->networkManager()->get(request);

    connect(m_activeDownloadReply, &QNetworkReply::readyRead, this, [this, reply = m_activeDownloadReply]() {
        if (!reply || !m_downloadPartFile || !m_downloadPartFile->isOpen()) {
            return;
        }
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty()) {
            m_downloadPartFile->write(chunk);
        }
    });

    connect(m_activeDownloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            const int pct = static_cast<int>((received * 95) / total);
            setProgress(qBound(1, pct, 95));
        } else if (received > 0 && m_progress < 90) {
            setProgress(m_progress + 1);
        }
    });

    connect(m_activeDownloadReply, &QNetworkReply::finished, this,
            [this, reply = m_activeDownloadReply, partPath, finalPath, expectedSha256, archiveType, silent]() {
        if (!reply) {
            return;
        }
        reply->deleteLater();
        m_activeDownloadReply.clear();

        if (m_downloadPartFile) {
            if (m_downloadPartFile->isOpen()) {
                const QByteArray trailing = reply->readAll();
                if (!trailing.isEmpty()) {
                    m_downloadPartFile->write(trailing);
                }
                m_downloadPartFile->flush();
                m_downloadPartFile->close();
            }
            m_downloadPartFile.reset();
        }

        if (reply->error() != QNetworkReply::NoError) {
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QFile::remove(partPath);
            qWarning() << "[UpdateManager] Download failed. HTTP:" << statusCode
                       << "URL:" << reply->url().toString()
                       << "Detail:" << reply->errorString();
            const QString detail = (statusCode >= 400)
                ? QStringLiteral("Gagal mengunduh paket pembaruan dari server (Kode HTTP %1). Silakan coba lagi nanti atau hubungi administrator.").arg(statusCode)
                : QStringLiteral("Gagal mengunduh paket pembaruan (Kode ERR-UPD-DL). Periksa koneksi internet Anda lalu coba lagi.");
            setError(detail, QStringLiteral("Gagal mengunduh pembaruan."), true);
            return;
        }

        QFile::remove(finalPath);
        if (!QFile::rename(partPath, finalPath)) {
            qWarning() << "[UpdateManager] Failed moving download file to:" << finalPath;
            QFile::remove(partPath);
            setError(QStringLiteral("Gagal menyimpan berkas paket pembaruan (Kode ERR-UPD-SAVE)."),
                     QStringLiteral("Gagal mengunduh pembaruan."), true);
            return;
        }

        verifyAndExtractDownloadedFile(finalPath, expectedSha256, archiveType, silent);
    });
}

void UpdateManager::verifyAndExtractDownloadedFile(const QString &archivePath,
                                                   const QString &expectedSha256,
                                                   const QString &archiveType,
                                                   bool silent)
{
    setState(State::Verifying);
    setProgress(96);

    if (!expectedSha256.isEmpty()) {
        const QString actualSha256 = computeFileSha256(archivePath);
        if (actualSha256.isEmpty() || actualSha256.compare(expectedSha256, Qt::CaseInsensitive) != 0) {
            qWarning() << "[UpdateManager] SHA-256 mismatch. Expected:" << expectedSha256 << "Actual:" << actualSha256;
            QFile::remove(archivePath);
            setError(QStringLiteral("Verifikasi keamanan paket pembaruan gagal (Kode ERR-UPD-CHECKSUM). Unduhan dibatalkan demi keamanan."),
                     QStringLiteral("Verifikasi paket pembaruan gagal."), true);
            return;
        }
    }

    const QString stagedBase = stagedRootPath(updatesRootDir());
    QDir(stagedBase).removeRecursively();

    const QString stagedVersionDir = QDir(stagedBase).filePath(m_availableVersion);
    if (!QDir().mkpath(stagedVersionDir)) {
        qWarning() << "[UpdateManager] Failed creating staging dir:" << stagedVersionDir;
        QFile::remove(archivePath);
        setError(QStringLiteral("Gagal menyiapkan direktori pembaruan di perangkat (Kode ERR-UPD-STAGE)."),
                 QStringLiteral("Gagal menyiapkan pembaruan."), true);
        return;
    }

    extractArchive(archivePath, stagedVersionDir, archiveType, silent);
}

void UpdateManager::extractArchive(const QString &archivePath,
                                   const QString &stagedVersionDir,
                                   const QString &archiveType,
                                   bool silent)
{
    setState(State::Extracting);
    setProgress(98);
    startExtractProcess(archivePath, stagedVersionDir, archiveType, false, silent);
}

void UpdateManager::startExtractProcess(const QString &archivePath,
                                        const QString &stagedVersionDir,
                                        const QString &archiveType,
                                        bool usePowershellFallback,
                                        bool silent)
{
    if (m_extractProcess) {
        m_extractProcess->deleteLater();
    }
    m_extractProcess = new QProcess(this);

    QString program;
    QStringList args;
#ifdef Q_OS_WIN
    if (usePowershellFallback) {
        program = QStringLiteral("powershell.exe");
        args << QStringLiteral("-NoProfile")
             << QStringLiteral("-NonInteractive")
             << QStringLiteral("-WindowStyle") << QStringLiteral("Hidden")
             << QStringLiteral("-Command")
             << QStringLiteral("Expand-Archive -Path %1 -DestinationPath %2 -Force")
                    .arg(psSingleQuote(QDir::toNativeSeparators(archivePath)),
                         psSingleQuote(QDir::toNativeSeparators(stagedVersionDir)));
    } else {
        program = QStringLiteral("tar");
        args << (archiveType == QStringLiteral("tar.gz") ? QStringLiteral("-xzf") : QStringLiteral("-xf"))
             << QDir::toNativeSeparators(archivePath)
             << QStringLiteral("-C")
             << QDir::toNativeSeparators(stagedVersionDir);
    }
#else
    Q_UNUSED(usePowershellFallback);
    if (archiveType == QStringLiteral("zip")) {
#ifdef Q_OS_MACOS
        program = QStringLiteral("ditto");
        args << QStringLiteral("-xk") << archivePath << stagedVersionDir;
#else
        program = QStringLiteral("tar");
        args << QStringLiteral("-xf") << archivePath << QStringLiteral("-C") << stagedVersionDir;
#endif
    } else {
        program = QStringLiteral("tar");
        args << QStringLiteral("-xzf") << archivePath << QStringLiteral("-C") << stagedVersionDir;
    }
#endif

    connect(m_extractProcess, &QProcess::finished, this,
            [this, proc = m_extractProcess, archivePath, stagedVersionDir, archiveType, usePowershellFallback, silent](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString errOutput = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        proc->deleteLater();
        m_extractProcess.clear();

#ifdef Q_OS_WIN
        if ((exitStatus != QProcess::NormalExit || exitCode != 0) && !usePowershellFallback && archiveType == QStringLiteral("zip")) {
            qWarning() << "[UpdateManager] tar extraction failed, retrying via Expand-Archive:" << errOutput;
            QDir(stagedVersionDir).removeRecursively();
            QDir().mkpath(stagedVersionDir);
            startExtractProcess(archivePath, stagedVersionDir, archiveType, true, silent);
            return;
        }
#endif

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qWarning() << "[UpdateManager] Extraction failed:" << errOutput;
            QFile::remove(archivePath);
            clearStagedUpdate(updatesRootDir());
            setError(QStringLiteral("Gagal mengekstrak berkas pembaruan pada perangkat (Kode ERR-UPD-EXTRACT)."),
                     QStringLiteral("Gagal menyiapkan pembaruan."), true);
            return;
        }

        finalizeExtractedArchive(archivePath, stagedVersionDir, archiveType, silent);
    });

    m_extractProcess->start(program, args);
}

void UpdateManager::finalizeExtractedArchive(const QString &archivePath,
                                             const QString &stagedVersionDir,
                                             const QString &archiveType,
                                             bool silent)
{
    Q_UNUSED(silent);
    const QString payloadDir = resolveExtractedPayloadDir(stagedVersionDir);
    QFile::remove(archivePath);
    if (!writeReadyMetadata(payloadDir, archiveType)) {
        qWarning() << "[UpdateManager] Failed writing ready.json in:" << stagedVersionDir;
        clearStagedUpdate(updatesRootDir());
        setError(QStringLiteral("Gagal menyimpan status kesiapan pembaruan (Kode ERR-UPD-META)."),
                 QStringLiteral("Gagal menyiapkan pembaruan."), true);
        return;
    }

    setProgress(100);
    setState(State::ReadyToRestart);
    emit updateReadyToRestart(m_availableVersion);
    emit showNotification(QStringLiteral("info"),
                          QStringLiteral("Pembaruan v%1 siap. Klik Restart Application untuk menerapkan.").arg(m_availableVersion));
}

QString UpdateManager::resolveExtractedPayloadDir(const QString &stagedVersionDir) const
{
    QDir dir(stagedVersionDir);
    const QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    if (entries.size() == 1 && entries.first().isDir()) {
        const QString singleDirName = entries.first().fileName();
#ifdef Q_OS_MACOS
        if (singleDirName.endsWith(QStringLiteral(".app"), Qt::CaseInsensitive)) {
            return stagedVersionDir;
        }
#endif
        return entries.first().absoluteFilePath();
    }
    return stagedVersionDir;
}

bool UpdateManager::writeReadyMetadata(const QString &payloadDir, const QString &archiveType)
{
    const QString readyPath = readyMetadataFilePath(updatesRootDir());
    QSaveFile file(readyPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("version"), m_availableVersion);
    obj.insert(QStringLiteral("releaseNotes"), m_releaseNotes);
    obj.insert(QStringLiteral("payloadDir"), payloadDir);
    obj.insert(QStringLiteral("archiveType"), archiveType);
    obj.insert(QStringLiteral("createdAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return file.commit();
}

void UpdateManager::armRestartToUpdate()
{
    if (m_state != State::ReadyToRestart) {
        return;
    }
    m_armedOnExit = true;
    m_relaunchOnExit = true;
    emit quitRequestedForUpdate();
}

void UpdateManager::armApplyOnQuitIfReady()
{
    if (m_state == State::ReadyToRestart && !m_armedOnExit) {
        m_armedOnExit = true;
        m_relaunchOnExit = false;
    }
}

bool UpdateManager::launchArmedExitHelper(const QString &appDirPath,
                                          const QString &appExecutablePath,
                                          qint64 currentPid)
{
    if (!m_armedOnExit) {
        return false;
    }
    const QString root = updatesRootDir();
    const StagedUpdateInfo info = loadReadyMetadata(root);
    if (!info.valid) {
        return false;
    }
    return launchDetachedSwapHelper(info, root, appDirPath, appExecutablePath, currentPid, m_relaunchOnExit);
}

bool UpdateManager::tryApplyStagedUpdateOnStartup(const QString &currentVersion,
                                                  const QString &appDirPath,
                                                  const QString &appExecutablePath,
                                                  qint64 currentPid,
                                                  const QString &updatesRootOverride)
{
    const QString updatesRoot = updatesRootOverride.isEmpty() ? defaultUpdatesRootDir() : updatesRootOverride;
    const StagedUpdateInfo info = loadReadyMetadata(updatesRoot);
    if (!info.valid) {
        return false;
    }

    const QVersionNumber stagedVer = QVersionNumber::fromString(info.version);
    const QVersionNumber currentVer = QVersionNumber::fromString(currentVersion);
    if (stagedVer <= currentVer) {
        clearStagedUpdate(updatesRoot);
        return false;
    }

    qInfo() << "[UpdateManager] Pending staged update found on startup:" << info.version << "-> applying...";
    return launchDetachedSwapHelper(info, updatesRoot, appDirPath, appExecutablePath, currentPid, true);
}

StagedUpdateInfo UpdateManager::loadReadyMetadata(const QString &updatesRoot)
{
    StagedUpdateInfo info;
    const QString readyPath = readyMetadataFilePath(updatesRoot);
    QFile file(readyPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return info;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return info;
    }

    const QJsonObject obj = doc.object();
    info.version = obj.value(QStringLiteral("version")).toString().trimmed();
    info.releaseNotes = obj.value(QStringLiteral("releaseNotes")).toString();
    info.payloadDir = obj.value(QStringLiteral("payloadDir")).toString().trimmed();
    info.archiveType = obj.value(QStringLiteral("archiveType")).toString().trimmed();

    QDir payload(info.payloadDir);
    if (!info.version.isEmpty() && !info.payloadDir.isEmpty() && payload.exists() && !payload.isEmpty()) {
        info.valid = true;
    }
    return info;
}

bool UpdateManager::clearStagedUpdate(const QString &updatesRoot)
{
    QDir stagedDir(stagedRootPath(updatesRoot));
    if (!stagedDir.exists()) {
        return true;
    }
    return stagedDir.removeRecursively();
}

QString UpdateManager::computeFileSha256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(65536);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            return {};
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString UpdateManager::buildSwapScriptContent(const QString &platformFamily,
                                              qint64 targetPid,
                                              const QString &payloadDir,
                                              const QString &appDirPath,
                                              const QString &stagedDir,
                                              const QString &executablePath,
                                              bool relaunch)
{
    const QString family = platformFamily.trimmed().toLower();

    if (family == QStringLiteral("windows")) {
        const QString winPayload = QDir::toNativeSeparators(payloadDir);
        const QString winAppDir = QDir::toNativeSeparators(appDirPath);
        const QString winStaged = QDir::toNativeSeparators(stagedDir);
        const QString winExe = QDir::toNativeSeparators(executablePath);

        return QStringLiteral(
            "param([switch]$Elevated)\n"
            "$ErrorActionPreference = 'Stop'\n"
            "$targetPid = %1\n"
            "$payloadDir = %2\n"
            "$appDir = %3\n"
            "$stagedDir = %4\n"
            "$exePath = %5\n"
            "$relaunch = %6\n"
            "$logFile = Join-Path (Split-Path $PSCommandPath -Parent) 'apply_update.log'\n"
            "function Write-UpdateLog([string]$msg) {\n"
            "    try { Add-Content -Path $logFile -Value (\"[{0}] {1}\" -f (Get-Date -Format 's'), $msg) -ErrorAction SilentlyContinue } catch {}\n"
            "}\n"
            "try {\n"
            "    for ($i = 0; $i -lt 60; $i++) {\n"
            "        if (-not (Get-Process -Id $targetPid -ErrorAction SilentlyContinue)) { break }\n"
            "        Start-Sleep -Milliseconds 250\n"
            "    }\n"
            "    $writable = $false\n"
            "    try {\n"
            "        $probe = Join-Path $appDir '.deskmon_write_probe'\n"
            "        [IO.File]::WriteAllText($probe, 'ok')\n"
            "        Remove-Item $probe -Force -ErrorAction SilentlyContinue\n"
            "        $writable = $true\n"
            "    } catch { $writable = $false }\n"
            "    if (-not $writable -and -not $Elevated) {\n"
            "        $quotedSelf = '\"' + $PSCommandPath + '\"'\n"
            "        $proc = Start-Process -FilePath 'powershell.exe' -Verb RunAs -PassThru -Wait "
            "-ArgumentList @('-NoProfile','-NonInteractive','-WindowStyle','Hidden','-ExecutionPolicy','Bypass','-File',$quotedSelf,'-Elevated')\n"
            "        if ($proc.ExitCode -ne 0) {\n"
            "            Write-UpdateLog \"Elevated swap failed with exit code $($proc.ExitCode)\"\n"
            "            exit $proc.ExitCode\n"
            "        }\n"
            "        if ($relaunch -and (Test-Path $exePath)) { Start-Process -FilePath $exePath -WorkingDirectory $appDir }\n"
            "        Remove-Item $PSCommandPath -Force -ErrorAction SilentlyContinue\n"
            "        exit 0\n"
            "    }\n"
            "    $null = & robocopy $payloadDir $appDir /E /IS /IT /R:5 /W:1 /NFL /NDL /NJH /NJS /NP\n"
            "    if ($LASTEXITCODE -ge 8) {\n"
            "        Write-UpdateLog \"robocopy failed with exit code $LASTEXITCODE\"\n"
            "        exit $LASTEXITCODE\n"
            "    }\n"
            "    if (Test-Path $stagedDir) { Remove-Item $stagedDir -Recurse -Force -ErrorAction SilentlyContinue }\n"
            "    if ($relaunch -and -not $Elevated -and (Test-Path $exePath)) {\n"
            "        Start-Process -FilePath $exePath -WorkingDirectory $appDir\n"
            "    }\n"
            "    if (-not $Elevated) { Remove-Item $PSCommandPath -Force -ErrorAction SilentlyContinue }\n"
            "    exit 0\n"
            "} catch {\n"
            "    Write-UpdateLog \"Swap error (Elevated=$Elevated): $($_.Exception.Message)\"\n"
            "    exit 1\n"
            "}\n")
            .arg(targetPid)
            .arg(psSingleQuote(winPayload),
                 psSingleQuote(winAppDir),
                 psSingleQuote(winStaged),
                 psSingleQuote(winExe),
                 relaunch ? QStringLiteral("$true") : QStringLiteral("$false"));
    }

    const QString unixPreamble = QStringLiteral(
        "#!/bin/bash\n"
        "set -e\n"
        "TARGET_PID=%1\n"
        "PAYLOAD_DIR=%2\n"
        "APP_DIR=%3\n"
        "STAGED_DIR=%4\n"
        "EXE_PATH=%5\n"
        "RELAUNCH=%6\n"
        "LOG_FILE=\"$(dirname \"$0\")/apply_update.log\"\n"
        "cleanup_on_exit() {\n"
        "  rc=$?\n"
        "  if [ \"$rc\" -ne 0 ]; then\n"
        "    printf '[%s] Update swap failed (exit %d)\\n' \"$(date '+%Y-%m-%dT%H:%M:%S')\" \"$rc\" >> \"$LOG_FILE\" 2>/dev/null || true\n"
        "  fi\n"
        "}\n"
        "trap cleanup_on_exit EXIT\n"
        "for i in $(seq 1 60); do\n"
        "  kill -0 \"$TARGET_PID\" 2>/dev/null || break\n"
        "  sleep 0.25\n"
        "done\n")
        .arg(targetPid)
        .arg(shSingleQuote(payloadDir),
             shSingleQuote(appDirPath),
             shSingleQuote(stagedDir),
             shSingleQuote(executablePath),
             relaunch ? QStringLiteral("1") : QStringLiteral("0"));

    if (family == QStringLiteral("macos")) {
        const QString bundleRoot = macAppBundleRootFromDir(appDirPath);
        return unixPreamble + QStringLiteral(
            "TARGET_BUNDLE=%1\n"
            "SRC_APP=$(find \"$PAYLOAD_DIR\" -maxdepth 2 -name '*.app' -type d | head -n 1)\n"
            "if [ -n \"$SRC_APP\" ] && [ -d \"$TARGET_BUNDLE\" ]; then\n"
            "  if [ -w \"$(dirname \"$TARGET_BUNDLE\")\" ]; then\n"
            "    ditto \"$SRC_APP\" \"$TARGET_BUNDLE\"\n"
            "  else\n"
            "    osascript -e \"do shell script \\\"ditto '$SRC_APP' '$TARGET_BUNDLE'\\\" with administrator privileges\"\n"
            "  fi\n"
            "else\n"
            "  if [ -w \"$APP_DIR\" ]; then\n"
            "    cp -af \"$PAYLOAD_DIR\"/. \"$APP_DIR\"/\n"
            "    chmod +x \"$EXE_PATH\" 2>/dev/null || true\n"
            "  else\n"
            "    osascript -e \"do shell script \\\"cp -af '$PAYLOAD_DIR'/. '$APP_DIR'/ && chmod +x '$EXE_PATH'\\\" with administrator privileges\"\n"
            "  fi\n"
            "fi\n"
            "rm -rf \"$STAGED_DIR\"\n"
            "if [ \"$RELAUNCH\" = \"1\" ]; then\n"
            "  if [ -d \"$TARGET_BUNDLE\" ] && [[ \"$TARGET_BUNDLE\" == *.app ]]; then\n"
            "    open -n \"$TARGET_BUNDLE\"\n"
            "  else\n"
            "    nohup \"$EXE_PATH\" >/dev/null 2>&1 &\n"
            "  fi\n"
            "fi\n"
            "rm -f \"$0\"\n")
            .arg(shSingleQuote(bundleRoot));
    }

    // Linux swap script
    return unixPreamble + QStringLiteral(
        "if [ -w \"$APP_DIR\" ]; then\n"
        "  cp -af \"$PAYLOAD_DIR\"/. \"$APP_DIR\"/\n"
        "  chmod +x \"$EXE_PATH\" 2>/dev/null || true\n"
        "  [ ! -f \"$APP_DIR/deskmon.sh\" ] || chmod +x \"$APP_DIR/deskmon.sh\" 2>/dev/null || true\n"
        "else\n"
        "  pkexec sh -c \"cp -af '$PAYLOAD_DIR'/. '$APP_DIR'/ && chmod +x '$EXE_PATH' && ([ ! -f '$APP_DIR/deskmon.sh' ] || chmod +x '$APP_DIR/deskmon.sh')\"\n"
        "fi\n"
        "rm -rf \"$STAGED_DIR\"\n"
        "if [ \"$RELAUNCH\" = \"1\" ]; then\n"
        "  if [ -x \"$APP_DIR/deskmon.sh\" ]; then\n"
        "    nohup \"$APP_DIR/deskmon.sh\" >/dev/null 2>&1 &\n"
        "  else\n"
        "    nohup \"$EXE_PATH\" >/dev/null 2>&1 &\n"
        "  fi\n"
        "fi\n"
        "rm -f \"$0\"\n");
}

bool UpdateManager::launchDetachedSwapHelper(const StagedUpdateInfo &info,
                                             const QString &updatesRoot,
                                             const QString &appDirPath,
                                             const QString &appExecutablePath,
                                             qint64 targetPid,
                                             bool relaunch)
{
    if (!info.valid) {
        return false;
    }

    QDir rootDir(updatesRoot);
    if (!rootDir.exists() && !rootDir.mkpath(QStringLiteral("."))) {
        return false;
    }

    const QString stagedDir = stagedRootPath(updatesRoot);

#if defined(Q_OS_WIN)
    const QString family = QStringLiteral("windows");
    const QString scriptPath = rootDir.filePath(QStringLiteral("apply_update.ps1"));
#elif defined(Q_OS_MACOS)
    const QString family = QStringLiteral("macos");
    const QString scriptPath = rootDir.filePath(QStringLiteral("apply_update.sh"));
#else
    const QString family = QStringLiteral("linux");
    const QString scriptPath = rootDir.filePath(QStringLiteral("apply_update.sh"));
#endif

    const QString scriptContent = buildSwapScriptContent(
        family, targetPid, info.payloadDir, appDirPath, stagedDir, appExecutablePath, relaunch);

    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();

#ifdef Q_OS_WIN
    return QProcess::startDetached(QStringLiteral("powershell.exe"), {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-NonInteractive"),
        QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
        QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
        QStringLiteral("-File"), QDir::toNativeSeparators(scriptPath)
    });
#else
    scriptFile.setPermissions(scriptFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser);
    return QProcess::startDetached(QStringLiteral("/bin/bash"), {scriptPath});
#endif
}

QString UpdateManager::stateToString(State state)
{
    switch (state) {
    case State::Idle:           return QStringLiteral("idle");
    case State::Checking:       return QStringLiteral("checking");
    case State::Downloading:    return QStringLiteral("downloading");
    case State::Verifying:      return QStringLiteral("verifying");
    case State::Extracting:     return QStringLiteral("extracting");
    case State::ReadyToRestart: return QStringLiteral("ready");
    case State::ManualFallback: return QStringLiteral("manual");
    case State::Error:          return QStringLiteral("error");
    }
    return QStringLiteral("idle");
}
