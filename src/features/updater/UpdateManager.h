#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QUrl>
#include <memory>

class ApiClient;
class QNetworkReply;
class QFile;
class QProcess;

/**
 * @brief Result metadata describing a staged update ready to apply on restart or reopen.
 */
struct StagedUpdateInfo {
    QString version;
    QString releaseNotes;
    QString payloadDir;
    QString archiveType;
    bool valid = false;
};

/**
 * @brief Cross-platform in-app auto-updater manager.
 *
 * Fetches update manifest JSON, auto-downloads OS-specific update archives
 * in the background (0-100%), validates SHA-256 checksums, extracts to staging,
 * and schedules platform swap scripts applied on restart, quit, or next startup.
 */
class UpdateManager : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Idle,
        Checking,
        Downloading,
        Verifying,
        Extracting,
        ReadyToRestart,
        ManualFallback,
        Error
    };
    Q_ENUM(State)

    explicit UpdateManager(ApiClient *apiClient,
                           const QString &currentVersion,
                           QObject *parent = nullptr);
    ~UpdateManager() override;

    State state() const { return m_state; }
    QString updateState() const;
    int updateProgress() const { return m_progress; }
    QString availableVersion() const { return m_availableVersion; }
    QString releaseNotes() const { return m_releaseNotes; }
    QString releasePageUrl() const { return m_releasePageUrl; }
    QString updateErrorMessage() const { return m_errorMessage; }
    bool isUpdateBusy() const;
    bool isReadyToRestart() const { return m_state == State::ReadyToRestart; }

    void setManifestUrlOverride(const QString &url);
    void setUpdatesRootDir(const QString &dirPath);
    QString updatesRootDir() const;

    /**
     * @brief Checks manifest JSON and automatically begins downloading if an asset exists.
     * @param silent When true, suppresses "up-to-date" / transient offline toasts.
     */
    void checkForUpdates(bool silent = false);

    /**
     * @brief Arms update swap on exit (with relaunch) for Restart to Update action.
     */
    void armRestartToUpdate();

    /**
     * @brief Arms silent update swap on exit if already staged 100%, without relaunching.
     */
    void armApplyOnQuitIfReady();

    /**
     * @brief Spawns the detached swap script if armed (called after lock/log teardown on exit).
     */
    bool launchArmedExitHelper(const QString &appDirPath,
                               const QString &appExecutablePath,
                               qint64 currentPid);

    /**
     * @brief Startup bootstrapper invoked before QML/locks to apply pending staged updates.
     */
    static bool tryApplyStagedUpdateOnStartup(const QString &currentVersion,
                                              const QString &appDirPath,
                                              const QString &appExecutablePath,
                                              qint64 currentPid,
                                              const QString &updatesRootOverride = QString());

    static QString defaultManifestUrl();
    static QString defaultReleasePageUrl();
    static QString defaultUpdatesRootDir();
    static QStringList candidatePlatformKeys();
    static StagedUpdateInfo loadReadyMetadata(const QString &updatesRoot);
    static bool clearStagedUpdate(const QString &updatesRoot);
    static QString computeFileSha256(const QString &filePath);

    /**
     * @brief Builds OS-specific update swap script content (exposed for unit testing).
     */
    static QString buildSwapScriptContent(const QString &platformFamily,
                                          qint64 targetPid,
                                          const QString &payloadDir,
                                          const QString &appDirPath,
                                          const QString &stagedDir,
                                          const QString &executablePath,
                                          bool relaunch);

signals:
    void updateStateChanged();
    void updateProgressChanged();
    void availableVersionChanged();
    void releaseNotesChanged();
    void releasePageUrlChanged();
    void updateErrorMessageChanged();
    void updateAvailable(const QString &newVersion, const QString &releaseNotes);
    void updateReadyToRestart(const QString &newVersion);
    void updateFailed(const QString &toastSummary);
    void showNotification(const QString &type, const QString &message);
    void quitRequestedForUpdate();

private:
    void setState(State newState);
    void setProgress(int progress);
    void setError(const QString &sanitizedDetail, const QString &toastSummary, bool notifyUser);
    void startDownload(const QUrl &assetUrl, const QString &expectedSha256, const QString &archiveType, bool silent);
    void verifyAndExtractDownloadedFile(const QString &archivePath,
                                        const QString &expectedSha256,
                                        const QString &archiveType,
                                        bool silent);
    void extractArchive(const QString &archivePath, const QString &stagedVersionDir, const QString &archiveType, bool silent);
    void startExtractProcess(const QString &archivePath, const QString &stagedVersionDir, const QString &archiveType, bool usePowershellFallback, bool silent);
    void finalizeExtractedArchive(const QString &archivePath, const QString &stagedVersionDir, const QString &archiveType, bool silent);
    QString resolveExtractedPayloadDir(const QString &stagedVersionDir) const;
    bool writeReadyMetadata(const QString &payloadDir, const QString &archiveType);
    void restoreStagedStateIfPending();
    QUrl effectiveManifestUrl() const;
    static QString stateToString(State state);
    static bool launchDetachedSwapHelper(const StagedUpdateInfo &info,
                                         const QString &updatesRoot,
                                         const QString &appDirPath,
                                         const QString &appExecutablePath,
                                         qint64 targetPid,
                                         bool relaunch);

    ApiClient *m_apiClient;
    QString m_currentVersion;
    QString m_manifestUrlOverride;
    QString m_updatesRootOverride;

    State m_state = State::Idle;
    int m_progress = 0;
    QString m_availableVersion;
    QString m_releaseNotes;
    QString m_releasePageUrl;
    QString m_errorMessage;

    QPointer<QNetworkReply> m_activeCheckReply;
    QPointer<QNetworkReply> m_activeDownloadReply;
    std::unique_ptr<QFile> m_downloadPartFile;
    QPointer<QProcess> m_extractProcess;

    bool m_armedOnExit = false;
    bool m_relaunchOnExit = false;
};

#endif // UPDATEMANAGER_H
