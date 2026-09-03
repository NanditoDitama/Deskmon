#ifndef TASKREPOSITORY_H
#define TASKREPOSITORY_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QPair>
#include <QString>

class TaskRepository : public QObject
{
    Q_OBJECT
public:
    explicit TaskRepository(QObject *parent = nullptr);
    ~TaskRepository() override = default;

    QVariantList getTasksForUser(int userId) const;
    QMap<int, QPair<int, int>> getExistingTasksMap(int userId) const;

    bool deleteTask(int taskId, int userId);
    bool upsertTask(int taskId, int userId, const QString &projectName, const QString &taskDesc,
                    int maxTime, int timeUsage, const QString &createdAt, const QString &status = "pending");

    QList<int> getTaskIdsForUser(int userId) const;
    bool findActiveTask(int userId, int &outTaskId, bool &outIsPaused, int &outTimeUsage);
    bool resetActiveTaskStatus(int taskIdToKeep = -1);

    bool updateTaskStatus(int taskId, const QString &status);
    bool updateTaskTiming(int taskId, int timeUsage, bool active, bool paused);
    bool updateTaskMaxTime(int taskId, int maxTime);

    bool logPauseEvent(int taskId, const QString &startTime);
    bool logPlayEvent(int taskId, const QString &startTime);
    bool logPausePlayData(int taskId, const QString &startTime, const QString &endTime, const QString &status);

    bool archiveCompletedTask(int taskId, int userId);
    int getPendingStartedTaskCount(int userId) const;
    QString getTaskName(int taskId) const;
};

#endif // TASKREPOSITORY_H
