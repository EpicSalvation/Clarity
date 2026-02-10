#include "ControlWindow.h"
#include "SettingsDialog.h"
#include "SlideEditorDialog.h"
#include "ScriptureDialog.h"
#include "SongLibraryDialog.h"
#include "ThemeSelectorDialog.h"
#include "Core/SlidePreviewRenderer.h"
#include "Core/PresentationItem.h"
#include "Core/SongItem.h"
#include "Core/ScriptureItem.h"
#include "Core/CustomSlideItem.h"
#include "Core/SlideGroupItem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QShortcut>
#include <QInputDialog>
#include <QDialog>
#include <QPixmap>
#include <QBuffer>
#include <QDebug>

namespace Clarity {

ControlWindow::ControlWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_slideListView(nullptr)
    , m_slideGridView(nullptr)
    , m_slideDelegate(nullptr)
    , m_livePreviewPanel(nullptr)
    , m_mediaDrawer(nullptr)
    , m_mainSplitter(nullptr)
    , m_toggleMediaDrawerAction(nullptr)
    , m_addSlideButton(nullptr)
    , m_deleteSlideButton(nullptr)
    , m_moveUpButton(nullptr)
    , m_moveDownButton(nullptr)
    , m_statusLabel(nullptr)
    , m_presentationModel(new PresentationModel(this))
    , m_itemListModel(new ItemListModel(this))
    , m_slideFilterProxy(new SlideFilterProxyModel(this))
    , m_ipcServer(new IpcServer(this))
    , m_processManager(new ProcessManager(this))
    , m_settingsManager(new SettingsManager(this))
    , m_bibleDatabase(new BibleDatabase(this))
    , m_songLibrary(new SongLibrary(this))
    , m_themeManager(new ThemeManager(this))
    , m_mediaLibrary(new MediaLibrary(this))
    , m_thumbnailGenerator(new VideoThumbnailGenerator(this))
    , m_remoteServer(new RemoteServer(8080, this))
    , m_autoAdvanceTimer(new AutoAdvanceTimer(this))
    , m_remoteStatusLabel(nullptr)
    , m_currentFilePath("")
    , m_isDirty(false)
{
    setupUI();
    setupShortcuts();

    // Set up video thumbnail generator for slide previews
    SlidePreviewRenderer::setVideoThumbnailGenerator(m_thumbnailGenerator);

    // Refresh slide views when video thumbnails are generated
    connect(m_thumbnailGenerator, &VideoThumbnailGenerator::thumbnailReady,
            this, [this](const QString& /*videoPath*/, const QImage& /*thumbnail*/) {
        // Invalidate cached thumbnails and repaint
        m_slideDelegate->invalidateCache();
        m_slideGridView->viewport()->update();
        m_slideListView->viewport()->update();
    });

    createDemoPresentation();

    // Initialize Bible database
    initializeBibleDatabase();

    // Load song library
    m_songLibrary->loadLibrary();

    // Pass settings manager to process manager
    m_processManager->setSettingsManager(m_settingsManager);

    // Start IPC server
    if (m_ipcServer->start()) {
        m_statusLabel->setText(tr("IPC Server: Running"));
    } else {
        m_statusLabel->setText(tr("IPC Server: Failed to start"));
    }

    // Connect IPC signals
    connect(m_ipcServer, &IpcServer::clientConnected, this, &ControlWindow::onClientConnected);
    connect(m_ipcServer, &IpcServer::clientDisconnected, this, &ControlWindow::onClientDisconnected);
    connect(m_ipcServer, &IpcServer::messageReceived, this, &ControlWindow::onMessageReceived);

    // Start remote control server if enabled
    if (m_settingsManager->remoteControlEnabled()) {
        m_remoteServer->setPort(m_settingsManager->remoteControlPort());
        m_remoteServer->setPin(m_settingsManager->remoteControlPinEnabled(),
                               m_settingsManager->remoteControlPin());
        if (m_remoteServer->start()) {
            qDebug() << "Remote control server started at" << m_remoteServer->serverUrl();
            m_remoteStatusLabel->setText(tr("Remote: %1").arg(m_remoteServer->serverUrl()));
        }
    }

    // Connect remote server signals
    connect(m_remoteServer, &RemoteServer::navigationRequested,
            this, &ControlWindow::onRemoteNavigation);
    connect(m_remoteServer, &RemoteServer::clientConnected, this, [this]() {
        updateRemoteServer();
        // Update status label with client count
        int clients = m_remoteServer->connectedClientCount();
        m_remoteStatusLabel->setText(tr("Remote: %1 (%n client(s))", "", clients)
            .arg(m_remoteServer->serverUrl()));
    });
    connect(m_remoteServer, &RemoteServer::clientDisconnected, this, [this]() {
        int clients = m_remoteServer->connectedClientCount();
        if (clients > 0) {
            m_remoteStatusLabel->setText(tr("Remote: %1 (%n client(s))", "", clients)
                .arg(m_remoteServer->serverUrl()));
        } else {
            m_remoteStatusLabel->setText(tr("Remote: %1").arg(m_remoteServer->serverUrl()));
        }
    });
    connect(m_remoteServer, &RemoteServer::runningChanged, this, [this](bool running) {
        if (running) {
            m_remoteStatusLabel->setText(tr("Remote: %1").arg(m_remoteServer->serverUrl()));
        } else {
            m_remoteStatusLabel->setText("");
        }
    });

    // Update remote server settings when they change
    connect(m_settingsManager, &SettingsManager::remoteControlSettingsChanged, this, [this]() {
        bool shouldBeRunning = m_settingsManager->remoteControlEnabled();
        quint16 newPort = m_settingsManager->remoteControlPort();
        bool isRunning = m_remoteServer->isRunning();
        quint16 currentPort = m_remoteServer->port();

        // Handle enable/disable and port changes
        if (shouldBeRunning) {
            if (!isRunning) {
                // Start the server
                m_remoteServer->setPort(newPort);
                m_remoteServer->setPin(m_settingsManager->remoteControlPinEnabled(),
                                       m_settingsManager->remoteControlPin());
                if (m_remoteServer->start()) {
                    qDebug() << "Remote control server started at" << m_remoteServer->serverUrl();
                    m_remoteStatusLabel->setText(tr("Remote: %1").arg(m_remoteServer->serverUrl()));
                }
            } else if (currentPort != newPort) {
                // Port changed - restart on new port
                m_remoteServer->stop();
                m_remoteServer->setPort(newPort);
                m_remoteServer->setPin(m_settingsManager->remoteControlPinEnabled(),
                                       m_settingsManager->remoteControlPin());
                if (m_remoteServer->start()) {
                    qDebug() << "Remote control server restarted at" << m_remoteServer->serverUrl();
                    m_remoteStatusLabel->setText(tr("Remote: %1").arg(m_remoteServer->serverUrl()));
                }
            } else {
                // Just update PIN settings
                m_remoteServer->setPin(m_settingsManager->remoteControlPinEnabled(),
                                       m_settingsManager->remoteControlPin());
            }
        } else if (isRunning) {
            // Stop the server
            m_remoteServer->stop();
            m_remoteStatusLabel->setText("");
            qDebug() << "Remote control server stopped";
        }
    });

    // Connect auto-advance timer
    connect(m_autoAdvanceTimer, &AutoAdvanceTimer::expired, this, &ControlWindow::onAutoAdvanceExpired);
    connect(m_autoAdvanceTimer, &AutoAdvanceTimer::tick, this, [this](int seconds) {
        m_livePreviewPanel->setAutoAdvanceCountdown(seconds, m_autoAdvanceTimer->totalDuration());
        broadcastAutoAdvanceState();
    });
    connect(m_autoAdvanceTimer, &AutoAdvanceTimer::stateChanged, this, [this]() {
        m_livePreviewPanel->setAutoAdvanceActive(m_autoAdvanceTimer->isActive());
        m_livePreviewPanel->setAutoAdvancePaused(m_autoAdvanceTimer->isPaused());
        if (!m_autoAdvanceTimer->isActive()) {
            m_livePreviewPanel->setAutoAdvanceCountdown(0, 0);
        }
        broadcastAutoAdvanceState();
    });

    // Connect presentation modification signal
    connect(m_presentationModel, &PresentationModel::presentationModified, this, &ControlWindow::onPresentationModified);

    // Update UI after drag-drop reorder in playlist
    connect(m_presentationModel, &PresentationModel::itemsChanged, this, [this]() {
        m_slideDelegate->invalidateCache();
        updateUI();
        broadcastCurrentSlide();
    });

    // Invalidate thumbnail cache when model data changes
    connect(m_presentationModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
            m_slideDelegate->invalidateCacheFor(i);
        }
        m_slideGridView->viewport()->update();
    });
    connect(m_presentationModel, &QAbstractItemModel::modelReset, this, [this]() {
        m_slideDelegate->invalidateCache();
    });
    connect(m_presentationModel, &QAbstractItemModel::rowsMoved, this, [this]() {
        m_slideDelegate->invalidateCache();
        m_slideGridView->viewport()->update();
    });

    // Broadcast settings changes to confidence monitor
    connect(m_settingsManager, &SettingsManager::confidenceDisplaySettingsChanged, this, [this]() {
        QJsonObject message;
        message["type"] = "settingsChanged";
        m_ipcServer->sendToClientType("confidence", message);
    });

    // Handle slide grid mode changes from settings
    connect(m_settingsManager, &SettingsManager::slideGridModeChanged, this, [this](bool showAll) {
        m_slideFilterProxy->setShowAllSlides(showAll);

        if (!showAll) {
            // When switching to item-only mode, set the filter to the current item
            int currentIndex = m_presentationModel->currentSlideIndex();
            int itemIndex = m_itemListModel->itemIndexForSlide(currentIndex);
            if (itemIndex >= 0) {
                m_slideFilterProxy->setFilterItemIndex(itemIndex);
            }
        }

        // Invalidate the delegate cache and refresh the view
        m_slideDelegate->invalidateCache();
        m_slideGridView->viewport()->update();
        updateUI();
    });

    // Handle slide preview size changes from settings
    connect(m_settingsManager, &SettingsManager::slidePreviewSizeChanged, this, [this](const QString& size) {
        applySlidePreviewSize(size);
    });

    updateUI();
    updateWindowTitle();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::setupUI()
{
    setWindowTitle(tr("Clarity - Control"));
    resize(1000, 700);  // Larger default size for new layout

    // Menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* fileMenu = menuBar->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &ControlWindow::newPresentation);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &ControlWindow::openPresentation);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &ControlWindow::savePresentation);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &ControlWindow::saveAsPresentation);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Settings..."), this, &ControlWindow::onSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    // Slide menu
    QMenu* slideMenu = menuBar->addMenu(tr("&Slide"));
    slideMenu->addAction(tr("&Add Slide"), QKeySequence("Ctrl+Shift+N"), this, &ControlWindow::onAddSlide);
    slideMenu->addAction(tr("&Edit Slide"), QKeySequence("Ctrl+E"), this, &ControlWindow::onEditSlide);
    slideMenu->addAction(tr("&Delete Slide"), QKeySequence::Delete, this, &ControlWindow::onDeleteSlide);
    slideMenu->addSeparator();
    slideMenu->addAction(tr("Insert &Scripture..."), QKeySequence("Ctrl+B"), this, &ControlWindow::onInsertScripture);
    slideMenu->addAction(tr("Insert S&ong..."), QKeySequence("Ctrl+L"), this, &ControlWindow::onInsertSong);
    slideMenu->addSeparator();
    slideMenu->addAction(tr("Apply &Theme..."), QKeySequence("Ctrl+T"), this, &ControlWindow::onApplyTheme);
    slideMenu->addAction(tr("Apply Theme to Current Slide..."), this, &ControlWindow::onApplyThemeToSlide);
    slideMenu->addAction(tr("Apply Theme to &Group..."), QKeySequence("Ctrl+Shift+T"), this, &ControlWindow::onApplyThemeToGroup);
    slideMenu->addAction(tr("Clone &Format to Group"), QKeySequence("Ctrl+Shift+F"), this, &ControlWindow::onCloneFormatToGroup);

    // View menu
    QMenu* viewMenu = menuBar->addMenu(tr("&View"));
    m_toggleMediaDrawerAction = viewMenu->addAction(tr("&Media Library"), QKeySequence("Ctrl+M"),
                                                     this, &ControlWindow::toggleMediaDrawer);
    m_toggleMediaDrawerAction->setCheckable(true);
    m_toggleMediaDrawerAction->setChecked(false);

    // Format menu (for theme management)
    QMenu* formatMenu = menuBar->addMenu(tr("F&ormat"));
    formatMenu->addAction(tr("&Manage Themes..."), this, &ControlWindow::onManageThemes);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&Keyboard Shortcuts..."), QKeySequence("F1"), this, &ControlWindow::showKeyboardShortcuts);

    setMenuBar(menuBar);

    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout - vertical with content area on top and buttons at bottom
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Content area: horizontal layout with list on left, grid in center, preview on right
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Left panel: Item list with add/remove/reorder buttons at the bottom
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(180);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(2);

    m_slideListView = new QListView(this);
    m_slideListView->setModel(m_itemListModel);
    m_slideListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_slideListView->setDragEnabled(true);
    m_slideListView->setAcceptDrops(true);
    m_slideListView->setDropIndicatorShown(true);
    m_slideListView->setDragDropMode(QAbstractItemView::InternalMove);
    m_slideListView->setDefaultDropAction(Qt::MoveAction);
    connect(m_slideListView, &QListView::clicked, this, &ControlWindow::onItemClicked);
    connect(m_slideListView, &QListView::doubleClicked, this, &ControlWindow::onItemDoubleClicked);
    leftLayout->addWidget(m_slideListView);

    // Compact playlist buttons (+, -, up, down)
    QHBoxLayout* playlistButtonLayout = new QHBoxLayout();
    playlistButtonLayout->setContentsMargins(0, 0, 0, 0);
    playlistButtonLayout->setSpacing(2);

    m_addSlideButton = new QPushButton("+", this);
    m_deleteSlideButton = new QPushButton(QStringLiteral("\u2212"), this);  // minus sign
    m_moveUpButton = new QPushButton(QStringLiteral("\u25B2"), this);      // up triangle
    m_moveDownButton = new QPushButton(QStringLiteral("\u25BC"), this);    // down triangle

    // Make buttons compact
    for (QPushButton* btn : {m_addSlideButton, m_deleteSlideButton, m_moveUpButton, m_moveDownButton}) {
        btn->setFixedSize(36, 28);
        btn->setToolTip("");
    }
    m_addSlideButton->setToolTip(tr("Add slide (Ctrl+Shift+N)"));
    m_deleteSlideButton->setToolTip(tr("Delete slide (Delete)"));
    m_moveUpButton->setToolTip(tr("Move slide up (Ctrl+Up)"));
    m_moveDownButton->setToolTip(tr("Move slide down (Ctrl+Down)"));

    connect(m_addSlideButton, &QPushButton::clicked, this, &ControlWindow::onAddSlide);
    connect(m_deleteSlideButton, &QPushButton::clicked, this, &ControlWindow::onDeleteSlide);
    connect(m_moveUpButton, &QPushButton::clicked, this, &ControlWindow::onMoveSlideUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &ControlWindow::onMoveSlideDown);

    playlistButtonLayout->addWidget(m_addSlideButton);
    playlistButtonLayout->addWidget(m_deleteSlideButton);
    playlistButtonLayout->addStretch();
    playlistButtonLayout->addWidget(m_moveUpButton);
    playlistButtonLayout->addWidget(m_moveDownButton);

    leftLayout->addLayout(playlistButtonLayout);
    contentLayout->addWidget(leftPanel);

    // Center panel: Slide grid view (custom subclass for insert-between drag-and-drop)
    m_slideGridView = new SlideGridView(this);

    // Set up proxy model for filtering slides by item
    m_slideFilterProxy->setSourceModel(m_presentationModel);
    m_slideFilterProxy->setShowAllSlides(m_settingsManager->showAllSlidesInGrid());
    m_slideGridView->setModel(m_slideFilterProxy);

    // Configure for grid/icon mode
    m_slideGridView->setViewMode(QListView::IconMode);
    m_slideGridView->setGridSize(QSize(180, 120));  // Thumbnail size + spacing
    m_slideGridView->setResizeMode(QListView::Adjust);
    m_slideGridView->setWrapping(true);
    m_slideGridView->setFlow(QListView::LeftToRight);
    m_slideGridView->setSpacing(10);
    m_slideGridView->setUniformItemSizes(true);
    m_slideGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_slideGridView->setDragEnabled(true);
    m_slideGridView->setAcceptDrops(true);
    m_slideGridView->setDropIndicatorShown(false);  // SlideGridView draws its own indicator
    m_slideGridView->setDragDropMode(QAbstractItemView::InternalMove);
    m_slideGridView->setDefaultDropAction(Qt::MoveAction);

    // Create and set the custom delegate for grid thumbnails
    m_slideDelegate = new SlideGridDelegate(this);
    m_slideDelegate->setRedLetterColor(m_settingsManager->redLetterColor());
    m_slideGridView->setItemDelegate(m_slideDelegate);

    // Apply saved slide preview size
    applySlidePreviewSize(m_settingsManager->slidePreviewSize());

    connect(m_slideGridView, &QListView::clicked, this, &ControlWindow::onSlideClicked);
    connect(m_slideGridView, &QListView::doubleClicked, this, &ControlWindow::onSlideDoubleClicked);

    // Enable context menu for slide grid
    m_slideGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_slideGridView, &QListView::customContextMenuRequested, this, &ControlWindow::onSlideContextMenu);

    // Media drag-and-drop from the media drawer onto slides
    connect(m_slideGridView, &SlideGridView::mediaDropped, this, &ControlWindow::onMediaDroppedOnSlide);

    contentLayout->addWidget(m_slideGridView, 1);  // Grid takes remaining space

    // Right panel: Live preview
    m_livePreviewPanel = new LivePreviewPanel(this);
    m_livePreviewPanel->setSettingsManager(m_settingsManager);
    contentLayout->addWidget(m_livePreviewPanel);

    // Wrap content in a widget for the splitter
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setLayout(contentLayout);

    // Create media drawer (bottom panel — toggle bar always visible, content starts collapsed)
    m_mediaDrawer = new MediaDrawer(m_mediaLibrary, m_thumbnailGenerator, this);

    // Sync View menu and adjust splitter when drawer is toggled
    connect(m_mediaDrawer, &MediaDrawer::expandedChanged, this, [this](bool expanded) {
        m_toggleMediaDrawerAction->setChecked(expanded);

        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() != 2) return;

        int total = sizes[0] + sizes[1];

        if (expanded) {
            // Expand: give the drawer its remembered height
            int drawerHeight = qMin(m_drawerExpandedHeight, total * 2 / 3);
            sizes[0] = total - drawerHeight;
            sizes[1] = drawerHeight;
        } else {
            // Collapse: remember current drawer height, then shrink to toggle bar only
            if (sizes[1] > 30) {
                m_drawerExpandedHeight = sizes[1];
            }
            int barHeight = 22;
            sizes[0] = total - barHeight;
            sizes[1] = barHeight;
        }
        m_mainSplitter->setSizes(sizes);
    });

    // Vertical splitter: content on top, media drawer on bottom
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->addWidget(contentWidget);
    m_mainSplitter->addWidget(m_mediaDrawer);
    m_mainSplitter->setStretchFactor(0, 1);  // Content stretches
    m_mainSplitter->setStretchFactor(1, 0);  // Drawer stays at set size

    mainLayout->addWidget(m_mainSplitter, 1);

    // Connect live preview double-click signals to toggle output/confidence displays
    connect(m_livePreviewPanel, &LivePreviewPanel::outputDoubleClicked, this, &ControlWindow::toggleOutputDisplay);
    connect(m_livePreviewPanel, &LivePreviewPanel::confidenceDoubleClicked, this, &ControlWindow::toggleConfidenceMonitor);

    // Connect blackout/whiteout buttons
    connect(m_livePreviewPanel, &LivePreviewPanel::blackoutClicked, this, &ControlWindow::blackScreen);
    connect(m_livePreviewPanel, &LivePreviewPanel::whiteoutClicked, this, &ControlWindow::whiteScreen);

    // Connect timer buttons
    connect(m_livePreviewPanel, &LivePreviewPanel::timerStartClicked, this, &ControlWindow::onStartTimer);
    connect(m_livePreviewPanel, &LivePreviewPanel::timerPauseClicked, this, &ControlWindow::onPauseTimer);
    connect(m_livePreviewPanel, &LivePreviewPanel::timerResetClicked, this, &ControlWindow::onResetTimer);

    // Status bar
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);

    // Remote server status label (right side of status bar) - clickable for QR code
    m_remoteStatusLabel = new QLabel("", this);
    m_remoteStatusLabel->setOpenExternalLinks(false);
    m_remoteStatusLabel->setCursor(Qt::PointingHandCursor);
    m_remoteStatusLabel->setToolTip(tr("Click to show QR code for mobile remote control"));
    m_remoteStatusLabel->setStyleSheet("QLabel { color: #0066cc; } QLabel:hover { text-decoration: underline; }");
    m_remoteStatusLabel->installEventFilter(this);
    statusBar()->addPermanentWidget(m_remoteStatusLabel);
}

