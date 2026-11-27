#include "AppIconProvider.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#endif

namespace {

constexpr qint64 kWebCacheTtlSecs = 7 * 24 * 3600;
constexpr qint64 kNegativeCacheTtlMs = 3600 * 1000;
constexpr int kHttpTimeoutMs = 3500;

QMutex &exeRegistryMutex()
{
    static QMutex m;
    return m;
}

QHash<QString, QString> &exeRegistry()
{
    static QHash<QString, QString> map;
    return map;
}

QString iconCacheRoot()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        base = QDir::tempPath() + QStringLiteral("/Deskmon");
    }
    return base + QStringLiteral("/icon_cache");
}

QString sanitizeCacheFilename(const QString &key)
{
    QString out = key.toLower();
    out.replace(QRegularExpression(QStringLiteral("[^a-z0-9._-]")), QStringLiteral("_"));
    return out;
}

QString lookupCuratedAppDomain(const QString &appName)
{
    const QString key = appName.toLower().trimmed();
    if (key.isEmpty()) return QString();

    static const QList<QPair<QString, QString>> map = {
        { QStringLiteral("chrome"), QStringLiteral("google.com/chrome") },
        { QStringLiteral("firefox"), QStringLiteral("mozilla.org") },
        { QStringLiteral("msedge"), QStringLiteral("microsoft.com") },
        { QStringLiteral("edge"), QStringLiteral("microsoft.com") },
        { QStringLiteral("safari"), QStringLiteral("apple.com") },
        { QStringLiteral("opera"), QStringLiteral("opera.com") },
        { QStringLiteral("brave"), QStringLiteral("brave.com") },
        { QStringLiteral("vivaldi"), QStringLiteral("vivaldi.com") },
        { QStringLiteral("arc"), QStringLiteral("arc.net") },
        { QStringLiteral("visual studio code"), QStringLiteral("code.visualstudio.com") },
        { QStringLiteral("vscode"), QStringLiteral("code.visualstudio.com") },
        { QStringLiteral("code"), QStringLiteral("code.visualstudio.com") },
        { QStringLiteral("qtcreator"), QStringLiteral("qt.io") },
        { QStringLiteral("qt creator"), QStringLiteral("qt.io") },
        { QStringLiteral("android studio"), QStringLiteral("developer.android.com") },
        { QStringLiteral("intellij"), QStringLiteral("jetbrains.com") },
        { QStringLiteral("pycharm"), QStringLiteral("jetbrains.com") },
        { QStringLiteral("xcode"), QStringLiteral("developer.apple.com") },
        { QStringLiteral("figma"), QStringLiteral("figma.com") },
        { QStringLiteral("canva"), QStringLiteral("canva.com") },
        { QStringLiteral("blender"), QStringLiteral("blender.org") },
        { QStringLiteral("capcut"), QStringLiteral("capcut.com") },
        { QStringLiteral("discord"), QStringLiteral("discord.com") },
        { QStringLiteral("slack"), QStringLiteral("slack.com") },
        { QStringLiteral("zoom"), QStringLiteral("zoom.us") },
        { QStringLiteral("notion"), QStringLiteral("notion.so") },
        { QStringLiteral("obsidian"), QStringLiteral("obsidian.md") },
        { QStringLiteral("spotify"), QStringLiteral("spotify.com") },
        { QStringLiteral("whatsapp"), QStringLiteral("whatsapp.com") },
        { QStringLiteral("telegram"), QStringLiteral("telegram.org") },
        { QStringLiteral("postman"), QStringLiteral("postman.com") },
        { QStringLiteral("github"), QStringLiteral("github.com") },
        { QStringLiteral("docker"), QStringLiteral("docker.com") },
        { QStringLiteral("acrord32"), QStringLiteral("adobe.com") },
        { QStringLiteral("adobe"), QStringLiteral("adobe.com") },
        { QStringLiteral("photoshop"), QStringLiteral("adobe.com") },
        { QStringLiteral("antigravity"), QStringLiteral("antigravity.google") },
        { QStringLiteral("anti gravity"), QStringLiteral("antigravity.google") },
        { QStringLiteral("teams"), QStringLiteral("teams.microsoft.com") },
        { QStringLiteral("outlook"), QStringLiteral("outlook.live.com") },
        { QStringLiteral("excel"), QStringLiteral("office.com") },
        { QStringLiteral("winword"), QStringLiteral("office.com") },
        { QStringLiteral("microsoft word"), QStringLiteral("office.com") },
        { QStringLiteral("powerpnt"), QStringLiteral("office.com") },
        { QStringLiteral("powerpoint"), QStringLiteral("office.com") },
        { QStringLiteral("explorer"), QStringLiteral("microsoft.com") }
    };

    for (const auto &entry : map) {
        if (key.contains(entry.first)) {
            return entry.second;
        }
    }
    return QString();
}

