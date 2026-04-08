#include "deskmon.h"
#include <QSqlQuery>
#include <cmath>
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
#include <QBuffer>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QFile>

void Deskmon::checkTaskStatusBeforeStart()
{
    m_taskController->syncActiveTask(m_currentUserId);
}

void Deskmon::fetchAndStoreTasks()
{
    // 1. Pastikan database produktivitas terbuka sebelum memulai proses
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot fetch and store tasks: Productivity database is not open";
        return;
    }

    // 2. Pastikan ada pengguna yang login dan memiliki ID yang valid
    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch tasks: No user logged in";
        return;
    }

    // 3. Periksa dan ambil token autentikasi jika belum ada
    if (m_authToken.isEmpty()) {
        QSqlQuery tokenQuery(m_dataManager->activityDb());
        tokenQuery.prepare("SELECT token FROM users WHERE id = ?");
        tokenQuery.addBindValue(m_currentUserId);
        if (tokenQuery.exec() && tokenQuery.next()) {
            m_authToken = tokenQuery.value(0).toString();
            qDebug() << "Token retrieved from database:" << m_authToken;
        }

        // Jika token masih kosong, hentikan proses dan beri sinyal kesalahan
        if (m_authToken.isEmpty()) {
            qWarning() << "Skipping task fetch: No auth token available";
            emit authTokenError("No authentication token");
            return;
        }
    }

    // 4. Siapkan permintaan HTTP GET ke endpoint server dengan user_id
    QString apiUrl = QString("https://deskmon.pranala-dt.co.id/api/task-by-user/%1").arg(m_currentUserId);
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    // 5. Kirim permintaan ke server dan hubungkan respons ke slot penanganan
    QNetworkReply *reply = m_apiManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleTaskFetchReply(reply);
    });
}

void Deskmon::handleTaskFetchReply(QNetworkReply *reply)
{
    // 5. Periksa kode status HTTP dari respons
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 6. Tangani kesalahan jaringan jika ada
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch tasks: Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    // 7. Baca data respons dari server
    QByteArray responseData = reply->readAll();
    // Debug respons data dihilangkan karena ukuran JSON yang besar menyebabkan lag/Not Responding pada GUI

    // 8. Tangani kasus autentikasi gagal (token tidak valid atau kedaluwarsa)
    if (statusCode == 401) {
        qWarning() << "Unauthorized access. Token may be invalid or expired.";
        showAuthTokenErrorMessage();
        reply->deleteLater();
        return;
    }

    // 9. Parse data JSON dari respons
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response from tasks endpoint. Response:" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 10. Periksa apakah permintaan berhasil berdasarkan field 'success'
    if (!jsonObj["success"].toBool()) {
        qWarning() << "Tasks API returned failure:" << jsonObj["message"].toString();
        reply->deleteLater();
        return;
    }

    // 11. Ambil array tugas dari respons
    QJsonArray tasksArray = jsonObj["data"].toArray();

    QSet<int> serverTaskIds;
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();
        if (taskObj.contains("id")) {
            // Hanya masukkan task yang BELUM COMPLETE ke daftar ID server
            // agar task yang sudah complete di hapus dari database lokal (stale)
            if (taskObj["status"].toString() != "completed") {
                serverTaskIds.insert(taskObj["id"].toInt());
            }
        }
    }

    // 12. Mulai transaksi database untuk memastikan integritas data
    QSqlQuery query(m_dataManager->productivityDb());
    m_dataManager->productivityDb().transaction();

    // 13. Ambil tugas yang sudah ada di database lokal untuk perbandingan
    QMap<int, QPair<int, int>> existingTasks; // taskId -> (max_time, time_usage)
    query.prepare("SELECT id, max_time, time_usage FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);
    if(query.exec()) {
        while (query.next()) {
            int taskId = query.value(0).toInt();
            int maxTime = query.value(1).toInt();
            int timeUsage = query.value(2).toInt();
            existingTasks.insert(taskId, QPair<int, int>(maxTime, timeUsage));
        }
    }

    QList<int> taskIdsToDelete;
    for (int localTaskId : existingTasks.keys()) {
        if (!serverTaskIds.contains(localTaskId)) {
            taskIdsToDelete.append(localTaskId);
        }
    }

    if (!taskIdsToDelete.isEmpty()) {
        qDebug() << "Tasks to be deleted from local DB (not on server):" << taskIdsToDelete;
        QSqlQuery deleteQuery(m_dataManager->productivityDb());
        for (int taskIdToDelete : taskIdsToDelete) {
            if (taskIdToDelete == m_activeTaskId) {
                qDebug() << "Active task" << m_activeTaskId << "is being deleted. Resetting active task state.";
                m_activeTaskId = -1;
                m_isTaskPaused = false;
                m_taskTimeOffset = 0;
                m_taskStartTime = 0;
                emit activeTaskChanged();
                emit taskPausedChanged();
            }

            deleteQuery.prepare("DELETE FROM task WHERE id = :id AND user_id = :user_id");
            deleteQuery.bindValue(":id", taskIdToDelete);
            deleteQuery.bindValue(":user_id", m_currentUserId);
            if (!deleteQuery.exec()) {
                qWarning() << "Failed to delete stale task ID" << taskIdToDelete << ":" << deleteQuery.lastError().text();
            }
        }
    }

    // 14. Proses setiap tugas dari respons server (logika insert/update)
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();

        QString status = taskObj["status"].toString();
        if (status == "completed") {
            continue;
        }

        if (!taskObj.contains("id") || !taskObj.contains("title") ||
            !taskObj.contains("description") || !taskObj.contains("user_id")) {
            qWarning() << "Skipping task with missing required fields";
            continue;
        }

        int taskId = taskObj["id"].toInt();
        QString projectName = taskObj["title"].toString();
        QString taskDesc = taskObj["description"].toString();
        int userId = taskObj["user_id"].toInt();

        if (userId != m_currentUserId) {
            qDebug() << "Skipping task ID" << taskId << "for user ID" << userId << "(not current user)";
            continue;
        }

        QJsonValue durationValue = taskObj["duration"];
        QJsonValue totalDurationValue = taskObj["total_duration"];

        int serverMaxTime = 0;
        if (!durationValue.isNull()) {
            serverMaxTime = qRound(durationValue.toVariant().toDouble() * 3600); // jam -> detik
        }

        int serverTimeUsage = 0;
        if (!totalDurationValue.isNull()) {
            serverTimeUsage = qRound(totalDurationValue.toVariant().toDouble() * 3600); // jam -> detik
        }

        bool taskExists = existingTasks.contains(taskId);
        QString createdAt = taskObj["created_at"].toString();

        if (taskExists) {
            QPair<int, int> currentValues = existingTasks[taskId];
            int currentMaxTime = currentValues.first;
            int currentTimeUsage = currentValues.second;

            // <-- DIUBAH: Logika untuk `max_time` dan `time_usage` -->
            // Selalu prioritaskan nilai dari server jika ada. Jika tidak, pertahankan nilai lokal.
            int finalMaxTime = !durationValue.isNull() ? serverMaxTime : currentMaxTime;
            int finalTimeUsage = !totalDurationValue.isNull() ? serverTimeUsage : currentTimeUsage;

            query.prepare("UPDATE task SET project_name = :projectName, task = :taskDesc, "
                          "max_time = :maxTime, time_usage = :timeUsage, status = :status, created_at = :createdAt WHERE id = :id");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);
            query.bindValue(":timeUsage", finalTimeUsage);
            query.bindValue(":status", status); // <--- Update status
            query.bindValue(":createdAt", createdAt);

        } else {
            // <-- DIUBAH: Logika untuk task baru -->
            // Gunakan nilai dari server, atau 0 jika server tidak menyediakannya. TIDAK ADA DEFAULT.
            int finalMaxTime = serverMaxTime;
            int finalTimeUsage = serverTimeUsage;

            query.prepare("INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id, created_at) "
                          "VALUES (:id, :projectName, :taskDesc, :maxTime, :timeUsage, 0, :status, 0, :userId, :createdAt)");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);
            query.bindValue(":timeUsage", finalTimeUsage);
            query.bindValue(":status", status); // <--- Set initial status from server
            query.bindValue(":userId", userId);
            query.bindValue(":createdAt", createdAt);
        }

        if (!query.exec()) {
            qWarning() << "Failed to save task ID" << taskId << ":" << query.lastError().text();
            continue;
        }
    }

    if (!m_dataManager->productivityDb().commit()) {
        qWarning() << "Failed to commit transaction:" << m_dataManager->productivityDb().lastError().text();
        m_dataManager->productivityDb().rollback();
    } else {
        qDebug() << "Successfully processed tasks from server";
        emit taskListChanged();
    }

    reply->deleteLater();
}



