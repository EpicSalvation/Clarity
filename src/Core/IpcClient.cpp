// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "IpcClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace Clarity {

const QString IpcClient::SERVER_NAME = QStringLiteral("clarity-ipc");

IpcClient::IpcClient(const QString& clientType, QObject* parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_clientType(clientType)
    , m_reconnectTimer(new QTimer(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &IpcClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::onError);

    // Reconnect timer: fires periodically when disconnected to try reconnecting
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    connect(m_reconnectTimer, &QTimer::timeout, this, &IpcClient::attemptReconnect);
}

IpcClient::~IpcClient()
{
    m_reconnectTimer->stop();
    disconnectFromServer();
}

void IpcClient::connectToServer()
{
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        qDebug() << "IpcClient: Already connected";
        return;
    }

    qDebug() << "IpcClient: Connecting to server...";
    m_socket->connectToServer(SERVER_NAME);
}

void IpcClient::disconnectFromServer()
{
    // Stop reconnection attempts when intentionally disconnecting
    m_reconnectTimer->stop();

    if (m_socket->state() == QLocalSocket::ConnectedState) {
        m_socket->disconnectFromServer();
    }
}

bool IpcClient::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

void IpcClient::sendMessage(const QJsonObject& message)
{
    if (!isConnected()) {
        qWarning() << "IpcClient: Cannot send message, not connected";
        return;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    m_socket->write(data);
    m_socket->flush();
}

void IpcClient::onConnected()
{
    qDebug() << "IpcClient: Connected to server";

    // Stop reconnection attempts now that we're connected
    m_reconnectTimer->stop();

    // Send identification message
    QJsonObject connectMsg;
    connectMsg["type"] = "connect";
    connectMsg["clientType"] = m_clientType;
    sendMessage(connectMsg);

    emit connected();
}

void IpcClient::onDisconnected()
{
    qDebug() << "IpcClient: Disconnected from server — will attempt to reconnect";
    m_receiveBuffer.clear();
    emit disconnected();

    // Start reconnection attempts
    m_reconnectTimer->start();
}

void IpcClient::onReadyRead()
{
    // Append new data to buffer
    m_receiveBuffer.append(m_socket->readAll());

    // Process complete messages (newline-delimited)
    int newlineIndex;

    while ((newlineIndex = m_receiveBuffer.indexOf('\n')) != -1) {
        QByteArray message = m_receiveBuffer.left(newlineIndex);
        m_receiveBuffer.remove(0, newlineIndex + 1);

        if (!message.isEmpty()) {
            processMessage(message);
        }
    }
}

void IpcClient::onError(QLocalSocket::LocalSocketError error)
{
    // Connection-refused and server-not-found are expected when the controller
    // isn't running yet — don't spam warnings for those during reconnection.
    if (error == QLocalSocket::ServerNotFoundError ||
        error == QLocalSocket::ConnectionRefusedError) {
        // Silently continue reconnection attempts
        return;
    }

    QString errorString = m_socket->errorString();
    qWarning() << "IpcClient: Socket error:" << error << errorString;
    emit errorOccurred(errorString);
}

void IpcClient::attemptReconnect()
{
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        m_reconnectTimer->stop();
        return;
    }

    // Only attempt if socket is fully disconnected (not mid-connection)
    if (m_socket->state() == QLocalSocket::UnconnectedState) {
        m_socket->connectToServer(SERVER_NAME);
    }
}

void IpcClient::processMessage(const QByteArray& data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "IpcClient: JSON parse error:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "IpcClient: Received non-object JSON";
        return;
    }

    QJsonObject message = doc.object();
    emit messageReceived(message);
}

} // namespace Clarity