#ifdef Q_OS_WIN
QString queryProcessPathByPid(DWORD pid)
{
    if (pid == 0) return QString();
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return QString();

    wchar_t buf[MAX_PATH * 2];
    DWORD len = sizeof(buf) / sizeof(buf[0]);
    QString result;
    if (QueryFullProcessImageNameW(hProc, 0, buf, &len) && len > 0) {
        result = QString::fromWCharArray(buf, static_cast<int>(len));
    }
    CloseHandle(hProc);
    return result;
}

QString findExeFromRunningProcesses(const QString &normalizedKey)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return QString();

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    QString foundPath;

    if (Process32FirstW(snap, &pe)) {
        do {
            QString procBase = QFileInfo(QString::fromWCharArray(pe.szExeFile)).baseName();
            if (AppIconProvider::normalizeAppKey(procBase) == normalizedKey) {
                QString p = queryProcessPathByPid(pe.th32ProcessID);
                if (!p.isEmpty() && QFileInfo::exists(p)) {
                    foundPath = p;
                    break;
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return foundPath;
}

QString findExeFromRegistryAppPaths(const QString &appName)
{
    const QString exeName = appName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
                                ? appName
                                : (appName + QStringLiteral(".exe"));
    const QString subKey = QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\") + exeName;

    const HKEY roots[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    for (HKEY root : roots) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(root, reinterpret_cast<const wchar_t*>(subKey.utf16()), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t val[MAX_PATH * 2];
            DWORD valSize = sizeof(val);
            DWORD type = 0;
            if (RegQueryValueExW(hKey, nullptr, nullptr, &type, reinterpret_cast<LPBYTE>(val), &valSize) == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ)) {
                QString raw = QString::fromWCharArray(val).trimmed();
                if (raw.startsWith('"') && raw.endsWith('"') && raw.size() >= 2) {
                    raw = raw.mid(1, raw.size() - 2);
                }
                if (QFileInfo::exists(raw)) {
                    RegCloseKey(hKey);
                    return raw;
                }
            }
            RegCloseKey(hKey);
        }
    }
    return QString();
}

QImage extractWindowsExeIcon(const QString &exePath)
{
    if (exePath.isEmpty() || !QFileInfo::exists(exePath)) {
        return QImage();
    }

    const std::wstring wPath = exePath.toStdWString();
    HICON hIcon = nullptr;
    UINT iconId = 0;

    // Try high-res 64x64 extraction first
    UINT extracted = PrivateExtractIconsW(wPath.c_str(), 0, 64, 64, &hIcon, &iconId, 1, 0);
    if (extracted == 0 || !hIcon) {
        SHFILEINFOW sfi{};
        if (SHGetFileInfoW(wPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
            hIcon = sfi.hIcon;
        }
    }

    if (!hIcon) {
        return QImage();
    }

    QImage img = QImage::fromHICON(hIcon);
    DestroyIcon(hIcon);
    return img;
}
#elif defined(Q_OS_MACOS)
QImage extractMacAppIcon(const QString &appName, const QString &normKey)
{
    const QStringList searchDirs = {
        QStringLiteral("/Applications"),
        QDir::homePath() + QStringLiteral("/Applications"),
        QStringLiteral("/System/Applications")
    };

    QString matchedBundle;
    for (const QString &dirPath : searchDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.app")}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString base = entry.completeBaseName();
            if (base.compare(appName, Qt::CaseInsensitive) == 0 ||
                AppIconProvider::normalizeAppKey(base) == normKey) {
                matchedBundle = entry.absoluteFilePath();
                break;
            }
        }
        if (!matchedBundle.isEmpty()) break;
    }

    if (matchedBundle.isEmpty()) return QImage();

    const QString resourcesDir = matchedBundle + QStringLiteral("/Contents/Resources");
    QString iconFile;

    QSettings plist(matchedBundle + QStringLiteral("/Contents/Info.plist"), QSettings::NativeFormat);
    iconFile = plist.value(QStringLiteral("CFBundleIconFile")).toString().trimmed();
    if (iconFile.isEmpty()) {
        iconFile = plist.value(QStringLiteral("CFBundleIconName")).toString().trimmed();
    }

    if (!iconFile.isEmpty()) {
        QString candidate = QDir(resourcesDir).filePath(iconFile);
        if (!QFileInfo::exists(candidate) && !iconFile.contains('.')) {
            if (QFileInfo::exists(candidate + QStringLiteral(".icns"))) candidate += QStringLiteral(".icns");
            else if (QFileInfo::exists(candidate + QStringLiteral(".png"))) candidate += QStringLiteral(".png");
        }
        if (QFileInfo::exists(candidate)) {
            QImage img(candidate);
            if (!img.isNull()) return img;
        }
    }

    const QFileInfoList icnsList = QDir(resourcesDir).entryInfoList({QStringLiteral("*.icns"), QStringLiteral("*.png")}, QDir::Files);
    for (const QFileInfo &fi : icnsList) {
        QImage img(fi.absoluteFilePath());
        if (!img.isNull()) return img;
    }
    return QImage();
}
#else
QImage loadIconFileToImage(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) return QImage();
    if (path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
        QSvgRenderer renderer(path);
        if (!renderer.isValid()) return QImage();
        QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter painter(&img);
        renderer.render(&painter);
        return img;
    }
    return QImage(path);
}

QImage extractLinuxAppIcon(const QString &appName, const QString &normKey)
{
    const QStringList appDirs = {
        QDir::homePath() + QStringLiteral("/.local/share/applications"),
        QStringLiteral("/usr/local/share/applications"),
        QStringLiteral("/usr/share/applications"),
        QStringLiteral("/var/lib/flatpak/exports/share/applications")
    };

    QString iconKey;
    for (const QString &dirPath : appDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        const QFileInfoList desktopFiles = dir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QFileInfo &fi : desktopFiles) {
            const QString fileBase = fi.completeBaseName();
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QString desktopName, execName, desktopIcon;
            while (!f.atEnd()) {
                const QString line = QString::fromUtf8(f.readLine()).trimmed();
                if (line.startsWith(QStringLiteral("Name=")) && desktopName.isEmpty()) {
                    desktopName = line.mid(5).trimmed();
                } else if (line.startsWith(QStringLiteral("Exec=")) && execName.isEmpty()) {
                    execName = QFileInfo(line.mid(5).trimmed().section(' ', 0, 0)).baseName();
                } else if (line.startsWith(QStringLiteral("Icon=")) && desktopIcon.isEmpty()) {
                    desktopIcon = line.mid(5).trimmed();
                }
            }

            const bool matches = fileBase.compare(appName, Qt::CaseInsensitive) == 0
                || AppIconProvider::normalizeAppKey(fileBase) == normKey
                || (!desktopName.isEmpty() && AppIconProvider::normalizeAppKey(desktopName) == normKey)
                || (!execName.isEmpty() && AppIconProvider::normalizeAppKey(execName) == normKey);
            if (matches && !desktopIcon.isEmpty()) {
                iconKey = desktopIcon;
                break;
            }
        }
        if (!iconKey.isEmpty()) break;
    }

    if (iconKey.isEmpty()) iconKey = appName.toLower().trimmed();

    if (QFileInfo::exists(iconKey)) {
        return loadIconFileToImage(iconKey);
    }

    const QStringList iconRoots = {
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor"),
        QStringLiteral("/usr/share/icons/hicolor")
    };
    const QStringList sizeDirs = {
        QStringLiteral("128x128/apps"),
        QStringLiteral("64x64/apps"),
        QStringLiteral("48x48/apps"),
        QStringLiteral("scalable/apps")
    };
    const QStringList exts = {QStringLiteral(".png"), QStringLiteral(".svg")};

    for (const QString &root : iconRoots) {
        for (const QString &sub : sizeDirs) {
            for (const QString &ext : exts) {
                const QString candidate = root + '/' + sub + '/' + iconKey + ext;
                if (QFileInfo::exists(candidate)) {
                    QImage img = loadIconFileToImage(candidate);
                    if (!img.isNull()) return img;
                }
            }
        }
    }

    for (const QString &ext : exts) {
        const QString pixmapCandidate = QStringLiteral("/usr/share/pixmaps/") + iconKey + ext;
        if (QFileInfo::exists(pixmapCandidate)) {
            QImage img = loadIconFileToImage(pixmapCandidate);
            if (!img.isNull()) return img;
        }
    }
    return QImage();
}
#endif

