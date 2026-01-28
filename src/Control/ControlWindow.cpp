#include "ControlWindow.h"
#include "SettingsDialog.h"
#include "SlideEditorDialog.h"
#include "ScriptureDialog.h"
#include "SongLibraryDialog.h"
#include "ThemeSelectorDialog.h"
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
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QFrame>
#include <QShortcut>
#include <QInputDialog>
#include <QDebug>

namespace Clarity {

ControlWindow::ControlWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_slideListView(nullptr)
    , m_slideGridView(nullptr)
    , m_slideDelegate(nullptr)
    , m_livePreviewPanel(nullptr)
    , m_prevButton(nullptr)
    , m_nextButton(nullptr)
    , m_clearButton(nullptr)
    , m_outputDisabledButton(nullptr)
    , m_launchOutputButton(nullptr)
    , m_launchConfidenceButton(nullptr)
    , m_settingsButton(nullptr)
    , m_addSlideButton(nullptr)
    , m_editSlideButton(nullptr)
    , m_deleteSlideButton(nullptr)
    , m_moveUpButton(nullptr)
    , m_moveDownButton(nullptr)
    , m_timerStartButton(nullptr)
    , m_timerPauseButton(nullptr)
    , m_timerResetButton(nullptr)
    , m_statusLabel(nullptr)
    , m_presentationModel(new PresentationModel(this))
    , m_ipcServer(new IpcServer(this))
    , m_processManager(new ProcessManager(this))
    , m_settingsManager(new SettingsManager(this))
    , m_bibleDatabase(new BibleDatabase(this))
    , m_songLibrary(new SongLibrary(this))
    , m_themeManager(new ThemeManager(this))
    , m_currentFilePath("")
    , m_isDirty(false)
{
    setupUI();
    setupShortcuts();
    createDemoPresentation();

    // Initialize Bible database
    initializeBibleDatabase();

    // Load song library
    m_songLibrary->loadLibrary();

    // Pass settings manager to process manager
    m_processManager->setSettingsManager(m_settingsManager);

    // Start IPC server
    if (m_ipcServer->start()) {
        m_statusLabel->setText("IPC Server: Running");
    } else {
        m_statusLabel->setText("IPC Server: Failed to start");
    }

    // Connect IPC signals
    connect(m_ipcServer, &IpcServer::clientConnected, this, &ControlWindow::onClientConnected);
    connect(m_ipcServer, &IpcServer::clientDisconnected, this, &ControlWindow::onClientDisconnected);
    connect(m_ipcServer, &IpcServer::messageReceived, this, &ControlWindow::onMessageReceived);

    // Connect presentation modification signal
    connect(m_presentationModel, &PresentationModel::presentationModified, this, &ControlWindow::onPresentationModified);

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

    // Broadcast settings changes to confidence monitor
    connect(m_settingsManager, &SettingsManager::confidenceDisplaySettingsChanged, this, [this]() {
        QJsonObject message;
        message["type"] = "settingsChanged";
        m_ipcServer->sendToClientType("confidence", message);
    });

    updateUI();
    updateWindowTitle();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::setupUI()
{
    setWindowTitle("Clarity - Control");
    resize(1000, 700);  // Larger default size for new layout

    // Menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* fileMenu = menuBar->addMenu("&File");

    fileMenu->addAction("&New", QKeySequence::New, this, &ControlWindow::newPresentation);
    fileMenu->addAction("&Open...", QKeySequence::Open, this, &ControlWindow::openPresentation);
    fileMenu->addAction("&Save", QKeySequence::Save, this, &ControlWindow::savePresentation);
    fileMenu->addAction("Save &As...", QKeySequence::SaveAs, this, &ControlWindow::saveAsPresentation);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);

    // Slide menu
    QMenu* slideMenu = menuBar->addMenu("&Slide");
    slideMenu->addAction("&Add Slide", QKeySequence("Ctrl+Shift+N"), this, &ControlWindow::onAddSlide);
    slideMenu->addAction("&Edit Slide", QKeySequence("Ctrl+E"), this, &ControlWindow::onEditSlide);
    slideMenu->addAction("&Delete Slide", QKeySequence::Delete, this, &ControlWindow::onDeleteSlide);
    slideMenu->addSeparator();
    slideMenu->addAction("Insert &Scripture...", QKeySequence("Ctrl+B"), this, &ControlWindow::onInsertScripture);
    slideMenu->addAction("Insert S&ong...", QKeySequence("Ctrl+L"), this, &ControlWindow::onInsertSong);
    slideMenu->addSeparator();
    slideMenu->addAction("Apply &Theme...", QKeySequence("Ctrl+T"), this, &ControlWindow::onApplyTheme);
    slideMenu->addAction("Apply Theme to Current Slide...", this, &ControlWindow::onApplyThemeToSlide);

    // Format menu (for theme management)
    QMenu* formatMenu = menuBar->addMenu("F&ormat");
    formatMenu->addAction("&Manage Themes...", this, &ControlWindow::onManageThemes);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&Keyboard Shortcuts...", QKeySequence("F1"), this, &ControlWindow::showKeyboardShortcuts);

    setMenuBar(menuBar);

    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout - vertical with content area on top and buttons at bottom
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Content area: horizontal layout with list on left, grid in center, preview on right
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Left panel: Slide list (playlist view)
    m_slideListView = new QListView(this);
    m_slideListView->setModel(m_presentationModel);
    m_slideListView->setFixedWidth(180);
    m_slideListView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_slideListView, &QListView::clicked, this, &ControlWindow::onSlideClicked);
    connect(m_slideListView, &QListView::doubleClicked, this, &ControlWindow::onSlideDoubleClicked);
    contentLayout->addWidget(m_slideListView);

