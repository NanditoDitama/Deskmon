#ifndef WINDOWINFOPROVIDER_H
#define WINDOWINFOPROVIDER_H

#include <QHostAddress>
#include <QSet>
#include <QString>
#include <QStringList>

struct WindowInfo {
    QString appName;
    QString title;
    QString url;
    QString exePath;
};

class WindowInfoProvider
{
public:
    static WindowInfo getActiveWindowInfo();

    static bool isValidWebHost(const QString &host)
    {
        const QString h = host.trimmed().toLower();
        if (h.isEmpty()) return false;
        if (h == QStringLiteral("localhost")) return true;

        QHostAddress ipAddr;
        if (ipAddr.setAddress(h) && ipAddr.protocol() == QAbstractSocket::IPv4Protocol) {
            return true;
        }

        if (!h.contains(QLatin1Char('.')) || h.startsWith(QLatin1Char('.')) || h.endsWith(QLatin1Char('.'))) {
            return false;
        }

        const QStringList labels = h.split(QLatin1Char('.'));
        if (labels.size() < 2) return false;

        for (const QString &label : labels) {
            if (label.isEmpty() || label.startsWith(QLatin1Char('-')) || label.endsWith(QLatin1Char('-'))) {
                return false;
            }
            for (const QChar &ch : label) {
                if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z')) ||
                      (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) ||
                      ch == QLatin1Char('-'))) {
                    return false;
                }
            }
        }

        const QString &tld = labels.last();
        bool hasAlpha = false;
        for (const QChar &ch : tld) {
            if (ch >= QLatin1Char('a') && ch <= QLatin1Char('z')) {
                hasAlpha = true;
                break;
            }
        }
        if (!hasAlpha || tld.size() < 2) {
            return false;
        }

        static const QSet<QString> blockedExtensions = {
            QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("gif"),
            QStringLiteral("svg"), QStringLiteral("webp"), QStringLiteral("bmp"), QStringLiteral("ico"),
            QStringLiteral("tif"), QStringLiteral("tiff"), QStringLiteral("pdf"), QStringLiteral("txt"),
            QStringLiteral("md"), QStringLiteral("csv"), QStringLiteral("tsv"), QStringLiteral("json"),
            QStringLiteral("xml"), QStringLiteral("yaml"), QStringLiteral("yml"), QStringLiteral("toml"),
            QStringLiteral("ini"), QStringLiteral("cfg"), QStringLiteral("conf"), QStringLiteral("log"),
            QStringLiteral("sql"), QStringLiteral("db"), QStringLiteral("sqlite"), QStringLiteral("zip"),
            QStringLiteral("rar"), QStringLiteral("7z"), QStringLiteral("tar"), QStringLiteral("gz"),
            QStringLiteral("bz2"), QStringLiteral("xz"), QStringLiteral("exe"), QStringLiteral("msi"),
            QStringLiteral("dll"), QStringLiteral("so"), QStringLiteral("dylib"), QStringLiteral("app"),
            QStringLiteral("dmg"), QStringLiteral("deb"), QStringLiteral("rpm"), QStringLiteral("apk"),
            QStringLiteral("ipa"), QStringLiteral("cpp"), QStringLiteral("hpp"), QStringLiteral("c"),
            QStringLiteral("h"), QStringLiteral("cc"), QStringLiteral("cxx"), QStringLiteral("cs"),
            QStringLiteral("java"), QStringLiteral("kt"), QStringLiteral("swift"), QStringLiteral("py"),
            QStringLiteral("rb"), QStringLiteral("go"), QStringLiteral("rs"), QStringLiteral("php"),
            QStringLiteral("js"), QStringLiteral("jsx"), QStringLiteral("ts"), QStringLiteral("tsx"),
            QStringLiteral("mjs"), QStringLiteral("cjs"), QStringLiteral("qml"), QStringLiteral("css"),
            QStringLiteral("scss"), QStringLiteral("less"), QStringLiteral("vue"), QStringLiteral("svelte"),
            QStringLiteral("doc"), QStringLiteral("docx"), QStringLiteral("xls"), QStringLiteral("xlsx"),
            QStringLiteral("ppt"), QStringLiteral("pptx"), QStringLiteral("odt"), QStringLiteral("ods"),
            QStringLiteral("odp"), QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"),
            QStringLiteral("aac"), QStringLiteral("ogg"), QStringLiteral("mp4"), QStringLiteral("mkv"),
            QStringLiteral("avi"), QStringLiteral("mov"), QStringLiteral("wmv"), QStringLiteral("webm"),
            QStringLiteral("bin"), QStringLiteral("iso"), QStringLiteral("img"), QStringLiteral("tmp"),
            QStringLiteral("bak"), QStringLiteral("lock"), QStringLiteral("env")
        };

        if (blockedExtensions.contains(tld)) {
            return false;
        }
        return true;
    }
};

#endif // WINDOWINFOPROVIDER_H