QImage scaleImageForRequest(const QImage &img, const QSize &requestedSize)
{
    if (img.isNull()) return img;
    if (requestedSize.isValid() && !requestedSize.isEmpty()) {
        return img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (img.width() > 64 || img.height() > 64) {
        return img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return img;
}

class AppIconResponse : public QQuickImageResponse
{
    Q_OBJECT
public:
    explicit AppIconResponse(const QSize &requestedSize)
        : m_requestedSize(requestedSize)
    {}

    QQuickTextureFactory *textureFactory() const override
    {
        if (m_image.isNull()) return nullptr;
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override { return m_error; }

public slots:
    void completeWithImage(const QImage &image)
    {
        if (m_cancelled) return;
        m_image = scaleImageForRequest(image, m_requestedSize);
        if (m_image.isNull()) {
            m_error = QStringLiteral("Icon not available");
        }
        emit finished();
    }

    void cancel() override
    {
        m_cancelled = true;
    }

private:
    QSize m_requestedSize;
    QImage m_image;
    QString m_error;
    bool m_cancelled = false;
};

} // namespace

class AppIconProvider::Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr)
        : QObject(parent)
    {}

    ~Worker() override
    {
        delete m_nam;
        m_nam = nullptr;
    }

public slots:
    void initOnThread()
    {
#ifdef Q_OS_WIN
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif
        m_nam = new QNetworkAccessManager(this);
        QDir().mkpath(iconCacheRoot() + QStringLiteral("/apps"));
        QDir().mkpath(iconCacheRoot() + QStringLiteral("/web"));
    }

    void cleanupOnThread()
    {
#ifdef Q_OS_WIN
        CoUninitialize();
#endif
    }

    void resolveIcon(const QString &appName, const QString &rawUrl, QPointer<AppIconResponse> response)
    {
        if (!response) return;

        const QString domain = AppIconProvider::extractDomain(rawUrl);
        if (!domain.isEmpty()) {
            resolveWebsite(domain, response);
            return;
        }

        resolveDesktopApp(appName, response);
    }

private:
    struct WebCascadeState {
        QString cacheKey;
        QString diskPath;
        QStringList candidateUrls;
        QString htmlPageUrl;
        bool htmlTried = false;
        int index = 0;
        QVector<QPointer<AppIconResponse>> waiters;
    };

    QNetworkAccessManager *m_nam = nullptr;
    QHash<QString, QImage> m_memoryCache;
    QHash<QString, qint64> m_negativeCacheMs;
    QHash<QString, WebCascadeState*> m_inFlightWeb;

    void finishWaiters(const QString &cacheKey, const QImage &image, const QString &diskPath = QString())
    {
        if (!image.isNull()) {
            m_memoryCache.insert(cacheKey, image);
            m_negativeCacheMs.remove(cacheKey);
            if (!diskPath.isEmpty()) {
                image.save(diskPath, "PNG");
            }
        } else {
            m_negativeCacheMs.insert(cacheKey, QDateTime::currentMSecsSinceEpoch());
        }

        if (WebCascadeState *state = m_inFlightWeb.take(cacheKey)) {
            for (const QPointer<AppIconResponse> &w : std::as_const(state->waiters)) {
                if (w) {
                    QMetaObject::invokeMethod(w, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, image));
                }
            }
            delete state;
        }
    }

    void resolveDesktopApp(const QString &appName, QPointer<AppIconResponse> response)
    {
        const QString normKey = AppIconProvider::normalizeAppKey(appName);
        if (normKey.isEmpty()) {
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, QImage()));
            return;
        }

        if (normKey == QStringLiteral("deskmon") || normKey == QStringLiteral("windowlogger")) {
            QImage deskmonImg(QStringLiteral(":/icon.ico"));
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, deskmonImg));
            return;
        }

        const QString cacheKey = QStringLiteral("app:") + normKey;
        if (m_memoryCache.contains(cacheKey)) {
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, m_memoryCache.value(cacheKey)));
            return;
        }

        const QString diskPath = iconCacheRoot() + QStringLiteral("/apps/") + sanitizeCacheFilename(normKey) + QStringLiteral(".png");
        if (QFileInfo::exists(diskPath)) {
            QImage diskImg(diskPath);
            if (!diskImg.isNull()) {
                m_memoryCache.insert(cacheKey, diskImg);
                QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, diskImg));
                return;
            }
        }

