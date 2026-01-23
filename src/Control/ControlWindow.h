#pragma once

#include "Core/Presentation.h"
#include "Core/PresentationModel.h"
#include "Core/IpcServer.h"
#include "ProcessManager.h"
#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QLabel>

namespace Clarity {

/**
 * @brief Main control window for Clarity
 *
 * Provides UI for:
 * - Slide list display
 * - Navigation (prev/next)
 * - Process management (launching output/confidence displays)
 */
class ControlWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ControlWindow(QWidget* parent = nullptr);
    ~ControlWindow();

private slots:
    void onPrevSlide();
    void onNextSlide();
    void onClearOutput();
    void onLaunchOutput();
    void onLaunchConfidence();
    void onSlideClicked(const QModelIndex& index);

    // IPC handlers
    void onClientConnected(QLocalSocket* client);
    void onClientDisconnected(QLocalSocket* client);
    void onMessageReceived(QLocalSocket* client, const QJsonObject& message);

private:
    void setupUI();
    void createDemoPresentation();
    void updateUI();
    void broadcastCurrentSlide();

    // UI components
    QListView* m_slideListView;
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_clearButton;
    QPushButton* m_launchOutputButton;
    QPushButton* m_launchConfidenceButton;
    QLabel* m_statusLabel;

    // Data
    PresentationModel* m_presentationModel;
    IpcServer* m_ipcServer;
    ProcessManager* m_processManager;
};

} // namespace Clarity
