#pragma once

#include "Slide.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>

namespace Clarity {

/**
 * @brief HTTP and WebSocket server for remote control of presentations
 *
 * Provides a web interface accessible from mobile devices on the local network.
 * Supports real-time slide updates via WebSocket and navigation commands.
 */
class RemoteServer : public QObject {
    Q_OBJECT

public:
    explicit RemoteServer(quint16 port = 8080, QObject* parent = nullptr);
    ~RemoteServer();

    /**
     * @brief Start the server
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const;

    /**
     * @brief Get the server URL for display
     */
    QString serverUrl() const;

    /**
     * @brief Get the port number
     */
    quint16 port() const { return m_port; }

    /**
     * @brief Set the port number (must be stopped first)
     */
    void setPort(quint16 port);

    /**
     * @brief Get number of connected clients
     */
    int connectedClientCount() const { return m_webSocketClients.size(); }

    /**
     * @brief Send slide update to all connected clients
     */
    void broadcastSlideUpdate(int currentIndex, int totalSlides,
                              const QString& currentText, const QString& nextText);

    /**
     * @brief Send connection status update
     */
    void broadcastStatus(bool outputConnected, bool confidenceConnected);

signals:
    /**
     * @brief Emitted when a client connects
     */
    void clientConnected();

    /**
     * @brief Emitted when a client disconnects
     */
    void clientDisconnected();

    /**
     * @brief Emitted when navigation is requested from remote
     * @param action "next", "prev", "first", "last", "goto:N", "clear", "black", "white"
     */
    void navigationRequested(const QString& action);

    /**
     * @brief Emitted when server starts or stops
     */
    void runningChanged(bool running);

private slots:
    void onHttpConnection();
    void onHttpReadyRead();
    void onHttpDisconnected();
    void onWebSocketConnection();
    void onWebSocketTextMessage(const QString& message);
    void onWebSocketDisconnected();

private:
    void handleHttpRequest(QTcpSocket* client, const QByteArray& request);
    QByteArray serveFile(const QString& path, const QString& contentType);
    QByteArray getIndexHtml();
    QByteArray getStyleCss();
    QByteArray getAppJs();
    QString getLocalIpAddress() const;

    quint16 m_port;
    QTcpServer* m_httpServer;
    QWebSocketServer* m_webSocketServer;
    QList<QWebSocket*> m_webSocketClients;
    QList<QTcpSocket*> m_httpClients;

    // Current state for new clients
    int m_currentIndex = 0;
    int m_totalSlides = 0;
    QString m_currentText;
    QString m_nextText;
    bool m_outputConnected = false;
    bool m_confidenceConnected = false;
};

} // namespace Clarity
