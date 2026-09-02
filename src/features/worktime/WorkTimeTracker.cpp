#include "WorkTimeTracker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>

WorkTimeTracker::WorkTimeTracker(ApiClient *apiClient,
                                 ProductivityAppRepository *prodRepo,
                                 AuthManager *authManager,
                                 QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_prodRepo(prodRepo)
    , m_authManager(authManager)
{
    m_apiWorkTimeTimer.setInterval(60000); // 1 menit
    connect(&m_apiWorkTimeTimer, &QTimer::timeout, this, &WorkTimeTracker::fetchWorkTimeFromAPI);
}

WorkTimeTracker::~WorkTimeTracker()
{
    saveWorkTimeData();
    sendWorkTimeToAPI();
}

void WorkTimeTracker::startSyncTimer()
{
    fetchWorkTimeFromAPI();
    m_apiWorkTimeTimer.start();
}

void WorkTimeTracker::stopSyncTimer()
{
    m_apiWorkTimeTimer.stop();
}

void WorkTimeTracker::checkAndCreateNewDayRecord()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT elapsed_seconds FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        return; // Record already exists
    } else {
        m_workTimeElapsedSeconds = 0;
        emit workTimeElapsedSecondsChanged();
        saveWorkTimeData();
        qDebug() << "New day detected. Work time reset for user:" << userId;
    }
}

void WorkTimeTracker::loadWorkTimeData()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) {
        m_workTimeElapsedSeconds = 0;
        emit workTimeElapsedSecondsChanged();
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT elapsed_seconds FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        m_workTimeElapsedSeconds = query.value(0).toInt();
    } else {
        m_workTimeElapsedSeconds = 0;
        saveWorkTimeData();
    }
    qDebug() << "Loaded work time for" << today << ":" << m_workTimeElapsedSeconds << "seconds";
    emit workTimeElapsedSecondsChanged();
}

void WorkTimeTracker::saveWorkTimeData()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_prodRepo->database());
    query.prepare("INSERT OR REPLACE INTO work_time (user_id, date, elapsed_seconds) "
                  "VALUES (:user_id, :date, :seconds)");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);
    query.bindValue(":seconds", m_workTimeElapsedSeconds);

    if (!query.exec()) {
        qWarning() << "Failed to save work time:" << query.lastError().text();
    }
}

void WorkTimeTracker::sendWorkTimeToAPI()
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) {
        return;
    }

    QJsonObject payload;
    payload["user_id"] = userId;
    payload["time_at_work"] = m_workTimeElapsedSeconds;

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/send-time-at-work"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Work time successfully sent to API:" << reply->readAll();
        } else {
            qWarning() << "Failed to send work time to API:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void WorkTimeTracker::fetchWorkTimeFromAPI()
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) {
        return;
    }

    QUrl url(QString("https://deskmon.pranala-dt.co.id/api/get-time-at-work/%1").arg(userId));
    QNetworkReply *reply = m_apiClient->get(url, true);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj["success"].toBool() && jsonObj.contains("data") && jsonObj["data"].isObject()) {
                    QJsonObject dataObj = jsonObj["data"].toObject();
                    if (dataObj.contains("raw")) {
                        int serverSeconds = dataObj["raw"].toInt();
                        if (serverSeconds != m_workTimeElapsedSeconds) {
                            m_workTimeElapsedSeconds = serverSeconds;
                            qDebug() << "Work time updated from server:" << m_workTimeElapsedSeconds << "seconds";
                            emit workTimeElapsedSecondsChanged();
                        }
                    }
                }
            }
        }
        reply->deleteLater();
    });
}

void WorkTimeTracker::submitEarlyLeaveReason(const QString &reason)
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) {
        emit earlyLeaveReasonSubmitted();
        return;
    }

    QJsonObject payload;
    payload["email"] = m_authManager->currentUserEmail();
    payload["alasan"] = reason;

    QNetworkReply* reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/send-alasan"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QString responseText = QString::fromUtf8(responseData);
        bool shouldQuit = false;
        QString errorMessage;

        if (reply->error() != QNetworkReply::NoError) {
            errorMessage = "Koneksi gagal, silahkan coba lagi.\nError: " + reply->errorString();
        } else {
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                errorMessage = "Koneksi gagal, silahkan coba lagi.\nInvalid response format.";
            } else if (jsonDoc.isObject()) {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj.contains("success") && jsonObj["success"].isBool()) {
                    bool success = jsonObj["success"].toBool();
                    if (success) {
                        shouldQuit = true;
                    } else {
                        QString serverMessage = jsonObj.value("message").toString();
                        errorMessage = serverMessage.isEmpty() ? "Koneksi gagal, silahkan coba lagi." : "Gagal: " + serverMessage;
                    }
                } else {
                    errorMessage = "Koneksi gagal, silahkan coba lagi.\nNo success field in response.";
                }
            } else {
                errorMessage = "Koneksi gagal, silahkan coba lagi.\nInvalid JSON response.";
            }
        }

        reply->deleteLater();

        if (shouldQuit) {
            emit earlyLeaveReasonSubmitted();
        } else {
            QMessageBox::warning(nullptr, "Submit Gagal", errorMessage);
        }
    });
}