int Deskmon::getPendingStartedTaskCount()
{
    if (!ensureProductivityDatabaseOpen() || m_currentUserId == -1) return 0;

    QSqlQuery query(m_dataManager->productivityDb());
    // Hitung task yang status Pending TAPI time_usage > 0 (sudah pernah jalan)
    query.prepare("SELECT COUNT(*) FROM task WHERE user_id = :uid AND status = 'Pending' AND time_usage > 0");
    query.bindValue(":uid", m_currentUserId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QVariantList Deskmon::taskList() const
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot fetch task list: Database is not open";
        return QVariantList();
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch task list: No user logged in";
        return QVariantList();
    }
    QVariantList tasks;
    QSqlQuery query(m_dataManager->productivityDb());

    // --- PERUBAHAN 1: Tambahkan 'created_at' ke dalam SELECT ---
    query.prepare("SELECT id, project_name, task, max_time, time_usage, active, status, created_at FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);

    if (!query.exec()) {
        qWarning() << "Failed to fetch tasks:" << query.lastError().text();
        return tasks;
    }

    QDate today = QDate::currentDate(); // Ambil tanggal hari ini sekali saja di luar loop

    while (query.next()) {
        QVariantMap task;
        int taskId = query.value(0).toInt();
        task["id"] = taskId;
        task["project_name"] = query.value(1).toString();
        task["task"] = query.value(2).toString();
        int maxTime = query.value(3).toInt();
        task["max_time"] = maxTime;
        task["time_usage"] = query.value(4).toInt();
        task["active"] = query.value(5).toBool();
        QString status = query.value(6).toString();
        QString createdAtStr = query.value(7).toString(); // Ambil created_at

        // --- PERUBAHAN 2: Logika Cek Expired langsung di sini ---
        bool isExpired = false;
        QDateTime createdDateTime = QDateTime::fromString(createdAtStr, Qt::ISODate);

        if (createdDateTime.isValid()) {
            QDate createdDate = createdDateTime.date();

            // Cek apakah dibuat di masa lalu
            if (createdDate.year() < today.year() || (createdDate.year() == today.year() && createdDate.month() < today.month())) {
                // Hitung estimasi selesai
                double workHoursPerDay = 8.0 * 3600.0;
                // Gunakan std::ceil dari <cmath>, pastikan sudah di-include di atas file
                int daysDuration = std::ceil((double)maxTime / workHoursPerDay);
                QDate estimatedFinishDate = createdDate.addDays(daysDuration);

                // Jika estimasi selesai juga masih di masa lalu -> EXPIRED
                if (estimatedFinishDate.year() < today.year() || (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() < today.month())) {
                    isExpired = true;
                }
            }
        }
        // Masukkan status expired ke dalam map data
        task["isExpired"] = isExpired;
        // ---------------------------------------------------------


        // Handle status logic (Kode lama tetap sama)
        if (status.toLower() == "review") {
            task["status"] = "Review";
        } else if (task["active"].toBool()) {
            status = m_isTaskPaused ? "Paused" : "Is Running";
            task["status"] = status;
        } else {
            task["status"] = status;
        }

        // --- TAMBAHAN: Map Icon Berdasarkan Status ---
        QString iconSource = "qrc:/assets/icon.png"; // Default
        if (isExpired) {
            iconSource = "qrc:/assets/icons/danger.svg";
        } else if (status.toLower() == "review") {
            iconSource = "qrc:/assets/icons/review.svg";
        } else if (status.toLower() == "need review") {
            iconSource = "qrc:/assets/icons/search.svg";
        } else if (status.toLower() == "need revise") {
            iconSource = "qrc:/assets/icons/edit.svg";
        } else if (task["active"].toBool()) {
            iconSource = m_isTaskPaused ? "qrc:/assets/pause_icon_app.png" : "qrc:/assets/play_icon_app.png";
        }
        task["status_icon"] = iconSource;
        // ------------------------------------------

        tasks.append(task);
    }
    return tasks;
}

bool Deskmon::isTaskExpired(int taskId) const
{
    if (!ensureProductivityDatabaseOpen()) return false;

    QSqlQuery query(m_dataManager->productivityDb());
    // Kita perlu created_at DAN max_time
    query.prepare("SELECT created_at, max_time FROM task WHERE id = :id");
    query.bindValue(":id", taskId);

    if (query.exec() && query.next()) {
        QString dateStr = query.value(0).toString();
        int maxTimeSeconds = query.value(1).toInt();

        // 1. Validasi Tanggal Pembuatan
        QDateTime createdDateTime = QDateTime::fromString(dateStr, Qt::ISODate);
        if (!createdDateTime.isValid()) return false; // Jika error, anggap task baru (aman)

        QDate createdDate = createdDateTime.date();
        QDate today = QDate::currentDate();

        // Jika dibuat di tahun/bulan yang sama (atau masa depan), task valid
        if (createdDate.year() > today.year()) return false;
        if (createdDate.year() == today.year() && createdDate.month() >= today.month()) return false;

        // --- LOGIKA LINTAS BULAN ---
        // Jika sampai sini, berarti task dibuat di bulan/tahun lalu.
        // Kita cek apakah durasinya panjang menembus ke bulan ini.

        // Asumsi: 1 hari kerja = 8 jam (28800 detik)
        // Kita gunakan ceil agar 9 jam dihitung 2 hari, bukan 1 hari koma sekian.
        double workHoursPerDay = 8.0 * 3600.0;
        int daysDuration = std::ceil((double)maxTimeSeconds / workHoursPerDay);

        // Tambahkan durasi ke tanggal pembuatan
        // Contoh: Dibuat 31 Jan, MaxTime 16 jam (2 hari).
        // 31 Jan + 2 hari = 2 Feb.
        QDate estimatedFinishDate = createdDate.addDays(daysDuration);

        // Cek apakah tanggal estimasi selesai masuk ke bulan ini (atau lebih)
        if (estimatedFinishDate.year() > today.year()) return false; // Valid (lanjut tahun depan)
        if (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() >= today.month()) {
            return false; // Valid (Menyeberang ke bulan ini)
        }

        // Jika tanggal estimasi selesai pun masih di masa lalu -> EXPIRED
        return true;
    }

    return false;
}


void Deskmon::setActiveTask(int taskId)
{
    if (m_activeTaskId != -1 && m_activeTaskId != taskId && !m_isTaskPaused) {
        qDebug() << "Active task " << m_activeTaskId << " is running. Requesting details before switching.";
        // Jika ya, tampilkan dialog dan hentikan eksekusi fungsi ini untuk sementara.
        emit requestTaskDetails(m_activeTaskId, "switch", taskId);
    }

    if (taskId == m_activeTaskId && !m_isTaskPaused) {
        qDebug() << "Task" << taskId << "is already active and running. No action needed.";
        return;
    }

    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set active task: Database is not open";
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());

    // Simpan time_usage untuk tugas aktif sebelumnya (jika ada) dan kirim status stop
    if (m_activeTaskId != -1) {
        // Cek status tugas sebelumnya
        query.prepare("SELECT status, task FROM task WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec() || !query.next()) {
            qWarning() << "Failed to fetch status for previous task:" << query.lastError().text();
            return;
        }

        QString prevStatus = query.value(0).toString().toLower();
        QString taskName = query.value(1).toString();

        QStringList restrictedStatuses = {"Review", "Need Review", "Need Revise", "completed"};
        QString newStatus = restrictedStatuses.contains(prevStatus) ? prevStatus : "Pending";

        // Hitung time_usage
        qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
        qint64 timeUsed = m_taskTimeOffset + (m_isTaskPaused ? 0 : (currentEpoch - m_taskStartTime));

        // Update tugas sebelumnya
        query.prepare("UPDATE task SET active = 0, status = :status, time_usage = :timeUsage, paused = 0 WHERE id = :id");
        query.bindValue(":status", newStatus);
        query.bindValue(":timeUsage", timeUsed);
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec()) {
            qWarning() << "Failed to update previous task:" << query.lastError().text();
        }

        // // Kirim status stop untuk tugas sebelumnya
        // if (!m_isTaskPaused) {
        //     QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        //     QString startTime = QDateTime::fromSecsSinceEpoch(m_taskStartTime).toString(Qt::ISODateWithMs);
        //     sendPausePlayDataToAPI(m_activeTaskId, startTime, currentTime, "pause");
        // }

        if (!m_isTaskPaused) {
            toggleTaskPause();
        }

    }

    // Jika taskId valid, set tugas baru
    if (taskId != -1) {
        // Ambil time_usage dari tugas baru
        query.prepare("SELECT time_usage FROM task WHERE id = :id");
        query.bindValue(":id", taskId);
        if (!query.exec() || !query.next()) {
            qWarning() << "Failed to fetch time_usage for task:" << query.lastError().text();
            return;
        }
        m_taskTimeOffset = query.value(0).toInt();
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();

        // Aktifkan tugas baru dengan status paused
        query.prepare("UPDATE task SET active = 1, status = 'Paused', paused = 1 WHERE id = :id");
        query.bindValue(":id", taskId);
        if (!query.exec()) {
            qWarning() << "Failed to activate task:" << query.lastError().text();
            return;
        }

        // Kirim status pause untuk tugas baru
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        //sendPausePlayDataToAPI(taskId, currentTime, currentTime, "pause");

        // Set max time untuk tugas
        setMaxTimeForTask(taskId);

        // Gunakan QTimer untuk memulai task setelah delay 3 detik
        QTimer::singleShot(2000, this, [this, taskId, currentTime]() {
            if (m_activeTaskId == taskId) { // Pastikan task masih aktif
                // Resume task setelah delay
                QSqlQuery resumeQuery(m_dataManager->productivityDb());
                resumeQuery.prepare("UPDATE task SET paused = 0, status = 'on-progress' WHERE id = :id");
                resumeQuery.bindValue(":id", taskId);
                if (!resumeQuery.exec()) {
                    qWarning() << "Failed to resume task:" << resumeQuery.lastError().text();
                    return;
                }

                // Update local state
                m_isTaskPaused = false;
                m_isTrackingActive = true;
                m_taskStartTime = QDateTime::currentSecsSinceEpoch();
                m_pauseStartTime = 0;

                // Mulai periode play baru
                QString newTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
                QSqlQuery logQuery(m_dataManager->productivityDb());
                logQuery.prepare(
                    "UPDATE log_paused "
                    "SET end_reality = ? "
                    "WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL");
                logQuery.addBindValue(newTime);
                logQuery.addBindValue(taskId);
                logQuery.exec();

                logQuery.prepare(
                    "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                    "VALUES (?, ?, NULL, 'play')");
                logQuery.addBindValue(taskId);
                logQuery.addBindValue(newTime);
                logQuery.exec();
                sendPing(m_activeTaskId);


                // Emit signals
                emit taskPausedChanged();
                emit trackingActiveChanged();
                emit taskListChanged();

                qDebug() << "Task automatically resumed after delay";
            }
        });
    }

    // Sinkronkan tugas
    //fetchAndStoreTasks();
    //syncActiveTask();

    // Update state
    m_activeTaskId = taskId;
    m_isTaskPaused = true; // Awalnya di-pause, akan di-resume setelah delay
    m_isTrackingActive = false;
    m_pauseStartTime = QDateTime::currentSecsSinceEpoch();

    // Emit sinyal
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
    emit taskListChanged();
}

