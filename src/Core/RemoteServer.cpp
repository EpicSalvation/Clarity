#include "RemoteServer.h"
#include "QrCode.h"
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace Clarity {

RemoteServer::RemoteServer(quint16 port, QObject* parent)
    : QObject(parent)
    , m_port(port)
    , m_httpServer(new QTcpServer(this))
    , m_webSocketServer(new QWebSocketServer("Clarity Remote",
                                              QWebSocketServer::NonSecureMode, this))
{
    // HTTP server connections
    connect(m_httpServer, &QTcpServer::newConnection,
            this, &RemoteServer::onHttpConnection);

    // WebSocket server connections
    connect(m_webSocketServer, &QWebSocketServer::newConnection,
            this, &RemoteServer::onWebSocketConnection);
}

RemoteServer::~RemoteServer()
{
    stop();
}

bool RemoteServer::start()
{
    if (isRunning()) {
        return true;
    }

    // Start HTTP server
    if (!m_httpServer->listen(QHostAddress::Any, m_port)) {
        qWarning() << "RemoteServer: Failed to start HTTP server on port" << m_port
                   << ":" << m_httpServer->errorString();
        return false;
    }

    // Start WebSocket server on port + 1
    if (!m_webSocketServer->listen(QHostAddress::Any, m_port + 1)) {
        qWarning() << "RemoteServer: Failed to start WebSocket server on port" << (m_port + 1)
                   << ":" << m_webSocketServer->errorString();
        m_httpServer->close();
        return false;
    }

    qDebug() << "RemoteServer: Started on" << serverUrl();
    qDebug() << "RemoteServer: WebSocket on port" << (m_port + 1);
    emit runningChanged(true);
    return true;
}

void RemoteServer::stop()
{
    // Close all WebSocket clients
    for (QWebSocket* client : m_webSocketClients) {
        client->close();
        client->deleteLater();
    }
    m_webSocketClients.clear();

    // Close all HTTP clients
    for (QTcpSocket* client : m_httpClients) {
        client->close();
        client->deleteLater();
    }
    m_httpClients.clear();

    // Stop servers
    if (m_webSocketServer->isListening()) {
        m_webSocketServer->close();
    }
    if (m_httpServer->isListening()) {
        m_httpServer->close();
    }

    qDebug() << "RemoteServer: Stopped";
    emit runningChanged(false);
}

bool RemoteServer::isRunning() const
{
    return m_httpServer->isListening();
}

QString RemoteServer::serverUrl() const
{
    QString ip = getLocalIpAddress();
    return QString("http://%1:%2").arg(ip).arg(m_port);
}

QImage RemoteServer::qrCode(int moduleSize, int margin) const
{
    return QrCode::generate(serverUrl(), moduleSize, margin);
}

void RemoteServer::setPort(quint16 port)
{
    if (!isRunning()) {
        m_port = port;
    }
}

void RemoteServer::setPin(bool enabled, const QString& pin)
{
    bool pinChanged = (m_pinEnabled != enabled) || (m_pin != pin);
    m_pinEnabled = enabled;
    m_pin = pin;
    qDebug() << "RemoteServer: PIN protection" << (enabled ? "enabled" : "disabled");

    // If PIN settings changed, clear authenticated clients and notify them
    if (pinChanged && !m_authenticatedClients.isEmpty()) {
        m_authenticatedClients.clear();
        qDebug() << "RemoteServer: Cleared authenticated clients due to PIN change";

        // Notify all connected clients to re-check authentication
        QJsonObject notification;
        notification["type"] = "authStatus";
        notification["pinRequired"] = m_pinEnabled;
        notification["authenticated"] = !m_pinEnabled;  // Only auto-authenticated if PIN disabled

        QByteArray message = QJsonDocument(notification).toJson(QJsonDocument::Compact);
        for (QWebSocket* client : m_webSocketClients) {
            client->sendTextMessage(QString::fromUtf8(message));
        }
    }
}

QString RemoteServer::getLocalIpAddress() const
{
    // Find the best non-loopback IPv4 address, preferring real network interfaces
    // over virtual ones (e.g. Miracast, VPN, virtual display adapters)
    QString fallbackAddress;

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        // Skip interfaces that are down or loopback
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        // Skip known virtual interface name patterns (case-insensitive)
        QString name = iface.name().toLower();
        QString humanName = iface.humanReadableName().toLower();

        // Check for virtual adapter patterns in the technical name
        bool isVirtualByName = name.contains("virtual") ||
                               name.contains("vethernet") ||
                               name.contains("vmnet") ||
                               name.contains("vmware") ||
                               name.contains("vbox") ||
                               name.contains("docker") ||
                               name.contains("br-") ||
                               name.contains("virbr") ||
                               name.contains("vnic") ||
                               name.contains("vpn") ||
                               name.contains("tap") ||
                               name.contains("tun");

        // Check for virtual adapter patterns in the human-readable name
        // This catches Windows adapters like "Microsoft Wi-Fi Direct Virtual Adapter"
        bool isVirtualByHumanName = humanName.contains("virtual") ||
                                    humanName.contains("wi-fi direct") ||
                                    humanName.contains("wifi direct") ||
                                    humanName.contains("direct virtual") ||
                                    humanName.contains("miracast") ||
                                    humanName.contains("hosted network") ||
                                    humanName.contains("loopback") ||
                                    humanName.contains("hyper-v") ||
                                    humanName.contains("vmware") ||
                                    humanName.contains("virtualbox") ||
                                    humanName.contains("vpn") ||
                                    humanName.contains("tunnel") ||
                                    humanName.contains("pseudo") ||
                                    humanName.contains("teredo");

        // Windows assigns "Local Area Connection* X" to Wi-Fi Direct adapters
        bool isWifiDirectConnection = humanName.contains("local area connection*");

        bool isVirtual = isVirtualByName || isVirtualByHumanName || isWifiDirectConnection;

        // Find the first IPv4 address on this interface
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol || addr.isLoopback()) {
                continue;
            }

            if (!isVirtual) {
                return addr.toString();  // Preferred: real interface
            }

            if (fallbackAddress.isEmpty()) {
                fallbackAddress = addr.toString();  // Remember first virtual as fallback
            }
        }
    }

    return fallbackAddress.isEmpty() ? "127.0.0.1" : fallbackAddress;
}

