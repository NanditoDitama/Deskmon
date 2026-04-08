#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

void Deskmon::updateProductivityCache()
{
    m_dataManager->updateProductivityCache(m_currentUserId);
}

int Deskmon::getAppProductivityType(const QString &appName, const QString &url) const
{
    return m_dataManager->getAppProductivityType(appName, url);
}

int Deskmon::calculateTodayProductiveSeconds() const
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    return m_dataManager->calculateProductiveSeconds(m_currentUserId, today);
}

QVariantMap Deskmon::productivityStats() const
{
    return m_dataManager->getProductivityStats(m_currentUserId, m_startDateFilter, m_endDateFilter);
}

void Deskmon::sendProductiveTimeToAPI()
{
    m_apiManager->submitWorkTime(m_currentUserId, calculateTodayProductiveSeconds());
}

int Deskmon::getIdleThreshold() const
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot access productivity database, returning default idle threshold: 120 seconds";
        return 180;
    }

    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("SELECT threshold_seconds FROM idle_settings LIMIT 1");
    if (query.exec() && query.next()) {
        int threshold = query.value(0).toInt();
        if (threshold > 0) {
            qDebug() << "Retrieved idle threshold from database:" << threshold << "seconds";
            return threshold;
        }
        qWarning() << "Invalid threshold in database (null or <= 0), returning default: 180 seconds";
        return 180;
    }
    qWarning() << "No threshold found in database, returning default: 120 seconds";
    return 180;
}

QVariantList Deskmon::getProductivityApps() const {
    if (!ensureProductivityDatabaseOpen() || m_currentUserId == -1) {
        return QVariantList();
    }

    QVariantList apps;
    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare(R"(
        SELECT aplikasi, jenis, url, for_user
        FROM aplikasi
        WHERE for_user = '0'
           OR ',' || for_user || ',' LIKE :userPattern
    )");
    query.bindValue(":userPattern", "%," + QString::number(m_currentUserId) + ",%");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap app;
            app["appName"] = query.value(0).toString();
            app["type"] = query.value(1).toInt();
            app["url"] = query.value(2).toString();
            apps.append(app);
        }
    } else {
        qWarning() << "Failed to fetch productivity apps:" << query.lastError().text();
    }

    return apps;
}

void Deskmon::setIdleThreshold(int seconds)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set idle threshold: Database is not open";
        return;
    }

    if (seconds <= 0) {
        qWarning() << "Invalid idle threshold value:" << seconds;
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("INSERT OR REPLACE INTO idle_settings (id, threshold_seconds) VALUES (1, :threshold)");
    query.bindValue(":threshold", seconds);
    if (!query.exec()) {
        qWarning() << "Failed to set idle threshold:" << query.lastError().text();
    } else {
        qDebug() << "Idle threshold updated to:" << seconds << "seconds";
        emit idleThresholdChanged();
    }
}

QVariantList Deskmon::getAvailableApps() const
{
    QVariantList apps = {
        "Other", "Chrome", "Firefox", "Edge", "Safari", "Opera",
        "Visual Studio Code", "Qt Creator", "Android Studio", "IntelliJ IDEA", "PyCharm", "Xcode",
        "Microsoft Word", "Excel", "PowerPoint", "WPS Office", "LibreOffice", "OneNote", "Obsidian",
        "Photoshop", "GIMP", "Figma", "Canva", "Blender", "Premiere Pro", "After Effects",
        "Slack", "Microsoft Teams", "Zoom", "Google Meet", "Discord", "Skype",
        "Notion", "Trello", "Jira", "Asana", "ClickUp",
        "Postman", "FileZilla", "Docker", "GitHub Desktop", "GitKraken", "Terminal",
        "Deskmon", "Desklog-Client", "Explorer", "Outlook", "Thunderbird"
    };

    return apps;
}

QString extractDomain(const QString &urlString) {
    if (urlString.isEmpty()) {
        return QString();
    }

    QUrl url(urlString);
    QString host = url.host();

    // Menghilangkan subdomain "www." agar lebih konsisten
    if (host.startsWith("www.")) {
        host = host.mid(4);
    }

    return host;
}

