#include "ControlWindow.h"
#include "SettingsDialog.h"
#include "SlideEditorDialog.h"
#include "ScriptureDialog.h"
#include "SongLibraryDialog.h"
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
#include <QDebug>

namespace Clarity {

ControlWindow::ControlWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_slideListView(nullptr)
    , m_prevButton(nullptr)
    , m_nextButton(nullptr)
    , m_clearButton(nullptr)
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
    , m_currentFilePath("")
    , m_isDirty(false)
{
    setupUI();
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
    resize(800, 600);

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

    setMenuBar(menuBar);

    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Slide list view
    m_slideListView = new QListView(this);
    m_slideListView->setModel(m_presentationModel);
    connect(m_slideListView, &QListView::clicked, this, &ControlWindow::onSlideClicked);
    connect(m_slideListView, &QListView::doubleClicked, this, &ControlWindow::onSlideDoubleClicked);
    mainLayout->addWidget(m_slideListView);

    // Slide editing buttons
    QHBoxLayout* editLayout = new QHBoxLayout();

    m_addSlideButton = new QPushButton("Add Slide", this);
    m_editSlideButton = new QPushButton("Edit Slide", this);
    m_deleteSlideButton = new QPushButton("Delete Slide", this);
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

    m_prevButton = new QPushButton("Previous", this);
    m_nextButton = new QPushButton("Next", this);
    m_clearButton = new QPushButton("Clear Output", this);

    connect(m_prevButton, &QPushButton::clicked, this, &ControlWindow::onPrevSlide);
    connect(m_nextButton, &QPushButton::clicked, this, &ControlWindow::onNextSlide);
    connect(m_clearButton, &QPushButton::clicked, this, &ControlWindow::onClearOutput);

    buttonLayout->addWidget(m_prevButton);
    buttonLayout->addWidget(m_nextButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();

    // Timer control buttons
    m_timerStartButton = new QPushButton("Start Timer", this);
    m_timerPauseButton = new QPushButton("Pause Timer", this);
    m_timerResetButton = new QPushButton("Reset Timer", this);

    connect(m_timerStartButton, &QPushButton::clicked, this, &ControlWindow::onStartTimer);
    connect(m_timerPauseButton, &QPushButton::clicked, this, &ControlWindow::onPauseTimer);
    connect(m_timerResetButton, &QPushButton::clicked, this, &ControlWindow::onResetTimer);

    buttonLayout->addWidget(m_timerStartButton);
    buttonLayout->addWidget(m_timerPauseButton);
    buttonLayout->addWidget(m_timerResetButton);

    mainLayout->addLayout(buttonLayout);

    // Process management buttons
    QHBoxLayout* processLayout = new QHBoxLayout();

    m_launchOutputButton = new QPushButton("Launch Output Display", this);
    m_launchConfidenceButton = new QPushButton("Launch Confidence Monitor", this);
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

    // Update list selection
    if (slideCount > 0) {
        QModelIndex index = m_presentationModel->index(currentIndex);
        m_slideListView->setCurrentIndex(index);
    }
}

void ControlWindow::broadcastCurrentSlide()
{
    Presentation presentation = m_presentationModel->presentation();
    if (presentation.slideCount() == 0) {
        return;
    }

    Slide currentSlide = presentation.currentSlide();
    int currentIndex = presentation.currentSlideIndex();

    // Send standard slideData message to output displays
    QJsonObject outputMessage;
    outputMessage["type"] = "slideData";
    outputMessage["index"] = currentIndex;
    outputMessage["slide"] = currentSlide.toJson();
    m_ipcServer->sendToClientType("output", outputMessage);

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

    // Send current slide to newly connected client
    broadcastCurrentSlide();
}

void ControlWindow::onClientDisconnected(QLocalSocket* client)
{
    Q_UNUSED(client);
    m_statusLabel->setText("IPC Server: Client disconnected");
}

void ControlWindow::onMessageReceived(QLocalSocket* client, const QJsonObject& message)
{
    Q_UNUSED(client);

    QString type = message["type"].toString();
    qDebug() << "ControlWindow: Received message type:" << type;

    // Handle client messages if needed in the future
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
    SlideEditorDialog dialog(this);
    dialog.setSlide(newSlide);

    if (dialog.exec() == QDialog::Accepted) {
        Slide editedSlide = dialog.slide();
        m_presentationModel->addSlide(editedSlide);

        // Select the new slide
        int newIndex = m_presentationModel->rowCount() - 1;
        m_slideListView->setCurrentIndex(m_presentationModel->index(newIndex, 0));
        m_presentationModel->setCurrentSlideIndex(newIndex);
        broadcastCurrentSlide();
    }
}

void ControlWindow::onEditSlide()
{
    QModelIndex currentIndex = m_slideListView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, "No Slide Selected", "Please select a slide to edit.");
        return;
    }

    int index = currentIndex.row();
    Slide currentSlide = m_presentationModel->getSlide(index);

    // Open editor dialog
    SlideEditorDialog dialog(this);
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
    QModelIndex currentIndex = m_slideListView->currentIndex();
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
            m_slideListView->setCurrentIndex(m_presentationModel->index(newIndex, 0));
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
    QModelIndex currentIndex = m_slideListView->currentIndex();
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
    m_slideListView->setCurrentIndex(m_presentationModel->index(index - 1, 0));

    // Broadcast updated slide to output and confidence monitor
    broadcastCurrentSlide();
}

void ControlWindow::onMoveSlideDown()
{
    QModelIndex currentIndex = m_slideListView->currentIndex();
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
    m_slideListView->setCurrentIndex(m_presentationModel->index(index + 1, 0));

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
        m_slideListView->setCurrentIndex(m_presentationModel->index(firstInsertedIndex, 0));
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
        m_slideListView->setCurrentIndex(m_presentationModel->index(firstInsertedIndex, 0));
        m_presentationModel->setCurrentSlideIndex(firstInsertedIndex);
        broadcastCurrentSlide();
        updateUI();

        qDebug() << "Inserted" << slides.count() << "song slide(s)";
    }
}

} // namespace Clarity