    // Center panel: Slide grid view
    m_slideGridView = new QListView(this);
    m_slideGridView->setModel(m_presentationModel);

    // Configure for grid/icon mode
    m_slideGridView->setViewMode(QListView::IconMode);
    m_slideGridView->setGridSize(QSize(180, 120));  // Thumbnail size + spacing
    m_slideGridView->setResizeMode(QListView::Adjust);
    m_slideGridView->setWrapping(true);
    m_slideGridView->setFlow(QListView::LeftToRight);
    m_slideGridView->setSpacing(10);
    m_slideGridView->setUniformItemSizes(true);
    m_slideGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_slideGridView->setDragEnabled(false);  // Disable drag for now

    // Create and set the custom delegate for grid thumbnails
    m_slideDelegate = new SlideGridDelegate(this);
    m_slideGridView->setItemDelegate(m_slideDelegate);

    connect(m_slideGridView, &QListView::clicked, this, &ControlWindow::onSlideClicked);
    connect(m_slideGridView, &QListView::doubleClicked, this, &ControlWindow::onSlideDoubleClicked);

    contentLayout->addWidget(m_slideGridView, 1);  // Grid takes remaining space

    // Right panel: Live preview
    m_livePreviewPanel = new LivePreviewPanel(this);
    m_livePreviewPanel->setSettingsManager(m_settingsManager);
    contentLayout->addWidget(m_livePreviewPanel);

    mainLayout->addLayout(contentLayout, 1);  // Content area stretches

    // Slide editing buttons
    QHBoxLayout* editLayout = new QHBoxLayout();

    m_addSlideButton = new QPushButton("Add", this);
    m_editSlideButton = new QPushButton("Edit", this);
    m_deleteSlideButton = new QPushButton("Delete", this);
    m_moveUpButton = new QPushButton("Move Up", this);
    m_moveDownButton = new QPushButton("Move Down", this);

    connect(m_addSlideButton, &QPushButton::clicked, this, &ControlWindow::onAddSlide);
    connect(m_editSlideButton, &QPushButton::clicked, this, &ControlWindow::onEditSlide);
    connect(m_deleteSlideButton, &QPushButton::clicked, this, &ControlWindow::onDeleteSlide);
    connect(m_moveUpButton, &QPushButton::clicked, this, &ControlWindow::onMoveSlideUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &ControlWindow::onMoveSlideDown);