void RemoteServer::broadcastSlideUpdate(int currentIndex, int totalSlides,
                                        const QString& currentText, const QString& nextText)
{
    m_currentIndex = currentIndex;
    m_totalSlides = totalSlides;
    m_currentText = currentText;
    m_nextText = nextText;

    QJsonObject json;
    json["type"] = "slideUpdate";
    json["currentIndex"] = currentIndex;
    json["totalSlides"] = totalSlides;
    json["currentText"] = currentText;
    json["nextText"] = nextText;

    QString message = QJsonDocument(json).toJson(QJsonDocument::Compact);

    for (QWebSocket* client : m_webSocketClients) {
        client->sendTextMessage(message);
    }
}

void RemoteServer::broadcastStatus(bool outputConnected, bool confidenceConnected)
{
    m_outputConnected = outputConnected;
    m_confidenceConnected = confidenceConnected;

    QJsonObject json;
    json["type"] = "status";
    json["outputConnected"] = outputConnected;
    json["confidenceConnected"] = confidenceConnected;

    QString message = QJsonDocument(json).toJson(QJsonDocument::Compact);

    for (QWebSocket* client : m_webSocketClients) {
        client->sendTextMessage(message);
    }
}

void RemoteServer::onHttpConnection()
{
    QTcpSocket* client = m_httpServer->nextPendingConnection();
    if (!client) return;

    m_httpClients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &RemoteServer::onHttpReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &RemoteServer::onHttpDisconnected);
}

void RemoteServer::onHttpReadyRead()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray request = client->readAll();
    handleHttpRequest(client, request);
}

void RemoteServer::onHttpDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_httpClients.removeAll(client);
        client->deleteLater();
    }
}

