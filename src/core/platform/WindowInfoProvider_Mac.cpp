#include "WindowInfoProvider.h"

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QProcess>
#include <QRegularExpression>

static QString getBrowserUrlMac() {
    QProcess frontAppProc;
    frontAppProc.start("osascript", {"-e", "tell application \"System Events\" to get name of first application process whose frontmost is true"});
    frontAppProc.waitForFinished(1000);
    QString frontApp = frontAppProc.readAllStandardOutput().trimmed();

    QString url;
    QStringList script;
    if (frontApp == "Safari") {
        script << "tell application \"Safari\" to try" << "return URL of front document" << "on error" << "return \"\"" << "end try";
    } else if (frontApp == "Google Chrome" || frontApp == "Brave Browser" || frontApp == "Microsoft Edge") {
        script << QString("tell application \"%1\" to try").arg(frontApp)
               << "return URL of active tab of front window"
               << "on error" << "return \"\"" << "end try";
    } else {
        QProcess titleProc;
        titleProc.start("osascript", {
            "-e",
            "tell application \"System Events\" to tell (first application process whose frontmost is true) to get name of front window"
        });
        titleProc.waitForFinished(1000);
        QString windowTitle = titleProc.readAllStandardOutput().trimmed();

        QRegularExpression urlRegex(R"(https?://[^\s/$.?#].[^\s]*)");
        QRegularExpressionMatch match = urlRegex.match(windowTitle);
        if (match.hasMatch()) {
            return match.captured(0);
        } else {
            return "";
        }
    }

    if (!script.isEmpty()) {
        QProcess scriptProc;
        scriptProc.start("osascript", QStringList() << "-e" << script.join("\n"));
        scriptProc.waitForFinished(1000);
        url = scriptProc.readAllStandardOutput().trimmed();
    }

    return url;
}

WindowInfo WindowInfoProvider::getActiveWindowInfo() {
    WindowInfo info;

    NSRunningApplication *frontmostApp = [[NSWorkspace sharedWorkspace] frontmostApplication];

    if (frontmostApp) {
        info.appName = QString::fromNSString([frontmostApp localizedName]);

        pid_t pid = [frontmostApp processIdentifier];
        AXUIElementRef appElem = AXUIElementCreateApplication(pid);

        if (appElem) {
            AXUIElementRef window = NULL;
            if (AXUIElementCopyAttributeValue(appElem, kAXFocusedWindowAttribute, (CFTypeRef*)&window) == kAXErrorSuccess) {
                if (window) {
                    CFStringRef title = NULL;
                    if (AXUIElementCopyAttributeValue(window, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess) {
                        if (title) {
                            info.title = QString::fromCFString(title);
                            CFRelease(title);
                        }
                    }
                    CFRelease(window);
                }
            }
            CFRelease(appElem);
        }
    }

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    info.url = getBrowserUrlMac();
    return info;
}

#endif // Q_OS_MACOS
