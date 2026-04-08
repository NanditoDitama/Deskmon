// deskmon_worktime.cpp
// Menangani semua logika terkait waktu kerja (work time):
//   - Fetch & simpan waktu kerja dari/ke database lokal
//   - Kirim waktu kerja ke API server
//   - Laporan harian penggunaan
#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>

// ─────────────────────────────────────────────────
//  Work Time — Database Lokal
// ─────────────────────────────────────────────────

void Deskmon::checkAndCreateNewDayRecord()
{
    m_dataManager->checkAndCreateNewDayRecord(m_currentUserId);
}

void Deskmon::loadWorkTimeData()
{
    m_workTimeElapsedSeconds = m_dataManager->loadWorkTimeData(m_currentUserId);
    emit workTimeElapsedSecondsChanged();
}

void Deskmon::saveWorkTimeData()
{
    m_dataManager->saveWorkTimeData(m_currentUserId, m_workTimeElapsedSeconds);
}

int Deskmon::workTimeElapsedSeconds() const
{
    return m_workTimeElapsedSeconds;
}

// ─────────────────────────────────────────────────
//  Work Time — API
// ─────────────────────────────────────────────────

void Deskmon::sendWorkTimeToAPI()
{
    m_apiManager->submitWorkTime(m_currentUserId, m_workTimeElapsedSeconds);
}

void Deskmon::fetchWorkTimeFromAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) {
        qWarning() << "Cannot fetch work time: No user logged in or no auth token.";
        return;
    }

    QUrl url(QString("https://deskmon.pranala-dt.co.id/api/get-time-at-work/%1").arg(m_currentUserId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Fetching work time from API...";
    QNetworkReply *reply = m_apiManager->networkManager()->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleFetchWorkTimeResponse(reply);
    });
}

void Deskmon::handleFetchWorkTimeResponse(QNetworkReply *reply)
{
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
                } else {
                    qWarning() << "API response 'data' object is missing the 'raw' key.";
                }
            } else {
                qWarning() << "Failed to get 'data' from API response:" << jsonObj["message"].toString();
            }
        }
    } else {
        qWarning() << "Failed to fetch work time from API:" << reply->errorString();
    }
    reply->deleteLater();
}

// ─────────────────────────────────────────────────
//  Laporan Harian
// ─────────────────────────────────────────────────

void Deskmon::sendDailyUsageReport()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QJsonArray data = m_dataManager->getDailyUsageReportData(m_currentUserId, today);
    if (!data.isEmpty()) {
        m_apiManager->sendDailyUsageReport(data, m_authToken);
    }
}
