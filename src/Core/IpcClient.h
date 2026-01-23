#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>

namespace Clarity {

/**
 * @brief IPC Client for output and confidence displays
 *
 * Connects to the control application's IPC server.
 * Receives slide data and display commands.
 */
class IpcClient : public QObject {
    Q_OBJECT

public:
    explicit IpcClient(const QString& clientType, QObject* parent = nullptr);
    ~IpcClient();

    void connectToServer();
    void disconnectFromServer();
    bool isConnected() const;

    void sendMessage(const QJsonObject& message);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject& message);
    void errorOccurred(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QLocalSocket::LocalSocketError error);

private:
    void processMessage(const QByteArray& data);

    QLocalSocket* m_socket;
    QString m_clientType;
    QByteArray m_receiveBuffer;

    static const QString SERVER_NAME;
};

} // namespace Clarity
