#ifndef APPICONPROVIDER_H
#define APPICONPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QHash>
#include <QMutex>
#include <QThread>
#include <QPointer>
#include <QVector>

class QNetworkAccessManager;

class AppIconProvider : public QQuickAsyncImageProvider
{
public:
    AppIconProvider();
    ~AppIconProvider() override;

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    // Thread-safe registration from window tracking when a live .exe path is observed
    static void registerAppExecutable(const QString &appName, const QString &exePath);
    static QString lookupRegisteredExecutable(const QString &appName);
    static QString normalizeAppKey(const QString &appName);
    static QString extractDomain(const QString &rawUrl);
    static QString extractRootDomain(const QString &domain);

private:
    class Worker;
    QThread m_workerThread;
    Worker *m_worker = nullptr;
};

#endif // APPICONPROVIDER_H
