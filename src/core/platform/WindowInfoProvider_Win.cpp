#include "WindowInfoProvider.h"
#include "AppIconProvider.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <UIAutomation.h>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QUrl>
#include <QDebug>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uiautomationcore.lib")

namespace {
struct BrowserUrlCache {
    QMutex mutex;
    HWND hwnd = nullptr;
    QString title;
    QString url;
    ULONGLONG timestampMs = 0;
};

BrowserUrlCache& browserUrlCache()
{
    static BrowserUrlCache s_cache;
    return s_cache;
}
} // namespace

static QString queryExePathForPid(DWORD processId) {
    if (processId == 0) return QString();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess != NULL) {
        wchar_t exePath[MAX_PATH * 2];
        DWORD len = sizeof(exePath) / sizeof(exePath[0]);
        QString result;
        if (QueryFullProcessImageNameW(hProcess, 0, exePath, &len) && len > 0) {
            result = QString::fromWCharArray(exePath, static_cast<int>(len));
        }
        CloseHandle(hProcess);
        return result;
    }
    return QString();
}

struct UwpChildSearch {
    DWORD hostPid = 0;
    QString childExePath;
};

static BOOL CALLBACK enumUwpChildProc(HWND childHwnd, LPARAM lParam) {
    auto *search = reinterpret_cast<UwpChildSearch*>(lParam);
    DWORD childPid = 0;
    GetWindowThreadProcessId(childHwnd, &childPid);
    if (childPid != 0 && childPid != search->hostPid) {
        const QString p = queryExePathForPid(childPid);
        if (!p.isEmpty()) {
            const QString base = QFileInfo(p).baseName();
            if (base.compare(QStringLiteral("ApplicationFrameHost"), Qt::CaseInsensitive) != 0) {
                search->childExePath = p;
                return FALSE; // Found real UWP app process
            }
        }
    }
    return TRUE;
}

static void resolveProcessInfoFromHwnd(HWND hwnd, QString &outAppName, QString &outExePath) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    QString exePath = queryExePathForPid(processId);
    if (!exePath.isEmpty()) {
        QFileInfo fileInfo(exePath);
        QString baseName = fileInfo.baseName();

        if (baseName.compare(QStringLiteral("ApplicationFrameHost"), Qt::CaseInsensitive) == 0) {
            UwpChildSearch search;
            search.hostPid = processId;
            EnumChildWindows(hwnd, enumUwpChildProc, reinterpret_cast<LPARAM>(&search));
            if (!search.childExePath.isEmpty()) {
                exePath = search.childExePath;
                baseName = QFileInfo(exePath).baseName();
            }
        }

        outExePath = exePath;
        outAppName = baseName;
        return;
    }

    outExePath.clear();
    outAppName = QStringLiteral("Unknown");
}

static QString sanitizeBrowserUrl(const QString &rawUrl, bool requireExplicitScheme = false) {
    const QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(' ')) {
        return QString();
    }

    const QString lower = trimmed.toLower();
    const bool hasExplicitScheme = lower.startsWith(QStringLiteral("http://")) ||
                                   lower.startsWith(QStringLiteral("https://"));
    if (requireExplicitScheme && !hasExplicitScheme) {
        return QString();
    }

    static const QStringList blockedPrefixes = {
        QStringLiteral("chrome://"),
        QStringLiteral("chrome-extension://"),
        QStringLiteral("edge://"),
        QStringLiteral("edge-extension://"),
        QStringLiteral("moz-extension://"),
        QStringLiteral("about:"),
        QStringLiteral("devtools://"),
        QStringLiteral("file://"),
        QStringLiteral("view-source:"),
        QStringLiteral("brave://"),
        QStringLiteral("opera://"),
        QStringLiteral("vivaldi://"),
        QStringLiteral("data:"),
        QStringLiteral("blob:"),
        QStringLiteral("javascript:")
    };
    for (const QString &prefix : blockedPrefixes) {
        if (lower.startsWith(prefix)) {
            return QString();
        }
    }

    const QString candidate = trimmed.contains(QStringLiteral("://"))
                                  ? trimmed
                                  : (QStringLiteral("https://") + trimmed);
    const QUrl parsed(candidate);
    if (!parsed.isValid()) {
        return QString();
    }
    const QString scheme = parsed.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        return QString();
    }
    const QString host = parsed.host().toLower();
    if (!WindowInfoProvider::isValidWebHost(host)) {
        return QString();
    }
    return trimmed;
}