void ControlWindow::createDemoPresentation()
{
    // Create a demo presentation for Phase 1 testing
    Presentation* demo = new Presentation("Demo Presentation");

    demo->addSlide(Slide("Welcome to Clarity", QColor("#1e3a8a"), QColor("#ffffff")));
    demo->addSlide(Slide("Amazing Grace\nHow sweet the sound", QColor("#064e3b"), QColor("#ffffff")));
    demo->addSlide(Slide("That saved a wretch like me", QColor("#064e3b"), QColor("#ffffff")));
    demo->addSlide(Slide("I once was lost\nBut now am found", QColor("#064e3b"), QColor("#ffffff")));
    demo->addSlide(Slide("Was blind but now I see", QColor("#064e3b"), QColor("#ffffff")));
    demo->addSlide(Slide("John 3:16\n\nFor God so loved the world...", QColor("#7c2d12"), QColor("#ffffff")));

    m_presentationModel->setPresentation(demo);
    m_itemListModel->setPresentation(demo);

    // Initialize filter to first item
    m_slideFilterProxy->setFilterItemIndex(0);
}

void ControlWindow::updateUI()
{
    int currentIndex = m_presentationModel->currentSlideIndex();
    int slideCount = m_presentationModel->rowCount();

    // Update selection in item list view (items)
    int itemIndex = m_itemListModel->itemIndexForSlide(currentIndex);
    if (itemIndex >= 0) {
        QModelIndex itemModelIndex = m_itemListModel->index(itemIndex);
        m_slideListView->setCurrentIndex(itemModelIndex);

        // Update the filter if not showing all slides
        if (!m_settingsManager->showAllSlidesInGrid()) {
            int oldFilterIndex = m_slideFilterProxy->filterItemIndex();
            if (oldFilterIndex != itemIndex) {
                m_slideFilterProxy->setFilterItemIndex(itemIndex);
                // Invalidate delegate cache when filter changes
                m_slideDelegate->invalidateCache();
            }
        }
    }

    // Update selection in grid view (slides) - use proxy model
    if (slideCount > 0) {
        QModelIndex sourceIndex = m_presentationModel->index(currentIndex);
        QModelIndex proxyIndex = m_slideFilterProxy->mapFromSource(sourceIndex);
        if (proxyIndex.isValid()) {
            m_slideGridView->setCurrentIndex(proxyIndex);
        }
    }

    // Update the delegate to show "live" indicator on current slide
    m_slideDelegate->setCurrentSlideIndex(currentIndex);
    m_slideGridView->viewport()->update();  // Force repaint
}

