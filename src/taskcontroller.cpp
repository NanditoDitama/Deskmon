#include "taskcontroller.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

TaskController::TaskController(QObject *parent) : QObject(parent)
{
}

void TaskController::setActiveTask(int taskId, int userId)
{
    if (userId == -1) return;
    
    QSqlDatabase db = QSqlDatabase::database("productivity_db");
    if (!db.isOpen()) return;

    if (taskId != -1) {
        // Verification logic
        QSqlQuery query(db);
        query.prepare("SELECT status FROM task WHERE id = :id AND user_id = :user_id");
        query.bindValue(":id", taskId);
        query.bindValue(":user_id", userId);
        
        if (query.exec() && query.next()) {
            QString status = query.value(0).toString();
            if (status != "on-progress" && status != "started") {
                qWarning() << "Cannot set active task: Task status is" << status;
                return;
            }
        } else {
            qWarning() << "Task not found or does not belong to user";
            return;
        }

        // Update DB
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE task SET active = 0 WHERE user_id = :user_id");
        updateQuery.bindValue(":user_id", userId);
        updateQuery.exec();

        updateQuery.prepare("UPDATE task SET active = 1 WHERE id = :id");
        updateQuery.bindValue(":id", taskId);
        updateQuery.exec();

        m_activeTaskId = taskId;
        m_startTime = QDateTime::currentSecsSinceEpoch();
        m_isPaused = false;
        
        // Load offset
        QSqlQuery offsetQuery(db);
        offsetQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
        offsetQuery.bindValue(":id", taskId);
        if (offsetQuery.exec() && offsetQuery.next()) {
            m_timeOffset = offsetQuery.value(0).toLongLong();
        }
    } else {
        // Deactivate
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE task SET active = 0 WHERE user_id = :user_id");
        updateQuery.bindValue(":user_id", userId);
        updateQuery.exec();
        
        m_activeTaskId = -1;
        m_startTime = 0;
        m_timeOffset = 0;
    }

    emit activeTaskChanged(m_activeTaskId);
    emit taskPausedChanged(m_isPaused);
    emit taskListChanged();
}

void TaskController::finishTask(int taskId)
{
    QSqlDatabase db = QSqlDatabase::database("productivity_db");
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("UPDATE task SET status = 'finished', active = 0 WHERE id = :id");
    query.bindValue(":id", taskId);
    
    if (query.exec()) {
        if (m_activeTaskId == taskId) {
            m_activeTaskId = -1;
            m_startTime = 0;
            emit activeTaskChanged(-1);
        }
        emit taskListChanged();
    }
}

void TaskController::toggleTaskPause(int taskId)
{
    if (taskId == -1) return;

    QSqlDatabase db = QSqlDatabase::database("productivity_db");
    if (!db.isOpen()) return;

    m_isPaused = !m_isPaused;
    
    QSqlQuery query(db);
    query.prepare("UPDATE task SET paused = :paused WHERE id = :id");
    query.bindValue(":paused", m_isPaused ? 1 : 0);
    query.bindValue(":id", taskId);
    
    if (query.exec()) {
        if (m_isPaused) {
            // Calculate time used so far and add to offset
            qint64 now = QDateTime::currentSecsSinceEpoch();
            m_timeOffset += (now - m_startTime);
            m_startTime = 0;
        } else {
            m_startTime = QDateTime::currentSecsSinceEpoch();
        }
        emit taskPausedChanged(m_isPaused);
        emit taskListChanged();
    }
}

void TaskController::updateTaskStatus(int taskId)
{
    // Logic to update status based on time_usage vs max_time
    // ... implement if needed ...
}

void TaskController::syncActiveTask(int userId)
{
    if (userId == -1) return;
    
    QSqlDatabase db = QSqlDatabase::database("productivity_db");
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("SELECT id, paused, time_usage FROM task WHERE user_id = :user_id AND active = 1 LIMIT 1");
    query.bindValue(":user_id", userId);
    
    if (query.exec() && query.next()) {
        m_activeTaskId = query.value(0).toInt();
        m_isPaused = query.value(1).toBool();
        m_timeOffset = query.value(2).toLongLong();
        if (!m_isPaused) {
            m_startTime = QDateTime::currentSecsSinceEpoch();
        } else {
            m_startTime = 0;
        }
    } else {
        m_activeTaskId = -1;
        m_isPaused = false;
        m_timeOffset = 0;
        m_startTime = 0;
    }
    
    emit activeTaskChanged(m_activeTaskId);
    emit taskPausedChanged(m_isPaused);
}

void TaskController::refreshTasks(int userId)
{
    if (userId == -1) return;

    QSqlDatabase db = QSqlDatabase::database("productivity_db");
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("SELECT id FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);

    if (query.exec()) {
        while (query.next()) {
            updateTaskStatus(query.value(0).toInt());
        }
    }
}