    editLayout->addWidget(m_addSlideButton);
    editLayout->addWidget(m_editSlideButton);
    editLayout->addWidget(m_deleteSlideButton);
    editLayout->addWidget(m_moveUpButton);
    editLayout->addWidget(m_moveDownButton);
    editLayout->addStretch();

    mainLayout->addLayout(editLayout);

    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_prevButton = new QPushButton("Prev", this);
    m_nextButton = new QPushButton("Next", this);
    m_clearButton = new QPushButton("Clear", this);

    // Output disable toggle - checkable button that stays pressed when output is disabled
    m_outputDisabledButton = new QPushButton("Disable Output", this);
    m_outputDisabledButton->setCheckable(true);
    m_outputDisabledButton->setToolTip("When enabled, blacks out the output display and keeps it disabled during navigation");

    connect(m_prevButton, &QPushButton::clicked, this, &ControlWindow::onPrevSlide);
    connect(m_nextButton, &QPushButton::clicked, this, &ControlWindow::onNextSlide);
    connect(m_clearButton, &QPushButton::clicked, this, &ControlWindow::onClearOutput);
    connect(m_outputDisabledButton, &QPushButton::toggled, this, &ControlWindow::onOutputDisabledToggled);

    buttonLayout->addWidget(m_prevButton);
    buttonLayout->addWidget(m_nextButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_outputDisabledButton);

    // Separator
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    buttonLayout->addWidget(separator);

    // Timer control buttons
    m_timerStartButton = new QPushButton("Start", this);
    m_timerPauseButton = new QPushButton("Pause", this);
    m_timerResetButton = new QPushButton("Reset", this);

    connect(m_timerStartButton, &QPushButton::clicked, this, &ControlWindow::onStartTimer);
    connect(m_timerPauseButton, &QPushButton::clicked, this, &ControlWindow::onPauseTimer);
    connect(m_timerResetButton, &QPushButton::clicked, this, &ControlWindow::onResetTimer);

    buttonLayout->addWidget(m_timerStartButton);
    buttonLayout->addWidget(m_timerPauseButton);
    buttonLayout->addWidget(m_timerResetButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // Process management buttons
    QHBoxLayout* processLayout = new QHBoxLayout();

    m_launchOutputButton = new QPushButton("Launch Output", this);
    m_launchConfidenceButton = new QPushButton("Launch Confidence", this);
    m_settingsButton = new QPushButton("Settings", this);

    connect(m_launchOutputButton, &QPushButton::clicked, this, &ControlWindow::onLaunchOutput);
    connect(m_launchConfidenceButton, &QPushButton::clicked, this, &ControlWindow::onLaunchConfidence);
    connect(m_settingsButton, &QPushButton::clicked, this, &ControlWindow::onSettings);

    processLayout->addWidget(m_launchOutputButton);
    processLayout->addWidget(m_launchConfidenceButton);
    processLayout->addStretch();
    processLayout->addWidget(m_settingsButton);

    mainLayout->addLayout(processLayout);

    // Status bar
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel);
}

void ControlWindow::createDemoPresentation()
{
    // Create a demo presentation for Phase 1 testing
    Presentation demo("Demo Presentation");

    demo.addSlide(Slide("Welcome to Clarity", QColor("#1e3a8a"), QColor("#ffffff")));
    demo.addSlide(Slide("Amazing Grace\nHow sweet the sound", QColor("#064e3b"), QColor("#ffffff")));
    demo.addSlide(Slide("That saved a wretch like me", QColor("#064e3b"), QColor("#ffffff")));
    demo.addSlide(Slide("I once was lost\nBut now am found", QColor("#064e3b"), QColor("#ffffff")));
    demo.addSlide(Slide("Was blind but now I see", QColor("#064e3b"), QColor("#ffffff")));
    demo.addSlide(Slide("John 3:16\n\nFor God so loved the world...", QColor("#7c2d12"), QColor("#ffffff")));

    m_presentationModel->setPresentation(demo);
}

