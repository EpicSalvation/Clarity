#pragma once

#include "Core/Presentation.h"
#include "Core/PresentationModel.h"
#include "Core/IpcServer.h"
#include "Core/SettingsManager.h"
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

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File operations
    void newPresentation();
    void openPresentation();
    void savePresentation();
    void saveAsPresentation();

    // Navigation and control
    void onPrevSlide();
    void onNextSlide();
    void onClearOutput();
    void onLaunchOutput();
    void onLaunchConfidence();
    void onSlideClicked(const QModelIndex& index);
    void onSlideDoubleClicked(const QModelIndex& index);
    void onSettings();
    void onPresentationModified();

    // Slide editing
    void onAddSlide();
    void onEditSlide();
    void onDeleteSlide();
    void onMoveSlideUp();
    void onMoveSlideDown();

    // Timer controls
    void onStartTimer();
    void onPauseTimer();
    void onResetTimer();

    // IPC handlers
    void onClientConnected(QLocalSocket* client);
    void onClientDisconnected(QLocalSocket* client);
    void onMessageReceived(QLocalSocket* client, const QJsonObject& message);

private:
    void setupUI();
    void createDemoPresentation();
    void updateUI();
    void broadcastCurrentSlide();
    void updateWindowTitle();
    bool promptSaveIfDirty();
    void markDirty();
    void markClean();

    // UI components
    QListView* m_slideListView;
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_clearButton;
    QPushButton* m_launchOutputButton;
    QPushButton* m_launchConfidenceButton;
    QPushButton* m_settingsButton;
    QPushButton* m_addSlideButton;
    QPushButton* m_editSlideButton;
    QPushButton* m_deleteSlideButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    QPushButton* m_timerStartButton;
    QPushButton* m_timerPauseButton;
    QPushButton* m_timerResetButton;
    QLabel* m_statusLabel;

    // Data
    PresentationModel* m_presentationModel;
    IpcServer* m_ipcServer;
    ProcessManager* m_processManager;
    SettingsManager* m_settingsManager;

    // File management
    QString m_currentFilePath;
    bool m_isDirty;
};

} // namespace Clarity
