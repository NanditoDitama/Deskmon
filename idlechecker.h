#ifndef IDLECHECKER_H
#define IDLECHECKER_H
#include <QObject>
#include <QTimer>
class Logger;

class IdleChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int idleThreshold READ idleThreshold WRITE setIdleThreshold NOTIFY idleThresholdChanged)
public:
    explicit IdleChecker(Logger *logger, QObject *parent = nullptr);
    ~IdleChecker();
    int idleThreshold() const;
    void setIdleThreshold(int seconds);
    bool isIdle() const;
    void updateIdleThresholdFromDatabase();
signals:
    void idleDetected(qint64 startTime, qint64 endTime);
    void idleThresholdChanged();
    void showIdleNotification(QString message);
    void handleSystemNotification(const QString &message);
private slots:
    void checkIdleTime();
private:
    qint64 getSystemIdleTime() const;

#ifdef Q_OS_WIN
    qint64 getSystemIdleTimeWindows() const;
#elif defined(Q_OS_MACOS)
    qint64 getSystemIdleTimeMacOS() const;
#else
    qint64 getSystemIdleTimeLinux() const;
#endif
    QTimer m_timer;
    int m_idleThreshold = 180; // Default 2 menit
    qint64 m_lastActiveTime = 0;
    qint64 m_lastIdleLogTime = 0; // Waktu terakhir log idle dicatat
    qint64 m_lastThresholdCheckTime = 0; // Waktu terakhir pemeriksaan threshold
    bool m_isIdle = false;
    Logger* m_logger;
};
#endif