void Deskmon::sendPing(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot send ping: Database is not open";
        return;
    }

    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send ping: No authentication token available";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot send ping: No user logged in";
        return;
    }

    QJsonObject payload;
    if (taskId != -1) {
        payload["task_id"] = QString::number(taskId);
    } else {
        payload["user_id"] = m_currentUserId;
    }

    QNetworkRequest request;
    request.setUrl(QUrl("https://deskmon.pranala-dt.co.id/api/ping"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending ping with payload:" << QJsonDocument(payload).toJson();

    QNetworkReply* reply = m_apiManager->networkManager()->post(request, QJsonDocument(payload).toJson());
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    // --- LOGIKA BARU DIMULAI DI SINI ---
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        bool isAuthError = false;
        bool isConnectionError = false;
        QString connectionErrorMessage;

        if (reply->error() != QNetworkReply::NoError) {
            // PRIORITAS UTAMA: Cek error autentikasi
            if (reply->errorString().contains("Host requires authentication", Qt::CaseInsensitive)) {
                isAuthError = true;
            } else {
                // Ini adalah error koneksi jaringan
                isConnectionError = true;
                connectionErrorMessage = "Koneksi gagal: " + reply->errorString();
            }
        } else {
            bool success = jsonObj.value("success").toBool();
            if (!success) {
                QString errMsg = jsonObj.value("message").toString("Terjadi kesalahan.");
                // PRIORITAS UTAMA: Cek error autentikasi dari server
                if (errMsg.contains("Host requires authentication", Qt::CaseInsensitive)) {
                    isAuthError = true;
                } else {
                    // Ini adalah error dari server (bukan auth), dianggap error koneksi
                    isConnectionError = true;
                    connectionErrorMessage = errMsg;
                }
            }
            // else: Ini adalah kasus SUKSES
        }

        // --- Logika Penanganan Error dan Retry ---

        if (isAuthError) {
            // Error Autentikasi: Berhenti mencoba dan tampilkan error login
            qWarning() << "Ping failed: Authentication error.";
            m_pingTimer.setInterval(30000); // Kembalikan ke 30 detik
            m_pingRetryCount = 0;
            showAuthTokenErrorMessage(); // Tampilkan jendela error login

        } else if (isConnectionError) {
            // Error Koneksi: Mulai atau lanjutkan logika retry 5 detik

            // Cek apakah ini kegagalan PERTAMA (saat timer masih 30 detik)
            if (m_pingTimer.interval() == 30000) {
                m_pingRetryCount = 0; // Mulai hitungan
                m_pingTimer.setInterval(5000); // Ubah ke 5 detik
                emit showPingErrorDialog(connectionErrorMessage); // Tampilkan error
                qDebug() << "Ping failed. Starting 5-second retries." << connectionErrorMessage;
            } else {
                // Ini adalah kegagalan BERIKUTNYA (saat timer sudah 5 detik)
                m_pingRetryCount++;
                qDebug() << "Ping retry" << m_pingRetryCount << "failed.";

                if (m_pingRetryCount >= 22) {
                    // Gagal 22 kali: Menyerah, kembalikan timer ke 30 detik
                    qDebug() << "Ping retries failed 22 times. Giving up, returning to 30s interval.";
                    m_pingTimer.setInterval(30000); // Kembalikan ke 30 detik
                    m_pingRetryCount = 0; // Reset hitungan
                    emit hidePingErrorDialog(); // Sembunyikan dialog error (sesuai permintaan "tampilkan hidePingErrorDialog")
                }
                // Jika kurang dari 22, tidak melakukan apa-apa (timer 5 detik akan berjalan lagi)
            }
        } else {
            // KONEKSI SUKSES

            // Cek apakah sukses ini terjadi saat mode retry 5 detik
            if (m_pingTimer.interval() != 30000) {
                qDebug() << "Ping retry successful. Returning to 30s interval.";
                m_pingTimer.setInterval(30000); // Kembalikan ke 30 detik
                m_pingRetryCount = 0; // Reset hitungan
            }
            emit hidePingErrorDialog(); // Sembunyikan dialog error
        }

        reply->deleteLater();
    });
    // --- LOGIKA BARU BERAKHIR DI SINI ---
}