void ControlWindow::broadcastCurrentSlide()
{
    // Clear blackout/whiteout state when broadcasting a slide
    m_isBlackout = false;
    m_isWhiteout = false;
    m_livePreviewPanel->setBlackoutActive(false);
    m_livePreviewPanel->setWhiteoutActive(false);

    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation || presentation->slideCount() == 0) {
        return;
    }

    Slide currentSlide = presentation->currentSlide();
    int currentIndex = presentation->currentSlideIndex();
    int totalSlides = presentation->slideCount();

    // Get next slide if available
    Slide nextSlide;
    if (currentIndex < totalSlides - 1) {
        nextSlide = presentation->getSlide(currentIndex + 1);
    }

    // Update live preview panel with current and next slides
    m_livePreviewPanel->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);

    // Send standard slideData message to output displays
    QJsonObject outputMessage;
    outputMessage["type"] = "slideData";
    outputMessage["index"] = currentIndex;
    outputMessage["slide"] = currentSlide.toJson();

    // Include transition settings (per-slide override or global default)
    QString transitionType = currentSlide.transitionType();
    if (transitionType.isEmpty()) {
        transitionType = m_settingsManager->transitionType();
    }
    outputMessage["transitionType"] = transitionType;

    int transitionDuration = currentSlide.transitionDuration();
    if (transitionDuration < 0) {
        transitionDuration = m_settingsManager->transitionDuration();
    }
    outputMessage["transitionDuration"] = transitionDuration;

    // Include red letter color for scripture slides with red letter markup
    outputMessage["redLetterColor"] = m_settingsManager->redLetterColor();

    m_ipcServer->sendToClientType("output", outputMessage);

    // Send enhanced confidenceData message to confidence monitors
    QJsonObject confidenceMessage;
    confidenceMessage["type"] = "confidenceData";
    confidenceMessage["currentIndex"] = currentIndex;
    confidenceMessage["totalSlides"] = presentation->slideCount();
    confidenceMessage["currentSlide"] = currentSlide.toJson();

    // Add next slide if available
    if (currentIndex < presentation->slideCount() - 1) {
        confidenceMessage["nextSlide"] = presentation->getSlide(currentIndex + 1).toJson();
    }

    // Add item info for confidence monitor
    PresentationItem* currentItem = presentation->currentItem();
    if (currentItem) {
        QJsonObject itemInfo;
        itemInfo["itemType"] = currentItem->typeName();
        itemInfo["itemName"] = currentItem->displayName();
        itemInfo["slideInItem"] = presentation->currentSlideInItem();
        itemInfo["slidesInItem"] = presentation->slidesInCurrentItem();
        confidenceMessage["itemInfo"] = itemInfo;

        // Record song usage for CCLI reporting (only once per song per session)
        if (currentItem->type() == PresentationItem::SongItemType &&
            m_settingsManager->usageTrackingEnabled()) {
            SongItem* songItem = qobject_cast<SongItem*>(currentItem);
            if (songItem && songItem->songId() > 0) {
                int songId = songItem->songId();
                if (!m_recordedSongUsage.contains(songId)) {
                    m_recordedSongUsage.insert(songId);

                    // Get event name from presentation title or default setting
                    QString eventName = m_settingsManager->defaultEventName();
                    if (presentation->title() != "Untitled" && !presentation->title().isEmpty()) {
                        eventName = presentation->title();
                    }

                    m_songLibrary->recordUsage(songId, eventName);
                    m_songLibrary->saveLibrary();
                }
            }
        }
    }

    m_ipcServer->sendToClientType("confidence", confidenceMessage);

    // Update remote control clients
    updateRemoteServer();

    // Start auto-advance timer for the new slide
    startAutoAdvanceForCurrentSlide();
}

void ControlWindow::onPrevSlide()
{
    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->prevSlide()) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onNextSlide()
{
    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->nextSlide()) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onClearOutput()
{
    // Stop auto-advance timer when output is cleared
    m_autoAdvanceTimer->stop();

    // Clear the live preview panel
    m_livePreviewPanel->clearAll();

    // Send clear command to displays
    QJsonObject message;
    message["type"] = "clearOutput";
    m_ipcServer->sendToAll(message);
}

void ControlWindow::onSlideClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    // Map from proxy index to source index
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(index);
    if (!sourceIndex.isValid()) {
        return;
    }

    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->gotoSlide(sourceIndex.row())) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onClientConnected(QLocalSocket* client)
{
    Q_UNUSED(client);
    m_statusLabel->setText(tr("IPC Server: Client connected"));
    // Note: Current slide is sent when client identifies itself in onMessageReceived
}

void ControlWindow::onClientDisconnected(QLocalSocket* client)
{
    Q_UNUSED(client);
    m_statusLabel->setText(tr("IPC Server: Client disconnected"));
    updatePreviewStates();
}

void ControlWindow::onMessageReceived(QLocalSocket* client, const QJsonObject& message)
{
    QString type = message["type"].toString();
    qDebug() << "ControlWindow: Received message type:" << type;

    // When a client identifies itself, send it the current slide
    if (type == "connect") {
        QString clientType = message["clientType"].toString();
        qDebug() << "ControlWindow: Client identified as:" << clientType;

        // Send current slide to newly identified client
        if (clientType == "output") {
            m_outputVisible = true;
            QTimer::singleShot(100, this, &ControlWindow::broadcastCurrentSlide);
        } else if (clientType == "confidence") {
            m_confidenceVisible = true;
            QTimer::singleShot(100, this, &ControlWindow::broadcastCurrentSlide);
        }

        // Update preview borders to reflect new connection state
        updatePreviewStates();
    }
}

void ControlWindow::onSettings()
{
    SettingsDialog dialog(m_settingsManager, this);
    dialog.setBibleDatabase(m_bibleDatabase);
    dialog.exec();
}

void ControlWindow::updateWindowTitle()
{
    QString title = "Clarity";

    if (m_currentFilePath.isEmpty()) {
        title += " - Untitled";
    } else {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.fileName();
    }

    if (m_isDirty) {
        title += "*";
    }

    setWindowTitle(title);
}

void ControlWindow::markDirty()
{
    if (!m_isDirty) {
        m_isDirty = true;
        updateWindowTitle();
    }
}

void ControlWindow::markClean()
{
    if (m_isDirty) {
        m_isDirty = false;
        updateWindowTitle();
    }
}

void ControlWindow::updatePreviewStates()
{
    // If the client disconnected entirely, it's no longer visible
    bool wasOutputVisible = m_outputVisible;
    if (!m_ipcServer->hasClientType("output"))
        m_outputVisible = false;
    if (!m_ipcServer->hasClientType("confidence"))
        m_confidenceVisible = false;

    m_livePreviewPanel->setOutputActive(m_outputVisible);
    m_livePreviewPanel->setConfidenceActive(m_confidenceVisible);

    // Stop auto-advance if output just went away
    if (wasOutputVisible && !m_outputVisible) {
        m_autoAdvanceTimer->stop();
    }
}

