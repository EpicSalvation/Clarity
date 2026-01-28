#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScreen>
#include <QGuiApplication>
#include <QColorDialog>
#include <QDebug>
#include <climits>

namespace Clarity {

SettingsDialog::SettingsDialog(SettingsManager* settingsManager, QWidget* parent)
    : QDialog(parent)
    , m_categoryList(nullptr)
    , m_pageStack(nullptr)
    , m_outputScreenComboBox(nullptr)
    , m_confidenceScreenComboBox(nullptr)
    , m_confidenceFontComboBox(nullptr)
    , m_confidenceFontSizeSpinBox(nullptr)
    , m_confidenceTextColorButton(nullptr)
    , m_confidenceBackgroundColorButton(nullptr)
    , m_confidenceTextColor(Qt::white)
    , m_confidenceBackgroundColor("#2a2a2a")
    , m_transitionTypeComboBox(nullptr)
    , m_transitionDurationComboBox(nullptr)
    , m_scrollWheelChangesInputsCheckBox(nullptr)
    , m_remoteControlEnabledCheckBox(nullptr)
    , m_remoteControlPortSpinBox(nullptr)
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
    m_categoryList->addItem("General");
    m_categoryList->addItem("Display");
    m_categoryList->addItem("Remote Control");

    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsDialog::onCategoryChanged);

    contentLayout->addWidget(m_categoryList);

    // Right side: Stacked widget for pages
    m_pageStack = new QStackedWidget(this);
    contentLayout->addWidget(m_pageStack, 1); // Stretch factor 1 to take remaining space

    // Create settings pages (order must match category list order)
    createGeneralPage();
    createDisplayPage();
    createRemoteControlPage();

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

