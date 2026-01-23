#pragma once

#include <QObject>
#include <QProcess>
#include <QList>

namespace Clarity {

/**
 * @brief Manages output and confidence display processes
 *
 * Launches and tracks child processes for displays
 */
class ProcessManager : public QObject {
    Q_OBJECT

public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager();

    bool launchOutput();
    bool launchConfidence();

    void terminateAll();

signals:
    void outputStarted();
    void outputFinished(int exitCode);
    void confidenceStarted();
    void confidenceFinished(int exitCode);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QString getExecutablePath() const;

    QList<QProcess*> m_processes;
};

} // namespace Clarity