void RemoteServer::handleHttpRequest(QTcpSocket* client, const QByteArray& request)
{
    // Parse request line
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");
    if (lines.isEmpty()) {
        client->close();
        return;
    }

    QStringList requestLine = lines.first().split(" ");
    if (requestLine.size() < 2) {
        client->close();
        return;
    }

    QString method = requestLine[0];
    QString path = requestLine[1];

    QByteArray response;
    QByteArray body;
    QString contentType = "text/html; charset=utf-8";

    if (path == "/" || path == "/index.html") {
        body = getIndexHtml();
    } else if (path == "/style.css") {
        body = getStyleCss();
        contentType = "text/css; charset=utf-8";
    } else if (path == "/app.js") {
        body = getAppJs();
        contentType = "application/javascript; charset=utf-8";
    } else {
        // 404 Not Found
        body = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += body;
        client->write(response);
        client->flush();
        client->close();
        return;
    }

    // Build response
    response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n\r\n";
    response += body;

    client->write(response);
    client->flush();
    client->close();
}

void RemoteServer::onWebSocketConnection()
{
    QWebSocket* client = m_webSocketServer->nextPendingConnection();
    if (!client) return;

    m_webSocketClients.append(client);
    connect(client, &QWebSocket::textMessageReceived,
            this, &RemoteServer::onWebSocketTextMessage);
    connect(client, &QWebSocket::disconnected,
            this, &RemoteServer::onWebSocketDisconnected);

    qDebug() << "RemoteServer: Client connected from" << client->peerAddress().toString();
    emit clientConnected();

    // Send current state to new client
    QJsonObject json;
    json["type"] = "init";
    json["currentIndex"] = m_currentIndex;
    json["totalSlides"] = m_totalSlides;
    json["currentText"] = m_currentText;
    json["nextText"] = m_nextText;
    json["outputConnected"] = m_outputConnected;
    json["confidenceConnected"] = m_confidenceConnected;

    client->sendTextMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void RemoteServer::onWebSocketTextMessage(const QString& message)
{
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject json = doc.object();
    QString type = json["type"].toString();

    // Handle PIN authentication
    if (type == "auth") {
        QString pin = json["pin"].toString();
        QJsonObject response;
        response["type"] = "authResponse";

        if (!m_pinEnabled) {
            // No PIN required, auto-authenticate
            if (!m_authenticatedClients.contains(client)) {
                m_authenticatedClients.append(client);
            }
            response["success"] = true;
            response["pinRequired"] = false;
        } else if (pin == m_pin) {
            // Correct PIN
            if (!m_authenticatedClients.contains(client)) {
                m_authenticatedClients.append(client);
            }
            response["success"] = true;
            response["pinRequired"] = true;
            qDebug() << "RemoteServer: Client authenticated with PIN";
        } else {
            // Wrong PIN
            response["success"] = false;
            response["pinRequired"] = true;
            response["error"] = "Incorrect PIN";
            qDebug() << "RemoteServer: Client failed PIN authentication";
        }

        client->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    // Handle PIN requirement check
    if (type == "checkAuth") {
        QJsonObject response;
        response["type"] = "authStatus";
        response["pinRequired"] = m_pinEnabled;
        response["authenticated"] = !m_pinEnabled || m_authenticatedClients.contains(client);
        client->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    // For navigation actions, check if client is authenticated
    QString action = json["action"].toString();
    if (!action.isEmpty()) {
        // Check authentication if PIN is enabled
        if (m_pinEnabled && !m_authenticatedClients.contains(client)) {
            qDebug() << "RemoteServer: Unauthenticated client attempted action:" << action;
            QJsonObject response;
            response["type"] = "error";
            response["error"] = "Authentication required";
            client->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        qDebug() << "RemoteServer: Navigation requested:" << action;
        emit navigationRequested(action);
    }
}

void RemoteServer::onWebSocketDisconnected()
{
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        qDebug() << "RemoteServer: Client disconnected";
        m_webSocketClients.removeAll(client);
        m_authenticatedClients.removeAll(client);
        client->deleteLater();
        emit clientDisconnected();
    }
}

QByteArray RemoteServer::getIndexHtml()
{
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="mobile-web-app-capable" content="yes">
    <title>Clarity Remote</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <!-- PIN Entry Dialog -->
    <div id="pinOverlay" class="pin-overlay" style="display: none;">
        <div class="pin-dialog">
            <h2>Enter PIN</h2>
            <p>A PIN is required to access the remote control.</p>
            <input type="password" id="pinInput" class="pin-input" maxlength="8"
                   pattern="[0-9]*" inputmode="numeric" placeholder="Enter PIN">
            <div id="pinError" class="pin-error" style="display: none;">Incorrect PIN</div>
            <button id="pinSubmit" class="pin-submit" onclick="submitPin()">Connect</button>
        </div>
    </div>

    <div id="mainContent" class="container">
        <header>
            <h1>Clarity Remote</h1>
            <div id="status" class="status disconnected">Connecting...</div>
        </header>

        <div class="slide-info">
            <div class="slide-counter">
                <span id="currentNum">-</span> / <span id="totalNum">-</span>
            </div>
        </div>

        <div class="preview-section">
            <div class="preview-card current">
                <div class="preview-label">Current Slide</div>
                <div id="currentSlide" class="preview-content">-</div>
            </div>
            <div class="preview-card next">
                <div class="preview-label">Next Slide</div>
                <div id="nextSlide" class="preview-content">-</div>
            </div>
        </div>

        <div class="controls">
            <div class="nav-row">
                <button id="prevBtn" class="nav-btn" onclick="send('prev')">
                    <span class="arrow">&#9664;</span>
                    <span class="label">Previous</span>
                </button>
                <button id="nextBtn" class="nav-btn primary" onclick="send('next')">
                    <span class="label">Next</span>
                    <span class="arrow">&#9654;</span>
                </button>
            </div>
            <div class="action-row">
                <button class="action-btn" onclick="send('first')">First</button>
                <button class="action-btn" onclick="send('last')">Last</button>
                <button class="action-btn warning" onclick="send('black')">Black</button>
                <button class="action-btn" onclick="send('clear')">Clear</button>
            </div>
        </div>

        <div class="connection-info">
            <div class="conn-item">
                <span class="conn-label">Output:</span>
                <span id="outputStatus" class="conn-status off">-</span>
            </div>
            <div class="conn-item">
                <span class="conn-label">Confidence:</span>
                <span id="confStatus" class="conn-status off">-</span>
            </div>
        </div>
    </div>
    <script src="/app.js"></script>
</body>
</html>)HTML";
}

QByteArray RemoteServer::getStyleCss()
{
    return R"CSS(* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
    color: #ffffff;
    min-height: 100vh;
    overflow-x: hidden;
}

/* PIN Overlay Styles */
.pin-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.9);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 1000;
}