void ControlWindow::applySlidePreviewSize(const QString& size)
{
    QSize thumbnailSize;
    QSize gridCellSize;

    if (size == "large") {
        thumbnailSize = QSize(640, 360);
        gridCellSize = QSize(660, 390);
    } else if (size == "medium") {
        thumbnailSize = QSize(320, 180);
        gridCellSize = QSize(340, 210);
    } else {
        // "small" (default)
        thumbnailSize = QSize(160, 90);
        gridCellSize = QSize(180, 120);
    }

    m_slideDelegate->setThumbnailSize(thumbnailSize);
    m_slideGridView->setGridSize(gridCellSize);
    m_slideGridView->viewport()->update();
}

void ControlWindow::onPresentationModified()
{
    markDirty();
}

void ControlWindow::onSlideDoubleClicked(const QModelIndex& index)
{
    // Double-click to edit slide
    if (index.isValid()) {
        onEditSlide();
    }
}

void ControlWindow::onItemClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    // When an item is clicked, navigate to its first slide
    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) {
        return;
    }

    int itemIndex = index.row();

    // Update the slide filter to show this item's slides (unless showing all)
    if (!m_settingsManager->showAllSlidesInGrid()) {
        m_slideFilterProxy->setFilterItemIndex(itemIndex);
        // Invalidate delegate cache when filter changes
        m_slideDelegate->invalidateCache();
        m_slideGridView->viewport()->update();
    }

    int firstSlideIndex = presentation->flatIndexForPosition(itemIndex, 0);

    if (firstSlideIndex >= 0 && presentation->gotoSlide(firstSlideIndex)) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onItemDoubleClicked(const QModelIndex& index)
{
    // Double-click on item - for now, just navigate to it
    // In the future, could open an item editor
    onItemClicked(index);
}

void ControlWindow::onAddSlide()
{
    // Create a new slide with default values
    Slide newSlide;
    newSlide.setText("New Slide");
    newSlide.setBackgroundColor(QColor("#1e3a8a"));
    newSlide.setTextColor(QColor("#ffffff"));

    // Open editor dialog
    SlideEditorDialog dialog(m_settingsManager, m_mediaLibrary, m_thumbnailGenerator, this);
    dialog.setSlide(newSlide);

    if (dialog.exec() == QDialog::Accepted) {
        Slide editedSlide = dialog.slide();
        m_presentationModel->addSlide(editedSlide);

        // Select the new slide
        int newIndex = m_presentationModel->rowCount() - 1;
        m_slideGridView->setCurrentIndex(m_presentationModel->index(newIndex, 0));
        m_presentationModel->setCurrentSlideIndex(newIndex);
        broadcastCurrentSlide();
    }
}

void ControlWindow::onEditSlide()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("No Slide Selected"), tr("Please select a slide to edit."));
        return;
    }

    // Map from proxy index to source index
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
    int index = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();
    Slide currentSlide = m_presentationModel->getSlide(index);

    // Open editor dialog
    SlideEditorDialog dialog(m_settingsManager, m_mediaLibrary, m_thumbnailGenerator, this);
    dialog.setSlide(currentSlide);

    if (dialog.exec() == QDialog::Accepted) {
        Slide editedSlide = dialog.slide();
        m_presentationModel->updateSlide(index, editedSlide);

        // Broadcast updated slide if it's the current one
        if (index == m_presentationModel->currentSlideIndex()) {
            broadcastCurrentSlide();
        }
    }
}

void ControlWindow::onDeleteSlide()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("No Slide Selected"), tr("Please select a slide to delete."));
        return;
    }

    // Map from proxy index to source index
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
    int index = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Delete Slide"),
        tr("Are you sure you want to delete this slide?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_presentationModel->removeSlide(index);

        // Update UI to select next available slide
        if (m_presentationModel->rowCount() > 0) {
            int newIndex = qMin(index, m_presentationModel->rowCount() - 1);
            m_presentationModel->setCurrentSlideIndex(newIndex);
            broadcastCurrentSlide();
        } else {
            // No slides left, clear output
            onClearOutput();
        }

        updateUI();
    }
}

void ControlWindow::onMoveSlideUp()
{
    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) return;

    // Get the item index for the current slide
    int currentSlide = m_presentationModel->currentSlideIndex();
    int itemIndex = m_itemListModel->itemIndexForSlide(currentSlide);
    if (itemIndex <= 0) {
        return; // Already at top or no item
    }

    // Move item up in the playlist
    presentation->moveItem(itemIndex, itemIndex - 1);

    // Refresh models and UI
    m_slideDelegate->invalidateCache();
    updateUI();
    broadcastCurrentSlide();
}

void ControlWindow::onMoveSlideDown()
{
    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) return;

    // Get the item index for the current slide
    int currentSlide = m_presentationModel->currentSlideIndex();
    int itemIndex = m_itemListModel->itemIndexForSlide(currentSlide);
    if (itemIndex < 0 || itemIndex >= presentation->itemCount() - 1) {
        return; // Already at bottom or no item
    }

    // Move item down in the playlist
    presentation->moveItem(itemIndex, itemIndex + 1);

    // Refresh models and UI
    m_slideDelegate->invalidateCache();
    updateUI();
    broadcastCurrentSlide();
}

