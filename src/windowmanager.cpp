#include "windowmanager.h"
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <psapi.h>
#include <UIAutomation.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "uiautomationcore.lib")
#endif

WindowManager::WindowManager(QObject *parent) : QObject(parent)
{
}

WindowManager::WindowInfo WindowManager::getActiveWindowInfo()
{
#ifdef Q_OS_WIN
    return getActiveWindowInfoWindows();
#elif defined(Q_OS_MACOS)
    return getActiveWindowInfoMacOS();
#elif defined(Q_OS_LINUX)
    return getActiveWindowInfoLinux();
#else
    WindowInfo info;
    info.appName = "Unsupported OS";
    info.title = "Unsupported OS";
    return info;
#endif
}

#ifdef Q_OS_WIN
WindowManager::WindowInfo WindowManager::getActiveWindowInfoWindows() {
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
    info.appName = getAppNameFromHwnd(hwnd);
    info.url = getBrowserUrlWindows(hwnd);

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}

QString WindowManager::getAppNameFromHwnd(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess != NULL) {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, exePath, MAX_PATH)) {
            QFileInfo fileInfo(QString::fromWCharArray(exePath));
            CloseHandle(hProcess);
            return fileInfo.baseName();
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
}

QString WindowManager::getBrowserUrlWindows(HWND hwnd) {
    QString appName = getAppNameFromHwnd(hwnd).toLower();

    if (!appName.contains("chrome") && !appName.contains("firefox") &&
        !appName.contains("edge") && !appName.contains("opera")) {
        return QString();
    }

    CoInitialize(NULL);
    IUIAutomation *pAutomation = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&pAutomation);
    if (FAILED(hr) || !pAutomation) {
        CoUninitialize();
        return QString();
    }

    IUIAutomationElement *pRootElement = NULL;
    hr = pAutomation->ElementFromHandle(hwnd, &pRootElement);
    if (FAILED(hr) || !pRootElement) {
        pAutomation->Release();
        CoUninitialize();
        return QString();
    }

    VARIANT varControlType;
    varControlType.vt = VT_I4;
    varControlType.lVal = UIA_EditControlTypeId;

    IUIAutomationCondition *pCondition = NULL;
    hr = pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, varControlType, &pCondition);

    IUIAutomationElement *pAddressBar = NULL;
    if (SUCCEEDED(hr) && pCondition) {
        pRootElement->FindFirst(TreeScope_Descendants, pCondition, &pAddressBar);
        pCondition->Release();
    }

    QString url;
    if (pAddressBar) {
        VARIANT vtValue;
        VariantInit(&vtValue);
        hr = pAddressBar->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &vtValue);
        if (SUCCEEDED(hr) && vtValue.vt == VT_BSTR && vtValue.bstrVal != NULL) {
            url = QString::fromWCharArray(vtValue.bstrVal);
            VariantClear(&vtValue);
        }
        pAddressBar->Release();
    }

    pRootElement->Release();
    pAutomation->Release();
    CoUninitialize();

    return url;
}
#endif

// Placeholder for other OS logic...
#ifndef Q_OS_WIN
WindowManager::WindowInfo WindowManager::getActiveWindowInfoLinux() { return {"Unknown", "No active window", ""}; }
QString WindowManager::getBrowserUrlLinux() { return ""; }
#endif
