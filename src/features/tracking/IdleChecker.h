#ifndef IDLECHECKER_H
#define IDLECHECKER_H

#include <QObject>
#include <QTimer>

class AppController;

class IdleChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int idleThreshold READ idleThreshold WRITE setIdleThreshold NOTIFY idleThresholdChanged)
public:
    explicit IdleChecker(AppController *appController, QObject *parent = nullptr);
    ~IdleChecker() override;

    int idleThreshold() const;
    void setIdleThreshold(int seconds);
    bool isIdle() const;
    void updateIdleThresholdFromDatabase();

signals:
    void idleDetected(qint64 startTime, qint64 endTime);
    void idleThresholdChanged();
    void showIdleNotification(QString message);
    void handleSystemNotification(const QString &message);
    void hideIdleNotification();

private slots:
    void checkIdleTime();

private:
    qint64 getSystemIdleTime() const;
    bool m_autoPausedByIdle = false;

#ifdef Q_OS_WIN
    qint64 getSystemIdleTimeWindows() const;
#elif defined(Q_OS_MACOS)
    qint64 getSystemIdleTimeMacOS() const;
#else
    qint64 getSystemIdleTimeLinux() const;
#endif

    QTimer m_timer;
    int m_idleThreshold = 180;
    qint64 m_lastActiveTime = 0;
    qint64 m_lastIdleLogTime = 0;
    qint64 m_lastThresholdCheckTime = 0;
    bool m_isIdle = false;
    AppController* m_appController;
};

#endif // IDLECHECKER_H
