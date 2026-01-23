#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>
#include <QMap>

namespace Clarity {

/**
 * @brief IPC Server for the control application
 *
 * Listens for connections from output and confidence displays.
 * Routes messages to connected clients.
 */
class IpcServer : public QObject {
    Q_OBJECT

public:
    explicit IpcServer(QObject* parent = nullptr);
    ~IpcServer();

    bool start();
    void stop();
    bool isListening() const;

    // Send messages to clients
    void sendToAll(const QJsonObject& message);
    void sendToClient(QLocalSocket* client, const QJsonObject& message);
    void sendToClientType(const QString& clientType, const QJsonObject& message);

signals:
    void clientConnected(QLocalSocket* client);
    void clientDisconnected(QLocalSocket* client);
    void messageReceived(QLocalSocket* client, const QJsonObject& message);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void processMessage(QLocalSocket* client, const QByteArray& data);

    QLocalServer* m_server;
    QMap<QLocalSocket*, QString> m_clients; // Maps socket to client type
    QMap<QLocalSocket*, QByteArray> m_receiveBuffers; // Buffered data per client

    static const QString SERVER_NAME;
};

} // namespace Clarity
