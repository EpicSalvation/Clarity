#include "ControlWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>
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
    , m_statusLabel(nullptr)
    , m_presentationModel(new PresentationModel(this))
    , m_ipcServer(new IpcServer(this))
    , m_processManager(new ProcessManager(this))
{
    setupUI();
    createDemoPresentation();

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

    updateUI();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::setupUI()
{
    setWindowTitle("Clarity - Control");
    resize(800, 600);

    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Slide list view
    m_slideListView = new QListView(this);
    m_slideListView->setModel(m_presentationModel);
    connect(m_slideListView, &QListView::clicked, this, &ControlWindow::onSlideClicked);
    mainLayout->addWidget(m_slideListView);

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

    mainLayout->addLayout(buttonLayout);

    // Process management buttons
    QHBoxLayout* processLayout = new QHBoxLayout();

    m_launchOutputButton = new QPushButton("Launch Output Display", this);
    m_launchConfidenceButton = new QPushButton("Launch Confidence Monitor", this);

    connect(m_launchOutputButton, &QPushButton::clicked, this, &ControlWindow::onLaunchOutput);
    connect(m_launchConfidenceButton, &QPushButton::clicked, this, &ControlWindow::onLaunchConfidence);

    processLayout->addWidget(m_launchOutputButton);
    processLayout->addWidget(m_launchConfidenceButton);
    processLayout->addStretch();

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

    QJsonObject message;
    message["type"] = "slideData";
    message["index"] = currentIndex;
    message["slide"] = currentSlide.toJson();

    m_ipcServer->sendToAll(message);
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

} // namespace Clarity