.pin-dialog {
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
    border-radius: 16px;
    padding: 32px;
    max-width: 320px;
    width: 90%;
    text-align: center;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
    border: 1px solid rgba(255, 255, 255, 0.1);
}

.pin-dialog h2 {
    font-size: 22px;
    margin-bottom: 12px;
    background: linear-gradient(90deg, #667eea, #764ba2);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
}

.pin-dialog p {
    color: #9ca3af;
    font-size: 14px;
    margin-bottom: 20px;
}

.pin-input {
    width: 100%;
    padding: 16px;
    font-size: 24px;
    text-align: center;
    letter-spacing: 8px;
    background: rgba(255, 255, 255, 0.1);
    border: 2px solid rgba(255, 255, 255, 0.2);
    border-radius: 12px;
    color: #ffffff;
    outline: none;
    transition: border-color 0.2s;
}

.pin-input:focus {
    border-color: #667eea;
}

.pin-input::placeholder {
    letter-spacing: normal;
    font-size: 16px;
}

.pin-error {
    color: #ef4444;
    font-size: 14px;
    margin-top: 12px;
}

.pin-submit {
    width: 100%;
    padding: 14px;
    margin-top: 20px;
    font-size: 16px;
    font-weight: 600;
    color: #ffffff;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    border: none;
    border-radius: 12px;
    cursor: pointer;
    transition: transform 0.1s, opacity 0.2s;
}

.pin-submit:hover {
    opacity: 0.9;
}

.pin-submit:active {
    transform: scale(0.98);
}

.container {
    max-width: 500px;
    margin: 0 auto;
    padding: 20px;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
}

header {
    text-align: center;
    margin-bottom: 20px;
}

header h1 {
    font-size: 24px;
    font-weight: 600;
    margin-bottom: 8px;
    background: linear-gradient(90deg, #667eea, #764ba2);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
}

.status {
    font-size: 12px;
    padding: 4px 12px;
    border-radius: 12px;
    display: inline-block;
}

.status.connected {
    background: rgba(16, 185, 129, 0.2);
    color: #10b981;
}

.status.disconnected {
    background: rgba(239, 68, 68, 0.2);
    color: #ef4444;
}

.slide-info {
    text-align: center;
    margin-bottom: 15px;
}

.slide-counter {
    font-size: 32px;
    font-weight: 700;
    color: #a5b4fc;
}

.preview-section {
    display: flex;
    flex-direction: column;
    gap: 12px;
    margin-bottom: 20px;
    flex: 1;
}

.preview-card {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 12px;
    padding: 12px;
    border: 1px solid rgba(255, 255, 255, 0.1);
}

.preview-card.current {
    border-color: rgba(102, 126, 234, 0.5);
}

.preview-label {
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #9ca3af;
    margin-bottom: 8px;
}

.preview-content {
    font-size: 14px;
    line-height: 1.5;
    color: #e5e7eb;
    min-height: 60px;
    max-height: 100px;
    overflow-y: auto;
    white-space: pre-wrap;
    word-break: break-word;
}

.controls {
    margin-bottom: 20px;
}

.nav-row {
    display: flex;
    gap: 12px;
    margin-bottom: 12px;
}

button {
    touch-action: manipulation;
}

.nav-btn {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 18px 20px;
    border: none;
    border-radius: 12px;
    font-size: 16px;
    font-weight: 600;
    cursor: pointer;
    background: rgba(255, 255, 255, 0.1);
    color: #ffffff;
    transition: all 0.2s ease;
}

.nav-btn:active {
    transform: scale(0.97);
}

.nav-btn.primary {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.nav-btn .arrow {
    font-size: 20px;
}

.action-row {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 8px;
}

.action-btn {
    padding: 12px 8px;
    border: none;
    border-radius: 8px;
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    background: rgba(255, 255, 255, 0.08);
    color: #d1d5db;
    transition: all 0.2s ease;
}

.action-btn:active {
    transform: scale(0.95);
    background: rgba(255, 255, 255, 0.15);
}

.action-btn.warning {
    background: rgba(239, 68, 68, 0.2);
    color: #fca5a5;
}

.connection-info {
    display: flex;
    justify-content: center;
    gap: 20px;
    padding-top: 15px;
    border-top: 1px solid rgba(255, 255, 255, 0.1);
}

.conn-item {
    font-size: 12px;
    display: flex;
    align-items: center;
    gap: 6px;
}

.conn-label {
    color: #9ca3af;
}

.conn-status {
    padding: 2px 8px;
    border-radius: 4px;
    font-weight: 500;
}

.conn-status.on {
    background: rgba(16, 185, 129, 0.2);
    color: #10b981;
}

.conn-status.off {
    background: rgba(107, 114, 128, 0.2);
    color: #9ca3af;
}

@media (max-height: 600px) {
    .preview-content {
        min-height: 40px;
        max-height: 60px;
    }
    .nav-btn {
        padding: 14px 16px;
    }
}
)CSS";
}

QByteArray RemoteServer::getAppJs()
{
    // Get WebSocket port (HTTP port + 1)
    QString wsPort = QString::number(m_port + 1);

    return QString(R"JS(
let ws = null;
let reconnectTimer = null;
let authenticated = false;

function connect() {
    const wsUrl = 'ws://' + location.hostname + ':%1';
    ws = new WebSocket(wsUrl);

    ws.onopen = function() {
        // Check if PIN is required
        ws.send(JSON.stringify({ type: 'checkAuth' }));
    };

    ws.onclose = function() {
        document.getElementById('status').className = 'status disconnected';
        document.getElementById('status').textContent = 'Disconnected';
        authenticated = false;
        // Reconnect after 2 seconds
        if (!reconnectTimer) {
            reconnectTimer = setTimeout(connect, 2000);
        }
    };

    ws.onerror = function() {
        ws.close();
    };

    ws.onmessage = function(event) {
        const data = JSON.parse(event.data);
        handleMessage(data);
    };
}

function handleMessage(data) {
    // Handle authentication status check response
    if (data.type === 'authStatus') {
        if (data.pinRequired && !data.authenticated) {
            showPinDialog();
        } else {
            hidePinDialog();
            authenticated = true;
            document.getElementById('status').className = 'status connected';
            document.getElementById('status').textContent = 'Connected';
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
        }
        return;
    }

    // Handle authentication response
    if (data.type === 'authResponse') {
        if (data.success) {
            hidePinDialog();
            authenticated = true;
            document.getElementById('status').className = 'status connected';
            document.getElementById('status').textContent = 'Connected';
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
        } else {
            document.getElementById('pinError').style.display = 'block';
            document.getElementById('pinError').textContent = data.error || 'Incorrect PIN';
            document.getElementById('pinInput').value = '';
            document.getElementById('pinInput').focus();
        }
        return;
    }

    // Handle error (e.g., auth required)
    if (data.type === 'error') {
        if (data.error === 'Authentication required') {
            showPinDialog();
        }
        return;
    }

    if (data.type === 'init' || data.type === 'slideUpdate') {
        document.getElementById('currentNum').textContent =
            data.totalSlides > 0 ? (data.currentIndex + 1) : '-';
        document.getElementById('totalNum').textContent =
            data.totalSlides > 0 ? data.totalSlides : '-';
        document.getElementById('currentSlide').textContent =
            data.currentText || '(No slide)';
        document.getElementById('nextSlide').textContent =
            data.nextText || '(End of presentation)';
    }

    if (data.type === 'init' || data.type === 'status') {
        const outputEl = document.getElementById('outputStatus');
        outputEl.textContent = data.outputConnected ? 'Connected' : 'Off';
        outputEl.className = 'conn-status ' + (data.outputConnected ? 'on' : 'off');

        const confEl = document.getElementById('confStatus');
        confEl.textContent = data.confidenceConnected ? 'Connected' : 'Off';
        confEl.className = 'conn-status ' + (data.confidenceConnected ? 'on' : 'off');
    }
}

function showPinDialog() {
    document.getElementById('pinOverlay').style.display = 'flex';
    document.getElementById('mainContent').style.display = 'none';
    document.getElementById('pinInput').focus();
}

function hidePinDialog() {
    document.getElementById('pinOverlay').style.display = 'none';
    document.getElementById('mainContent').style.display = 'flex';
    document.getElementById('pinError').style.display = 'none';
}

function submitPin() {
    const pin = document.getElementById('pinInput').value;
    if (pin && ws && ws.readyState === WebSocket.OPEN) {
        document.getElementById('pinError').style.display = 'none';
        ws.send(JSON.stringify({ type: 'auth', pin: pin }));
    }
}

// Handle Enter key in PIN input
document.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' && document.getElementById('pinOverlay').style.display === 'flex') {
        submitPin();
    }
});

function send(action) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ action: action }));
    }
}

// Use event delegation for button clicks with visual feedback
document.addEventListener('click', function(e) {
    const btn = e.target.closest('button');
    if (btn) {
        btn.style.opacity = '0.7';
        setTimeout(function() { btn.style.opacity = '1'; }, 100);
    }
});

// Start connection
connect();
)JS").arg(wsPort).toUtf8();
}

} // namespace Clarity
