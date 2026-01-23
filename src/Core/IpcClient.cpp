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
{
    connect(m_socket, &QLocalSocket::connected, this, &IpcClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::onError);
}

IpcClient::~IpcClient()
{
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

    // Send identification message
    QJsonObject connectMsg;
    connectMsg["type"] = "connect";
    connectMsg["clientType"] = m_clientType;
    sendMessage(connectMsg);

    emit connected();
}

void IpcClient::onDisconnected()
{
    qDebug() << "IpcClient: Disconnected from server";
    m_receiveBuffer.clear();
    emit disconnected();
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
    QString errorString = m_socket->errorString();
    qWarning() << "IpcClient: Socket error:" << error << errorString;
    emit errorOccurred(errorString);
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