void ControlWindow::onSlideContextMenu(const QPoint& pos)
{
    QModelIndex index = m_slideGridView->indexAt(pos);

    QMenu contextMenu(this);

    // Slide editing actions
    QAction* editAction = contextMenu.addAction(tr("&Edit Slide"));
    editAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(editAction, &QAction::triggered, this, &ControlWindow::onEditSlide);

    QAction* deleteAction = contextMenu.addAction(tr("&Delete Slide"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &ControlWindow::onDeleteSlide);

    contextMenu.addSeparator();

    // Theme/formatting actions
    QAction* applyThemeAction = contextMenu.addAction(tr("Apply &Theme..."));
    applyThemeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(applyThemeAction, &QAction::triggered, this, &ControlWindow::onApplyTheme);

    QAction* applyThemeToSlideAction = contextMenu.addAction(tr("Apply Theme to Current Slide..."));
    connect(applyThemeToSlideAction, &QAction::triggered, this, &ControlWindow::onApplyThemeToSlide);

    QAction* applyThemeToGroupAction = contextMenu.addAction(tr("Apply Theme to &Group..."));
    applyThemeToGroupAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(applyThemeToGroupAction, &QAction::triggered, this, &ControlWindow::onApplyThemeToGroup);

    QAction* cloneFormatAction = contextMenu.addAction(tr("Clone &Format to Group"));
    cloneFormatAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(cloneFormatAction, &QAction::triggered, this, &ControlWindow::onCloneFormatToGroup);

    contextMenu.addSeparator();

    QAction* autoAdvanceAction = contextMenu.addAction(tr("Set Auto-Advance..."));
    connect(autoAdvanceAction, &QAction::triggered, this, &ControlWindow::onSetSlideAutoAdvance);

    // Enable/disable actions based on whether a slide is selected
    bool hasSelection = index.isValid();
    editAction->setEnabled(hasSelection);
    deleteAction->setEnabled(hasSelection);
    applyThemeToSlideAction->setEnabled(hasSelection);
    applyThemeToGroupAction->setEnabled(hasSelection);
    cloneFormatAction->setEnabled(hasSelection);
    autoAdvanceAction->setEnabled(hasSelection);

    // Section operations for song slides
    if (hasSelection) {
        int itemType = index.data(PresentationModel::ItemTypeRole).toInt();
        int groupIndex = index.data(PresentationModel::GroupIndexRole).toInt();

        if (itemType == PresentationItem::SongItemType && groupIndex >= 0) {
            QString groupLabel = index.data(PresentationModel::GroupLabelRole).toString();
            QString labelSuffix = groupLabel.isEmpty() ? "" : " \"" + groupLabel + "\"";

            contextMenu.addSeparator();

            QAction* dupSectionAction = contextMenu.addAction(tr("Duplicate Section%1").arg(labelSuffix));
            connect(dupSectionAction, &QAction::triggered, this, &ControlWindow::onDuplicateSection);

            QAction* delSectionAction = contextMenu.addAction(tr("Delete Section%1").arg(labelSuffix));
            connect(delSectionAction, &QAction::triggered, this, &ControlWindow::onDeleteSection);
        }
    }

    // If clicked on a slide, select it first
    if (hasSelection) {
        m_slideGridView->setCurrentIndex(index);
        onSlideClicked(index);
    }

    contextMenu.exec(m_slideGridView->mapToGlobal(pos));
}

void ControlWindow::onDuplicateSection()
{
    QModelIndex proxyIndex = m_slideGridView->currentIndex();
    if (!proxyIndex.isValid()) return;

    // Map proxy index to source model
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    int itemIdx = sourceIndex.data(PresentationModel::ItemIndexRole).toInt();
    int slideInItem = sourceIndex.data(PresentationModel::SlideInItemRole).toInt();
    if (itemIdx < 0 || slideInItem < 0) return;

    PresentationItem* item = m_presentationModel->itemAt(itemIdx);
    SongItem* songItem = qobject_cast<SongItem*>(item);
    if (!songItem) return;

    int orderIndex = songItem->sectionOrderIndexForSlide(slideInItem);
    if (orderIndex < 0) return;

    songItem->duplicateSongSection(orderIndex);
    m_slideDelegate->invalidateCache();
    markDirty();
}

void ControlWindow::onDeleteSection()
{
    QModelIndex proxyIndex = m_slideGridView->currentIndex();
    if (!proxyIndex.isValid()) return;

    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    int itemIdx = sourceIndex.data(PresentationModel::ItemIndexRole).toInt();
    int slideInItem = sourceIndex.data(PresentationModel::SlideInItemRole).toInt();
    if (itemIdx < 0 || slideInItem < 0) return;

    PresentationItem* item = m_presentationModel->itemAt(itemIdx);
    SongItem* songItem = qobject_cast<SongItem*>(item);
    if (!songItem) return;

    int orderIndex = songItem->sectionOrderIndexForSlide(slideInItem);
    if (orderIndex < 0) return;

    QString sectionLabel = songItem->sectionLabelAt(orderIndex);
    QString msg = sectionLabel.isEmpty()
        ? tr("Delete this section?")
        : tr("Delete section \"%1\"?").arg(sectionLabel);

    if (QMessageBox::question(this, tr("Delete Section"), msg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    if (!songItem->removeSongSection(orderIndex)) {
        QMessageBox::information(this, tr("Cannot Delete"),
                                 tr("Cannot delete the only remaining section."));
        return;
    }

    m_slideDelegate->invalidateCache();
    markDirty();
}

void ControlWindow::onStartTimer()
{
    QJsonObject message;
    message["type"] = "timerStart";
    m_ipcServer->sendToClientType("confidence", message);
}

void ControlWindow::onPauseTimer()
{
    QJsonObject message;
    message["type"] = "timerPause";
    m_ipcServer->sendToClientType("confidence", message);
}

void ControlWindow::onResetTimer()
{
    QJsonObject message;
    message["type"] = "timerReset";
    m_ipcServer->sendToClientType("confidence", message);
}

void ControlWindow::onAutoAdvanceExpired()
{
    // Timer expired - advance to next slide
    onNextSlide();
}

void ControlWindow::toggleAutoAdvancePause()
{
    m_autoAdvanceTimer->togglePause();
}

void ControlWindow::toggleAutoAdvanceEnabled()
{
    m_autoAdvanceTimer->setEnabled(!m_autoAdvanceTimer->isEnabled());
}

void ControlWindow::onSetSlideAutoAdvance()
{
    QModelIndex proxyIndex = m_slideGridView->currentIndex();
    if (!proxyIndex.isValid()) return;

    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) return;

    int flatIndex = sourceIndex.row();
    Slide slide = presentation->slideAt(flatIndex);

    bool ok;
    int duration = QInputDialog::getInt(
        this,
        tr("Set Auto-Advance"),
        tr("Duration in seconds (0 = disabled):"),
        slide.autoAdvanceDuration(),
        0, 300, 1, &ok);

    if (!ok) return;

    slide.setAutoAdvanceDuration(duration);
    presentation->updateSlide(flatIndex, slide);
    markDirty();

    // If this is the currently displayed slide, apply immediately
    if (flatIndex == presentation->currentSlideIndex()) {
        startAutoAdvanceForCurrentSlide();
    }
}

void ControlWindow::startAutoAdvanceForCurrentSlide()
{
    // Don't start auto-advance if output is not actively displaying
    if (!m_outputVisible || m_isBlackout || m_isWhiteout) {
        m_autoAdvanceTimer->stop();
        return;
    }

    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation || presentation->slideCount() == 0) {
        m_autoAdvanceTimer->stop();
        return;
    }

    int currentIndex = presentation->currentSlideIndex();
    SlidePosition pos = presentation->positionForFlatIndex(currentIndex);
    if (!pos.isValid()) {
        m_autoAdvanceTimer->stop();
        return;
    }

    // Get the effective auto-advance duration:
    // 1. Per-slide override (from the Slide itself)
    // 2. Item-level default (from the PresentationItem)
    PresentationItem* item = presentation->itemAt(pos.itemIndex);
    int duration = 0;
    if (item) {
        duration = item->effectiveAutoAdvanceDuration(pos.slideInItem);
    }

    if (duration > 0) {
        m_autoAdvanceTimer->startCountdown(duration);
    } else {
        m_autoAdvanceTimer->stop();
    }
}

void ControlWindow::broadcastAutoAdvanceState()
{
    QJsonObject message;
    message["type"] = "autoAdvanceState";
    message["active"] = m_autoAdvanceTimer->isActive();
    message["paused"] = m_autoAdvanceTimer->isPaused();
    message["remainingSeconds"] = m_autoAdvanceTimer->remainingSeconds();
    message["totalDuration"] = m_autoAdvanceTimer->totalDuration();
    message["enabled"] = m_autoAdvanceTimer->isEnabled();
    m_ipcServer->sendToClientType("confidence", message);
}

bool ControlWindow::promptSaveIfDirty()
{
    if (!m_isDirty) {
        return true; // Nothing to save, proceed
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Unsaved Changes"),
        tr("Presentation has unsaved changes. Save before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Save) {
        savePresentation();
        return !m_isDirty; // If still dirty, save failed or was cancelled
    } else if (reply == QMessageBox::Discard) {
        return true;
    } else { // Cancel
        return false;
    }
}

void ControlWindow::closeEvent(QCloseEvent* event)
{
    if (promptSaveIfDirty()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void ControlWindow::newPresentation()
{
    if (!promptSaveIfDirty()) {
        return; // User cancelled
    }

    // Create a new blank presentation
    Presentation* newPres = new Presentation("Untitled");
    newPres->addSlide(Slide("New Slide", QColor("#1e3a8a"), QColor("#ffffff")));

    m_presentationModel->setPresentation(newPres);
    m_itemListModel->setPresentation(newPres);

    // Reset filter to first item
    m_slideFilterProxy->setFilterItemIndex(0);

    m_currentFilePath.clear();
    m_isDirty = false;
    updateWindowTitle();
    updateUI();
}

void ControlWindow::openPresentation()
{
    if (!promptSaveIfDirty()) {
        return; // User cancelled
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Presentation"),
        QDir::homePath(),
        tr("Clarity Presentations (*.cly);;All Files (*.*)")
    );

    if (filePath.isEmpty()) {
        return; // User cancelled dialog
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file: %1").arg(file.errorString()));
        qCritical() << "Failed to open file:" << filePath << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid presentation file format."));
        qCritical() << "Invalid JSON in file:" << filePath;
        return;
    }

    // Load presentation with song library, bible database, and settings for item resolution
    Presentation* loaded = Presentation::fromJson(doc.object(), m_songLibrary, m_bibleDatabase, m_settingsManager);
    m_presentationModel->setPresentation(loaded);
    m_itemListModel->setPresentation(loaded);

    // Reset filter to first item
    m_slideFilterProxy->setFilterItemIndex(0);

    m_currentFilePath = filePath;
    m_isDirty = false;
    updateWindowTitle();
    updateUI();
    broadcastCurrentSlide();

    qDebug() << "Loaded presentation from:" << filePath;
}

void ControlWindow::savePresentation()
{
    if (m_currentFilePath.isEmpty()) {
        // No file path yet, delegate to Save As
        saveAsPresentation();
        return;
    }

    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file: %1").arg(file.errorString()));
        qCritical() << "Failed to open file for writing:" << m_currentFilePath << file.errorString();
        return;
    }

    QJsonObject json = m_presentationModel->presentationToJson();
    QJsonDocument doc(json);

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written == -1) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write to file."));
        qCritical() << "Failed to write to file:" << m_currentFilePath;
        return;
    }

    markClean();
    qDebug() << "Saved presentation to:" << m_currentFilePath;
}

void ControlWindow::saveAsPresentation()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Presentation As"),
        QDir::homePath(),
        tr("Clarity Presentations (*.cly);;All Files (*.*)")
    );

    if (filePath.isEmpty()) {
        return; // User cancelled dialog
    }

    // Ensure .cly extension
    if (!filePath.endsWith(".cly", Qt::CaseInsensitive)) {
        filePath += ".cly";
    }

    m_currentFilePath = filePath;
    savePresentation();
}

void ControlWindow::initializeBibleDatabase()
{
    // Look for Bible database in standard locations
    QStringList searchPaths;

    // First check next to executable
    QString appDir = QCoreApplication::applicationDirPath();
    searchPaths << appDir + "/data/bible.db"
                << appDir + "/bible.db";

    // Check in app data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    searchPaths << dataDir + "/data/bible.db"
                << dataDir + "/bible.db";

    // Check in config directory
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/Clarity";
    searchPaths << configDir + "/data/bible.db"
                << configDir + "/bible.db";

    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            if (m_bibleDatabase->initialize(path)) {
                qDebug() << "Bible database loaded from:" << path;
                return;
            }
        }
    }

    qWarning() << "Bible database not found. Scripture lookup will be unavailable.";
    qWarning() << "Searched paths:";
    for (const QString& path : searchPaths) {
        qWarning() << "  -" << path;
    }
}

void ControlWindow::onInsertScripture()
{
    if (!m_bibleDatabase->isValid()) {
        QMessageBox::warning(
            this,
            tr("Bible Database Not Available"),
            tr("The Bible database could not be loaded.\n\n"
               "Please ensure the bible.db file is present in one of the following locations:\n"
               "- <app directory>/data/bible.db\n"
               "- <app data>/Clarity/data/bible.db\n\n"
               "You can download the database from the Clarity releases page.")
        );
        return;
    }

    ScriptureDialog dialog(m_bibleDatabase, m_settingsManager, m_themeManager, this);

    // Set default style based on current presentation theme (used if "Current Style" is selected)
    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->slideCount() > 0) {
        Slide currentSlide = presentation->currentSlide();
        dialog.setDefaultStyle(
            currentSlide.backgroundColor(),
            currentSlide.textColor(),
            currentSlide.fontFamily(),
            currentSlide.fontSize()
        );
    }

    if (dialog.exec() == QDialog::Accepted) {
        QString reference = dialog.reference();
        if (reference.isEmpty()) {
            return;
        }

        // Create a ScriptureItem with the selected passage
        ScriptureItem* scriptureItem = new ScriptureItem(
            reference,
            dialog.translation(),
            m_bibleDatabase
        );
        scriptureItem->setSettingsManager(m_settingsManager);  // For red letter settings
        scriptureItem->setOneVersePerSlide(dialog.oneVersePerSlide());
        scriptureItem->setIncludeVerseReferences(dialog.includeVerseReferences());
        scriptureItem->setIncludeHeaderSlide(true);

        // Apply custom style if set
        SlideStyle style = dialog.slideStyle();
        scriptureItem->setItemStyle(style);

        // Calculate insertion position - after current item
        SlidePosition currentPos = presentation->positionForFlatIndex(presentation->currentSlideIndex());
        int insertItemIndex = currentPos.isValid() ? currentPos.itemIndex + 1 : presentation->itemCount();

        // Insert the scripture item
        m_presentationModel->insertItem(insertItemIndex, scriptureItem);

        // Navigate to first slide of the new item (re-fetch presentation after insert)
        Presentation* updatedPres = m_presentationModel->presentation();
        int firstSlideIndex = updatedPres->flatIndexForPosition(insertItemIndex, 0);
        if (firstSlideIndex >= 0) {
            m_slideGridView->setCurrentIndex(m_presentationModel->index(firstSlideIndex, 0));
            m_presentationModel->setCurrentSlideIndex(firstSlideIndex);
        }
        broadcastCurrentSlide();
        updateUI();

        qDebug() << "Inserted scripture item:" << scriptureItem->displayName();
    }
}

