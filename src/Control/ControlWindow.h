#pragma once

#include "Core/Presentation.h"
#include "Core/PresentationModel.h"
#include "Core/IpcServer.h"
#include "Core/SettingsManager.h"
#include "Core/BibleDatabase.h"
#include "Core/SongLibrary.h"
#include "Core/ThemeManager.h"
#include "Core/MediaLibrary.h"
#include "Core/VideoThumbnailGenerator.h"
#include "ProcessManager.h"
#include "SlideGridDelegate.h"
#include "LivePreviewPanel.h"
#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QShortcut>

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

    // Content insertion
    void onInsertScripture();
    void onInsertSong();

    // Navigation shortcuts
    void gotoFirstSlide();
    void gotoLastSlide();
    void gotoSlide(int index);
    void promptGotoSlide();

    // Display control shortcuts
    void blackScreen();
    void whiteScreen();
    void onOutputDisabledToggled(bool disabled);
    void toggleOutputDisplay();
    void toggleOutputFullscreen();
    void toggleConfidenceMonitor();

    // Help
    void showKeyboardShortcuts();

    // Theme operations
    void onApplyTheme();
    void onApplyThemeToSlide();
    void onManageThemes();

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
    void setupShortcuts();
    void createDemoPresentation();
    void initializeBibleDatabase();
    void updateUI();
    void broadcastCurrentSlide();
    void updateWindowTitle();
    bool promptSaveIfDirty();
    void markDirty();
    void markClean();

    // UI components
    QListView* m_slideListView;              ///< Left panel list view (playlist)
    QListView* m_slideGridView;              ///< Center grid view for slide thumbnails
    SlideGridDelegate* m_slideDelegate;      ///< Custom delegate for grid rendering
    LivePreviewPanel* m_livePreviewPanel;    ///< Right panel live preview
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_clearButton;
    QPushButton* m_outputDisabledButton;  ///< Toggle to disable output display
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
    BibleDatabase* m_bibleDatabase;
    SongLibrary* m_songLibrary;
    ThemeManager* m_themeManager;
    MediaLibrary* m_mediaLibrary;
    VideoThumbnailGenerator* m_thumbnailGenerator;

    // File management
    QString m_currentFilePath;
    bool m_isDirty;

    // Display state tracking for toggle shortcuts
    bool m_isBlackout = false;
    bool m_isWhiteout = false;
    bool m_isOutputDisabled = false;  ///< Persistent output disable (survives navigation)
};

} // namespace Clarity
