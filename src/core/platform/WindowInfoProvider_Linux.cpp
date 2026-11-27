#include "WindowInfoProvider.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

static QString extractUrlFromTitle(const QString &title) {
    static const QRegularExpression urlRegex(QStringLiteral(R"(https?://[^\s/$.?#].[^\s]*)"));
    const QRegularExpressionMatch match = urlRegex.match(title);
    return match.hasMatch() ? match.captured(0) : QString();
}

static bool tryFindFocusedSwayNode(const QJsonObject &node, WindowInfo &info) {
    if (node.value(QStringLiteral("focused")).toBool()) {
        info.title = node.value(QStringLiteral("name")).toString().trimmed();
        info.appName = node.value(QStringLiteral("app_id")).toString().trimmed();
        if (info.appName.isEmpty()) {
            info.appName = node.value(QStringLiteral("window_properties")).toObject().value(QStringLiteral("class")).toString().trimmed();
        }
        return !info.title.isEmpty() || !info.appName.isEmpty();
    }

    const QJsonArray nodes = node.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue &child : nodes) {
        if (tryFindFocusedSwayNode(child.toObject(), info)) return true;
    }
    const QJsonArray floating = node.value(QStringLiteral("floating_nodes")).toArray();
    for (const QJsonValue &child : floating) {
        if (tryFindFocusedSwayNode(child.toObject(), info)) return true;
    }
    return false;
}

static bool tryWaylandWindowInfo(WindowInfo &info) {
    // 1. Hyprland
    if (qEnvironmentVariableIsSet("HYPRLAND_INSTANCE_SIGNATURE")) {
        QProcess proc;
        proc.start(QStringLiteral("hyprctl"), {QStringLiteral("activewindow"), QStringLiteral("-j")});
        if (proc.waitForFinished(150) && proc.exitCode() == 0) {
            const QJsonObject obj = QJsonDocument::fromJson(proc.readAllStandardOutput()).object();
            if (!obj.isEmpty()) {
                info.appName = obj.value(QStringLiteral("class")).toString().trimmed();
                info.title = obj.value(QStringLiteral("title")).toString().trimmed();
                if (!info.appName.isEmpty() || !info.title.isEmpty()) return true;
            }
        }
    }

    // 2. Sway / wlroots i3-compatible IPC
    if (qEnvironmentVariableIsSet("SWAYSOCK")) {
        QProcess proc;
        proc.start(QStringLiteral("swaymsg"), {QStringLiteral("-t"), QStringLiteral("get_tree")});
        if (proc.waitForFinished(150) && proc.exitCode() == 0) {
            const QJsonObject root = QJsonDocument::fromJson(proc.readAllStandardOutput()).object();
            if (tryFindFocusedSwayNode(root, info)) return true;
        }
    }

    return false;
}

WindowInfo WindowInfoProvider::getActiveWindowInfo() {
    WindowInfo info;

    const bool isWayland = qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
        || qEnvironmentVariable("XDG_SESSION_TYPE").compare(QStringLiteral("wayland"), Qt::CaseInsensitive) == 0;

    if (!isWayland || !tryWaylandWindowInfo(info)) {
        // X11 / XWayland fallback via xdotool + wmctrl
        QProcess process;
        process.start(QStringLiteral("xdotool"), {QStringLiteral("getwindowfocus"), QStringLiteral("getwindowname")});
        if (process.waitForFinished(100)) {
            info.title = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        }

        process.start(QStringLiteral("xdotool"), {QStringLiteral("getwindowfocus"), QStringLiteral("getwindowpid")});
        if (process.waitForFinished(100)) {
            const QString pid = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            if (!pid.isEmpty()) {
                process.start(QStringLiteral("ps"), {QStringLiteral("-p"), pid, QStringLiteral("-o"), QStringLiteral("comm=")});
                if (process.waitForFinished(100)) {
                    info.appName = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
                }
            }
        }

        if (info.title.isEmpty()) {
            process.start(QStringLiteral("wmctrl"), {QStringLiteral("-l")});
            if (process.waitForFinished(100)) {
                const QStringList windows = QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                for (const QString &window : windows) {
                    const QStringList parts = window.split(' ', Qt::SkipEmptyParts);
                    if (parts.size() >= 4 && parts[0].contains(QStringLiteral("0x"))) {
                        info.title = parts.mid(3).join(' ');
                        break;
                    }
                }
            }
        }
    }

    info.url = extractUrlFromTitle(info.title);

    if (info.appName.isEmpty()) info.appName = QStringLiteral("Unknown");
    if (info.title.isEmpty()) info.title = QStringLiteral("No active window");

    return info;
}

#endif // Linux
