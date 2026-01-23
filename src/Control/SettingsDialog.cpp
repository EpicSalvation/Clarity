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
    , m_screenComboBox(nullptr)
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

    // Main layout: horizontal split
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Left side: Category list
    m_categoryList = new QListWidget(this);
    m_categoryList->setMaximumWidth(200);
    m_categoryList->setMinimumWidth(150);

    // Add categories
    m_categoryList->addItem("Display");
    // Future categories can be added here (e.g., "General", "Network", etc.)

    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsDialog::onCategoryChanged);

    mainLayout->addWidget(m_categoryList);

    // Right side: Stacked widget for pages
    m_pageStack = new QStackedWidget(this);
    mainLayout->addWidget(m_pageStack, 1); // Stretch factor 1 to take remaining space

    // Create settings pages
    createDisplayPage();

    // Set initial selection
    m_categoryList->setCurrentRow(0);

    // Bottom buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancelClicked);

    // Main vertical layout to include button box at bottom
    QVBoxLayout* outerLayout = new QVBoxLayout();
    outerLayout->addLayout(mainLayout, 1);
    outerLayout->addWidget(buttonBox);

    delete layout(); // Remove the layout we created initially
    setLayout(outerLayout);
}

void SettingsDialog::createDisplayPage()
{
    QWidget* displayPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(displayPage);

    // Output Display Settings group
    QGroupBox* outputGroup = new QGroupBox("Output Display", displayPage);
    QFormLayout* outputLayout = new QFormLayout(outputGroup);

    // Screen selection
    m_screenComboBox = new QComboBox(outputGroup);

    // Populate with available screens
    QList<QScreen*> screens = QGuiApplication::screens();
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

        m_screenComboBox->addItem(screenInfo, i);
    }

    outputLayout->addRow("Screen:", m_screenComboBox);

    QLabel* helpLabel = new QLabel(
        "Select which screen the output display will appear on when launched.",
        outputGroup);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    outputLayout->addRow(helpLabel);

    pageLayout->addWidget(outputGroup);
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
    int screenIndex = m_settingsManager->outputScreenIndex();

    // Find the combo box item with this index
    for (int i = 0; i < m_screenComboBox->count(); ++i) {
        if (m_screenComboBox->itemData(i).toInt() == screenIndex) {
            m_screenComboBox->setCurrentIndex(i);
            break;
        }
    }
}

void SettingsDialog::saveSettings()
{
    if (!m_settingsManager) {
        qWarning() << "SettingsDialog: No settings manager provided";
        return;
    }

    // Save output screen index
    int screenIndex = m_screenComboBox->currentData().toInt();
    m_settingsManager->setOutputScreenIndex(screenIndex);
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
