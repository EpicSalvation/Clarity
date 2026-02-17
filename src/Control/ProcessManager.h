#pragma once

#include "Core/SettingsManager.h"
#include "Core/IpcServer.h"
#include <QObject>
#include <QProcess>
#include <QMap>

namespace Clarity {

/**
 * @brief Manages output and confidence display processes
 *
 * Launches display processes as detached OS processes so they survive
 * controller shutdown or crash. Uses IPC "quit" messages for intentional
 * shutdown rather than OS-level process termination.
 */
class ProcessManager : public QObject {
    Q_OBJECT

public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager();

    void setSettingsManager(SettingsManager* settingsManager);
    void setIpcServer(IpcServer* ipcServer);

    bool launchOutput();
    bool launchConfidence();
    bool launchNdi();

    /**
     * @brief Gracefully stop all running display processes via IPC "quit" message.
     *
     * Only sends IPC quit messages — does NOT use OS-level kill.
     * If a process has no IPC connection, it is left running.
     */
    void quitAll();

    /**
     * @brief Gracefully stop a specific process type via IPC "quit" message.
     * @param clientType The type of client to stop ("output", "confidence", "ndi")
     */
    void quitByType(const QString& clientType);

signals:
    void outputStarted();
    void outputFinished(int exitCode);
    void confidenceStarted();
    void confidenceFinished(int exitCode);
    void ndiStarted();
    void ndiFinished(int exitCode);

private:
    QString getExecutablePath() const;

    QMap<QString, qint64> m_pids;  // Maps client type to PID
    SettingsManager* m_settingsManager;
    IpcServer* m_ipcServer;
};

} // namespace Clarity
