#include "IpcServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace Clarity {

const QString IpcServer::SERVER_NAME = QStringLiteral("clarity-ipc");

IpcServer::IpcServer(QObject* parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

IpcServer::~IpcServer()
{
    stop();
}

bool IpcServer::start()
{
    // Remove any existing server (in case of previous crash)
    QLocalServer::removeServer(SERVER_NAME);

    if (!m_server->listen(SERVER_NAME)) {
        qCritical() << "IpcServer: Failed to start server:" << m_server->errorString();
        emit errorOccurred(m_server->errorString());
        return false;
    }

    qDebug() << "IpcServer: Server started, listening on" << SERVER_NAME;
    return true;
}

void IpcServer::stop()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "IpcServer: Server stopped";
    }

    // Disconnect all clients
    for (QLocalSocket* client : m_clients.keys()) {
        client->disconnectFromServer();
        client->deleteLater();
    }
    m_clients.clear();
    m_receiveBuffers.clear();
}

bool IpcServer::isListening() const
{
    return m_server->isListening();
}

void IpcServer::sendToAll(const QJsonObject& message)
{
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    for (QLocalSocket* client : m_clients.keys()) {
        client->write(data);
        client->flush();
    }
}

void IpcServer::sendToClient(QLocalSocket* client, const QJsonObject& message)
{
    if (!client || !m_clients.contains(client)) {
        qWarning() << "IpcServer: Attempted to send to invalid client";
        return;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    client->write(data);
    client->flush();
}

bool IpcServer::sendToClientType(const QString& clientType, const QJsonObject& message)
{
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    bool sentToAny = false;
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        if (it.value() == clientType) {
            it.key()->write(data);
            it.key()->flush();
            sentToAny = true;
        }
    }
    return sentToAny;
}

bool IpcServer::hasClientType(const QString& clientType) const
{
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        if (it.value() == clientType) {
            return true;
        }
    }
    return false;
}

void IpcServer::onNewConnection()
{
    QLocalSocket* client = m_server->nextPendingConnection();
    if (!client) {
        return;
    }

    qDebug() << "IpcServer: New client connection";

    // Store client with unknown type initially
    m_clients.insert(client, QString());
    m_receiveBuffers.insert(client, QByteArray());

    connect(client, &QLocalSocket::readyRead, this, &IpcServer::onClientReadyRead);
    connect(client, &QLocalSocket::disconnected, this, &IpcServer::onClientDisconnected);

    emit clientConnected(client);
}

void IpcServer::onClientReadyRead()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }

    // Append new data to buffer
    m_receiveBuffers[client].append(client->readAll());

    // Process complete messages (newline-delimited)
    QByteArray& buffer = m_receiveBuffers[client];
    int newlineIndex;

    while ((newlineIndex = buffer.indexOf('\n')) != -1) {
        QByteArray message = buffer.left(newlineIndex);
        buffer.remove(0, newlineIndex + 1);

        if (!message.isEmpty()) {
            processMessage(client, message);
        }
    }
}

void IpcServer::onClientDisconnected()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }

    qDebug() << "IpcServer: Client disconnected:" << m_clients.value(client, "unknown");

    m_clients.remove(client);
    m_receiveBuffers.remove(client);

    emit clientDisconnected(client);

    client->deleteLater();
}

void IpcServer::processMessage(QLocalSocket* client, const QByteArray& data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "IpcServer: JSON parse error:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "IpcServer: Received non-object JSON";
        return;
    }

    QJsonObject message = doc.object();

    // Handle client identification
    if (message["type"].toString() == "connect") {
        QString clientType = message["clientType"].toString();
        m_clients[client] = clientType;
        qDebug() << "IpcServer: Client identified as" << clientType;
    }

    emit messageReceived(client, message);
}

} // namespace Clarity
