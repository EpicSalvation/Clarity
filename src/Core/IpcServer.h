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

    /**
     * @brief Send a message to all clients of a specific type
     * @param clientType The type of client ("output" or "confidence")
     * @param message The message to send
     * @return true if at least one client received the message, false if no clients of this type are connected
     */
    bool sendToClientType(const QString& clientType, const QJsonObject& message);

    /**
     * @brief Check if any clients of a specific type are connected
     * @param clientType The type of client to check for
     * @return true if at least one client of this type is connected
     */
    bool hasClientType(const QString& clientType) const;

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