void ControlWindow::onInsertSong()
{
    SongLibraryDialog dialog(m_songLibrary, this);

    // Set default style based on current presentation theme
    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->slideCount() > 0) {
        Slide currentSlide = presentation->currentSlide();
        dialog.setDefaultStyle(
            currentSlide.backgroundColor(),
            currentSlide.textColor(),
            currentSlide.fontFamily(),
            currentSlide.fontSize()
        );
    }

    if (dialog.exec() == QDialog::Accepted) {
        int songId = dialog.selectedSongId();
        if (songId <= 0) {
            return;
        }

        // Mark the song as used
        m_songLibrary->markAsUsed(songId);
        m_songLibrary->saveLibrary();

        // Create a SongItem with the selected song
        SongItem* songItem = new SongItem(songId, m_songLibrary);
        songItem->setIncludeTitleSlide(true);
        songItem->setIncludeSectionLabels(dialog.includeSectionLabels());
        songItem->setMaxLinesPerSlide(dialog.maxLinesPerSlide());

        // Apply custom style if set
        SlideStyle style = dialog.slideStyle();
        songItem->setItemStyle(style);

        // Calculate insertion position - after current item
        SlidePosition currentPos = presentation->positionForFlatIndex(presentation->currentSlideIndex());
        int insertItemIndex = currentPos.isValid() ? currentPos.itemIndex + 1 : presentation->itemCount();

        // Insert the song item
        m_presentationModel->insertItem(insertItemIndex, songItem);

        // Navigate to first slide of the new item (re-fetch presentation after insert)
        Presentation* updatedPres = m_presentationModel->presentation();
        int firstSlideIndex = updatedPres->flatIndexForPosition(insertItemIndex, 0);
        if (firstSlideIndex >= 0) {
            m_slideGridView->setCurrentIndex(m_presentationModel->index(firstSlideIndex, 0));
            m_presentationModel->setCurrentSlideIndex(firstSlideIndex);
        }
        broadcastCurrentSlide();
        updateUI();

        qDebug() << "Inserted song item:" << songItem->displayName();
    }
}

void ControlWindow::onApplyTheme()
{
    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);

    if (dialog.exec() == QDialog::Accepted && dialog.hasSelection()) {
        Theme theme = dialog.selectedTheme();
        bool applyToAll = dialog.applyToAllSlides();

        if (applyToAll) {
            // Apply theme to all slides
            Presentation* presentation = m_presentationModel->presentation();
            if (!presentation) return;

            int slideCount = presentation->slideCount();

            for (int i = 0; i < slideCount; i++) {
                Slide slide = presentation->getSlide(i);
                theme.applyToSlide(slide);
                presentation->updateSlide(i, slide);
            }

            broadcastCurrentSlide();
            markDirty();

            qDebug() << "Applied theme" << theme.name() << "to all" << slideCount << "slides";
        } else {
            // Apply to current slide only
            QModelIndex currentIndex = m_slideGridView->currentIndex();
            if (currentIndex.isValid()) {
                // Map from proxy index to source index
                QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
                int index = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();
                Slide slide = m_presentationModel->getSlide(index);
                theme.applyToSlide(slide);
                m_presentationModel->updateSlide(index, slide);

                if (index == m_presentationModel->currentSlideIndex()) {
                    broadcastCurrentSlide();
                }

                qDebug() << "Applied theme" << theme.name() << "to slide" << index;
            }
        }
    }
}

void ControlWindow::onApplyThemeToSlide()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("No Slide Selected"), tr("Please select a slide to apply a theme to."));
        return;
    }

    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);

    if (dialog.exec() == QDialog::Accepted && dialog.hasSelection()) {
        Theme theme = dialog.selectedTheme();

        // Map from proxy index to source index
        QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
        int flatIndex = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();

        // Get the item containing this slide
        Presentation* presentation = m_presentationModel->presentation();
        if (!presentation) return;

        SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
        if (!pos.isValid()) return;

        PresentationItem* item = presentation->itemAt(pos.itemIndex);
        if (!item) return;

        if (qobject_cast<SongItem*>(item) || qobject_cast<ScriptureItem*>(item)) {
            // Use per-slide style override so only this slide changes
            item->setSlideStyleOverride(pos.slideInItem, theme.toSlideStyle());

            QModelIndex idx = m_presentationModel->index(flatIndex, 0);
            emit m_presentationModel->dataChanged(idx, idx);

            qDebug() << "Applied theme" << theme.name() << "to slide" << pos.slideInItem
                     << "in item" << item->displayName();
        } else {
            // For CustomSlideItem and SlideGroupItem, we can update the individual slide
            Slide slide = m_presentationModel->getSlide(flatIndex);
            theme.applyToSlide(slide);
            m_presentationModel->updateSlide(flatIndex, slide);

            qDebug() << "Applied theme" << theme.name() << "to slide" << flatIndex;
        }

        // Broadcast if this affects the current slide
        int currentSlideIndex = m_presentationModel->currentSlideIndex();
        SlidePosition currentPos = presentation->positionForFlatIndex(currentSlideIndex);
        if (currentPos.itemIndex == pos.itemIndex) {
            broadcastCurrentSlide();
        }

        markDirty();
    }
}

void ControlWindow::onApplyThemeToGroup()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("No Slide Selected"), tr("Please select a slide to identify the group."));
        return;
    }

    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);

    if (dialog.exec() == QDialog::Accepted && dialog.hasSelection()) {
        Theme theme = dialog.selectedTheme();

        // Map from proxy index to source index
        QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
        int flatIndex = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();

        // Get the item containing this slide
        Presentation* presentation = m_presentationModel->presentation();
        if (!presentation) return;

        SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
        if (!pos.isValid()) return;

        PresentationItem* item = presentation->itemAt(pos.itemIndex);
        if (!item) return;

        // Apply theme to all slides in the group/item
        if (auto* groupItem = qobject_cast<SlideGroupItem*>(item)) {
            // For SlideGroupItem, update each slide individually
            QList<Slide> slides = groupItem->slides();
            for (int i = 0; i < slides.count(); ++i) {
                Slide slide = slides[i];
                theme.applyToSlide(slide);
                groupItem->updateSlide(i, slide);
            }
            // Invalidate cache to reflect changes
            groupItem->invalidateSlideCache();
        } else {
            // For SongItem, ScriptureItem, and CustomSlideItem, use item-level styling
            item->setItemStyle(theme.toSlideStyle());
        }

        // Emit dataChanged for all slides in this item
        int itemStart = presentation->flatIndexForPosition(pos.itemIndex, 0);
        int itemEnd = itemStart + item->slideCount() - 1;
        QModelIndex startIdx = m_presentationModel->index(itemStart, 0);
        QModelIndex endIdx = m_presentationModel->index(itemEnd, 0);
        emit m_presentationModel->dataChanged(startIdx, endIdx);

        // Broadcast if current slide is in this item
        int currentSlideIndex = m_presentationModel->currentSlideIndex();
        SlidePosition currentPos = presentation->positionForFlatIndex(currentSlideIndex);
        if (currentPos.itemIndex == pos.itemIndex) {
            broadcastCurrentSlide();
        }

        markDirty();

        qDebug() << "Applied theme" << theme.name() << "to group" << item->displayName()
                 << "(" << item->slideCount() << "slides)";
    }
}

void ControlWindow::onCloneFormatToGroup()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("No Slide Selected"), tr("Please select a slide to clone formatting from."));
        return;
    }

    // Map from proxy index to source index
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(currentIndex);
    int flatIndex = sourceIndex.isValid() ? sourceIndex.row() : currentIndex.row();

    // Get the item containing this slide
    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) return;

    SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
    if (!pos.isValid()) return;

    PresentationItem* item = presentation->itemAt(pos.itemIndex);
    if (!item) return;

    // Get the source slide to clone formatting from
    Slide sourceSlide = m_presentationModel->getSlide(flatIndex);

    // Apply formatting to all slides in the group/item
    if (auto* groupItem = qobject_cast<SlideGroupItem*>(item)) {
        // For SlideGroupItem, copy full formatting to each slide
        QList<Slide> slides = groupItem->slides();
        for (int i = 0; i < slides.count(); ++i) {
            Slide& slide = slides[i];
            // Copy all formatting properties (but not text content)
            slide.setBackgroundType(sourceSlide.backgroundType());
            slide.setBackgroundColor(sourceSlide.backgroundColor());
            slide.setGradientStartColor(sourceSlide.gradientStartColor());
            slide.setGradientEndColor(sourceSlide.gradientEndColor());
            slide.setGradientAngle(sourceSlide.gradientAngle());
            slide.setBackgroundImageData(sourceSlide.backgroundImageData());
            slide.setBackgroundImagePath(sourceSlide.backgroundImagePath());
            slide.setBackgroundVideoPath(sourceSlide.backgroundVideoPath());
            slide.setVideoLoop(sourceSlide.videoLoop());
            slide.setTextColor(sourceSlide.textColor());
            slide.setFontFamily(sourceSlide.fontFamily());
            slide.setFontSize(sourceSlide.fontSize());
            // Drop shadow
            slide.setDropShadowEnabled(sourceSlide.dropShadowEnabled());
            slide.setDropShadowColor(sourceSlide.dropShadowColor());
            slide.setDropShadowOffsetX(sourceSlide.dropShadowOffsetX());
            slide.setDropShadowOffsetY(sourceSlide.dropShadowOffsetY());
            slide.setDropShadowBlur(sourceSlide.dropShadowBlur());
            // Overlay
            slide.setOverlayEnabled(sourceSlide.overlayEnabled());
            slide.setOverlayColor(sourceSlide.overlayColor());
            slide.setOverlayBlur(sourceSlide.overlayBlur());
            // Text container
            slide.setTextContainerEnabled(sourceSlide.textContainerEnabled());
            slide.setTextContainerColor(sourceSlide.textContainerColor());
            slide.setTextContainerPadding(sourceSlide.textContainerPadding());
            slide.setTextContainerRadius(sourceSlide.textContainerRadius());
            slide.setTextContainerBlur(sourceSlide.textContainerBlur());
            // Text band
            slide.setTextBandEnabled(sourceSlide.textBandEnabled());
            slide.setTextBandColor(sourceSlide.textBandColor());
            slide.setTextBandBlur(sourceSlide.textBandBlur());

            groupItem->updateSlide(i, slide);
        }
        // Invalidate cache to reflect changes
        groupItem->invalidateSlideCache();
    } else {
        // For SongItem, ScriptureItem, and CustomSlideItem, use item-level styling
        SlideStyle style;
        style.backgroundColor = sourceSlide.backgroundColor();
        style.textColor = sourceSlide.textColor();
        style.fontFamily = sourceSlide.fontFamily();
        style.fontSize = sourceSlide.fontSize();
        style.backgroundType = sourceSlide.backgroundType();
        style.gradientStartColor = sourceSlide.gradientStartColor();
        style.gradientEndColor = sourceSlide.gradientEndColor();
        style.gradientAngle = sourceSlide.gradientAngle();
        item->setItemStyle(style);
    }

    // Emit dataChanged for all slides in this item
    int itemStart = presentation->flatIndexForPosition(pos.itemIndex, 0);
    int itemEnd = itemStart + item->slideCount() - 1;
    QModelIndex startIdx = m_presentationModel->index(itemStart, 0);
    QModelIndex endIdx = m_presentationModel->index(itemEnd, 0);
    emit m_presentationModel->dataChanged(startIdx, endIdx);

    // Broadcast if current slide is in this item
    int currentSlideIndex = m_presentationModel->currentSlideIndex();
    SlidePosition currentPos = presentation->positionForFlatIndex(currentSlideIndex);
    if (currentPos.itemIndex == pos.itemIndex) {
        broadcastCurrentSlide();
    }

    markDirty();

    qDebug() << "Cloned formatting from slide" << flatIndex << "to group" << item->displayName()
             << "(" << item->slideCount() << "slides)";
}

