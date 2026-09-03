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

    int getAppProductivityType(const QString &appName, const QString &url, int userId) const;
    QVariantList getAvailableApps() const;
    QVariantList getProductivityApps(int userId) const;
    QVariantList getPendingApplicationRequests() const;

    bool addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType);
    bool storeProductivityAppsFromApi(const QJsonArray &appsArray, int userId);

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
};

#endif // PRODUCTIVITYAPPREPOSITORY_H