void Deskmon::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Database tidak terbuka";
        return;
    }

    // 1. Simpan ke database lokal terlebih dahulu
    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("INSERT INTO aplikasi (aplikasi, window_title, url, jenis, productivity) "
                  "VALUES (:app, :window, :url, :type, :prod)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":url", url.isEmpty() ? QVariant() : url);
    query.bindValue(":type", 0); // 0 = menunggu approval
    query.bindValue(":prod", productivityType);

    if (query.exec()) {
        qDebug() << "Aplikasi ditambahkan. Menunggu approval admin.";

        // 2. Kirim data ke API
        sendProductivityAppToAPI(appName, windowTitle, url, productivityType);

        // Refresh model
        QString productiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        QString nonProductiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        m_productiveAppsModel->setQuery(productiveQuery, m_dataManager->productivityDb());
        m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_dataManager->productivityDb());
        refreshProductivityModels();
        emit productivityAppsChanged();
        updateProductivityCache();
    } else {
        qWarning() << "Gagal menambahkan aplikasi:" << query.lastError();
    }
}

void Deskmon::sendProductivityAppToAPI(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send productivity app: No authentication token available";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot send productivity app: No user logged in";
        return;
    }

    QString status;
    switch(productivityType) {
    case 1: status = "productive"; break;
    case 2: status = "non-productive"; break;
    default: status = "neutral"; break;
    }

    QJsonObject payload;
    payload["application_name"] = appName;
    payload["productivity_status"] = status;
    payload["user_id"] = m_currentUserId;

    if (!windowTitle.isEmpty()) {
        payload["process_name"] = windowTitle;
    }

    if (!url.isEmpty()) {
        payload["url"] = url;
    }

    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/store"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending productivity app to API:" << QJsonDocument(payload).toJson();

    QNetworkReply* reply = m_apiManager->networkManager()->post(request, QJsonDocument(payload).toJson());
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Productivity app successfully sent to API. Response:" << response;
        } else {
            qWarning() << "Failed to send productivity app to API:" << reply->errorString();
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
                emit authTokenError("Authentication token expired");
            }
        }
        reply->deleteLater();
    });
}

void Deskmon::fetchAndStoreProductivityApps()
{
    m_apiManager->fetchProductivityApps(m_currentUserId);
}