void ControlWindow::updateUI()
{
    int currentIndex = m_presentationModel->currentSlideIndex();
    int slideCount = m_presentationModel->rowCount();

    m_prevButton->setEnabled(currentIndex > 0);
    m_nextButton->setEnabled(currentIndex < slideCount - 1);

    // Update selection in both list and grid views
    if (slideCount > 0) {
        QModelIndex index = m_presentationModel->index(currentIndex);
        m_slideListView->setCurrentIndex(index);
        m_slideGridView->setCurrentIndex(index);
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

    Presentation presentation = m_presentationModel->presentation();
    if (presentation.slideCount() == 0) {
        return;
    }

    Slide currentSlide = presentation.currentSlide();
    int currentIndex = presentation.currentSlideIndex();
    int totalSlides = presentation.slideCount();

    // Get next slide if available
    Slide nextSlide;
    if (currentIndex < totalSlides - 1) {
        nextSlide = presentation.getSlide(currentIndex + 1);
    }

    // Update live preview panel with current and next slides
    m_livePreviewPanel->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);

    // Send standard slideData message to output displays (unless output is disabled)
    if (!m_isOutputDisabled) {
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

        m_ipcServer->sendToClientType("output", outputMessage);
    }

    // Send enhanced confidenceData message to confidence monitors
    QJsonObject confidenceMessage;
    confidenceMessage["type"] = "confidenceData";
    confidenceMessage["currentIndex"] = currentIndex;
    confidenceMessage["totalSlides"] = presentation.slideCount();
    confidenceMessage["currentSlide"] = currentSlide.toJson();

    // Add next slide if available
    if (currentIndex < presentation.slideCount() - 1) {
        // Temporarily move to next slide to get its data
        Presentation tempPresentation = presentation;
        tempPresentation.nextSlide();
        Slide nextSlide = tempPresentation.currentSlide();
        confidenceMessage["nextSlide"] = nextSlide.toJson();
    }

    m_ipcServer->sendToClientType("confidence", confidenceMessage);
}