void SettingsDialog::createGeneralPage()
{
    QWidget* generalPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(generalPage);

    // UI Behavior Settings group
    QGroupBox* uiBehaviorGroup = new QGroupBox("UI Behavior", generalPage);
    QVBoxLayout* uiBehaviorLayout = new QVBoxLayout(uiBehaviorGroup);

    m_scrollWheelChangesInputsCheckBox = new QCheckBox(
        "Scroll wheel changes dropdown and number inputs without clicking", uiBehaviorGroup);
    uiBehaviorLayout->addWidget(m_scrollWheelChangesInputsCheckBox);

    QLabel* uiBehaviorHelpLabel = new QLabel(
        "When disabled (default), you must click on dropdowns and number inputs "
        "before the scroll wheel will change their values. This prevents accidental changes "
        "when scrolling through dialogs.",
        uiBehaviorGroup);
    uiBehaviorHelpLabel->setWordWrap(true);
    uiBehaviorHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    uiBehaviorLayout->addWidget(uiBehaviorHelpLabel);

    pageLayout->addWidget(uiBehaviorGroup);

    pageLayout->addStretch(); // Push content to top

    m_pageStack->addWidget(generalPage);
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

    // Confidence Monitor Screen Settings group
    QGroupBox* confidenceScreenGroup = new QGroupBox("Confidence Monitor Screen", displayPage);
    QFormLayout* confidenceScreenLayout = new QFormLayout(confidenceScreenGroup);

    m_confidenceScreenComboBox = new QComboBox(confidenceScreenGroup);

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

    confidenceScreenLayout->addRow("Screen:", m_confidenceScreenComboBox);

    QLabel* confidenceHelpLabel = new QLabel(
        "Select which screen the confidence monitor will appear on when launched.",
        confidenceScreenGroup);
    confidenceHelpLabel->setWordWrap(true);
    confidenceHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    confidenceScreenLayout->addRow(confidenceHelpLabel);

    pageLayout->addWidget(confidenceScreenGroup);

    // Confidence Monitor Display Settings group
    QGroupBox* confidenceDisplayGroup = new QGroupBox("Confidence Monitor Display", displayPage);
    QFormLayout* confidenceDisplayLayout = new QFormLayout(confidenceDisplayGroup);

    // Font family
    m_confidenceFontComboBox = new QFontComboBox(confidenceDisplayGroup);
    confidenceDisplayLayout->addRow("Font:", m_confidenceFontComboBox);

    // Font size
    m_confidenceFontSizeSpinBox = new QSpinBox(confidenceDisplayGroup);
    m_confidenceFontSizeSpinBox->setRange(8, 200);
    m_confidenceFontSizeSpinBox->setSuffix(" pt");
    confidenceDisplayLayout->addRow("Font Size:", m_confidenceFontSizeSpinBox);

    // Text color
    m_confidenceTextColorButton = new QPushButton(confidenceDisplayGroup);
    m_confidenceTextColorButton->setMinimumWidth(100);
    connect(m_confidenceTextColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onConfidenceTextColorClicked);
    confidenceDisplayLayout->addRow("Text Color:", m_confidenceTextColorButton);

    // Background color
    m_confidenceBackgroundColorButton = new QPushButton(confidenceDisplayGroup);
    m_confidenceBackgroundColorButton->setMinimumWidth(100);
    connect(m_confidenceBackgroundColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onConfidenceBackgroundColorClicked);
    confidenceDisplayLayout->addRow("Background Color:", m_confidenceBackgroundColorButton);

    QLabel* displayHelpLabel = new QLabel(
        "These settings control how slide text appears on the confidence monitor.",
        confidenceDisplayGroup);
    displayHelpLabel->setWordWrap(true);
    displayHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    confidenceDisplayLayout->addRow(displayHelpLabel);

    pageLayout->addWidget(confidenceDisplayGroup);

    // Transition Settings group
    QGroupBox* transitionGroup = new QGroupBox("Slide Transitions", displayPage);
    QFormLayout* transitionLayout = new QFormLayout(transitionGroup);

    // Transition type
    m_transitionTypeComboBox = new QComboBox(transitionGroup);
    m_transitionTypeComboBox->addItem("Cut (Instant)", "cut");
    m_transitionTypeComboBox->addItem("Fade", "fade");
    m_transitionTypeComboBox->addItem("Slide Left", "slideLeft");
    m_transitionTypeComboBox->addItem("Slide Right", "slideRight");
    m_transitionTypeComboBox->addItem("Slide Up", "slideUp");
    m_transitionTypeComboBox->addItem("Slide Down", "slideDown");
    transitionLayout->addRow("Transition:", m_transitionTypeComboBox);

    // Transition duration
    m_transitionDurationComboBox = new QComboBox(transitionGroup);
    m_transitionDurationComboBox->addItem("Instant (0 ms)", 0);
    m_transitionDurationComboBox->addItem("Very Fast (250 ms)", 250);
    m_transitionDurationComboBox->addItem("Fast (500 ms)", 500);
    m_transitionDurationComboBox->addItem("Normal (750 ms)", 750);
    m_transitionDurationComboBox->addItem("Slow (1000 ms)", 1000);
    m_transitionDurationComboBox->addItem("Very Slow (1500 ms)", 1500);
    m_transitionDurationComboBox->addItem("Extra Slow (2000 ms)", 2000);
    transitionLayout->addRow("Duration:", m_transitionDurationComboBox);

    QLabel* transitionHelpLabel = new QLabel(
        "Controls how slides transition on the output display. "
        "The confidence monitor always shows instant transitions.",
        transitionGroup);
    transitionHelpLabel->setWordWrap(true);
    transitionHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    transitionLayout->addRow(transitionHelpLabel);

    pageLayout->addWidget(transitionGroup);

    pageLayout->addStretch(); // Push content to top

    m_pageStack->addWidget(displayPage);
}

