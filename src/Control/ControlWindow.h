#pragma once

#include "Core/Presentation.h"
#include "Core/PresentationModel.h"
#include "Core/ItemListModel.h"
#include "Core/SlideFilterProxyModel.h"
#include "Core/IpcServer.h"
#include "Core/SettingsManager.h"
#include "Core/BibleDatabase.h"
#include "Core/SongLibrary.h"
#include <QSet>
#include "Core/ThemeManager.h"
#include "Core/MediaLibrary.h"
#include "Core/VideoThumbnailGenerator.h"
#include "Core/RemoteServer.h"
#include "ProcessManager.h"
#include "SlideGridDelegate.h"
#include "SlideGridView.h"
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
    bool eventFilter(QObject* watched, QEvent* event) override;

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
    void onSlideClicked(const QModelIndex& index);
    void onSlideDoubleClicked(const QModelIndex& index);
    void onItemClicked(const QModelIndex& index);
    void onItemDoubleClicked(const QModelIndex& index);
    void onSettings();
    void onPresentationModified();

    // Slide editing
    void onAddSlide();
    void onEditSlide();
    void onDeleteSlide();
    void onMoveSlideUp();
    void onMoveSlideDown();
    void onSlideContextMenu(const QPoint& pos);

    // Section operations (songs)
    void onDuplicateSection();
    void onDeleteSection();

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
    void toggleOutputDisplay();
    void toggleOutputFullscreen();
    void toggleConfidenceMonitor();

    // Help
    void showKeyboardShortcuts();

    // Theme operations
    void onApplyTheme();
    void onApplyThemeToSlide();
    void onApplyThemeToGroup();
    void onCloneFormatToGroup();
    void onManageThemes();

    // Timer controls
    void onStartTimer();
    void onPauseTimer();
    void onResetTimer();

    // Remote control
    void onRemoteNavigation(const QString& action);
    void updateRemoteServer();
    void showQrCode();

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
    void updatePreviewStates();
    void applySlidePreviewSize(const QString& size);

    // UI components
    QListView* m_slideListView;              ///< Left panel list view (playlist)
    SlideGridView* m_slideGridView;           ///< Center grid view for slide thumbnails
    SlideGridDelegate* m_slideDelegate;      ///< Custom delegate for grid rendering
    LivePreviewPanel* m_livePreviewPanel;    ///< Right panel live preview
    QPushButton* m_addSlideButton;
    QPushButton* m_deleteSlideButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    QLabel* m_statusLabel;

    // Data
    PresentationModel* m_presentationModel;
    ItemListModel* m_itemListModel;
    SlideFilterProxyModel* m_slideFilterProxy;  ///< Filters slides by current item
    IpcServer* m_ipcServer;
    ProcessManager* m_processManager;
    SettingsManager* m_settingsManager;
    BibleDatabase* m_bibleDatabase;
    SongLibrary* m_songLibrary;
    ThemeManager* m_themeManager;
    MediaLibrary* m_mediaLibrary;
    VideoThumbnailGenerator* m_thumbnailGenerator;
    RemoteServer* m_remoteServer;
    QLabel* m_remoteStatusLabel;

    // File management
    QString m_currentFilePath;
    bool m_isDirty;

    // Display state tracking for toggle shortcuts
    bool m_isBlackout = false;
    bool m_isWhiteout = false;
    // Display visibility tracking for preview borders
    bool m_outputVisible = false;       ///< Whether output display is visible (not just connected)
    bool m_confidenceVisible = false;   ///< Whether confidence monitor is visible (not just connected)

    // Song usage tracking (prevents duplicate records per session)
    QSet<int> m_recordedSongUsage;  ///< Song IDs that have been recorded this session
};

} // namespace Clarity