#ifdef Q_OS_WIN
        // Tier 1: Native Windows .exe icon extraction
        QString exePath = AppIconProvider::lookupRegisteredExecutable(appName);
        if (exePath.isEmpty() || !QFileInfo::exists(exePath)) {
            exePath = findExeFromRunningProcesses(normKey);
        }
        if (exePath.isEmpty()) {
            exePath = findExeFromRegistryAppPaths(appName);
        }
        if (exePath.isEmpty()) {
            const QString candidate = appName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
                                          ? appName
                                          : (appName + QStringLiteral(".exe"));
            exePath = QStandardPaths::findExecutable(candidate);
        }

        if (!exePath.isEmpty()) {
            AppIconProvider::registerAppExecutable(appName, exePath);
            QImage nativeImg = extractWindowsExeIcon(exePath);
            if (!nativeImg.isNull()) {
                m_memoryCache.insert(cacheKey, nativeImg);
                nativeImg.save(diskPath, "PNG");
                QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, nativeImg));
                return;
            }
        }
#elif defined(Q_OS_MACOS)
        QImage nativeImg = extractMacAppIcon(appName, normKey);
        if (!nativeImg.isNull()) {
            m_memoryCache.insert(cacheKey, nativeImg);
            nativeImg.save(diskPath, "PNG");
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, nativeImg));
            return;
        }