void ControlWindow::onMediaDroppedOnSlide(const QModelIndex& proxyIndex, const QString& path,
                                           const QString& mediaType, bool applyToGroup)
{
    if (!proxyIndex.isValid()) return;

    // Map proxy → source flat index
    QModelIndex sourceIndex = m_slideFilterProxy->mapToSource(proxyIndex);
    int flatIndex = sourceIndex.isValid() ? sourceIndex.row() : proxyIndex.row();

    Presentation* presentation = m_presentationModel->presentation();
    if (!presentation) return;

    // Prepare the background data based on media type
    Slide::BackgroundType bgType;
    QByteArray imageData;
    if (mediaType == "video") {
        bgType = Slide::Video;
    } else {
        bgType = Slide::Image;
        // Read raw file bytes directly — avoids expensive decode→re-encode cycle
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to read image file for drop:" << path;
            return;
        }
        imageData = file.readAll();
        if (imageData.isEmpty()) {
            qWarning() << "Image file is empty:" << path;
            return;
        }
    }

    // Helper lambda to apply the background to a single Slide object
    auto applyBackground = [&](Slide& slide) {
        slide.setBackgroundType(bgType);
        if (bgType == Slide::Image) {
            slide.setBackgroundImagePath(path);
            slide.setBackgroundImageData(imageData);
        } else {
            slide.setBackgroundVideoPath(path);
            slide.setVideoLoop(true);
        }
    };

    if (!applyToGroup) {
        // Single slide: apply background
        SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
        if (!pos.isValid()) return;

        PresentationItem* item = presentation->itemAt(pos.itemIndex);
        if (!item) return;

        if (qobject_cast<SongItem*>(item) || qobject_cast<ScriptureItem*>(item)) {
            // For SongItem/ScriptureItem, use per-slide style override
            // (same pattern as onApplyThemeToSlide)
            Slide refSlide = m_presentationModel->getSlide(flatIndex);
            SlideStyle style;
            style.backgroundColor = refSlide.backgroundColor();
            style.textColor = refSlide.textColor();
            style.fontFamily = refSlide.fontFamily();
            style.fontSize = refSlide.fontSize();
            style.backgroundType = bgType;
            style.gradientStartColor = refSlide.gradientStartColor();
            style.gradientEndColor = refSlide.gradientEndColor();
            style.gradientAngle = refSlide.gradientAngle();
            if (bgType == Slide::Image) {
                style.backgroundImagePath = path;
                style.backgroundImageData = imageData;
            } else {
                style.backgroundVideoPath = path;
                style.videoLoop = true;
            }
            item->setSlideStyleOverride(pos.slideInItem, style);
        } else {
            // For SlideGroupItem/CustomSlideItem, update the slide directly
            Slide slide = m_presentationModel->getSlide(flatIndex);
            applyBackground(slide);
            m_presentationModel->updateSlide(flatIndex, slide);
        }

        // Invalidate delegate cache for this slide
        m_slideDelegate->invalidateCacheFor(flatIndex);
        QModelIndex srcIdx = m_presentationModel->index(flatIndex, 0);
        emit m_presentationModel->dataChanged(srcIdx, srcIdx);

        qDebug() << "Applied" << mediaType << "background to slide" << flatIndex;
    } else {
        // Whole group: apply to every slide in the item
        SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
        if (!pos.isValid()) return;

        PresentationItem* item = presentation->itemAt(pos.itemIndex);
        if (!item) return;

        if (auto* groupItem = qobject_cast<SlideGroupItem*>(item)) {
            // For SlideGroupItem, apply directly to each slide
            QList<Slide> slides = groupItem->slides();
            for (int i = 0; i < slides.count(); ++i) {
                Slide& slide = slides[i];
                applyBackground(slide);
                groupItem->updateSlide(i, slide);
            }
            groupItem->invalidateSlideCache();
        } else {
            // For SongItem/ScriptureItem, update the item style so regenerated slides get the background
            SlideStyle style;
            Slide refSlide = m_presentationModel->getSlide(flatIndex);
            style.backgroundColor = refSlide.backgroundColor();
            style.textColor = refSlide.textColor();
            style.fontFamily = refSlide.fontFamily();
            style.fontSize = refSlide.fontSize();
            style.backgroundType = bgType;
            style.gradientStartColor = refSlide.gradientStartColor();
            style.gradientEndColor = refSlide.gradientEndColor();
            style.gradientAngle = refSlide.gradientAngle();
            if (bgType == Slide::Image) {
                style.backgroundImagePath = path;
                style.backgroundImageData = imageData;
            } else {
                style.backgroundVideoPath = path;
                style.videoLoop = true;
            }
            item->setItemStyle(style);
        }

        // Invalidate caches and emit dataChanged for all slides in the item
        int itemStart = presentation->flatIndexForPosition(pos.itemIndex, 0);
        int itemEnd = itemStart + item->slideCount() - 1;
        for (int i = itemStart; i <= itemEnd; ++i) {
            m_slideDelegate->invalidateCacheFor(i);
        }
        QModelIndex startIdx = m_presentationModel->index(itemStart, 0);
        QModelIndex endIdx = m_presentationModel->index(itemEnd, 0);
        emit m_presentationModel->dataChanged(startIdx, endIdx);

        qDebug() << "Applied" << mediaType << "background to group" << item->displayName()
                 << "(" << item->slideCount() << "slides)";
    }

    // Broadcast if the current slide was affected
    int currentSlideIndex = m_presentationModel->currentSlideIndex();
    if (!applyToGroup) {
        if (currentSlideIndex == flatIndex) {
            broadcastCurrentSlide();
        }
    } else {
        SlidePosition pos = presentation->positionForFlatIndex(flatIndex);
        SlidePosition currentPos = presentation->positionForFlatIndex(currentSlideIndex);
        if (currentPos.itemIndex == pos.itemIndex) {
            broadcastCurrentSlide();
        }
    }

    markDirty();
}

void ControlWindow::onManageThemes()
{
    // Open the theme selector dialog in management mode
    // User can create, edit, duplicate, and delete custom themes
    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);
    dialog.setWindowTitle(tr("Manage Themes"));
    dialog.exec();

    // No need to apply - this is just for theme management
}

void ControlWindow::setupShortcuts()
{
    // Navigation shortcuts - arrows, page up/down, space
    // Right Arrow, Page Down, Space = Next slide
    new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(onNextSlide()));
    new QShortcut(QKeySequence(Qt::Key_PageDown), this, SLOT(onNextSlide()));
    new QShortcut(QKeySequence(Qt::Key_Space), this, SLOT(onNextSlide()));

    // Left Arrow, Page Up = Previous slide
    new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(onPrevSlide()));
    new QShortcut(QKeySequence(Qt::Key_PageUp), this, SLOT(onPrevSlide()));

    // Home = First slide, End = Last slide
    new QShortcut(QKeySequence(Qt::Key_Home), this, SLOT(gotoFirstSlide()));
    new QShortcut(QKeySequence(Qt::Key_End), this, SLOT(gotoLastSlide()));

    // G = Go to slide (prompt for number)
    new QShortcut(QKeySequence(Qt::Key_G), this, SLOT(promptGotoSlide()));

    // Display control shortcuts
    // B = Black screen (clear output)
    new QShortcut(QKeySequence(Qt::Key_B), this, SLOT(blackScreen()));

    // W = White screen
    new QShortcut(QKeySequence(Qt::Key_W), this, SLOT(whiteScreen()));

    // Escape = Clear output
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(onClearOutput()));

    // O = Toggle output display visibility
    new QShortcut(QKeySequence(Qt::Key_O), this, SLOT(toggleOutputDisplay()));

    // F = Toggle output fullscreen
    new QShortcut(QKeySequence(Qt::Key_F), this, SLOT(toggleOutputFullscreen()));

    // C = Toggle confidence monitor
    new QShortcut(QKeySequence(Qt::Key_C), this, SLOT(toggleConfidenceMonitor()));

    // P = Pause/Resume auto-advance timer
    new QShortcut(QKeySequence(Qt::Key_P), this, SLOT(toggleAutoAdvancePause()));

    // Slide management shortcuts
    // Ctrl+Up = Move slide up
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Up), this, SLOT(onMoveSlideUp()));

    // Ctrl+Down = Move slide down
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Down), this, SLOT(onMoveSlideDown()));

    // Number keys 1-9 for quick navigation to slides 1-9
    for (int i = 1; i <= 9; i++) {
        QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_0 + i), this);
        connect(shortcut, &QShortcut::activated, this, [this, i]() {
            gotoSlide(i - 1);  // Convert to 0-indexed
        });
    }
}

