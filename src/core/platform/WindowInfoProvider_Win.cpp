#include "WindowInfoProvider.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <UIAutomation.h>
#include <QFileInfo>
#include <QDebug>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uiautomationcore.lib")

static QString getAppNameFromHwnd(HWND hwnd) {
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

static QString getBrowserUrlWindows(HWND hwnd) {
    QString appName = getAppNameFromHwnd(hwnd).toLower();

    // Hanya proses jika ini adalah peramban yang dikenal
    if (!appName.contains("chrome") && !appName.contains("firefox") &&
        !appName.contains("edge") && !appName.contains("opera")) {
        return QString();
    }

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        qWarning() << "Failed to initialize COM for UI Automation";
        return QString();
    }

    IUIAutomation *pAutomation = NULL;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&pAutomation);
    if (FAILED(hr) || !pAutomation) {
        qWarning() << "Failed to create UI Automation instance.";
        CoUninitialize();
        return QString();
    }

    IUIAutomationElement *pRootElement = NULL;
    hr = pAutomation->ElementFromHandle(hwnd, &pRootElement);
    if (FAILED(hr) || !pRootElement) {
        qWarning() << "Failed to get UI Automation element from handle.";
        pAutomation->Release();
        CoUninitialize();
        return QString();
    }

    IUIAutomationCondition *pCondition = NULL;
    VARIANT varControlType;
    varControlType.vt = VT_I4;
    varControlType.lVal = UIA_EditControlTypeId;

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
    info.appName = getAppNameFromHwnd(hwnd);
    info.url = getBrowserUrlWindows(hwnd);

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}

#endif // Q_OS_WIN