void SettingsDialog::createRemoteControlPage()
{
    QWidget* remotePage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(remotePage);

    // Remote Control Settings group
    QGroupBox* remoteGroup = new QGroupBox("Remote Control Server", remotePage);
    QFormLayout* remoteLayout = new QFormLayout(remoteGroup);

    // Enable checkbox
    m_remoteControlEnabledCheckBox = new QCheckBox("Enable remote control server", remoteGroup);
    remoteLayout->addRow(m_remoteControlEnabledCheckBox);

    // Port setting
    m_remoteControlPortSpinBox = new QSpinBox(remoteGroup);
    m_remoteControlPortSpinBox->setRange(1024, 65535);
    m_remoteControlPortSpinBox->setValue(8080);
    remoteLayout->addRow("Port:", m_remoteControlPortSpinBox);

    QLabel* remoteHelpLabel = new QLabel(
        "When enabled, Clarity runs a web server that allows remote control from mobile "
        "devices on the same network. Open the URL shown in the status bar on your phone "
        "or tablet to control the presentation remotely.\n\n"
        "Note: Changes to these settings require restarting Clarity to take effect.",
        remoteGroup);
    remoteHelpLabel->setWordWrap(true);
    remoteHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    remoteLayout->addRow(remoteHelpLabel);

    pageLayout->addWidget(remoteGroup);

    pageLayout->addStretch(); // Push content to top

    m_pageStack->addWidget(remotePage);
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

    // Load confidence monitor display settings
    QString fontFamily = m_settingsManager->confidenceFontFamily();
    m_confidenceFontComboBox->setCurrentFont(QFont(fontFamily));

    m_confidenceFontSizeSpinBox->setValue(m_settingsManager->confidenceFontSize());

    m_confidenceTextColor = m_settingsManager->confidenceTextColor();
    updateColorButtonStyle(m_confidenceTextColorButton, m_confidenceTextColor);

    m_confidenceBackgroundColor = m_settingsManager->confidenceBackgroundColor();
    updateColorButtonStyle(m_confidenceBackgroundColorButton, m_confidenceBackgroundColor);

    // Load transition settings
    QString transitionType = m_settingsManager->transitionType();
    for (int i = 0; i < m_transitionTypeComboBox->count(); ++i) {
        if (m_transitionTypeComboBox->itemData(i).toString() == transitionType) {
            m_transitionTypeComboBox->setCurrentIndex(i);
            break;
        }
    }

    int transitionDuration = m_settingsManager->transitionDuration();
    // Find closest match in duration combo box
    int closestIndex = 0;
    int closestDiff = INT_MAX;
    for (int i = 0; i < m_transitionDurationComboBox->count(); ++i) {
        int diff = qAbs(m_transitionDurationComboBox->itemData(i).toInt() - transitionDuration);
        if (diff < closestDiff) {
            closestDiff = diff;
            closestIndex = i;
        }
    }
    m_transitionDurationComboBox->setCurrentIndex(closestIndex);

    // Load UI behavior settings
    m_scrollWheelChangesInputsCheckBox->setChecked(m_settingsManager->scrollWheelChangesInputs());

    // Load remote control settings
    m_remoteControlEnabledCheckBox->setChecked(m_settingsManager->remoteControlEnabled());
    m_remoteControlPortSpinBox->setValue(m_settingsManager->remoteControlPort());
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

    // Save confidence monitor display settings
    m_settingsManager->setConfidenceFontFamily(m_confidenceFontComboBox->currentFont().family());
    m_settingsManager->setConfidenceFontSize(m_confidenceFontSizeSpinBox->value());
    m_settingsManager->setConfidenceTextColor(m_confidenceTextColor);
    m_settingsManager->setConfidenceBackgroundColor(m_confidenceBackgroundColor);

    // Save transition settings
    QString transitionType = m_transitionTypeComboBox->currentData().toString();
    m_settingsManager->setTransitionType(transitionType);

    int transitionDuration = m_transitionDurationComboBox->currentData().toInt();
    m_settingsManager->setTransitionDuration(transitionDuration);

    // Save UI behavior settings
    m_settingsManager->setScrollWheelChangesInputs(m_scrollWheelChangesInputsCheckBox->isChecked());

    // Save remote control settings
    m_settingsManager->setRemoteControlEnabled(m_remoteControlEnabledCheckBox->isChecked());
    m_settingsManager->setRemoteControlPort(static_cast<quint16>(m_remoteControlPortSpinBox->value()));
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

void SettingsDialog::onConfidenceTextColorClicked()
{
    QColor color = QColorDialog::getColor(m_confidenceTextColor, this, "Select Text Color");
    if (color.isValid()) {
        m_confidenceTextColor = color;
        updateColorButtonStyle(m_confidenceTextColorButton, color);
    }
}

void SettingsDialog::onConfidenceBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(m_confidenceBackgroundColor, this, "Select Background Color");
    if (color.isValid()) {
        m_confidenceBackgroundColor = color;
        updateColorButtonStyle(m_confidenceBackgroundColorButton, color);
    }
}

void SettingsDialog::updateColorButtonStyle(QPushButton* button, const QColor& color)
{
    // Set button background to the selected color
    // Use contrasting text color for readability
    int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    QString textColor = brightness > 128 ? "black" : "white";

    button->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; }")
                          .arg(color.name())
                          .arg(textColor));
    button->setText(color.name().toUpper());
}

} // namespace Clarity
