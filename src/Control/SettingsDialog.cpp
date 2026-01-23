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
    bool found = false;
    for (int i = 0; i < m_screenComboBox->count(); ++i) {
        if (m_screenComboBox->itemData(i).toInt() == screenIndex) {
            m_screenComboBox->setCurrentIndex(i);
            found = true;
            break;
        }
    }

    // If the saved screen index is not found (e.g., screen was disconnected),
    // default to the first available screen
    if (!found && m_screenComboBox->count() > 0) {
        m_screenComboBox->setCurrentIndex(0);
        qDebug() << "SettingsDialog: Saved screen" << screenIndex
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
