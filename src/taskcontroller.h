#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QSqlQueryModel>

class TaskController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeTaskId READ activeTaskId NOTIFY activeTaskIdChanged)
    Q_PROPERTY(bool isTaskPaused READ isTaskPaused NOTIFY taskPausedChanged)

public:
    explicit TaskController(QObject *parent = nullptr);

    void setActiveTask(int taskId, int userId);
    void finishTask(int taskId);
    void toggleTaskPause(int taskId);
    void updateTaskStatus(int taskId);
    void syncActiveTask(int userId);
    void refreshTasks(int userId);

    int activeTaskId() const { return m_activeTaskId; }
    bool isTaskPaused() const { return m_isPaused; }
    qint64 taskTimeOffset() const { return m_timeOffset; }
    qint64 taskStartTime() const { return m_startTime; }

signals:
    void activeTaskIdChanged();
    void activeTaskChanged(int taskId);
    void taskPausedChanged(bool isPaused);
    void taskPausedChanged();
    void taskListChanged();
    void taskStatusChanged(int taskId, const QString &status);
    void requestTaskDetails(int taskId, const QString &action, int nextTaskId);

private:
    int m_activeTaskId = -1;
    bool m_isPaused = false;
    qint64 m_timeOffset = 0;
    qint64 m_startTime = 0;
};

#endif // TASKCONTROLLER_H
