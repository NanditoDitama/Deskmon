#ifndef MOCKHTTPSERVER_H
#define MOCKHTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QMap>
#include <functional>

struct RecordedHttpRequest {
    QString method;
    QString path;
    QMap<QString, QString> headers;
    QByteArray body;
};

struct MockHttpResponse {
    int statusCode = 200;
    QByteArray reason = "OK";
    QByteArray body = R"({"success":true})";
    QByteArray contentType = "application/json";
    bool dropConnection = false;
};

class MockHttpServer : public QTcpServer
{
public:
    using Handler = std::function<MockHttpResponse(const RecordedHttpRequest &)>;

    explicit MockHttpServer(QObject *parent = nullptr)
        : QTcpServer(parent)
    {
        connect(this, &QTcpServer::newConnection, this, [this]() { onNewConnection(); });
    }

    bool start()
    {
        return listen(QHostAddress::LocalHost, 0);
    }

    QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1").arg(serverPort());
    }

    void setHandler(Handler handler)
    {
        m_handler = std::move(handler);
    }

    const QList<RecordedHttpRequest>& requests() const { return m_requests; }
    void clearRequests() { m_requests.clear(); }

private:
    void onNewConnection()
    {
        while (hasPendingConnections()) {
            QTcpSocket *socket = nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                QByteArray &buffer = m_buffers[socket];
                buffer.append(socket->readAll());

                int headerEnd = buffer.indexOf("\r\n\r\n");
                if (headerEnd == -1) return;

                QByteArray headerBytes = buffer.left(headerEnd);
                QList<QByteArray> lines = headerBytes.split('\n');
                if (lines.isEmpty()) return;

                QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
                if (requestLine.size() < 2) return;

                RecordedHttpRequest req;
                req.method = QString::fromUtf8(requestLine[0]);
                req.path = QString::fromUtf8(requestLine[1]);

                int contentLength = 0;
                for (int i = 1; i < lines.size(); ++i) {
                    QByteArray line = lines[i].trimmed();
                    int colon = line.indexOf(':');
                    if (colon > 0) {
                        QString key = QString::fromUtf8(line.left(colon)).trimmed().toLower();
                        QString val = QString::fromUtf8(line.mid(colon + 1)).trimmed();
                        req.headers[key] = val;
                        if (key == QStringLiteral("content-length")) {
                            contentLength = val.toInt();
                        }
                    }
                }

                int totalNeeded = headerEnd + 4 + contentLength;
                if (buffer.size() < totalNeeded) return;

                req.body = buffer.mid(headerEnd + 4, contentLength);
                m_buffers.remove(socket);
                m_requests.append(req);

                MockHttpResponse resp;
                if (m_handler) {
                    resp = m_handler(req);
                }

                if (resp.dropConnection) {
                    socket->abort();
                    socket->deleteLater();
                    return;
                }

                QByteArray raw;
                raw += "HTTP/1.1 " + QByteArray::number(resp.statusCode) + " " + resp.reason + "\r\n";
                raw += "Content-Type: " + resp.contentType + "\r\n";
                raw += "Content-Length: " + QByteArray::number(resp.body.size()) + "\r\n";
                raw += "Connection: close\r\n\r\n";
                raw += resp.body;

                socket->write(raw);
                socket->flush();
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                m_buffers.remove(socket);
                socket->deleteLater();
            });
        }
    }

private:
    Handler m_handler;
    QList<RecordedHttpRequest> m_requests;
    QMap<QTcpSocket*, QByteArray> m_buffers;
};

#endif // MOCKHTTPSERVER_H
