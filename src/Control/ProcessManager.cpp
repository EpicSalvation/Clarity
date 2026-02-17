#include "ProcessManager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

namespace Clarity {

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
    , m_settingsManager(nullptr)
    , m_ipcServer(nullptr)
{
}

ProcessManager::~ProcessManager()
{
    // Intentionally empty — detached processes survive controller shutdown.
    // Use quitAll() for intentional shutdown before destruction if desired.
}

void ProcessManager::setSettingsManager(SettingsManager* settingsManager)
{
    m_settingsManager = settingsManager;
}

void ProcessManager::setIpcServer(IpcServer* ipcServer)
{
    m_ipcServer = ipcServer;
}

bool ProcessManager::launchOutput()
{
    // Don't launch a duplicate if one is already connected via IPC
    if (m_ipcServer && m_ipcServer->hasClientType("output")) {
        qDebug() << "ProcessManager: Output client already connected, skipping launch";
        return true;
    }

    QString execPath = getExecutablePath();
    if (execPath.isEmpty()) {
        qWarning() << "ProcessManager: Cannot determine executable path";
        return false;
    }

    QStringList args;
    args << "--output";

    // Add screen index (default to 0 if no settings manager)
    int screenIndex = 0;
    if (m_settingsManager) {
        screenIndex = m_settingsManager->outputScreenIndex();
    }
    args << "--screen" << QString::number(screenIndex);
    qDebug() << "ProcessManager: Launching output on screen" << screenIndex;

    qint64 pid = 0;
    bool started = QProcess::startDetached(execPath, args, QString(), &pid);

    if (!started) {
        qWarning() << "ProcessManager: Failed to start detached output process";
        return false;
    }

    m_pids["output"] = pid;
    emit outputStarted();

    qDebug() << "ProcessManager: Output display launched (detached, PID:" << pid << ")";
    return true;
}

bool ProcessManager::launchConfidence()
{
    // Don't launch a duplicate if one is already connected via IPC
    if (m_ipcServer && m_ipcServer->hasClientType("confidence")) {
        qDebug() << "ProcessManager: Confidence client already connected, skipping launch";
        return true;
    }

    QString execPath = getExecutablePath();
    if (execPath.isEmpty()) {
        qWarning() << "ProcessManager: Cannot determine executable path";
        return false;
    }

    QStringList args;
    args << "--confidence";

    // Add screen index (default to 0 if no settings manager)
    int screenIndex = 0;
    if (m_settingsManager) {
        screenIndex = m_settingsManager->confidenceScreenIndex();
    }
    args << "--screen" << QString::number(screenIndex);
    qDebug() << "ProcessManager: Launching confidence on screen" << screenIndex;

    qint64 pid = 0;
    bool started = QProcess::startDetached(execPath, args, QString(), &pid);

    if (!started) {
        qWarning() << "ProcessManager: Failed to start detached confidence process";
        return false;
    }

    m_pids["confidence"] = pid;
    emit confidenceStarted();

    qDebug() << "ProcessManager: Confidence monitor launched (detached, PID:" << pid << ")";
    return true;
}

bool ProcessManager::launchNdi()
{
    // Don't launch a duplicate if one is already connected via IPC
    if (m_ipcServer && m_ipcServer->hasClientType("ndi")) {
        qDebug() << "ProcessManager: NDI client already connected, skipping launch";
        return true;
    }

    QString execPath = getExecutablePath();
    if (execPath.isEmpty()) {
        qWarning() << "ProcessManager: Cannot determine executable path";
        return false;
    }

    QStringList args;
    args << "--ndi";
    qDebug() << "ProcessManager: Launching NDI output";

    qint64 pid = 0;
    bool started = QProcess::startDetached(execPath, args, QString(), &pid);

    if (!started) {
        qWarning() << "ProcessManager: Failed to start detached NDI process";
        return false;
    }

    m_pids["ndi"] = pid;
    emit ndiStarted();

    qDebug() << "ProcessManager: NDI output launched (detached, PID:" << pid << ")";
    return true;
}

void ProcessManager::quitAll()
{
    if (!m_ipcServer) {
        qWarning() << "ProcessManager: No IPC server, cannot send quit messages";
        return;
    }

    QJsonObject message;
    message["type"] = "quit";

    QStringList types = {"output", "confidence", "ndi"};
    for (const QString& type : types) {
        if (m_ipcServer->hasClientType(type)) {
            m_ipcServer->sendToClientType(type, message);
            qDebug() << "ProcessManager: Sent quit to" << type;
        }
    }

    m_pids.clear();
}

void ProcessManager::quitByType(const QString& clientType)
{
    if (!m_ipcServer) {
        qWarning() << "ProcessManager: No IPC server, cannot send quit message";
        return;
    }

    QJsonObject message;
    message["type"] = "quit";

    if (m_ipcServer->sendToClientType(clientType, message)) {
        qDebug() << "ProcessManager: Sent quit to" << clientType;
    } else {
        qDebug() << "ProcessManager: No" << clientType << "client connected to quit";
    }

    m_pids.remove(clientType);
}

QString ProcessManager::getExecutablePath() const
{
    // Get the path to the current executable
    QString execPath = QCoreApplication::applicationFilePath();

    // Verify it exists
    QFileInfo fileInfo(execPath);
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        qWarning() << "ProcessManager: Executable not found or not executable:" << execPath;
        return QString();
    }

    return execPath;
}

} // namespace Clarity