void Deskmon::startPingTimer()
{
    sendPing(m_activeTaskId);
    m_pingTimer.start();
    qDebug() << "Started ping timer for client_id:";
}

void Deskmon::updateTaskStatus(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot update task status: Database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot update task status: No user logged in";
        return;
    }

    // Validasi taskId
    if (taskId <= 0) {
        qWarning() << "Invalid taskId:" << taskId;
        return;
    }

    // Validasi task ID dan kepemilikan user
    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("SELECT user_id FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Task not found or invalid task ID:" << taskId;
        return;
    }
    if (query.value(0).toInt() != m_currentUserId) {
        qWarning() << "Task ID" << taskId << "does not belong to current user:" << m_currentUserId;
        return;
    }

    // Buat URL dan request
    QString url = QString("https://deskmon.pranala-dt.co.id/api/get-current-task-status/%1").arg(taskId);
    if (url.isEmpty()) {
        qWarning() << "URL is empty for taskId:" << taskId;
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    } else {
        qWarning() << "No authentication token available for taskId:" << taskId;
        return;
    }

    // Kirim request
    QNetworkReply *reply = m_apiManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId]() {
        handleTaskStatusReply(reply, taskId);
    });
}

void Deskmon::handleTaskStatusReply(QNetworkReply *reply, int taskId)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to get task status for taskId" << taskId << ": Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response for taskId" << taskId << ":" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj["success"].toBool()) {
        qWarning() << "Task status API failed for taskId" << taskId << ":" << jsonObj["message"].toString();
        reply->deleteLater();
        return;
    }

    QString apiStatus = jsonObj["data"].toString();
    if (apiStatus.isEmpty()) {
        qWarning() << "No status data in response for taskId" << taskId;
        reply->deleteLater();
        return;
    }

    // Dapatkan nama task untuk notifikasi
    QString taskName = getTaskName(taskId);


    // Petakan status API ke status database
    QString dbStatus;
    bool isReviewStatus = false;
    bool isNeedReview = false;
    bool isNeedRevise = false;

    if (apiStatus == "created" || apiStatus == "pending") {
        dbStatus = "Pending";
    }
    else if (apiStatus == "on-progress") {
        // aktifkan task ini langsung
        setActiveTask(taskId);
        m_isTaskPaused = false;
        m_isTrackingActive = true;
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();
        dbStatus = "On Progress";

        QSqlQuery timeQuery(m_dataManager->productivityDb());
        timeQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
        timeQuery.bindValue(":id", taskId);
        if (timeQuery.exec() && timeQuery.next()) {
            m_taskTimeOffset = timeQuery.value(0).toInt();
        }

        qDebug() << "Task with status 'on-progress' set as active from handleTaskStatusReply. Task ID:" << taskId;
    }
    else if (apiStatus == "on-review") {
        dbStatus = "Review";
        isReviewStatus = true;
        QString message = QString("Task '%1' is under system review").arg(taskName);
        emit taskReviewNotification(message);
    }
    else if (apiStatus == "need-review") {
        dbStatus = "Need Review";
        isNeedReview = true;
    }
    else if (apiStatus == "need-revise") {
        dbStatus = "Need Revise";
        isNeedRevise = true;
    }
    else if (apiStatus == "completed") {
        dbStatus = "completed";
        finishTask(taskId);
        reply->deleteLater();
        return;
    }
    else {
        qWarning() << "Unknown status for taskId" << taskId << ":" << apiStatus;
        reply->deleteLater();
        return;
    }

    // Hanya pause task jika status on-review (bukan Need Review)
    if (isReviewStatus && m_activeTaskId == taskId) {
        QSqlQuery pauseQuery(m_dataManager->productivityDb());
        pauseQuery.prepare("UPDATE task SET active = 0, paused = 1 WHERE id = :id");
        pauseQuery.bindValue(":id", taskId);
        if (pauseQuery.exec()) {
            m_activeTaskId = -1;
            m_isTaskPaused = false;
            emit activeTaskChanged();
            emit taskPausedChanged();
        }
    }

    emit taskStatusChanged(taskId, dbStatus);

    // Update database
    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("UPDATE task SET status = :status WHERE id = :id AND user_id = :user_id");
    query.bindValue(":status", dbStatus);
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to update task status for taskId" << taskId << ":" << query.lastError().text();
    } else {
        qDebug() << "Task status updated to" << dbStatus << "for taskId" << taskId;
        // If task is active and status changed to Review, deselect it
        QStringList restrictedStatuses = {"Review", "Need Review", "Need Revise"};
        // Cek jika tugas yang statusnya berubah adalah tugas yang sedang aktif
        if (restrictedStatuses.contains(dbStatus) && m_activeTaskId == taskId) {
            qDebug() << "Active task" << taskId << "entered a restricted state:" << dbStatus << ". Attempting to switch.";

            // Cari tugas lain yang valid untuk diaktifkan
            QSqlQuery findNextTaskQuery(m_dataManager->productivityDb());
            findNextTaskQuery.prepare(
                "SELECT id FROM task WHERE user_id = :user_id AND id != :current_id AND status NOT IN ('Review', 'Need Review', 'Need Revise') LIMIT 1"
                );
            findNextTaskQuery.bindValue(":user_id", m_currentUserId);
            findNextTaskQuery.bindValue(":current_id", taskId);

            if (findNextTaskQuery.exec() && findNextTaskQuery.next()) {
                // Jika tugas lain ditemukan, panggil setActiveTask.
                int nextTaskId = findNextTaskQuery.value(0).toInt();
                qDebug() << "Found next available task:" << nextTaskId << ". Switching...";
                setActiveTask(nextTaskId);
            } else {
                // Jika tidak ada tugas lain, panggil setActiveTask dengan -1 untuk menonaktifkan.
                qDebug() << "No other available tasks found. Deactivating current task.";
                setActiveTask(-1);
            }
        }
        emit taskListChanged();
    }

    reply->deleteLater();
}

