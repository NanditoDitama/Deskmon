#ifndef PRODUCTIVITYAPPREPOSITORY_H
#define PRODUCTIVITYAPPREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QHash>
#include <QVariantList>
#include <QJsonArray>
#include <QString>

class ProductivityAppRepository : public QObject
{
    Q_OBJECT
public:
    explicit ProductivityAppRepository(QObject *parent = nullptr);
    ~ProductivityAppRepository() override = default;

    bool initialize();
    bool ensureDatabaseOpen() const;
    QSqlDatabase database() const;

    QSqlQueryModel* productiveAppsModel() const { return m_productiveAppsModel; }
    QSqlQueryModel* nonProductiveAppsModel() const { return m_nonProductiveAppsModel; }

    void refreshProductivityModels(int userId);
    void updateProductivityCache(int userId);
    void copyCachesForUser(int userId, QHash<QString, int> &outAppTypes, QHash<QString, int> &outDomainTypes) const;

    int getAppProductivityType(const QString &appName, const QString &url, int userId) const;
    static int lookupProductivityType(const QString &appName, const QString &url,
                                      const QHash<QString, int> &appTypes,
                                      const QHash<QString, int> &domainTypes);
    QVariantList getAvailableApps() const;
    QVariantList getProductivityApps(int userId) const;
    QVariantList getPendingApplicationRequests() const;

    bool addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType);
    bool storeProductivityAppsFromApi(const QJsonArray &appsArray, int userId);
    static bool syncAppsToDatabase(QSqlDatabase &db, const QJsonArray &appsArray);

    int getIdleThreshold() const;
    void setIdleThreshold(int seconds);

signals:
    void productivityAppsChanged();
    void idleThresholdChanged();

private:
    QSqlQueryModel *m_productiveAppsModel;
    QSqlQueryModel *m_nonProductiveAppsModel;

    mutable QHash<QString, int> m_cachedAppTypes;
    mutable QHash<QString, int> m_cachedDomainTypes;
    mutable int m_cachedUserId = -1;
};

#endif // PRODUCTIVITYAPPREPOSITORY_H