void ControlWindow::gotoFirstSlide()
{
    if (m_presentationModel->rowCount() == 0) {
        return;
    }

    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->gotoSlide(0)) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::gotoLastSlide()
{
    int lastIndex = m_presentationModel->rowCount() - 1;
    if (lastIndex < 0) {
        return;
    }

    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->gotoSlide(lastIndex)) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::gotoSlide(int index)
{
    if (index < 0 || index >= m_presentationModel->rowCount()) {
        return;
    }

    Presentation* presentation = m_presentationModel->presentation();
    if (presentation && presentation->gotoSlide(index)) {
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::promptGotoSlide()
{
    int slideCount = m_presentationModel->rowCount();
    if (slideCount == 0) {
        return;
    }

    bool ok;
    int slideNumber = QInputDialog::getInt(
        this,
        tr("Go to Slide"),
        tr("Enter slide number (1-%1):").arg(slideCount),
        m_presentationModel->currentSlideIndex() + 1,  // Default to current slide (1-indexed)
        1,         // Minimum
        slideCount, // Maximum
        1,         // Step
        &ok
    );

    if (ok) {
        gotoSlide(slideNumber - 1);  // Convert to 0-indexed
    }
}

void ControlWindow::blackScreen()
{
    // Toggle blackout - if already black, restore slide
    if (m_isBlackout) {
        // Restore the current slide (broadcastCurrentSlide clears both states + buttons)
        m_isBlackout = false;
        broadcastCurrentSlide();
        return;
    }

    // Clear any whiteout state
    m_isWhiteout = false;
    m_isBlackout = true;

    // Update button states
    m_livePreviewPanel->setBlackoutActive(true);
    m_livePreviewPanel->setWhiteoutActive(false);

    // Clear output - shows black screen
    onClearOutput();
}

void ControlWindow::whiteScreen()
{
    // Toggle whiteout - if already white, restore slide
    if (m_isWhiteout) {
        // Restore the current slide (broadcastCurrentSlide clears both states + buttons)
        m_isWhiteout = false;
        broadcastCurrentSlide();
        return;
    }

    // Clear any blackout state
    m_isBlackout = false;
    m_isWhiteout = true;

    // Update button states
    m_livePreviewPanel->setBlackoutActive(false);
    m_livePreviewPanel->setWhiteoutActive(true);

    // Stop auto-advance timer when output is blanked (matches blackout behavior)
    m_autoAdvanceTimer->stop();

    // Send a white screen command to displays
    // We'll create a temporary white slide to display
    QJsonObject message;
    message["type"] = "slideData";
    message["index"] = -1;  // Special index indicating temporary slide

    QJsonObject slideJson;
    slideJson["text"] = "";
    slideJson["backgroundColor"] = "#ffffff";
    slideJson["textColor"] = "#000000";
    slideJson["fontFamily"] = "Arial";
    slideJson["fontSize"] = 48;

    message["slide"] = slideJson;
    message["transitionType"] = "cut";  // Instant switch to white
    message["transitionDuration"] = 0;

    m_ipcServer->sendToClientType("output", message);

    // Update live preview to show white screen
    Slide whiteSlide;
    whiteSlide.setText("");
    whiteSlide.setBackgroundColor(QColor("#ffffff"));
    whiteSlide.setTextColor(QColor("#000000"));
    m_livePreviewPanel->setOutputSlide(whiteSlide);
}

void ControlWindow::toggleOutputDisplay()
{
    // Toggle output display visibility
    // Send toggle visibility command to any connected output clients
    QJsonObject message;
    message["type"] = "toggleVisibility";

    // Check if we have any output clients connected
    // If the message isn't sent to anyone, launch the output display
    if (!m_ipcServer->sendToClientType("output", message)) {
        // No output clients connected, launch one
        m_processManager->launchOutput();
    } else {
        // Client received the toggle, flip our visibility tracking
        m_outputVisible = !m_outputVisible;
        updatePreviewStates();

        // Stop or restart auto-advance based on new visibility
        if (m_outputVisible) {
            startAutoAdvanceForCurrentSlide();
        } else {
            m_autoAdvanceTimer->stop();
        }
    }
}

void ControlWindow::toggleOutputFullscreen()
{
    // Send fullscreen toggle command to output display
    QJsonObject message;
    message["type"] = "toggleFullscreen";
    m_ipcServer->sendToClientType("output", message);
}

void ControlWindow::toggleConfidenceMonitor()
{
    // Toggle confidence monitor visibility
    // Send toggle visibility command to any connected confidence clients
    QJsonObject message;
    message["type"] = "toggleVisibility";

    // Check if we have any confidence clients connected
    // If the message isn't sent to anyone, launch the confidence monitor
    if (!m_ipcServer->sendToClientType("confidence", message)) {
        // No confidence clients connected, launch one
        m_processManager->launchConfidence();
    } else {
        // Client received the toggle, flip our visibility tracking
        m_confidenceVisible = !m_confidenceVisible;
        updatePreviewStates();
    }
}

void ControlWindow::toggleMediaDrawer()
{
    m_mediaDrawer->setExpanded(!m_mediaDrawer->isExpanded());
}

void ControlWindow::showKeyboardShortcuts()
{
    QString shortcuts = R"(
<h2>Keyboard Shortcuts</h2>

<h3>Navigation</h3>
<table>
<tr><td><b>Right Arrow / Page Down / Space</b></td><td>Next slide</td></tr>
<tr><td><b>Left Arrow / Page Up</b></td><td>Previous slide</td></tr>
<tr><td><b>Home</b></td><td>First slide</td></tr>
<tr><td><b>End</b></td><td>Last slide</td></tr>
<tr><td><b>1-9</b></td><td>Go to slide 1-9</td></tr>
<tr><td><b>G</b></td><td>Go to slide (prompt for number)</td></tr>
</table>

<h3>Display Control</h3>
<table>
<tr><td><b>B</b></td><td>Toggle black screen</td></tr>
<tr><td><b>W</b></td><td>Toggle white screen</td></tr>
<tr><td><b>Escape</b></td><td>Clear output</td></tr>
<tr><td><b>O</b></td><td>Toggle output display</td></tr>
<tr><td><b>F</b></td><td>Toggle output fullscreen</td></tr>
<tr><td><b>C</b></td><td>Toggle confidence monitor</td></tr>
</table>

<h3>Slide Management</h3>
<table>
<tr><td><b>Ctrl+Shift+N</b></td><td>New slide</td></tr>
<tr><td><b>Ctrl+E</b></td><td>Edit current slide</td></tr>
<tr><td><b>Delete</b></td><td>Delete selected slide</td></tr>
<tr><td><b>Ctrl+Up</b></td><td>Move slide up</td></tr>
<tr><td><b>Ctrl+Down</b></td><td>Move slide down</td></tr>
</table>

<h3>File</h3>
<table>
<tr><td><b>Ctrl+N</b></td><td>New presentation</td></tr>
<tr><td><b>Ctrl+O</b></td><td>Open presentation</td></tr>
<tr><td><b>Ctrl+S</b></td><td>Save presentation</td></tr>
</table>

<h3>Auto-Advance</h3>
<table>
<tr><td><b>P</b></td><td>Pause/resume auto-advance timer</td></tr>
</table>

<h3>Content</h3>
<table>
<tr><td><b>Ctrl+B</b></td><td>Insert Bible verse</td></tr>
<tr><td><b>Ctrl+L</b></td><td>Insert song lyrics</td></tr>
<tr><td><b>Ctrl+T</b></td><td>Apply theme</td></tr>
<tr><td><b>Ctrl+M</b></td><td>Toggle media library drawer</td></tr>
</table>
)";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Keyboard Shortcuts"));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(shortcuts);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void ControlWindow::onRemoteNavigation(const QString& action)
{
    qDebug() << "Remote navigation requested:" << action;

    if (action == "next") {
        onNextSlide();
    } else if (action == "prev") {
        onPrevSlide();
    } else if (action == "first") {
        gotoFirstSlide();
    } else if (action == "last") {
        gotoLastSlide();
    } else if (action == "clear") {
        onClearOutput();
    } else if (action == "black") {
        blackScreen();
    } else if (action == "white") {
        whiteScreen();
    } else if (action.startsWith("goto:")) {
        bool ok;
        int index = action.mid(5).toInt(&ok);
        if (ok) {
            gotoSlide(index);
        }
    }
}

void ControlWindow::updateRemoteServer()
{
    if (!m_remoteServer->isRunning()) {
        return;
    }

    Presentation* pres = m_presentationModel->presentation();
    if (!pres) {
        return;
    }

    int currentIndex = pres->currentSlideIndex();
    int totalSlides = pres->slideCount();

    QString currentText;
    QString nextText;

    if (currentIndex >= 0 && currentIndex < totalSlides) {
        currentText = pres->getSlide(currentIndex).text();
    }

    if (currentIndex + 1 < totalSlides) {
        nextText = pres->getSlide(currentIndex + 1).text();
    }

    m_remoteServer->broadcastSlideUpdate(currentIndex, totalSlides, currentText, nextText);

    // Update connection status
    bool outputConnected = m_ipcServer->hasClientType("output");
    bool confidenceConnected = m_ipcServer->hasClientType("confidence");
    m_remoteServer->broadcastStatus(outputConnected, confidenceConnected);
}

bool ControlWindow::eventFilter(QObject* watched, QEvent* event)
{
    // Handle click on remote status label to show QR code
    if (watched == m_remoteStatusLabel && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_remoteServer->isRunning()) {
            showQrCode();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void ControlWindow::showQrCode()
{
    if (!m_remoteServer->isRunning()) {
        return;
    }

    // Create a dialog to show the QR code
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Remote Control QR Code"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 24, 24, 24);

    // Title
    QLabel* titleLabel = new QLabel(tr("Scan to connect"), dialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    // QR Code image
    QImage qrImage = m_remoteServer->qrCode(6, 4);
    QLabel* qrLabel = new QLabel(dialog);
    qrLabel->setPixmap(QPixmap::fromImage(qrImage));
    qrLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(qrLabel);

    // URL text (selectable)
    QString url = m_remoteServer->serverUrl();
    QLabel* urlLabel = new QLabel(url, dialog);
    urlLabel->setAlignment(Qt::AlignCenter);
    urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    urlLabel->setCursor(Qt::IBeamCursor);
    QFont urlFont = urlLabel->font();
    urlFont.setPointSize(12);
    urlLabel->setFont(urlFont);
    layout->addWidget(urlLabel);

    // Instructions
    QLabel* instructionsLabel = new QLabel(
        tr("Open this URL on your phone or tablet to control\n"
           "the presentation remotely from the same network."),
        dialog);
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setStyleSheet("color: gray;");
    layout->addWidget(instructionsLabel);

    // Close button
    QPushButton* closeButton = new QPushButton(tr("Close"), dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);

    dialog->setFixedSize(dialog->sizeHint());
    dialog->exec();
}

} // namespace Clarity