QString Deskmon::getTaskName(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot get task name: Database is not open";
        return "Unknown Task";
    }

    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("SELECT task FROM task WHERE id = :id AND user_id = :user_id");
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", m_currentUserId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to get task name for ID" << taskId << ":" << query.lastError().text();
        return "Unknown Task";
    }

    return query.value(0).toString();
}

void Deskmon::finishTask(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot finish task: Database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot finish task: No user logged in";
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());

    // Verifikasi bahwa taskId milik user saat ini
    query.prepare("SELECT user_id, project_name, task, max_time, time_usage FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch task details:" << query.lastError().text();
        return;
    }
    if (query.value(0).toInt() != m_currentUserId) {
        qWarning() << "Task ID" << taskId << "does not belong to current user:" << m_currentUserId;
        return;
    }

    QString projectName = query.value(1).toString();
    QString taskDesc = query.value(2).toString();
    int maxTime = query.value(3).toInt();
    int timeUsage = m_activeTaskId == taskId && !m_isTaskPaused
                        ? m_taskTimeOffset + (QDateTime::currentSecsSinceEpoch() - m_taskStartTime)
                        : query.value(4).toInt();
    qint64 completedTime = QDateTime::currentSecsSinceEpoch();

    query.prepare("INSERT INTO completed_tasks (project_name, task, max_time, time_usage, completed_time, user_id) "
                  "VALUES (:projectName, :task, :maxTime, :timeUsage, :completedTime, :user_id)");
    query.bindValue(":projectName", projectName);
    query.bindValue(":task", taskDesc);
    query.bindValue(":maxTime", maxTime);
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":completedTime", completedTime);
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to insert into completed_tasks:" << query.lastError().text();
        return;
    }

    query.prepare("DELETE FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec()) {
        qWarning() << "Failed to delete task:" << query.lastError().text();
        return;
    }

    if (m_activeTaskId == taskId) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_pauseStartTime = 0;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        emit activeTaskChanged();
        emit taskPausedChanged();
    }

    emit taskListChanged();
}

