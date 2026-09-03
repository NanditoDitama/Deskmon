#ifndef WORKTIMEREPOSITORY_H
#define WORKTIMEREPOSITORY_H

#include <QObject>
#include <QString>

class WorkTimeRepository : public QObject
{
    Q_OBJECT
public:
    explicit WorkTimeRepository(QObject *parent = nullptr);
    ~WorkTimeRepository() override = default;

    int getElapsedSeconds(int userId, const QString &date) const;
    bool saveElapsedSeconds(int userId, const QString &date, int seconds);
    bool hasRecordForDate(int userId, const QString &date) const;
};

#endif // WORKTIMEREPOSITORY_H
