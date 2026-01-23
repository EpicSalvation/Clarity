#include "ProcessManager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

namespace Clarity {

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
{
}

ProcessManager::~ProcessManager()
{
    terminateAll();
}

bool ProcessManager::launchOutput()
{
    QString execPath = getExecutablePath();
    if (execPath.isEmpty()) {
        qWarning() << "ProcessManager: Cannot determine executable path";
        return false;
    }

    QProcess* process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &ProcessManager::onProcessError);

    QStringList args;
    args << "--output";

    process->setProgram(execPath);
    process->setArguments(args);
    process->start();

    if (!process->waitForStarted(3000)) {
        qWarning() << "ProcessManager: Failed to start output process:" << process->errorString();
        process->deleteLater();
        return false;
    }

    m_processes.append(process);
    emit outputStarted();

    qDebug() << "ProcessManager: Output display launched";
    return true;
}

bool ProcessManager::launchConfidence()
{
    QString execPath = getExecutablePath();
    if (execPath.isEmpty()) {
        qWarning() << "ProcessManager: Cannot determine executable path";
        return false;
    }

    QProcess* process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &ProcessManager::onProcessError);

    QStringList args;
    args << "--confidence";

    process->setProgram(execPath);
    process->setArguments(args);
    process->start();

    if (!process->waitForStarted(3000)) {
        qWarning() << "ProcessManager: Failed to start confidence process:" << process->errorString();
        process->deleteLater();
        return false;
    }

    m_processes.append(process);
    emit confidenceStarted();

    qDebug() << "ProcessManager: Confidence monitor launched";
    return true;
}

void ProcessManager::terminateAll()
{
    for (QProcess* process : m_processes) {
        if (process->state() == QProcess::Running) {
            process->terminate();
            if (!process->waitForFinished(3000)) {
                process->kill();
            }
        }
        process->deleteLater();
    }
    m_processes.clear();
}

void ProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }

    QString status = (exitStatus == QProcess::NormalExit) ? "normally" : "crashed";
    qDebug() << "ProcessManager: Process finished" << status << "with exit code" << exitCode;

    m_processes.removeOne(process);
    process->deleteLater();
}

void ProcessManager::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }

    qWarning() << "ProcessManager: Process error:" << error << process->errorString();
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