void Deskmon::setMaxTimeForTask(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set max time for task: Database is not open";
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("SELECT max_time FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch max_time for task:" << query.lastError().text();
        return;
    }

    int maxTime = query.value(0).toInt();
    if (maxTime == 0) {
        int newMaxTime = 8 * 3600; // 8 hours in seconds
        query.prepare("UPDATE task SET max_time = :maxTime WHERE id = :id");
        query.bindValue(":maxTime", newMaxTime);
        query.bindValue(":id", taskId);
        if (!query.exec()) {
            qWarning() << "Failed to set max_time:" << query.lastError().text();
        }
    }
}

void Deskmon::toggleTaskPause()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot toggle pause: Productivity database is not open";
        return;
    }

    if (m_activeTaskId == -1) {
        qWarning() << "No active task to pause/resume";
        return;
    }


    // Get current timestamp in ISO format
    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    QSqlQuery query(m_dataManager->productivityDb());
    m_dataManager->productivityDb().transaction();  // Start transaction for atomic operations

    try {
        if (!m_isTaskPaused) {
            // CASE 1: Pausing an active task (Play -> Pause)

            // 1. Update time_usage in task table
            qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 timeUsed = m_taskTimeOffset + (currentEpoch - m_taskStartTime);

            query.prepare("UPDATE task SET time_usage = ?, paused = 1, status = 'Paused' WHERE id = ?");
            query.addBindValue(timeUsed);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to update task status");
            }

            // 2. Close the open 'play' period in log_paused
            query.prepare(
                "UPDATE log_paused "
                "SET end_reality = ? "
                "WHERE task_id = ? AND current_status = 'play' AND end_reality IS NULL"
                );
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to close play period");
            }

            // 3. Start new 'pause' period
            query.prepare(
                "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                "VALUES (?, ?, NULL, 'pause')"
                );
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            if (!query.exec()) {
                throw std::runtime_error("Failed to log pause start");
            }
            sendPausePlayDataToAPI(m_activeTaskId,
                                   m_lastPlayStartTime.toString(Qt::ISODateWithMs),
                                   currentTime,
                                   "pause");

            saveWorkTimeData();

            // Update local state
            m_isTaskPaused = true;
            m_isTrackingActive = false;
            m_taskTimeOffset = timeUsed;
            m_pauseStartTime = currentEpoch;

            qDebug() << "Task paused at" << currentTime;
        } else {
            // CASE 2: Resuming a paused task (Pause -> Play)

            // 1. Close the open 'pause' period in log_paused
            query.prepare(
                "UPDATE log_paused "
                "SET end_reality = ? "
                "WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL"
                );
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to close pause period");
            }

            // 2. Start new 'play' period
            query.prepare(
                "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                "VALUES (?, ?, NULL, 'play')"
                );
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            if (!query.exec()) {
                throw std::runtime_error("Failed to log play start");
            }

            // sendPausePlayDataToAPI(m_activeTaskId,
            //                        m_lastPlayStartTime.toString(Qt::ISODateWithMs),
            //                        currentTime,
            //                        "start");
            // Update local state

            sendPing(m_activeTaskId);
            m_isTaskPaused = false;
            m_isTrackingActive = true;
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            m_pauseStartTime = 0;

            qDebug() << "Task resumed at" << currentTime;
        }


        if (!m_dataManager->productivityDb().commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        // Emit signals after successful commit
        emit taskPausedChanged();
        emit trackingActiveChanged();
        emit taskListChanged();

    } catch (const std::exception& e) {
        m_dataManager->productivityDb().rollback();
        qCritical() << "Error in toggleTaskPause:" << e.what();
    }
}

