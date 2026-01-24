#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

namespace Clarity {

SettingsDialog::SettingsDialog(SettingsManager* settingsManager, QWidget* parent)
    : QDialog(parent)
    , m_categoryList(nullptr)
    , m_pageStack(nullptr)
    , m_outputScreenComboBox(nullptr)
    , m_confidenceScreenComboBox(nullptr)
    , m_settingsManager(settingsManager)
{
    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle("Settings");
    resize(700, 500);

    // Main vertical layout
    QVBoxLayout* outerLayout = new QVBoxLayout(this);

    // Horizontal layout for category list and pages
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Left side: Category list
    m_categoryList = new QListWidget(this);
    m_categoryList->setMaximumWidth(200);
    m_categoryList->setMinimumWidth(150);

    // Add categories
    m_categoryList->addItem("Display");
    // Future categories can be added here (e.g., "General", "Network", etc.)

    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsDialog::onCategoryChanged);

    contentLayout->addWidget(m_categoryList);

    // Right side: Stacked widget for pages
    m_pageStack = new QStackedWidget(this);
    contentLayout->addWidget(m_pageStack, 1); // Stretch factor 1 to take remaining space

    // Create settings pages
    createDisplayPage();

    // Set initial selection
    m_categoryList->setCurrentRow(0);

    // Add content layout to main layout
    outerLayout->addLayout(contentLayout, 1);

    // Bottom buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancelClicked);

    outerLayout->addWidget(buttonBox);
}

void SettingsDialog::createDisplayPage()
{
    QWidget* displayPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(displayPage);

    // Get available screens (used by both output and confidence)
    QList<QScreen*> screens = QGuiApplication::screens();

    // Output Display Settings group
    QGroupBox* outputGroup = new QGroupBox("Output Display", displayPage);
    QFormLayout* outputLayout = new QFormLayout(outputGroup);

    m_outputScreenComboBox = new QComboBox(outputGroup);

    // Populate with available screens
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QString screenInfo = QString("Screen %1").arg(i + 1);

        // Add screen name if available
        if (!screen->name().isEmpty()) {
            screenInfo += QString(" (%1)").arg(screen->name());
        }

        // Add resolution
        QRect geometry = screen->geometry();
        screenInfo += QString(" - %1x%2").arg(geometry.width()).arg(geometry.height());

        // Mark primary screen
        if (screen == QGuiApplication::primaryScreen()) {
            screenInfo += " [Primary]";
        }

        m_outputScreenComboBox->addItem(screenInfo, i);
    }

    outputLayout->addRow("Screen:", m_outputScreenComboBox);

    QLabel* outputHelpLabel = new QLabel(
        "Select which screen the output display will appear on when launched.",
        outputGroup);
    outputHelpLabel->setWordWrap(true);
    outputHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    outputLayout->addRow(outputHelpLabel);

    pageLayout->addWidget(outputGroup);

    // Confidence Monitor Settings group
    QGroupBox* confidenceGroup = new QGroupBox("Confidence Monitor", displayPage);
    QFormLayout* confidenceLayout = new QFormLayout(confidenceGroup);

    m_confidenceScreenComboBox = new QComboBox(confidenceGroup);

    // Populate with available screens
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QString screenInfo = QString("Screen %1").arg(i + 1);

        // Add screen name if available
        if (!screen->name().isEmpty()) {
            screenInfo += QString(" (%1)").arg(screen->name());
        }

        // Add resolution
        QRect geometry = screen->geometry();
        screenInfo += QString(" - %1x%2").arg(geometry.width()).arg(geometry.height());

        // Mark primary screen
        if (screen == QGuiApplication::primaryScreen()) {
            screenInfo += " [Primary]";
        }

        m_confidenceScreenComboBox->addItem(screenInfo, i);
    }

    confidenceLayout->addRow("Screen:", m_confidenceScreenComboBox);

    QLabel* confidenceHelpLabel = new QLabel(
        "Select which screen the confidence monitor will appear on when launched.",
        confidenceGroup);
    confidenceHelpLabel->setWordWrap(true);
    confidenceHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    confidenceLayout->addRow(confidenceHelpLabel);

    pageLayout->addWidget(confidenceGroup);

    pageLayout->addStretch(); // Push content to top

    m_pageStack->addWidget(displayPage);
}

void SettingsDialog::onCategoryChanged(int row)
{
    m_pageStack->setCurrentIndex(row);
}

void SettingsDialog::loadSettings()
{
    if (!m_settingsManager) {
        qWarning() << "SettingsDialog: No settings manager provided";
        return;
    }

    // Load output screen index
    int outputScreenIndex = m_settingsManager->outputScreenIndex();

    // Find the combo box item with this index
    bool outputFound = false;
    for (int i = 0; i < m_outputScreenComboBox->count(); ++i) {
        if (m_outputScreenComboBox->itemData(i).toInt() == outputScreenIndex) {
            m_outputScreenComboBox->setCurrentIndex(i);
            outputFound = true;
            break;
        }
    }

    // If the saved screen index is not found (e.g., screen was disconnected),
    // default to the first available screen
    if (!outputFound && m_outputScreenComboBox->count() > 0) {
        m_outputScreenComboBox->setCurrentIndex(0);
        qDebug() << "SettingsDialog: Saved output screen" << outputScreenIndex
                 << "not found, defaulting to first screen";
    }

    // Load confidence screen index
    int confidenceScreenIndex = m_settingsManager->confidenceScreenIndex();

    // Find the combo box item with this index
    bool confidenceFound = false;
    for (int i = 0; i < m_confidenceScreenComboBox->count(); ++i) {
        if (m_confidenceScreenComboBox->itemData(i).toInt() == confidenceScreenIndex) {
            m_confidenceScreenComboBox->setCurrentIndex(i);
            confidenceFound = true;
            break;
        }
    }

    // If the saved screen index is not found (e.g., screen was disconnected),
    // default to the first available screen
    if (!confidenceFound && m_confidenceScreenComboBox->count() > 0) {
        m_confidenceScreenComboBox->setCurrentIndex(0);
        qDebug() << "SettingsDialog: Saved confidence screen" << confidenceScreenIndex
                 << "not found, defaulting to first screen";
    }
}

void SettingsDialog::saveSettings()
{
    if (!m_settingsManager) {
        qWarning() << "SettingsDialog: No settings manager provided";
        return;
    }

    // Save output screen index
    int outputScreenIndex = m_outputScreenComboBox->currentData().toInt();
    m_settingsManager->setOutputScreenIndex(outputScreenIndex);

    // Save confidence screen index
    int confidenceScreenIndex = m_confidenceScreenComboBox->currentData().toInt();
    m_settingsManager->setConfidenceScreenIndex(confidenceScreenIndex);
}

void SettingsDialog::onOkClicked()
{
    saveSettings();
    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}

} // namespace Clarity
