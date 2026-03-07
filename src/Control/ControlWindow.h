// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Presentation.h"
#include "Core/PresentationModel.h"
#include "Core/Slide.h"
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
#include "Core/SlideGroupLibrary.h"
#include "Core/RemoteServer.h"
#include "Core/AutoAdvanceTimer.h"
#include "Core/UndoManager.h"
#include "Core/EsvApiClient.h"
#include "Core/ApiBibleClient.h"
#include "ProcessManager.h"
#include "SlideGridDelegate.h"
#include "SlideGridView.h"
#include "LivePreviewPanel.h"
#include "MediaDrawer.h"
#include "StartupWidget.h"
#include <QMainWindow>
#include <QListView>
#include <QSplitter>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QShortcut>
#include <QToolBar>

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
    void onPurgeEsvCache();
    void onInsertSong();
    void onInsertSlideGroup();
    void onInsertSlideGroupFromLibrary(int groupId);
    void onImportPowerPoint();
    void onImportSlideImages();
    void onSaveCurrentGroupToLibrary();
    void onAddSlideToGroup();
    void onDuplicateSlideInGroup();
    void onUpdateLibraryGroup();

    // Navigation shortcuts
    void gotoFirstSlide();
    void gotoLastSlide();
    void gotoSlide(int index);
    void promptGotoSlide();

    // Display control shortcuts
    void blackScreen();
    void whiteScreen();
    void clearText();
    void clearBackground();
    void toggleOutputDisplay();
    void toggleOutputFullscreen();
    void toggleConfidenceMonitor();
    void toggleNdiOutput();

    // View
    void toggleMediaDrawer();

    // Help
    void showKeyboardShortcuts();
    void showAbout();
    void startMainTour();

    // Media drag-and-drop
    void onMediaDroppedOnSlide(const QModelIndex& proxyIndex, const QString& path,
                               const QString& mediaType, bool applyToGroup);

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

    // Auto-advance
    void onAutoAdvanceExpired();
    void toggleAutoAdvancePause();
    void toggleAutoAdvanceEnabled();
    void onSetSlideAutoAdvance();

    // Remote control
    void onRemoteNavigation(const QString& action);
    void updateRemoteServer();
    void showQrCode();

    // Undo/redo
    void onUndo();
    void onRedo();
    void saveUndoSnapshot(const QString& description);

    // IPC handlers
    void onClientConnected(QLocalSocket* client);
    void onClientDisconnected(QLocalSocket* client);
    void onMessageReceived(QLocalSocket* client, const QJsonObject& message);

private:
    void setupUI();
    void setupShortcuts();
    void addSlideWithTemplate(SlideTemplate tmpl); ///< Open editor pre-loaded with template, then insert
    void initializeBibleDatabase();
    void updateUI();
    void broadcastCurrentSlide();
    void updateWindowTitle();
    bool promptSaveIfDirty();
    void markDirty();
    void markClean();
    void updatePreviewStates();
    void applySlidePreviewSize(const QString& size);
    void startAutoAdvanceForCurrentSlide();
    void broadcastAutoAdvanceState();
    void autoSyncCurrentGroupToLibrary();
    void restoreFromSnapshot(const QJsonObject& snapshot);
    void showStartupScreen();
    void showEditingUI();
    void openFile(const QString& filePath);
    void closePresentation();
    void updateMenuStates();

    // UI components
    QToolBar* m_insertToolBar;               ///< Quick-access toolbar for common inserts
    QList<QAction*> m_insertActions;         ///< Insert actions that toggle with presentation state
    QAction* m_settingsAction;              ///< Settings toolbar action (always enabled)
    QStackedWidget* m_stackedWidget;         ///< Switches between startup and editing pages
    StartupWidget* m_startupWidget;          ///< Welcome/startup screen (page 0)
    QWidget* m_editingWidget;                ///< Editing UI container (page 1)
    QListView* m_slideListView;              ///< Left panel list view (playlist)
    SlideGridView* m_slideGridView;           ///< Center grid view for slide thumbnails
    SlideGridDelegate* m_slideDelegate;      ///< Custom delegate for grid rendering
    LivePreviewPanel* m_livePreviewPanel;    ///< Right panel live preview
    MediaDrawer* m_mediaDrawer;              ///< Bottom media library drawer
    QPushButton* m_addSlideToGroupBtn;       ///< '+' button for adding slides to a group
    QSplitter* m_mainSplitter;               ///< Vertical splitter for content + drawer
    QAction* m_toggleMediaDrawerAction;      ///< View menu toggle action
    QPushButton* m_addSlideButton;
    QPushButton* m_deleteSlideButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    QLabel* m_statusLabel;

    // Menu/action pointers for state management
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_closeAction;
    QMenu* m_editMenu;
    QMenu* m_slideMenu;
    QMenu* m_viewMenu;
    QMenu* m_formatMenu;

    // Data
    PresentationModel* m_presentationModel;
    ItemListModel* m_itemListModel;
    SlideFilterProxyModel* m_slideFilterProxy;  ///< Filters slides by current item
    IpcServer* m_ipcServer;
    ProcessManager* m_processManager;
    SettingsManager* m_settingsManager;
    BibleDatabase* m_bibleDatabase;
    SongLibrary* m_songLibrary;
    SlideGroupLibrary* m_slideGroupLibrary;
    ThemeManager* m_themeManager;
    MediaLibrary* m_mediaLibrary;
    VideoThumbnailGenerator* m_thumbnailGenerator;
    RemoteServer* m_remoteServer;
    AutoAdvanceTimer* m_autoAdvanceTimer;
    UndoManager* m_undoManager;
    EsvApiClient* m_esvApiClient;
    ApiBibleClient* m_apiBibleClient;
    QAction* m_undoAction;
    QAction* m_redoAction;
    QLabel* m_remoteStatusLabel;

    // File management
    QString m_currentFilePath;
    bool m_isDirty;
    bool m_hasPresentation = false;

    // Display state tracking for toggle shortcuts
    bool m_isBlackout = false;
    bool m_isWhiteout = false;
    bool m_isTextCleared = false;
    bool m_isBackgroundCleared = false;
    // Display visibility tracking for preview borders
    bool m_outputVisible = false;       ///< Whether output display is visible (not just connected)
    bool m_confidenceVisible = false;   ///< Whether confidence monitor is visible (not just connected)
    bool m_ndiVisible = false;          ///< Whether NDI output is running

    // Media drawer state
    int m_drawerExpandedHeight = 200;  ///< Remembered drawer height when expanded

    // Song usage tracking (prevents duplicate records per session)
    QSet<int> m_recordedSongUsage;  ///< Song IDs that have been recorded this session
};

} // namespace Clarity