#else
        QImage nativeImg = extractLinuxAppIcon(appName, normKey);
        if (!nativeImg.isNull()) {
            m_memoryCache.insert(cacheKey, nativeImg);
            nativeImg.save(diskPath, "PNG");
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, nativeImg));
            return;
        }
#endif

        // Tier 2: Curated fallback domain (e.g., for uninstalled preset apps)
        const QString curatedTarget = lookupCuratedAppDomain(appName);
        if (!curatedTarget.isEmpty()) {
            const QString hostOnly = curatedTarget.section('/', 0, 0);
            resolveWebsite(hostOnly, response, diskPath, cacheKey);
            return;
        }

        QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, QImage()));
    }

    void resolveWebsite(const QString &domain,
                        QPointer<AppIconResponse> response,
                        const QString &overrideDiskPath = QString(),
                        const QString &overrideCacheKey = QString())
    {
        const QString cacheKey = overrideCacheKey.isEmpty() ? (QStringLiteral("web:") + domain) : overrideCacheKey;

        if (m_memoryCache.contains(cacheKey)) {
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, m_memoryCache.value(cacheKey)));
            return;
        }

        const qint64 negTs = m_negativeCacheMs.value(cacheKey, 0);
        if (negTs > 0 && (QDateTime::currentMSecsSinceEpoch() - negTs) < kNegativeCacheTtlMs) {
            QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, QImage()));
            return;
        }

        const QString diskPath = overrideDiskPath.isEmpty()
                                     ? (iconCacheRoot() + QStringLiteral("/web/") + sanitizeCacheFilename(domain) + QStringLiteral(".png"))
                                     : overrideDiskPath;

        QFileInfo diskInfo(diskPath);
        if (diskInfo.exists() && diskInfo.lastModified().secsTo(QDateTime::currentDateTime()) < kWebCacheTtlSecs) {
            QImage diskImg(diskPath);
            if (!diskImg.isNull()) {
                m_memoryCache.insert(cacheKey, diskImg);
                QMetaObject::invokeMethod(response, "completeWithImage", Qt::QueuedConnection, Q_ARG(QImage, diskImg));
                return;
            }
        }

        if (m_inFlightWeb.contains(cacheKey)) {
            m_inFlightWeb[cacheKey]->waiters.append(response);
            return;
        }

        auto *state = new WebCascadeState;
        state->cacheKey = cacheKey;
        state->diskPath = diskPath;
        state->htmlPageUrl = QStringLiteral("https://") + domain + QStringLiteral("/");
        state->waiters.append(response);

        const QString rootDom = AppIconProvider::extractRootDomain(domain);
        // Cascade: Direct /favicon.ico -> HTML <link rel="icon"> -> DuckDuckGo -> Root /favicon.ico -> DuckDuckGo Root -> Google S2
        state->candidateUrls << (QStringLiteral("https://") + domain + QStringLiteral("/favicon.ico"));
        state->candidateUrls << (QStringLiteral("https://icons.duckduckgo.com/ip3/") + domain + QStringLiteral(".ico"));
        if (!rootDom.isEmpty() && rootDom != domain) {
            state->candidateUrls << (QStringLiteral("https://") + rootDom + QStringLiteral("/favicon.ico"));
            state->candidateUrls << (QStringLiteral("https://icons.duckduckgo.com/ip3/") + rootDom + QStringLiteral(".ico"));
        }
        state->candidateUrls << (QStringLiteral("https://www.google.com/s2/favicons?domain=") + domain + QStringLiteral("&sz=64"));
        if (!rootDom.isEmpty() && rootDom != domain) {
            state->candidateUrls << (QStringLiteral("https://www.google.com/s2/favicons?domain=") + rootDom + QStringLiteral("&sz=64"));
        }

        m_inFlightWeb.insert(cacheKey, state);
        tryNextWebStep(cacheKey);
    }

    void tryNextWebStep(const QString &cacheKey)
    {
        WebCascadeState *state = m_inFlightWeb.value(cacheKey, nullptr);
        if (!state || !m_nam) return;

        // After direct /favicon.ico (index 1) fails, try HTML <link rel="icon"> discovery once
        if (state->index == 1 && !state->htmlTried) {
            state->htmlTried = true;
            fetchHtmlForIconLink(cacheKey, state->htmlPageUrl);
            return;
        }

        if (state->index >= state->candidateUrls.size()) {
            finishWaiters(cacheKey, QImage());
            return;
        }

        const QString urlStr = state->candidateUrls.at(state->index++);
        QNetworkRequest req{QUrl(urlStr)};
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setRawHeader("User-Agent", "Mozilla/5.0 Deskmon/1.0");

        QNetworkReply *reply = m_nam->get(req);
        auto *timer = new QTimer(reply);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
        timer->start(kHttpTimeoutMs);

        connect(reply, &QNetworkReply::finished, this, [this, cacheKey, reply, timer]() {
            timer->stop();
            QImage img;
            if (reply->error() == QNetworkReply::NoError) {
                const QByteArray bytes = reply->readAll();
                if (!bytes.isEmpty()) {
                    img.loadFromData(bytes);
                }
            }
            reply->deleteLater();

            if (!img.isNull() && img.width() >= 12 && img.height() >= 12) {
                if (WebCascadeState *st = m_inFlightWeb.value(cacheKey, nullptr)) {
                    finishWaiters(cacheKey, img, st->diskPath);
                    return;
                }
            }
            tryNextWebStep(cacheKey);
        });
    }

    void fetchHtmlForIconLink(const QString &cacheKey, const QString &pageUrlStr)
    {
        WebCascadeState *state = m_inFlightWeb.value(cacheKey, nullptr);
        if (!state || !m_nam) return;

        const QUrl pageUrl(pageUrlStr);
        QNetworkRequest req(pageUrl);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setRawHeader("User-Agent", "Mozilla/5.0 Deskmon/1.0");

        QNetworkReply *reply = m_nam->get(req);
        auto *timer = new QTimer(reply);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
        timer->start(kHttpTimeoutMs);

        connect(reply, &QNetworkReply::finished, this, [this, cacheKey, pageUrl, reply, timer]() {
            timer->stop();
            QString foundIconHref;
            if (reply->error() == QNetworkReply::NoError) {
                const QString htmlSnippet = QString::fromUtf8(reply->read(49152));
                static const QRegularExpression linkRe(
                    QStringLiteral("<link\\b[^>]*rel\\s*=\\s*[\"'][^\"']*\\bicon\\b[^\"']*[\"'][^>]*>"),
                    QRegularExpression::CaseInsensitiveOption);
                static const QRegularExpression hrefRe(
                    QStringLiteral("href\\s*=\\s*[\"']([^\"']+)[\"']"),
                    QRegularExpression::CaseInsensitiveOption);

                auto it = linkRe.globalMatch(htmlSnippet);
                while (it.hasNext()) {
                    const QString tag = it.next().captured(0);
                    const QRegularExpressionMatch hm = hrefRe.match(tag);
                    if (hm.hasMatch()) {
                        foundIconHref = hm.captured(1).trimmed();
                        if (!foundIconHref.isEmpty()) break;
                    }
                }
            }
            const QUrl finalBase = reply->url().isValid() ? reply->url() : pageUrl;
            reply->deleteLater();

            if (!foundIconHref.isEmpty()) {
                if (foundIconHref.startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive)) {
                    const int comma = foundIconHref.indexOf(',');
                    if (comma != -1) {
                        const QByteArray raw = QByteArray::fromBase64(foundIconHref.mid(comma + 1).toLatin1());
                        QImage dataImg;
                        if (dataImg.loadFromData(raw) && !dataImg.isNull()) {
                            if (WebCascadeState *st = m_inFlightWeb.value(cacheKey, nullptr)) {
                                finishWaiters(cacheKey, dataImg, st->diskPath);
                                return;
                            }
                        }
                    }
                } else {
                    const QUrl resolved = finalBase.resolved(QUrl(foundIconHref));
                    if (resolved.isValid()) {
                        if (WebCascadeState *st = m_inFlightWeb.value(cacheKey, nullptr)) {
                            st->candidateUrls.insert(st->index, resolved.toString());
                        }
                    }
                }
            }
            tryNextWebStep(cacheKey);
        });
    }
};