void Deskmon::sendPausePlayDataToAPI(int taskId, const QString& startTime,
                                    const QString& endTime, const QString& status)
{
    // 1. Validasi token
    if (m_authToken.isEmpty()) {
        qWarning() << "No authentication token available";
        return;
    }

    // 3. Siapkan payload JSON dan URL secara dinamis berdasarkan status
    QJsonObject payload;
    QString url; // Variabel untuk menyimpan URL API

    if (status == "pause") {
        payload["status"] = "stop";
        url = QString("https://deskmon.pranala-dt.co.id/api/end-implementation/%1").arg(taskId);
    }
    // else if (status == "start") {
    //     payload["status"] = "start";
    //     url = QString("https://deskmon.pranala-dt.co.id/api/start-implementation/%1").arg(taskId);
    // }
    else {
        qWarning() << "Unknown status for API call:" << status;
        return; // Jangan kirim jika status tidak dikenali
    }

    // 4. Konfigurasi request PUT
    QNetworkRequest request;
    request.setUrl(QUrl(url)); // Gunakan URL yang sudah ditentukan
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    // 5. Debug output
    qDebug() << "Sending PUT to:" << url;
    qDebug() << "Payload:" << QJsonDocument(payload).toJson(QJsonDocument::Indented);

    // 6. Kirim request PUT
    QBuffer *buffer = new QBuffer();
    buffer->setData(QJsonDocument(payload).toJson());
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply* reply = m_apiManager->networkManager()->put(request, buffer);
    buffer->setParent(reply);  // Auto-delete buffer ketika reply dihapus

    // 7. Handle timeout (30 detik)
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    // 8. Handle response
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        bool isAuthError = false;

        if (reply->error() != QNetworkReply::NoError) {
            // PRIORITAS UTAMA
            if (reply->errorString().contains("Host requires authentication", Qt::CaseInsensitive)) {
                isAuthError = true;
                showAuthTokenErrorMessage();
            } else {
                emit showPingErrorDialog("Koneksi gagal: " + reply->errorString());
            }
        } else {
            bool success = jsonObj.value("success").toBool();
            if (!success) {
                QString errMsg = jsonObj.value("message").toString("Terjadi kesalahan.");
                // PRIORITAS UTAMA
                if (errMsg.contains("Host requires authentication", Qt::CaseInsensitive)) {
                    isAuthError = true;
                    showAuthTokenErrorMessage();
                } else {
                    emit showPingErrorDialog(errMsg);
                }
            } else {
                emit hidePingErrorDialog(); // tutup dialog kalau sukses
            }
        }

        reply->deleteLater();
    });
}

void Deskmon::revertTaskChange()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot revert task change: Database is not open";
        return;
    }

    QSqlQuery query(m_dataManager->productivityDb());

    // Revert the active task to previous state
    if (m_activeTaskId != -1) {
        // Reactivate the previous task if it was paused
        query.prepare("UPDATE task SET active = 1, paused = 1 WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec()) {
            qWarning() << "Failed to reactivate previous task:" << query.lastError().text();
        }

        // Update local state
        m_isTaskPaused = true;
        m_isTrackingActive = false;
    }

    emit taskListChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
}

