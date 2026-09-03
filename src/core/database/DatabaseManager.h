#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool initialize();
    bool ensureOpen() const;
    QSqlDatabase database() const;
    QString databasePath() const;

    bool transaction();
    bool commit();
    bool rollback();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool createTables();
    bool createViews();
    void migrateLegacyDatabases();

    mutable QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // DATABASEMANAGER_H