AppIconProvider::AppIconProvider()
    : m_worker(new Worker)
{
    m_worker->moveToThread(&m_workerThread);
    QObject::connect(&m_workerThread, &QThread::started, m_worker, &Worker::initOnThread);
    QObject::connect(&m_workerThread, &QThread::finished, m_worker, &Worker::cleanupOnThread);
    QObject::connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread.start();
}

AppIconProvider::~AppIconProvider()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

QQuickImageResponse *AppIconProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto *response = new AppIconResponse(requestedSize);

    QString appName;
    QString rawUrl;
    const int qIdx = id.indexOf('?');
    if (qIdx != -1) {
        const QUrlQuery query(id.mid(qIdx + 1));
        appName = query.queryItemValue(QStringLiteral("app"), QUrl::FullyDecoded);
        rawUrl = query.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded);
    } else {
        appName = id;
    }

    QPointer<AppIconResponse> safeResp(response);
    QMetaObject::invokeMethod(m_worker, "resolveIcon", Qt::QueuedConnection,
                              Q_ARG(QString, appName),
                              Q_ARG(QString, rawUrl),
                              Q_ARG(QPointer<AppIconResponse>, safeResp));

    return response;
}

QString AppIconProvider::normalizeAppKey(const QString &appName)
{
    QString key = appName.toLower().trimmed();
    if (key.endsWith(QStringLiteral(".exe"))) {
        key.chop(4);
    }
    key.remove(' ').remove('-').remove('_').remove('.');
    return key;
}