static QString getBrowserUrlWindows(HWND hwnd, const QString &appName, const QString &windowTitle) {
    const QString lowerApp = appName.toLower();

    // Hanya proses jika ini adalah peramban yang dikenal
    if (!lowerApp.contains("chrome") && !lowerApp.contains("firefox") &&
        !lowerApp.contains("edge") && !lowerApp.contains("opera") &&
        !lowerApp.contains("brave") && !lowerApp.contains("vivaldi") &&
        !lowerApp.contains("arc")) {
        return QString();
    }

    const ULONGLONG nowMs = GetTickCount64();
    {
        QMutexLocker locker(&browserUrlCache().mutex);
        if (browserUrlCache().hwnd == hwnd &&
            browserUrlCache().title == windowTitle &&
            (nowMs - browserUrlCache().timestampMs) < 5000) {
            return browserUrlCache().url;
        }
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    const bool shouldUninit = (hr == S_OK || hr == S_FALSE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        qWarning() << "Failed to initialize COM for UI Automation";
        return QString();
    }

    IUIAutomation *pAutomation = NULL;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&pAutomation);
    if (FAILED(hr) || !pAutomation) {
        qWarning() << "Failed to create UI Automation instance.";
        if (shouldUninit) CoUninitialize();
        return QString();
    }

    IUIAutomationElement *pRootElement = NULL;
    hr = pAutomation->ElementFromHandle(hwnd, &pRootElement);
    if (FAILED(hr) || !pRootElement) {
        qWarning() << "Failed to get UI Automation element from handle.";
        pAutomation->Release();
        if (shouldUninit) CoUninitialize();
        return QString();
    }

    IUIAutomationCondition *pCondition = NULL;
    VARIANT varControlType;
    varControlType.vt = VT_I4;
    varControlType.lVal = UIA_EditControlTypeId;

    hr = pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, varControlType, &pCondition);

    QString url;
    QString fallbackUrl;
    if (SUCCEEDED(hr) && pCondition) {
        IUIAutomationElementArray *pElements = NULL;
        if (SUCCEEDED(pRootElement->FindAll(TreeScope_Descendants, pCondition, &pElements)) && pElements) {
            int count = 0;
            pElements->get_Length(&count);
            const int maxInspect = qMin(count, 20);
            for (int i = 0; i < maxInspect; ++i) {
                IUIAutomationElement *pElem = NULL;
                if (FAILED(pElements->GetElement(i, &pElem)) || !pElem) continue;

                BSTR bstrName = NULL;
                QString elemName;
                if (SUCCEEDED(pElem->get_CurrentName(&bstrName)) && bstrName) {
                    elemName = QString::fromWCharArray(bstrName).toLower();
                    SysFreeString(bstrName);
                }

                VARIANT vtValue;
                VariantInit(&vtValue);
                QString val;
                if (SUCCEEDED(pElem->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &vtValue))) {
                    if (vtValue.vt == VT_BSTR && vtValue.bstrVal != NULL) {
                        val = QString::fromWCharArray(vtValue.bstrVal);
                    }
                    VariantClear(&vtValue);
                }
                pElem->Release();

                const bool isAddressBar = elemName.contains(QStringLiteral("address")) ||
                                          elemName.contains(QStringLiteral("alamat")) ||
                                          elemName.contains(QStringLiteral("url")) ||
                                          elemName.contains(QStringLiteral("search or enter"));
                const QString cleanUrl = sanitizeBrowserUrl(val, !isAddressBar);
                if (cleanUrl.isEmpty()) continue;

                if (isAddressBar) {
                    url = cleanUrl;
                    break;
                }
                if (fallbackUrl.isEmpty()) {
                    fallbackUrl = cleanUrl;
                }
            }
            pElements->Release();
        }
        pCondition->Release();
    }
    if (url.isEmpty()) {
        url = fallbackUrl;
    }

    pRootElement->Release();
    pAutomation->Release();
    if (shouldUninit) CoUninitialize();

    {
        QMutexLocker locker(&browserUrlCache().mutex);
        browserUrlCache().hwnd = hwnd;
        browserUrlCache().title = windowTitle;
        browserUrlCache().url = url;
        browserUrlCache().timestampMs = GetTickCount64();
    }

    return url;
}

WindowInfo WindowInfoProvider::getActiveWindowInfo() {
    WindowInfo info;
    HWND hwnd = GetForegroundWindow();

    if (hwnd == NULL) {
        info.appName = "Unknown";
        info.title = "No active window";
        return info;
    }

    wchar_t buffer[256];
    GetWindowTextW(hwnd, buffer, 256);
    info.title = QString::fromWCharArray(buffer);
    resolveProcessInfoFromHwnd(hwnd, info.appName, info.exePath);
    if (!info.exePath.isEmpty()) {
        AppIconProvider::registerAppExecutable(info.appName, info.exePath);
    }
    info.url = getBrowserUrlWindows(hwnd, info.appName, info.title);

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}

#endif // Q_OS_WIN