void Deskmon::handleProductivityAppsResponse(QNetworkReply *reply)
{
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch productivity apps:" << reply->errorString();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj["success"].toBool()) {
        qWarning() << "API returned error:" << jsonObj["message"].toString();
        return;
    }

    QJsonArray appsArray = jsonObj["data"].isArray() ? jsonObj["data"].toArray() : QJsonArray();

    // qDebug() << "==============================================";
    // qDebug() << "Received" << appsArray.size() << "productivity apps from server:";
    // qDebug() << "==============================================";

    // for (const QJsonValue &appValue : appsArray) {
    //     if (!appValue.isObject()) continue;

    //     QJsonObject appObj = appValue.toObject();
    //     QString appName = appObj["application_name"].toString();
    //     QString status = appObj["productivity_status"].toString().toLower();
    //     QString processName = appObj["process_name"].toString();
    //     QString url = appObj["url"].toString();
    //     int userId = appObj["user_id"].toInt();

    //     qDebug() << "App:" << appName
    //              << "| Process:" << (processName.isEmpty() ? "N/A" : processName)
    //              << "| URL:" << (url.isEmpty() ? "N/A" : url)
    //              << "| Status:" << status
    //              << "| User ID:" << userId;
    // }
    // qDebug() << "==============================================";

    if (!m_dataManager->productivityDb().transaction()) {
        qWarning() << "Failed to start transaction";
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());
    bool success = true;
    int insertCount = 0;
    int updateCount = 0;
    int unchangedCount = 0;

    // Struktur untuk menyimpan data server
    struct ServerApp {
        QString appName;
        QString processName;
        QString url;
        int jenis;
        int userId;
        QString forUsers;
    };

    // Parse semua data dari server
    QList<ServerApp> serverApps;
    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) continue;

        QJsonObject appObj = appValue.toObject();
        ServerApp serverApp;
        serverApp.appName = appObj["application_name"].toString();
        serverApp.processName = appObj["process_name"].toString();
        serverApp.url = appObj["url"].toString();
        serverApp.userId = appObj["user_id"].toInt();
        serverApp.forUsers = QString::number(serverApp.userId);

        QString status = appObj["productivity_status"].toString().toLower();
        if (status == "productive") serverApp.jenis = 1;
        else if (status == "non-productive") serverApp.jenis = 2;
        else serverApp.jenis = 0;

        serverApps.append(serverApp);
    }

    // Proses setiap aplikasi dari server
    for (const ServerApp &serverApp : serverApps) {
        // Cek apakah aplikasi dengan nama dan URL yang sama sudah ada di database
        QString checkQuery;

        // Query untuk mencari aplikasi berdasarkan aplikasi dan URL saja (tanpa for_user)
        if (serverApp.url.isEmpty() || serverApp.url == "N/A") {
            checkQuery = "SELECT id, jenis, window_title, for_user FROM aplikasi "
                         "WHERE aplikasi = :appName AND (url IS NULL OR url = '' OR url = 'N/A')";
        } else {
            checkQuery = "SELECT id, jenis, window_title, for_user FROM aplikasi "
                         "WHERE aplikasi = :appName AND url = :url";
        }

        query.prepare(checkQuery);
        query.bindValue(":appName", serverApp.appName);

        if (!serverApp.url.isEmpty() && serverApp.url != "N/A") {
            query.bindValue(":url", serverApp.url);
        }

        if (!query.exec()) {
            qWarning() << "Failed to check existing app:" << query.lastError();
            success = false;
            break;
        }

        bool foundExactMatch = false;
        bool foundZeroTypeRecord = false;
        int zeroTypeRecordId = -1;

        // Cek semua record yang cocok dengan aplikasi dan URL
        while (query.next()) {
            int existingId = query.value(0).toInt();
            int existingJenis = query.value(1).toInt();
            QString existingWindowTitle = query.value(2).toString();
            QString existingForUser = query.value(3).toString();

            // Cek apakah ada record dengan for_user yang sama (exact match)
            if (existingForUser == serverApp.forUsers) {
                foundExactMatch = true;

                // Cek apakah perlu diupdate
                bool needsUpdate = false;

                if (existingJenis != serverApp.jenis) {
                    needsUpdate = true;
                    qDebug() << "Status mismatch for" << serverApp.appName << ": DB=" << existingJenis << "Server=" << serverApp.jenis;
                }

                QString expectedWindowTitle = (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QString() : serverApp.processName;
                if (existingWindowTitle != expectedWindowTitle) {
                    needsUpdate = true;
                    qDebug() << "Process mismatch for" << serverApp.appName << ": DB=" << existingWindowTitle << "Server=" << expectedWindowTitle;
                }

                if (needsUpdate) {
                    // Update record yang sama persis
                    QString updateQuery = "UPDATE aplikasi SET jenis = :jenis, window_title = :windowTitle "
                                          "WHERE id = :id";
                    QSqlQuery updateQ(m_dataManager->productivityDb());
                    updateQ.prepare(updateQuery);
                    updateQ.bindValue(":jenis", serverApp.jenis);
                    updateQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                    updateQ.bindValue(":id", existingId);

                    if (!updateQ.exec()) {
                        qWarning() << "Failed to update exact match app:" << serverApp.appName << updateQ.lastError();
                        success = false;
                        break;
                    } else {
                        updateCount++;
                        qDebug() << "Updated exact match app:" << serverApp.appName << "for user" << serverApp.userId;
                    }
                } else {
                    unchangedCount++;
                }
                break; // Keluar dari loop karena sudah menemukan exact match
            }
            // Cek apakah ada record dengan jenis = 0 (default/unset)
            else if (existingJenis == 0) {
                foundZeroTypeRecord = true;
                zeroTypeRecordId = existingId;
            }
        }

        if (!foundExactMatch) {
            if (foundZeroTypeRecord) {
                // Update record dengan jenis = 0 karena tidak ada exact match
                QString expectedWindowTitle = (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QString() : serverApp.processName;

                QString updateQuery = "UPDATE aplikasi SET jenis = :jenis, window_title = :windowTitle, for_user = :forUsers "
                                      "WHERE id = :id";
                QSqlQuery updateQ(m_dataManager->productivityDb());
                updateQ.prepare(updateQuery);
                updateQ.bindValue(":jenis", serverApp.jenis);
                updateQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                updateQ.bindValue(":forUsers", serverApp.forUsers);
                updateQ.bindValue(":id", zeroTypeRecordId);

                if (!updateQ.exec()) {
                    qWarning() << "Failed to update zero-type record:" << serverApp.appName << updateQ.lastError();
                    success = false;
                    break;
                } else {
                    updateCount++;
                    qDebug() << "Updated zero-type record:" << serverApp.appName << "from jenis=0 to jenis=" << serverApp.jenis << "for user" << serverApp.userId;
                }
            } else {
                // Tidak ada record yang cocok sama sekali, insert baru
                QString insertQuery = "INSERT INTO aplikasi (aplikasi, window_title, url, jenis, for_user) "
                                      "VALUES (:app, :window, :url, :type, :forUsers)";
                QSqlQuery insertQ(m_dataManager->productivityDb());
                insertQ.prepare(insertQuery);
                insertQ.bindValue(":app", serverApp.appName);
                insertQ.bindValue(":window", (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QVariant() : serverApp.processName);
                insertQ.bindValue(":url", (serverApp.url.isEmpty() || serverApp.url == "N/A") ? QVariant() : serverApp.url);
                insertQ.bindValue(":type", serverApp.jenis);
                insertQ.bindValue(":forUsers", serverApp.forUsers);

                if (!insertQ.exec()) {
                    qWarning() << "Failed to insert new app:" << serverApp.appName << insertQ.lastError();
                    success = false;
                    break;
                } else {
                    insertCount++;
                    qDebug() << "Inserted new app:" << serverApp.appName << "for user" << serverApp.userId;
                }
            }
        }
    }

    if (success) {
        if (!m_dataManager->productivityDb().commit()) {
            qWarning() << "Failed to commit transaction";
            m_dataManager->productivityDb().rollback();
        } else {
            qDebug() << "==============================================";
            qDebug() << "Database sync completed successfully:";
            qDebug() << "- Inserted:" << insertCount << "new apps";
            qDebug() << "- Updated:" << updateCount << "existing apps";
            qDebug() << "- Unchanged:" << unchangedCount << "apps";
            qDebug() << "- Total processed:" << serverApps.size() << "apps";
            qDebug() << "==============================================";

            refreshProductivityModels();
            emit productivityAppsChanged();
        }
    } else {
        qWarning() << "Failed to sync productivity apps with database";
        m_dataManager->productivityDb().rollback();
    }
    updateProductivityCache();
}

void Deskmon::refreshProductivityModels()
{
    if (!ensureProductivityDatabaseOpen()) return;

    // Gabungkan aturan global (for_user=0) dan spesifik user
    QString productiveQuery = QString(
                                  "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
                                  "FROM aplikasi a "
                                  "WHERE a.jenis = 1 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
                                  "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
                                  "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
                                  "GROUP BY a.aplikasi, COALESCE(a.url, '') "  // Hindari duplikat
                                  "ORDER BY a.for_user = '0' DESC"  // Prioritaskan aturan spesifik user
                                  ).arg(m_currentUserId);

    QString nonProductiveQuery = QString(
                                     "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
                                     "FROM aplikasi a "
                                     "WHERE a.jenis = 2 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
                                     "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
                                     "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
                                     "GROUP BY a.aplikasi, COALESCE(a.url, '') "
                                     "ORDER BY a.for_user = '0' DESC"
                                     ).arg(m_currentUserId);

    m_productiveAppsModel->setQuery(productiveQuery, m_dataManager->productivityDb());
    m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_dataManager->productivityDb());

    // Debug output
    qDebug() << "Productive apps count:" << m_productiveAppsModel->rowCount();
    qDebug() << "Non-productive apps count:" << m_nonProductiveAppsModel->rowCount();
}

QVariantList Deskmon::getPendingApplicationRequests() {
    QVariantList requests;

    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot get pending requests: Database is not open";
        return requests;
    }

    QSqlQuery query(m_dataManager->productivityDb());
    // Perbarui query untuk menyertakan kolom url dan productivity
    query.prepare("SELECT id, aplikasi, window_title, url, productivity, for_user FROM aplikasi WHERE jenis = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap request;
            request["id"] = query.value(0);
            request["app_name"] = query.value(1); // aplikasi
            request["window_title"] = query.value(2);
            request["url"] = query.value(3); // URL/domain
            request["productivity"] = query.value(4); // Nilai produktivitas

            // Konversi jenis produktivitas ke string yang lebih deskriptif
            int productivity = query.value(4).toInt();
            switch(productivity) {
            case 1: request["productivity_text"] = "Productive"; break;
            case 2: request["productivity_text"] = "Non-Productive"; break;
            default: request["productivity_text"] = "Neutral"; break;
            }

            // Format for_user untuk tampilan yang lebih baik
            QString forUsers = query.value(5).toString();
            if (forUsers == "0") {
                request["for_users"] = "All Users";
            } else {
                QStringList userIds = forUsers.split(',', Qt::SkipEmptyParts);
                request["for_users"] = userIds.join(", ");
            }

            requests.append(request);
        }
    } else {
        qWarning() << "Error getting pending requests:" << query.lastError().text();
    }

    return requests;
}

