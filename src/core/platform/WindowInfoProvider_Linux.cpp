#include "WindowInfoProvider.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

static QString getBrowserUrlLinux() {
    QProcess process;
    process.start("xdotool", {"getactivewindow", "getwindowname"});
    if (process.waitForFinished(100)) {
        QString title = QString(process.readAllStandardOutput()).trimmed();
        QRegularExpression urlRegex(R"(https?://[^\s/$.?#].[^\s]*)");
        QRegularExpressionMatch match = urlRegex.match(title);
        if (match.hasMatch()) {
            return match.captured(0);
        }
    }
    return "";
}

WindowInfo WindowInfoProvider::getActiveWindowInfo() {
    WindowInfo info;

    QProcess process;
    process.start("xdotool", {"getwindowfocus", "getwindowname"});
    if (process.waitForFinished(100)) {
        info.title = QString(process.readAllStandardOutput()).trimmed();
    }

    process.start("xdotool", {"getwindowfocus", "getwindowpid"});
    if (process.waitForFinished(100)) {
        QString pid = QString(process.readAllStandardOutput()).trimmed();
        if (!pid.isEmpty()) {
            process.start("ps", {"-p", pid, "-o", "comm="});
            if (process.waitForFinished(100)) {
                info.appName = QString(process.readAllStandardOutput()).trimmed();
            }
        }
    }

    if (info.title.isEmpty()) {
        process.start("wmctrl", {"-l"});
        if (process.waitForFinished(100)) {
            QStringList windows = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
            for (const QString &window : windows) {
                QStringList parts = window.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 4 && parts[0].contains("0x")) {
                    info.title = parts.mid(3).join(' ');
                    break;
                }
            }
        }
    }

    info.url = getBrowserUrlLinux();

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}

#endif // Linux