void Deskmon::syncActiveTask()
{
    // 1. Pastikan database produktivitas terbuka
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot sync tasks: Database is not open";
        return;
    }

    // 2. Pastikan ada pengguna yang sedang login
    if (m_currentUserId == -1) {
        qWarning() << "Cannot sync tasks: No user logged in";
        return;
    }

    // 3. Debug: Tampilkan semua tugas untuk pengguna saat ini
    QSqlQuery debugQuery(m_dataManager->productivityDb());
    debugQuery.prepare("SELECT id, active, user_id, status FROM task WHERE user_id = :user_id");
    debugQuery.bindValue(":user_id", m_currentUserId);
    if (debugQuery.exec()) {
        qDebug() << "Daftar tugas untuk user_id" << m_currentUserId << ":";
        while (debugQuery.next()) {
            qDebug() << "ID:" << debugQuery.value(0).toInt()
            << "Active:" << debugQuery.value(1).toBool()
            << "User ID:" << debugQuery.value(2).toInt()
            << "Status:" << debugQuery.value(3).toString();
        }
    } else {
        qWarning() << "Gagal menampilkan daftar tugas:" << debugQuery.lastError().text();
    }

    // 4. Ambil semua tugas dari database lokal untuk pengguna saat ini
    QSqlQuery query(m_dataManager->productivityDb());
    query.prepare("SELECT id, paused, time_usage, active FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to query tasks:" << query.lastError().text();
        return;
    }

    // 5. Simpan ID tugas untuk pembaruan status
    QList<int> taskIds;
    bool hasActiveTask = false;
    while (query.next()) {
        int taskId = query.value(0).toInt();
        if (taskId <= 0) {
            qWarning() << "Invalid task ID found:" << taskId;
            continue;
        }
        taskIds.append(taskId);

        // 6. Perbarui informasi tugas aktif jika ada
        if (query.value(3).toBool()) { // Kolom active
            if (hasActiveTask) {
                qWarning() << "Multiple active tasks detected, resetting previous active task";
                // Nonaktifkan tugas sebelumnya jika ada lebih dari satu tugas aktif
                QSqlQuery resetQuery(m_dataManager->productivityDb());
                resetQuery.prepare("UPDATE task SET active = 0, paused = 0 WHERE id = :id");
                resetQuery.bindValue(":id", m_activeTaskId);
                if (!resetQuery.exec()) {
                    qWarning() << "Failed to reset previous active task:" << resetQuery.lastError().text();
                }
            }
            m_activeTaskId = taskId;
            m_isTaskPaused = false; // Default to active on first sync after login
            m_isTrackingActive = true; 
            m_taskTimeOffset = query.value(2).toInt();
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            hasActiveTask = true;
            qDebug() << "Active task synchronized and set to Active state: ID =" << m_activeTaskId;
        }
    }

    // 7. Jika tidak ada tugas aktif, reset variabel terkait
    if (!hasActiveTask) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        qDebug() << "No active task found for user_id:" << m_currentUserId;
    }

    // 8. Sinkronkan semua tugas dengan server
    fetchAndStoreTasks();

    // 9. Emit sinyal untuk memberitahu perubahan ke UI (Loop updateTaskStatus dihapus karena menyebabkan lag)
    emit activeTaskChanged();
    emit taskPausedChanged();
    emit taskListChanged();
}

void Deskmon::taskDetailsDialogClosed(const QString &action)
{
    // Cek aksi apa yang sedang berlangsung saat dialog ditutup
    if (action == "quit") {
        // Jika sedang dalam proses QUIT, maka LANJUTKAN ke dialog early leave
        qDebug() << "Dialog details canceled during QUIT action. Proceeding to early leave check...";
        emit readyToProceedWithQuit();
    } else if (action == "logout") {
        qDebug() << "Dialog details canceled/skipped during LOGOUT. Proceeding to logout...";
        emit readyToProceedWithLogout();
    } else if (action == "switch") {
        // Jika hanya sedang PINDAH TASK, maka BATALKAN proses
        qDebug() << "Dialog details canceled during SWITCH action. Task switch aborted.";
        // Tidak melakukan apa-apa, sehingga proses pindah task berhenti.
    }
}

void Deskmon::submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId)
{
    // Cukup panggil fungsi API
    sendTaskDetailsToAPI(taskId, details, action, nextTaskId);
}

void Deskmon::sendTaskDetailsToAPI(int taskId, const QString &details, const QString &action, int nextTaskId)
{
    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send task details: No auth token.";

        // Kirim sinyal GAGAL ke dialog DAN notifikasi
        emit taskDetailsSubmissionFailed("Authentication token not found.");
        emit showNotification("error", "Gagal: Token otentikasi tidak ditemukan.");

        if (action == "quit") {
            emit readyToProceedWithQuit();
        }
        return;
    }

    QJsonObject payload;
    payload["task_id"] = taskId;
    payload["user_id"] = m_currentUserId;
    payload["message"] = details;

    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/send-detail-pekerjaan"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qWarning() << "Sending task details for action '" << action << "':" << QJsonDocument(payload).toJson();

    QNetworkReply *reply = m_apiManager->networkManager()->post(request, QJsonDocument(payload).toJson());
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId, details, action, nextTaskId]() {

        if (reply->error() != QNetworkReply::NoError) {
            // GAGAL JARINGAN
            qWarning() << "Failed to send task details (Network Error):" << reply->errorString();

            // Kirim sinyal GAGAL ke dialog DAN notifikasi
            QString errorMsg = "Network Error: " + reply->errorString();
            emit taskDetailsSubmissionFailed(errorMsg);
            emit showNotification("warning", "Detail pekerjaan Gagal dikirim!");

            if (action == "quit") {
                qDebug() << "Network error during quit, but proceeding to early leave check...";
                emit readyToProceedWithQuit();
            }
        } else {
            // SUKSES JARINGAN (HTTP 200 OK)
            qDebug() << "Task details submission (HTTP) successful for action:" << action;

            // (Logika parsing Anda sudah benar)
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject() && jsonDoc.object().contains("success") && jsonDoc.object()["success"].toBool() == true) {
                qDebug() << "Server response (parsed): Success";
            } else {
                qDebug() << "Server response (unparsed or failed):" << responseData;
            }

            // --- INI PERBAIKANNYA ---
            // 1. Kirim sinyal SUKSES ke dialog (untuk menutup)
            emit taskDetailsSubmissionSuccess();
            // 2. Kirim sinyal SUKSES ke Main.qml (untuk notifikasi)
            emit showNotification("success", "Detail pekerjaan berhasil dikirim!");
            // ------------------------

            if (action == "quit") {
                qDebug() << "Submission successful, proceeding to early leave check...";
                emit readyToProceedWithQuit();
            }
            else if (action == "switch") {
                qWarning() << "Submission successful, proceeding to switch task to:" << nextTaskId;
                setActiveTask(nextTaskId);
            }
            else if (action == "logout") {
                qDebug() << "Submission successful, proceeding to logout...";
                emit readyToProceedWithLogout();
            }
        }
        reply->deleteLater();
    });
}