void AppIconProvider::registerAppExecutable(const QString &appName, const QString &exePath)
{
    const QString key = normalizeAppKey(appName);
    if (key.isEmpty() || exePath.isEmpty()) return;

    QMutexLocker locker(&exeRegistryMutex());
    exeRegistry().insert(key, exePath);
}

QString AppIconProvider::lookupRegisteredExecutable(const QString &appName)
{
    const QString key = normalizeAppKey(appName);
    if (key.isEmpty()) return QString();

    QMutexLocker locker(&exeRegistryMutex());
    return exeRegistry().value(key);
}

QString AppIconProvider::extractDomain(const QString &rawUrl)
{
    const QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty()) return QString();

    const QString lower = trimmed.toLower();
    static const QStringList blockedPrefixes = {
        QStringLiteral("chrome://"), QStringLiteral("chrome-extension://"),
        QStringLiteral("edge://"), QStringLiteral("edge-extension://"),
        QStringLiteral("moz-extension://"), QStringLiteral("about:"),
        QStringLiteral("devtools://"), QStringLiteral("file://"),
        QStringLiteral("view-source:"), QStringLiteral("brave://"),
        QStringLiteral("opera://"), QStringLiteral("vivaldi://"),
        QStringLiteral("data:"), QStringLiteral("blob:"), QStringLiteral("javascript:")
    };
    for (const QString &prefix : blockedPrefixes) {
        if (lower.startsWith(prefix)) return QString();
    }

    const QString candidate = trimmed.contains(QStringLiteral("://"))
                                  ? trimmed
                                  : (QStringLiteral("https://") + trimmed);
    const QUrl parsed(candidate);
    QString host = parsed.host().toLower();
    if (host.startsWith(QStringLiteral("www."))) {
        host = host.mid(4);
    }
    if (host.isEmpty() || (!host.contains('.') && host != QStringLiteral("localhost"))) {
        return QString();
    }
    return host;
}

QString AppIconProvider::extractRootDomain(const QString &domain)
{
    if (domain.isEmpty()) return QString();
    const QStringList parts = domain.split('.', Qt::SkipEmptyParts);
    if (parts.size() <= 2) return QString();

    // Handle multi-label public suffixes such as .co.id, .or.id, .ac.id, .my.id, .co.uk, .com.au
    static const QStringList secondLevelSuffixes = {
        QStringLiteral("co"), QStringLiteral("com"), QStringLiteral("org"),
        QStringLiteral("net"), QStringLiteral("gov"), QStringLiteral("go"),
        QStringLiteral("edu"), QStringLiteral("ac"), QStringLiteral("or"),
        QStringLiteral("sch"), QStringLiteral("web"), QStringLiteral("my")
    };
    const QString penultimate = parts.at(parts.size() - 2).toLower();
    const QString last = parts.last().toLower();
    if (last.size() == 2 && secondLevelSuffixes.contains(penultimate)) {
        if (parts.size() <= 3) return QString();
        return parts.mid(parts.size() - 3).join('.');
    }

    return parts.mid(parts.size() - 2).join('.');
}

#include "AppIconProvider.moc"
