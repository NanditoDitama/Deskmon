#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class WindowManager : public QObject
{
    Q_OBJECT
public:
    struct WindowInfo {
        QString appName;
        QString title;
        QString url;
    };

    explicit WindowManager(QObject *parent = nullptr);

    WindowInfo getActiveWindowInfo();

private:
#ifdef Q_OS_WIN
    WindowInfo getActiveWindowInfoWindows();
    QString getBrowserUrlWindows(HWND hwnd);
    QString getAppNameFromHwnd(HWND hwnd);
#elif defined(Q_OS_MACOS)
    WindowInfo getActiveWindowInfoMacOS();
    QString getBrowserUrlMac();
#else
    WindowInfo getActiveWindowInfoLinux();
    QString getBrowserUrlLinux();
#endif
};

#endif // WINDOWMANAGER_H
