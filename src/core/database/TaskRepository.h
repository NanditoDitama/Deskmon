#ifndef TASKREPOSITORY_H
#define TASKREPOSITORY_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QDate>
#include <QVector>

class TaskRepository : public QObject
{
    Q_OBJECT
public:
    struct SyncedTaskRow {
        int id = 0;
        QString projectName;
        QString taskDesc;
        bool hasDuration = false;
        int serverMaxTime = 0;
        bool hasTotalDuration = false;
        int serverTimeUsage = 0;
        QString createdAt;
    };

    explicit TaskRepository(QObject *parent = nullptr);
    ~TaskRepository() override = default;

    QVariantList getTasksForUser(int userId, bool isTaskPaused = false) const;
    QMap<int, QPair<int, int>> getExistingTasksMap(int userId) const;

    static bool syncTasksToDatabase(QSqlDatabase &db,
                                    int userId,
                                    const QVector<SyncedTaskRow> &tasks,
                                    const QSet<int> &serverTaskIds,
                                    int currentActiveTaskId,
                                    bool &outActiveTaskRemoved);

    bool deleteTask(int taskId, int userId);
    bool upsertTask(int taskId, int userId, const QString &projectName, const QString &taskDesc,
                    int maxTime, int timeUsage, const QString &createdAt, const QString &status = QStringLiteral("Pending"));

    QList<int> getTaskIdsForUser(int userId) const;
    bool findActiveTask(int userId, int &outTaskId, bool &outIsPaused, int &outTimeUsage);
    bool findStartupTask(int userId, int &outTaskId, bool &outIsPaused, int &outTimeUsage, QString &outStatus) const;
    bool getTaskDetails(int taskId, int &outMaxTime, int &outTimeUsage, QString &outStatus) const;
    int findNextEligibleTaskId(int userId, int excludedTaskId) const;
    bool deactivateTask(int taskId, const QString &status, int timeUsage);
    bool resetActiveTaskStatus(int taskIdToKeep = -1);

    bool updateTaskStatus(int taskId, const QString &status);
    bool updateTaskTiming(int taskId, int timeUsage, bool active, bool paused);
    bool updateTaskMaxTime(int taskId, int maxTime);

    bool logPauseEvent(int taskId, const QString &startTime);
    bool logPlayEvent(int taskId, const QString &startTime);
    bool logPausePlayData(int taskId, const QString &startTime, const QString &endTime, const QString &status);

    bool archiveCompletedTask(int taskId, int userId, int overrideTimeUsage = -1);
    int getPendingStartedTaskCount(int userId) const;
    QString getTaskName(int taskId) const;

    static bool computeIsExpired(const QString &createdAtStr, int maxTimeSeconds, const QDate &today = QDate::currentDate());
    bool isTaskExpired(int taskId) const;
};

#endif // TASKREPOSITORY_H
