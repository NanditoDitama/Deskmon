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

    bool initialize(const QString &dbPathOverride = QString());
    void close();
    bool ensureOpen() const;
    QSqlDatabase database() const;
    QString databasePath() const;

    bool transaction();
    bool commit();
    bool rollback();

    void migrateLegacyDatabases();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool createTables();
    bool runMigrations();
    bool createViews();

    mutable QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // DATABASEMANAGER_H