void ControlWindow::onPrevSlide()
{
    Presentation presentation = m_presentationModel->presentation();
    if (presentation.prevSlide()) {
        m_presentationModel->setPresentation(presentation);
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onNextSlide()
{
    Presentation presentation = m_presentationModel->presentation();
    if (presentation.nextSlide()) {
        m_presentationModel->setPresentation(presentation);
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onClearOutput()
{
    // Clear the live preview panel
    m_livePreviewPanel->clearAll();

    // Send clear command to displays
    QJsonObject message;
    message["type"] = "clearOutput";
    m_ipcServer->sendToAll(message);
}

void ControlWindow::onLaunchOutput()
{
    if (m_processManager->launchOutput()) {
        qDebug() << "Output display launched";
    } else {
        qWarning() << "Failed to launch output display";
    }
}

void ControlWindow::onLaunchConfidence()
{
    if (m_processManager->launchConfidence()) {
        qDebug() << "Confidence monitor launched";
    } else {
        qWarning() << "Failed to launch confidence monitor";
    }
}

void ControlWindow::onSlideClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    Presentation presentation = m_presentationModel->presentation();
    if (presentation.gotoSlide(index.row())) {
        m_presentationModel->setPresentation(presentation);
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::onClientConnected(QLocalSocket* client)
{
    Q_UNUSED(client);
    m_statusLabel->setText("IPC Server: Client connected");
    // Note: Current slide is sent when client identifies itself in onMessageReceived
}

void ControlWindow::onClientDisconnected(QLocalSocket* client)
{
    Q_UNUSED(client);
    m_statusLabel->setText("IPC Server: Client disconnected");
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
        if (clientType == "output" || clientType == "confidence") {
            // Use a small delay to ensure the client is fully registered
            QTimer::singleShot(100, this, &ControlWindow::broadcastCurrentSlide);
        }
    }
}

void ControlWindow::onSettings()
{
    SettingsDialog dialog(m_settingsManager, this);
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

void ControlWindow::onAddSlide()
{
    // Create a new slide with default values
    Slide newSlide;
    newSlide.setText("New Slide");
    newSlide.setBackgroundColor(QColor("#1e3a8a"));
    newSlide.setTextColor(QColor("#ffffff"));

    // Open editor dialog
    SlideEditorDialog dialog(m_settingsManager, this);
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
        QMessageBox::information(this, "No Slide Selected", "Please select a slide to edit.");
        return;
    }

    int index = currentIndex.row();
    Slide currentSlide = m_presentationModel->getSlide(index);

    // Open editor dialog
    SlideEditorDialog dialog(m_settingsManager, this);
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
        QMessageBox::information(this, "No Slide Selected", "Please select a slide to delete.");
        return;
    }

    int index = currentIndex.row();

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Slide",
        "Are you sure you want to delete this slide?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_presentationModel->removeSlide(index);

        // Update UI to select next available slide
        if (m_presentationModel->rowCount() > 0) {
            int newIndex = qMin(index, m_presentationModel->rowCount() - 1);
            m_slideGridView->setCurrentIndex(m_presentationModel->index(newIndex, 0));
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
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    int index = currentIndex.row();
    if (index <= 0) {
        return; // Already at top
    }

    // Move slide up (swap with previous)
    m_presentationModel->moveSlide(index, index - 1);

    // The moved slide is now at index - 1, make it the current displayed slide
    m_presentationModel->setCurrentSlideIndex(index - 1);

    updateUI();

    // Set selection AFTER updateUI so the moved slide stays selected
    m_slideGridView->setCurrentIndex(m_presentationModel->index(index - 1, 0));

    // Broadcast updated slide to output and confidence monitor
    broadcastCurrentSlide();
}

void ControlWindow::onMoveSlideDown()
{
    QModelIndex currentIndex = m_slideGridView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    int index = currentIndex.row();
    if (index >= m_presentationModel->rowCount() - 1) {
        return; // Already at bottom
    }

    // Move slide down (swap with next)
    m_presentationModel->moveSlide(index, index + 1);

    // The moved slide is now at index + 1, make it the current displayed slide
    m_presentationModel->setCurrentSlideIndex(index + 1);

    updateUI();

    // Set selection AFTER updateUI so the moved slide stays selected
    m_slideGridView->setCurrentIndex(m_presentationModel->index(index + 1, 0));

    // Broadcast updated slide to output and confidence monitor
    broadcastCurrentSlide();
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

bool ControlWindow::promptSaveIfDirty()
{
    if (!m_isDirty) {
        return true; // Nothing to save, proceed
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Unsaved Changes",
        "Presentation has unsaved changes. Save before continuing?",
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
    Presentation newPres("Untitled");
    newPres.addSlide(Slide("New Slide", QColor("#1e3a8a"), QColor("#ffffff")));

    m_presentationModel->setPresentation(newPres);
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
        "Open Presentation",
        QDir::homePath(),
        "Clarity Presentations (*.cly);;All Files (*.*)"
    );

    if (filePath.isEmpty()) {
        return; // User cancelled dialog
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file: " + file.errorString());
        qCritical() << "Failed to open file:" << filePath << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid presentation file format.");
        qCritical() << "Invalid JSON in file:" << filePath;
        return;
    }

    Presentation loaded = Presentation::fromJson(doc.object());
    m_presentationModel->setPresentation(loaded);
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
        QMessageBox::critical(this, "Error", "Could not save file: " + file.errorString());
        qCritical() << "Failed to open file for writing:" << m_currentFilePath << file.errorString();
        return;
    }

    Presentation presentation = m_presentationModel->presentation();
    QJsonObject json = presentation.toJson();
    QJsonDocument doc(json);

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written == -1) {
        QMessageBox::critical(this, "Error", "Failed to write to file.");
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
        "Save Presentation As",
        QDir::homePath(),
        "Clarity Presentations (*.cly);;All Files (*.*)"
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
            "Bible Database Not Available",
            "The Bible database could not be loaded.\n\n"
            "Please ensure the bible.db file is present in one of the following locations:\n"
            "- <app directory>/data/bible.db\n"
            "- <app data>/Clarity/data/bible.db\n\n"
            "You can download the database from the Clarity releases page."
        );
        return;
    }

    ScriptureDialog dialog(m_bibleDatabase, this);

    // Set default style based on current presentation theme
    Presentation presentation = m_presentationModel->presentation();
    if (presentation.slideCount() > 0) {
        Slide currentSlide = presentation.currentSlide();
        dialog.setDefaultStyle(
            currentSlide.backgroundColor(),
            currentSlide.textColor(),
            currentSlide.fontFamily(),
            currentSlide.fontSize()
        );
    }

    if (dialog.exec() == QDialog::Accepted) {
        QList<Slide> slides = dialog.getSlides();

        if (slides.isEmpty()) {
            return;
        }

        // Insert slides after current position
        int insertPos = m_presentationModel->currentSlideIndex() + 1;

        for (const Slide& slide : slides) {
            m_presentationModel->insertSlide(insertPos, slide);
            insertPos++;
        }

        // Select the first inserted slide
        int firstInsertedIndex = m_presentationModel->currentSlideIndex() + 1;
        m_slideGridView->setCurrentIndex(m_presentationModel->index(firstInsertedIndex, 0));
        m_presentationModel->setCurrentSlideIndex(firstInsertedIndex);
        broadcastCurrentSlide();
        updateUI();

        qDebug() << "Inserted" << slides.count() << "scripture slide(s)";
    }
}

void ControlWindow::onInsertSong()
{
    SongLibraryDialog dialog(m_songLibrary, this);

    // Set default style based on current presentation theme
    Presentation presentation = m_presentationModel->presentation();
    if (presentation.slideCount() > 0) {
        Slide currentSlide = presentation.currentSlide();
        dialog.setDefaultStyle(
            currentSlide.backgroundColor(),
            currentSlide.textColor(),
            currentSlide.fontFamily(),
            currentSlide.fontSize()
        );
    }

    if (dialog.exec() == QDialog::Accepted) {
        QList<Slide> slides = dialog.getSlides();

        if (slides.isEmpty()) {
            return;
        }

        // Mark the song as used
        int songId = dialog.selectedSongId();
        if (songId > 0) {
            m_songLibrary->markAsUsed(songId);
            m_songLibrary->saveLibrary();
        }

        // Insert slides after current position
        int insertPos = m_presentationModel->currentSlideIndex() + 1;

        for (const Slide& slide : slides) {
            m_presentationModel->insertSlide(insertPos, slide);
            insertPos++;
        }

        // Select the first inserted slide
        int firstInsertedIndex = m_presentationModel->currentSlideIndex() + 1;
        m_slideGridView->setCurrentIndex(m_presentationModel->index(firstInsertedIndex, 0));
        m_presentationModel->setCurrentSlideIndex(firstInsertedIndex);
        broadcastCurrentSlide();
        updateUI();

        qDebug() << "Inserted" << slides.count() << "song slide(s)";
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
            Presentation presentation = m_presentationModel->presentation();
            int slideCount = presentation.slideCount();

            for (int i = 0; i < slideCount; i++) {
                Slide slide = presentation.getSlide(i);
                theme.applyToSlide(slide);
                presentation.updateSlide(i, slide);
            }

            m_presentationModel->setPresentation(presentation);
            broadcastCurrentSlide();
            markDirty();

            qDebug() << "Applied theme" << theme.name() << "to all" << slideCount << "slides";
        } else {
            // Apply to current slide only
            QModelIndex currentIndex = m_slideGridView->currentIndex();
            if (currentIndex.isValid()) {
                int index = currentIndex.row();
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
        QMessageBox::information(this, "No Slide Selected", "Please select a slide to apply a theme to.");
        return;
    }

    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);

    // Hide the "apply to all" checkbox for single-slide operation
    // The dialog will only apply to the current slide

    if (dialog.exec() == QDialog::Accepted && dialog.hasSelection()) {
        Theme theme = dialog.selectedTheme();

        int index = currentIndex.row();
        Slide slide = m_presentationModel->getSlide(index);
        theme.applyToSlide(slide);
        m_presentationModel->updateSlide(index, slide);

        if (index == m_presentationModel->currentSlideIndex()) {
            broadcastCurrentSlide();
        }

        qDebug() << "Applied theme" << theme.name() << "to slide" << index;
    }
}

void ControlWindow::onManageThemes()
{
    // Open the theme selector dialog in management mode
    // User can create, edit, duplicate, and delete custom themes
    ThemeSelectorDialog dialog(m_themeManager, m_settingsManager, this);
    dialog.setWindowTitle("Manage Themes");
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

    // D = Toggle output disable (persistent blackout)
    QShortcut* disableShortcut = new QShortcut(QKeySequence(Qt::Key_D), this);
    connect(disableShortcut, &QShortcut::activated, this, [this]() {
        m_outputDisabledButton->toggle();
    });

    // Escape = Clear output
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(onClearOutput()));

    // O = Toggle output display visibility
    new QShortcut(QKeySequence(Qt::Key_O), this, SLOT(toggleOutputDisplay()));

    // F = Toggle output fullscreen
    new QShortcut(QKeySequence(Qt::Key_F), this, SLOT(toggleOutputFullscreen()));

    // C = Toggle confidence monitor
    new QShortcut(QKeySequence(Qt::Key_C), this, SLOT(toggleConfidenceMonitor()));

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

    Presentation presentation = m_presentationModel->presentation();
    if (presentation.gotoSlide(0)) {
        m_presentationModel->setPresentation(presentation);
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

    Presentation presentation = m_presentationModel->presentation();
    if (presentation.gotoSlide(lastIndex)) {
        m_presentationModel->setPresentation(presentation);
        updateUI();
        broadcastCurrentSlide();
    }
}

void ControlWindow::gotoSlide(int index)
{
    if (index < 0 || index >= m_presentationModel->rowCount()) {
        return;
    }

    Presentation presentation = m_presentationModel->presentation();
    if (presentation.gotoSlide(index)) {
        m_presentationModel->setPresentation(presentation);
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
        "Go to Slide",
        QString("Enter slide number (1-%1):").arg(slideCount),
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
    // Ignore if output is persistently disabled
    if (m_isOutputDisabled) {
        return;
    }

    // Toggle blackout - if already black, restore slide
    if (m_isBlackout) {
        // Restore the current slide
        m_isBlackout = false;
        broadcastCurrentSlide();
        return;
    }

    // Clear any whiteout state
    m_isWhiteout = false;
    m_isBlackout = true;

    // Clear output (same as Clear button) - shows black screen
    onClearOutput();
}

void ControlWindow::whiteScreen()
{
    // Ignore if output is persistently disabled
    if (m_isOutputDisabled) {
        return;
    }

    // Toggle whiteout - if already white, restore slide
    if (m_isWhiteout) {
        // Restore the current slide
        m_isWhiteout = false;
        broadcastCurrentSlide();
        return;
    }

    // Clear any blackout state
    m_isBlackout = false;
    m_isWhiteout = true;

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

void ControlWindow::onOutputDisabledToggled(bool disabled)
{
    m_isOutputDisabled = disabled;

    if (disabled) {
        // Black out the output display
        m_isBlackout = false;  // Clear temporary blackout state
        m_isWhiteout = false;  // Clear temporary whiteout state

        // Send clear command to black out the output
        QJsonObject message;
        message["type"] = "clearOutput";
        m_ipcServer->sendToClientType("output", message);

        // Update button appearance to indicate active state
        m_outputDisabledButton->setText("Output Disabled");
        m_outputDisabledButton->setStyleSheet("QPushButton { background-color: #c0392b; color: white; font-weight: bold; }");
    } else {
        // Re-enable output and restore current slide
        m_outputDisabledButton->setText("Disable Output");
        m_outputDisabledButton->setStyleSheet("");  // Reset to default style

        // Broadcast current slide to restore display
        broadcastCurrentSlide();
    }
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
    }
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
<tr><td><b>D</b></td><td>Toggle output disable (persistent blackout)</td></tr>
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

<h3>Content</h3>
<table>
<tr><td><b>Ctrl+B</b></td><td>Insert Bible verse</td></tr>
<tr><td><b>Ctrl+L</b></td><td>Insert song lyrics</td></tr>
<tr><td><b>Ctrl+T</b></td><td>Apply theme</td></tr>
</table>
)";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Keyboard Shortcuts");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(shortcuts);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

} // namespace Clarity
