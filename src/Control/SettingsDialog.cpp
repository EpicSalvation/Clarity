// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SettingsDialog.h"
#include "TourOverlay.h"
#include "AppStyle.h"
#include "BibleImportDialog.h"
#include "CCLIReportDialog.h"
#include "Core/BibleDatabase.h"
#include "Core/SongLibrary.h"
#include "Core/ThemeManager.h"
#include "Core/WheelEventFilter.h"
#include <QAbstractSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QColorDialog>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QDebug>
#include <QShowEvent>
#include <QTimer>
#include <climits>

namespace Clarity {

SettingsDialog::SettingsDialog(SettingsManager* settingsManager, QWidget* parent)
    : QDialog(parent)
    , m_categoryList(nullptr)
    , m_pageStack(nullptr)
    , m_themeComboBox(nullptr)
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
    , m_showAllSlidesInGridCheckBox(nullptr)
    , m_slidePreviewSizeComboBox(nullptr)
    , m_languageComboBox(nullptr)
    , m_remoteControlEnabledCheckBox(nullptr)
    , m_remoteControlPortSpinBox(nullptr)
    , m_translationsListWidget(nullptr)
    , m_importTranslationButton(nullptr)
    , m_deleteTranslationButton(nullptr)
    , m_bibleDatabase(nullptr)
    , m_cascadingBackgroundsCheckBox(nullptr)
    , m_scriptureThemeOverrideCheckBox(nullptr)
    , m_scriptureThemeOverrideCombo(nullptr)
    , m_themeManager(nullptr)
    , m_songLibrary(nullptr)
    , m_autoSyncLibraryGroupsCheckBox(nullptr)
    , m_redLettersEnabledCheckBox(nullptr)
    , m_redLetterColorButton(nullptr)
    , m_redLetterColor("#cc0000")
    , m_ccliLicenseNumberEdit(nullptr)
    , m_showCcliOnTitleSlidesCheckBox(nullptr)
    , m_showCopyrightSlideCheckBox(nullptr)
    , m_esvApiKeyEdit(nullptr)
    , m_esvCacheStatusLabel(nullptr)
    , m_apiBibleApiKeyEdit(nullptr)
    , m_settingsManager(settingsManager)
    , m_replaySettingsTourButton(nullptr)
{
    setupUI();
    loadSettings();
}

void SettingsDialog::setBibleDatabase(BibleDatabase* database)
{
    m_bibleDatabase = database;
    refreshTranslationsList();
}

void SettingsDialog::setThemeManager(ThemeManager* themeManager)
{
    m_themeManager = themeManager;

    // Populate scripture theme override combo
    if (m_scriptureThemeOverrideCombo && m_themeManager) {
        m_scriptureThemeOverrideCombo->clear();
        QStringList names = m_themeManager->themeNames();
        for (const QString& name : names) {
            m_scriptureThemeOverrideCombo->addItem(name, name);
        }

        // Select the saved theme
        if (m_settingsManager) {
            QString saved = m_settingsManager->scriptureThemeOverrideName();
            int idx = m_scriptureThemeOverrideCombo->findData(saved);
            if (idx >= 0) {
                m_scriptureThemeOverrideCombo->setCurrentIndex(idx);
            }
        }
    }
}

void SettingsDialog::setSongLibrary(SongLibrary* library)
{
    m_songLibrary = library;
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("Settings"));
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
    m_categoryList->addItem(tr("General"));
    m_categoryList->addItem(tr("Display"));
    m_categoryList->addItem(tr("Bible"));
    m_categoryList->addItem(tr("Copyright"));
    m_categoryList->addItem(tr("Remote Control"));

    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsDialog::onCategoryChanged);

    contentLayout->addWidget(m_categoryList);

    // Right side: Stacked widget for pages
    m_pageStack = new QStackedWidget(this);
    contentLayout->addWidget(m_pageStack, 1); // Stretch factor 1 to take remaining space

    // Create settings pages (order must match category list order)
    createGeneralPage();
    createDisplayPage();
    createBiblePage();
    createCopyrightPage();
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

    // Install wheel event filter on all spinboxes and combo boxes so that
    // scrolling in the settings pages forwards to the page's scroll area
    // instead of changing input values.
    auto* wheelFilter = new WheelEventFilter(m_settingsManager, this);
    for (auto* w : findChildren<QAbstractSpinBox*>())
        w->installEventFilter(wheelFilter);
    for (auto* w : findChildren<QComboBox*>())
        w->installEventFilter(wheelFilter);
}

void SettingsDialog::createGeneralPage()
{
    QWidget* generalPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(generalPage);

    // Appearance Settings group
    QGroupBox* appearanceGroup = new QGroupBox(tr("Appearance"), generalPage);
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);

    m_themeComboBox = new QComboBox(appearanceGroup);
    m_themeComboBox->addItem(tr("System Default"), "system");
    m_themeComboBox->addItem(tr("Light"),          "light");
    m_themeComboBox->addItem(tr("Dark"),           "dark");
    appearanceLayout->addRow(tr("Theme:"), m_themeComboBox);

    // Live preview: apply theme immediately when the combo changes
    connect(m_themeComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        QString mode = m_themeComboBox->currentData().toString();
        AppStyle::apply(qApp, AppStyle::fromString(mode));
    });

    QLabel* themeHelpLabel = new QLabel(
        tr("Changes take effect immediately. Restart is not required."),
        appearanceGroup);
    themeHelpLabel->setWordWrap(true);
    themeHelpLabel->setStyleSheet("QLabel { color: palette(mid); font-size: 10pt; }");
    appearanceLayout->addRow(themeHelpLabel);

    pageLayout->addWidget(appearanceGroup);

    // Language Settings group
    QGroupBox* languageGroup = new QGroupBox(tr("Language"), generalPage);
    QFormLayout* languageLayout = new QFormLayout(languageGroup);

    m_languageComboBox = new QComboBox(languageGroup);
    m_languageComboBox->addItem(tr("System Default"), "system");
    m_languageComboBox->addItem("English", "en");
    m_languageComboBox->addItem("Español (Spanish)", "es");
    m_languageComboBox->addItem("Deutsch (German)", "de");
    m_languageComboBox->addItem("Français (French)", "fr");
    languageLayout->addRow(tr("Language:"), m_languageComboBox);

    QLabel* languageHelpLabel = new QLabel(
        tr("Select the interface language. Changes require restarting Clarity to take effect."),
        languageGroup);
    languageHelpLabel->setWordWrap(true);
    languageHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    languageLayout->addRow(languageHelpLabel);

    pageLayout->addWidget(languageGroup);

    // UI Behavior Settings group
    QGroupBox* uiBehaviorGroup = new QGroupBox(tr("UI Behavior"), generalPage);
    QVBoxLayout* uiBehaviorLayout = new QVBoxLayout(uiBehaviorGroup);

    m_scrollWheelChangesInputsCheckBox = new QCheckBox(
        tr("Scroll wheel changes dropdown and number inputs without clicking"), uiBehaviorGroup);
    uiBehaviorLayout->addWidget(m_scrollWheelChangesInputsCheckBox);

    m_showAllSlidesInGridCheckBox = new QCheckBox(
        tr("Show all slides in grid (instead of just the selected item's slides)"), uiBehaviorGroup);
    uiBehaviorLayout->addWidget(m_showAllSlidesInGridCheckBox);

    // Slide preview size selector
    QHBoxLayout* previewSizeLayout = new QHBoxLayout();
    QLabel* previewSizeLabel = new QLabel(tr("Slide preview size:"), uiBehaviorGroup);
    previewSizeLayout->addWidget(previewSizeLabel);

    m_slidePreviewSizeComboBox = new QComboBox(uiBehaviorGroup);
    m_slidePreviewSizeComboBox->addItem(tr("Small"), "small");
    m_slidePreviewSizeComboBox->addItem(tr("Medium"), "medium");
    m_slidePreviewSizeComboBox->addItem(tr("Large"), "large");
    previewSizeLayout->addWidget(m_slidePreviewSizeComboBox);
    previewSizeLayout->addStretch();

    uiBehaviorLayout->addLayout(previewSizeLayout);

    QLabel* uiBehaviorHelpLabel = new QLabel(
        tr("By default, the slide grid shows only slides from the currently selected item. "
           "Enable 'Show all slides' to see all presentation slides at once."),
        uiBehaviorGroup);
    uiBehaviorHelpLabel->setWordWrap(true);
    uiBehaviorHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    uiBehaviorLayout->addWidget(uiBehaviorHelpLabel);

    pageLayout->addWidget(uiBehaviorGroup);

    // Background Settings group
    QGroupBox* backgroundGroup = new QGroupBox(tr("Backgrounds"), generalPage);
    QVBoxLayout* backgroundLayout = new QVBoxLayout(backgroundGroup);

    m_cascadingBackgroundsCheckBox = new QCheckBox(
        tr("Cascade backgrounds (backgrounds persist until another is explicitly set)"), backgroundGroup);
    backgroundLayout->addWidget(m_cascadingBackgroundsCheckBox);

    m_scriptureThemeOverrideCheckBox = new QCheckBox(
        tr("Override scripture backgrounds with theme:"), backgroundGroup);
    backgroundLayout->addWidget(m_scriptureThemeOverrideCheckBox);

    m_scriptureThemeOverrideCombo = new QComboBox(backgroundGroup);
    m_scriptureThemeOverrideCombo->setEnabled(false);
    backgroundLayout->addWidget(m_scriptureThemeOverrideCombo);

    // Enable/disable combo based on checkbox
    connect(m_scriptureThemeOverrideCheckBox, &QCheckBox::toggled,
            m_scriptureThemeOverrideCombo, &QComboBox::setEnabled);

    QLabel* backgroundHelpLabel = new QLabel(
        tr("When cascading is enabled, a slide without its own background inherits "
           "the background of the previous slide that has one. This is useful for "
           "having a consistent background across songs and scripture."),
        backgroundGroup);
    backgroundHelpLabel->setWordWrap(true);
    backgroundHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    backgroundLayout->addWidget(backgroundHelpLabel);

    pageLayout->addWidget(backgroundGroup);

    // Library Settings group
    QGroupBox* libraryGroup = new QGroupBox(tr("Library"), generalPage);
    QVBoxLayout* libraryLayout = new QVBoxLayout(libraryGroup);

    m_autoSyncLibraryGroupsCheckBox = new QCheckBox(
        tr("Automatically update library when slide groups are edited"), libraryGroup);
    libraryLayout->addWidget(m_autoSyncLibraryGroupsCheckBox);

    QLabel* librarySyncHelpLabel = new QLabel(
        tr("When enabled, changes to slide groups that were inserted from the library "
           "are automatically saved back. You can always update manually via right-click."),
        libraryGroup);
    librarySyncHelpLabel->setWordWrap(true);
    librarySyncHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    libraryLayout->addWidget(librarySyncHelpLabel);

    pageLayout->addWidget(libraryGroup);

    // Tour replay button
    QGroupBox* tourGroup = new QGroupBox(tr("Help"), generalPage);
    QVBoxLayout* tourLayout = new QVBoxLayout(tourGroup);
    m_autoCheckUpdatesCheckbox = new QCheckBox(tr("Check for updates automatically"), tourGroup);
    m_autoCheckUpdatesCheckbox->setChecked(m_settingsManager->autoCheckForUpdates());
    tourLayout->addWidget(m_autoCheckUpdatesCheckbox);

    m_includeBetaCheckbox = new QCheckBox(tr("Include beta releases"), tourGroup);
    m_includeBetaCheckbox->setChecked(m_settingsManager->includeBetaUpdates());
    m_includeBetaCheckbox->setToolTip(tr("Notify about pre-release builds in addition to stable releases"));
    tourLayout->addWidget(m_includeBetaCheckbox);

    m_replaySettingsTourButton = new QPushButton(tr("Replay Settings Tour"), tourGroup);
    m_replaySettingsTourButton->setToolTip(tr("Watch the guided tour of Settings again"));
    connect(m_replaySettingsTourButton, &QPushButton::clicked,
            this, &SettingsDialog::startSettingsTour);
    tourLayout->addWidget(m_replaySettingsTourButton);
    pageLayout->addWidget(tourGroup);

    pageLayout->addStretch(); // Push content to top

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(generalPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_pageStack->addWidget(scrollArea);
}

void SettingsDialog::createDisplayPage()
{
    QWidget* displayPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(displayPage);

    // Get available screens (used by both output and confidence)
    QList<QScreen*> screens = QGuiApplication::screens();

    // Output Display Settings group
    QGroupBox* outputGroup = new QGroupBox(tr("Output Display"), displayPage);
    QFormLayout* outputLayout = new QFormLayout(outputGroup);

    m_outputScreenComboBox = new QComboBox(outputGroup);

    // Populate with available screens
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QString screenInfo = tr("Screen %1").arg(i + 1);

        // Add screen name if available
        if (!screen->name().isEmpty()) {
            screenInfo += QString(" (%1)").arg(screen->name());
        }

        // Add resolution
        QRect geometry = screen->geometry();
        screenInfo += QString(" - %1x%2").arg(geometry.width()).arg(geometry.height());

        // Mark primary screen
        if (screen == QGuiApplication::primaryScreen()) {
            screenInfo += " " + tr("[Primary]");
        }

        m_outputScreenComboBox->addItem(screenInfo, i);
    }

    outputLayout->addRow(tr("Screen:"), m_outputScreenComboBox);

    QLabel* outputHelpLabel = new QLabel(
        tr("Select which screen the output display will appear on when launched."),
        outputGroup);
    outputHelpLabel->setWordWrap(true);
    outputHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    outputLayout->addRow(outputHelpLabel);

    pageLayout->addWidget(outputGroup);

    // Confidence Monitor Screen Settings group
    QGroupBox* confidenceScreenGroup = new QGroupBox(tr("Confidence Monitor Screen"), displayPage);
    QFormLayout* confidenceScreenLayout = new QFormLayout(confidenceScreenGroup);

    m_confidenceScreenComboBox = new QComboBox(confidenceScreenGroup);

    // Populate with available screens
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QString screenInfo = tr("Screen %1").arg(i + 1);

        // Add screen name if available
        if (!screen->name().isEmpty()) {
            screenInfo += QString(" (%1)").arg(screen->name());
        }

        // Add resolution
        QRect geometry = screen->geometry();
        screenInfo += QString(" - %1x%2").arg(geometry.width()).arg(geometry.height());

        // Mark primary screen
        if (screen == QGuiApplication::primaryScreen()) {
            screenInfo += " " + tr("[Primary]");
        }

        m_confidenceScreenComboBox->addItem(screenInfo, i);
    }

    confidenceScreenLayout->addRow(tr("Screen:"), m_confidenceScreenComboBox);

    QLabel* confidenceHelpLabel = new QLabel(
        tr("Select which screen the confidence monitor will appear on when launched."),
        confidenceScreenGroup);
    confidenceHelpLabel->setWordWrap(true);
    confidenceHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    confidenceScreenLayout->addRow(confidenceHelpLabel);

    pageLayout->addWidget(confidenceScreenGroup);

    // Confidence Monitor Display Settings group
    QGroupBox* confidenceDisplayGroup = new QGroupBox(tr("Confidence Monitor Display"), displayPage);
    QFormLayout* confidenceDisplayLayout = new QFormLayout(confidenceDisplayGroup);

    // Font family
    m_confidenceFontComboBox = new QFontComboBox(confidenceDisplayGroup);
    confidenceDisplayLayout->addRow(tr("Font:"), m_confidenceFontComboBox);

    // Font size
    m_confidenceFontSizeSpinBox = new QSpinBox(confidenceDisplayGroup);
    m_confidenceFontSizeSpinBox->setRange(8, 200);
    m_confidenceFontSizeSpinBox->setSuffix(tr(" pt"));
    confidenceDisplayLayout->addRow(tr("Font Size:"), m_confidenceFontSizeSpinBox);

    // Text color
    m_confidenceTextColorButton = new QPushButton(confidenceDisplayGroup);
    m_confidenceTextColorButton->setMinimumWidth(100);
    connect(m_confidenceTextColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onConfidenceTextColorClicked);
    confidenceDisplayLayout->addRow(tr("Text Color:"), m_confidenceTextColorButton);

    // Background color
    m_confidenceBackgroundColorButton = new QPushButton(confidenceDisplayGroup);
    m_confidenceBackgroundColorButton->setMinimumWidth(100);
    connect(m_confidenceBackgroundColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onConfidenceBackgroundColorClicked);
    confidenceDisplayLayout->addRow(tr("Background Color:"), m_confidenceBackgroundColorButton);

    QLabel* displayHelpLabel = new QLabel(
        tr("These settings control how slide text appears on the confidence monitor."),
        confidenceDisplayGroup);
    displayHelpLabel->setWordWrap(true);
    displayHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    confidenceDisplayLayout->addRow(displayHelpLabel);

    pageLayout->addWidget(confidenceDisplayGroup);

    // Transition Settings group
    QGroupBox* transitionGroup = new QGroupBox(tr("Slide Transitions"), displayPage);
    QFormLayout* transitionLayout = new QFormLayout(transitionGroup);

    // Transition type
    m_transitionTypeComboBox = new QComboBox(transitionGroup);
    m_transitionTypeComboBox->addItem(tr("Cut (Instant)"), "cut");
    m_transitionTypeComboBox->addItem(tr("Fade"), "fade");
    m_transitionTypeComboBox->addItem(tr("Slide Left"), "slideLeft");
    m_transitionTypeComboBox->addItem(tr("Slide Right"), "slideRight");
    m_transitionTypeComboBox->addItem(tr("Slide Up"), "slideUp");
    m_transitionTypeComboBox->addItem(tr("Slide Down"), "slideDown");
    transitionLayout->addRow(tr("Transition:"), m_transitionTypeComboBox);

    // Transition duration
    m_transitionDurationComboBox = new QComboBox(transitionGroup);
    m_transitionDurationComboBox->addItem(tr("Instant (0 ms)"), 0);
    m_transitionDurationComboBox->addItem(tr("Very Fast (250 ms)"), 250);
    m_transitionDurationComboBox->addItem(tr("Fast (500 ms)"), 500);
    m_transitionDurationComboBox->addItem(tr("Normal (750 ms)"), 750);
    m_transitionDurationComboBox->addItem(tr("Slow (1000 ms)"), 1000);
    m_transitionDurationComboBox->addItem(tr("Very Slow (1500 ms)"), 1500);
    m_transitionDurationComboBox->addItem(tr("Extra Slow (2000 ms)"), 2000);
    transitionLayout->addRow(tr("Duration:"), m_transitionDurationComboBox);

    QLabel* transitionHelpLabel = new QLabel(
        tr("Controls how slides transition on the output display. "
           "The confidence monitor always shows instant transitions."),
        transitionGroup);
    transitionHelpLabel->setWordWrap(true);
    transitionHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    transitionLayout->addRow(transitionHelpLabel);

    pageLayout->addWidget(transitionGroup);

    pageLayout->addStretch(); // Push content to top

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(displayPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_pageStack->addWidget(scrollArea);
}

void SettingsDialog::createRemoteControlPage()
{
    QWidget* remotePage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(remotePage);

    // Remote Control Settings group
    QGroupBox* remoteGroup = new QGroupBox(tr("Remote Control Server"), remotePage);
    QFormLayout* remoteLayout = new QFormLayout(remoteGroup);

    // Enable checkbox
    m_remoteControlEnabledCheckBox = new QCheckBox(tr("Enable remote control server"), remoteGroup);
    remoteLayout->addRow(m_remoteControlEnabledCheckBox);

    // Port setting
    m_remoteControlPortSpinBox = new QSpinBox(remoteGroup);
    m_remoteControlPortSpinBox->setRange(1024, 65535);
    m_remoteControlPortSpinBox->setValue(8080);
    remoteLayout->addRow(tr("Port:"), m_remoteControlPortSpinBox);

    pageLayout->addWidget(remoteGroup);

    // PIN Security group
    QGroupBox* pinGroup = new QGroupBox(tr("PIN Security"), remotePage);
    QFormLayout* pinLayout = new QFormLayout(pinGroup);

    // PIN enable checkbox
    m_remoteControlPinEnabledCheckBox = new QCheckBox(tr("Require PIN to connect"), pinGroup);
    pinLayout->addRow(m_remoteControlPinEnabledCheckBox);

    // PIN entry
    m_remoteControlPinEdit = new QLineEdit(pinGroup);
    m_remoteControlPinEdit->setPlaceholderText(tr("4-8 digits"));
    m_remoteControlPinEdit->setMaxLength(8);
    m_remoteControlPinEdit->setEchoMode(QLineEdit::Password);
    // Only allow digits
    m_remoteControlPinEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("^[0-9]{0,8}$"), m_remoteControlPinEdit));
    pinLayout->addRow(tr("PIN:"), m_remoteControlPinEdit);

    // Enable/disable PIN edit based on checkbox
    connect(m_remoteControlPinEnabledCheckBox, &QCheckBox::toggled,
            m_remoteControlPinEdit, &QLineEdit::setEnabled);

    QLabel* pinHelpLabel = new QLabel(
        tr("When PIN is enabled, users must enter the correct PIN before they can "
           "control the presentation. This prevents unauthorized access from other "
           "devices on the network."),
        pinGroup);
    pinHelpLabel->setWordWrap(true);
    pinHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    pinLayout->addRow(pinHelpLabel);

    pageLayout->addWidget(pinGroup);

    // Help text
    QLabel* remoteHelpLabel = new QLabel(
        tr("When enabled, Clarity runs a web server that allows remote control from mobile "
           "devices on the same network. Open the URL shown in the status bar on your phone "
           "or tablet to control the presentation remotely.\n\n"
           "Note: Changes to these settings take effect immediately."),
        remotePage);
    remoteHelpLabel->setWordWrap(true);
    remoteHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    pageLayout->addWidget(remoteHelpLabel);

    pageLayout->addStretch(); // Push content to top

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(remotePage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_pageStack->addWidget(scrollArea);
}

void SettingsDialog::createBiblePage()
{
    QWidget* biblePage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(biblePage);

    // Default Translation group
    QGroupBox* defaultGroup = new QGroupBox(tr("Default Translation"), biblePage);
    QFormLayout* defaultLayout = new QFormLayout(defaultGroup);

    m_preferredTranslationComboBox = new QComboBox(defaultGroup);
    defaultLayout->addRow(tr("Preferred Translation:"), m_preferredTranslationComboBox);

    m_rememberLastTranslationCheckBox = new QCheckBox(
        tr("Remember last used translation instead"), defaultGroup);
    defaultLayout->addRow(m_rememberLastTranslationCheckBox);

    QLabel* defaultHelpLabel = new QLabel(
        tr("When enabled, the Scripture dialog will remember and use the last translation you selected. "
           "Otherwise, it will always start with the preferred translation above."),
        defaultGroup);
    defaultHelpLabel->setWordWrap(true);
    defaultHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    defaultLayout->addRow(defaultHelpLabel);

    pageLayout->addWidget(defaultGroup);

    // Scripture Formatting group
    QGroupBox* scriptureFormatGroup = new QGroupBox(tr("Scripture Formatting"), biblePage);
    QFormLayout* scriptureFormatLayout = new QFormLayout(scriptureFormatGroup);

    m_scriptureRefPositionCombo = new QComboBox(scriptureFormatGroup);
    m_scriptureRefPositionCombo->addItem(tr("Top"), "top");
    m_scriptureRefPositionCombo->addItem(tr("Bottom"), "bottom");
    scriptureFormatLayout->addRow(tr("Reference Position:"), m_scriptureRefPositionCombo);

    QLabel* refPosHelpLabel = new QLabel(
        tr("Controls where the scripture reference (e.g., \"John 3:16\") appears on new scripture slides. "
           "Existing slides are not affected."),
        scriptureFormatGroup);
    refPosHelpLabel->setWordWrap(true);
    refPosHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    scriptureFormatLayout->addRow(refPosHelpLabel);

    pageLayout->addWidget(scriptureFormatGroup);

    // Red Letter Edition group
    QGroupBox* redLetterGroup = new QGroupBox(tr("Red Letter Edition"), biblePage);
    QFormLayout* redLetterLayout = new QFormLayout(redLetterGroup);

    m_redLettersEnabledCheckBox = new QCheckBox(
        tr("Show words of Jesus in red"), redLetterGroup);
    redLetterLayout->addRow(m_redLettersEnabledCheckBox);

    // Color picker button
    m_redLetterColorButton = new QPushButton(redLetterGroup);
    m_redLetterColorButton->setMinimumWidth(100);
    connect(m_redLetterColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onRedLetterColorClicked);
    redLetterLayout->addRow(tr("Red Letter Color:"), m_redLetterColorButton);

    // Enable/disable color button based on checkbox
    connect(m_redLettersEnabledCheckBox, &QCheckBox::toggled,
            m_redLetterColorButton, &QPushButton::setEnabled);

    QLabel* redLetterHelpLabel = new QLabel(
        tr("When enabled, words spoken by Jesus will be displayed in the selected color. "
           "Not all translations include red letter markup. Supported sources include "
           "local imports (WEB, NET, etc.) and some API.bible versions. "
           "The ESV API does not provide red letter data."),
        redLetterGroup);
    redLetterHelpLabel->setWordWrap(true);
    redLetterHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    redLetterLayout->addRow(redLetterHelpLabel);

    pageLayout->addWidget(redLetterGroup);

    // Installed Translations group
    QGroupBox* translationsGroup = new QGroupBox(tr("Installed Translations"), biblePage);
    QVBoxLayout* translationsLayout = new QVBoxLayout(translationsGroup);

    m_translationsListWidget = new QListWidget(translationsGroup);
    m_translationsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_translationsListWidget->setMaximumHeight(150);
    translationsLayout->addWidget(m_translationsListWidget);

    // Buttons row
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_importTranslationButton = new QPushButton(tr("Import Translation..."), translationsGroup);
    connect(m_importTranslationButton, &QPushButton::clicked,
            this, &SettingsDialog::onImportTranslationClicked);
    buttonLayout->addWidget(m_importTranslationButton);

    m_deleteTranslationButton = new QPushButton(tr("Delete"), translationsGroup);
    m_deleteTranslationButton->setEnabled(false);
    connect(m_deleteTranslationButton, &QPushButton::clicked,
            this, &SettingsDialog::onDeleteTranslationClicked);
    buttonLayout->addWidget(m_deleteTranslationButton);

    buttonLayout->addStretch();

    translationsLayout->addLayout(buttonLayout);

    // Enable/disable delete button based on selection
    connect(m_translationsListWidget, &QListWidget::itemSelectionChanged,
            this, [this]() {
        m_deleteTranslationButton->setEnabled(
            m_translationsListWidget->currentItem() != nullptr);
    });

    pageLayout->addWidget(translationsGroup);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("Import Bible translations from common formats: OSIS XML, USFM, USX, Zefania XML, or TSV.\n"
           "Public domain translations like KJV and WEB can be downloaded from Crosswire Bible Society."),
        biblePage);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    pageLayout->addWidget(helpLabel);

    // ESV API group
    QGroupBox* esvGroup = new QGroupBox(tr("ESV Bible API"), biblePage);
    QFormLayout* esvLayout = new QFormLayout(esvGroup);

    m_esvApiKeyEdit = new QLineEdit(esvGroup);
    m_esvApiKeyEdit->setPlaceholderText(tr("Enter your ESV API key..."));
    m_esvApiKeyEdit->setEchoMode(QLineEdit::Password);
    esvLayout->addRow(tr("API Key:"), m_esvApiKeyEdit);

    m_esvCacheStatusLabel = new QLabel(esvGroup);
    m_esvCacheStatusLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    esvLayout->addRow(tr("Cache:"), m_esvCacheStatusLabel);

    QLabel* esvHelpLabel = new QLabel(
        tr("To use the ESV (English Standard Version) translation, you need an API key from "
           "api.esv.org. Create a free account and request an API key.\n\n"
           "ESV terms limit caching to 500 verses. Cached verses should be cleared periodically."),
        esvGroup);
    esvHelpLabel->setWordWrap(true);
    esvHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    esvLayout->addRow(esvHelpLabel);

    pageLayout->addWidget(esvGroup);

    // API.bible group
    QGroupBox* apiBibleGroup = new QGroupBox(tr("API.bible"), biblePage);
    QFormLayout* apiBibleLayout = new QFormLayout(apiBibleGroup);

    m_apiBibleApiKeyEdit = new QLineEdit(apiBibleGroup);
    m_apiBibleApiKeyEdit->setPlaceholderText(tr("Enter your API.bible key..."));
    m_apiBibleApiKeyEdit->setEchoMode(QLineEdit::Password);
    apiBibleLayout->addRow(tr("API Key:"), m_apiBibleApiKeyEdit);

    QLabel* apiBibleHelpLabel = new QLabel(
        tr("API.bible provides access to nearly 2500 Bible versions across 1600+ languages. "
           "Create a free account at scripture.api.bible to obtain an API key."),
        apiBibleGroup);
    apiBibleHelpLabel->setWordWrap(true);
    apiBibleHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    apiBibleLayout->addRow(apiBibleHelpLabel);

    pageLayout->addWidget(apiBibleGroup);

    pageLayout->addStretch(); // Push content to top

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(biblePage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_pageStack->addWidget(scrollArea);
}

void SettingsDialog::createCopyrightPage()
{
    QWidget* copyrightPage = new QWidget(this);
    QVBoxLayout* pageLayout = new QVBoxLayout(copyrightPage);

    // CCLI License group
    QGroupBox* ccliGroup = new QGroupBox(tr("CCLI License"), copyrightPage);
    QFormLayout* ccliLayout = new QFormLayout(ccliGroup);

    m_ccliLicenseNumberEdit = new QLineEdit(ccliGroup);
    m_ccliLicenseNumberEdit->setPlaceholderText(tr("e.g. 1234567"));
    ccliLayout->addRow(tr("License Number:"), m_ccliLicenseNumberEdit);

    m_showCcliOnTitleSlidesCheckBox = new QCheckBox(
        tr("Show CCLI info on song title slides"), ccliGroup);
    ccliLayout->addRow(m_showCcliOnTitleSlidesCheckBox);

    QLabel* ccliHelpLabel = new QLabel(
        tr("When enabled, the CCLI song number and your license number appear below the song title "
           "on title slides."),
        ccliGroup);
    ccliHelpLabel->setWordWrap(true);
    ccliHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    ccliLayout->addRow(ccliHelpLabel);

    pageLayout->addWidget(ccliGroup);

    // Copyright Slide group
    QGroupBox* copyrightSlideGroup = new QGroupBox(tr("Copyright Slide"), copyrightPage);
    QVBoxLayout* copyrightSlideLayout = new QVBoxLayout(copyrightSlideGroup);

    m_showCopyrightSlideCheckBox = new QCheckBox(
        tr("Auto-generate copyright slide at end of presentation"), copyrightSlideGroup);
    copyrightSlideLayout->addWidget(m_showCopyrightSlideCheckBox);

    QLabel* copyrightSlideHelpLabel = new QLabel(
        tr("Collects all copyright and attribution notices into a single slide at the end "
           "of the presentation."),
        copyrightSlideGroup);
    copyrightSlideHelpLabel->setWordWrap(true);
    copyrightSlideHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    copyrightSlideLayout->addWidget(copyrightSlideHelpLabel);

    pageLayout->addWidget(copyrightSlideGroup);

    // CCLI Usage Report button
    QPushButton* ccliReportButton = new QPushButton(tr("CCLI Usage Report..."), copyrightPage);
    connect(ccliReportButton, &QPushButton::clicked, this, [this]() {
        CCLIReportDialog dialog(m_songLibrary, this);
        dialog.exec();
    });
    pageLayout->addWidget(ccliReportButton);

    pageLayout->addStretch();

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(copyrightPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_pageStack->addWidget(scrollArea);
}

void SettingsDialog::refreshTranslationsList()
{
    if (!m_translationsListWidget) {
        return;
    }

    m_translationsListWidget->clear();
    m_preferredTranslationComboBox->clear();

    if (!m_bibleDatabase || !m_bibleDatabase->isValid()) {
        m_translationsListWidget->addItem(tr("(Database not available)"));
        m_preferredTranslationComboBox->addItem(tr("(None available)"), "");
        m_importTranslationButton->setEnabled(false);
        return;
    }

    m_importTranslationButton->setEnabled(true);

    QStringList translations = m_bibleDatabase->availableTranslations();
    if (translations.isEmpty()) {
        m_translationsListWidget->addItem(tr("(No translations installed)"));
        m_preferredTranslationComboBox->addItem(tr("(None installed)"), "");
        return;
    }

    for (const QString& code : translations) {
        // Add to list widget
        QListWidgetItem* item = new QListWidgetItem(code, m_translationsListWidget);
        item->setData(Qt::UserRole, code);

        // Add to combo box
        m_preferredTranslationComboBox->addItem(code, code);
    }

    // Select the preferred translation in the combo box
    if (m_settingsManager) {
        QString preferred = m_settingsManager->preferredBibleTranslation();
        int index = m_preferredTranslationComboBox->findData(preferred);
        if (index >= 0) {
            m_preferredTranslationComboBox->setCurrentIndex(index);
        }

        // Set the checkbox state
        m_rememberLastTranslationCheckBox->setChecked(
            m_settingsManager->rememberLastBibleTranslation());
    }
}

void SettingsDialog::onImportTranslationClicked()
{
    if (!m_bibleDatabase) {
        QMessageBox::warning(this, tr("Database Error"),
            tr("Bible database is not available."));
        return;
    }

    BibleImportDialog dialog(m_bibleDatabase, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshTranslationsList();
    }
}

void SettingsDialog::onDeleteTranslationClicked()
{
    QListWidgetItem* item = m_translationsListWidget->currentItem();
    if (!item) {
        return;
    }

    QString code = item->data(Qt::UserRole).toString();
    if (code.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(this, tr("Confirm Delete"),
        tr("Are you sure you want to delete the '%1' translation?\n\n"
           "This will remove all verses for this translation from the database.")
            .arg(code),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    if (m_bibleDatabase->deleteTranslation(code)) {
        refreshTranslationsList();
        QMessageBox::information(this, tr("Translation Deleted"),
            tr("Translation '%1' has been deleted.").arg(code));
    } else {
        QMessageBox::critical(this, tr("Delete Failed"),
            tr("Failed to delete translation '%1'.").arg(code));
    }
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

    // Load appearance (theme) setting
    {
        QString saved = m_settingsManager->themeMode();
        int idx = m_themeComboBox->findData(saved);
        m_themeComboBox->setCurrentIndex(idx >= 0 ? idx : 0);
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
    m_showAllSlidesInGridCheckBox->setChecked(m_settingsManager->showAllSlidesInGrid());
    m_autoSyncLibraryGroupsCheckBox->setChecked(m_settingsManager->autoSyncLibraryGroups());

    // Load cascading background settings
    m_cascadingBackgroundsCheckBox->setChecked(m_settingsManager->cascadingBackgrounds());
    m_scriptureThemeOverrideCheckBox->setChecked(m_settingsManager->scriptureThemeOverride());
    m_scriptureThemeOverrideCombo->setEnabled(m_settingsManager->scriptureThemeOverride());

    // Load slide preview size
    QString previewSize = m_settingsManager->slidePreviewSize();
    for (int i = 0; i < m_slidePreviewSizeComboBox->count(); ++i) {
        if (m_slidePreviewSizeComboBox->itemData(i).toString() == previewSize) {
            m_slidePreviewSizeComboBox->setCurrentIndex(i);
            break;
        }
    }

    // Load language setting
    QString languageCode = m_settingsManager->language();
    for (int i = 0; i < m_languageComboBox->count(); ++i) {
        if (m_languageComboBox->itemData(i).toString() == languageCode) {
            m_languageComboBox->setCurrentIndex(i);
            break;
        }
    }

    // Load remote control settings
    m_remoteControlEnabledCheckBox->setChecked(m_settingsManager->remoteControlEnabled());
    m_remoteControlPortSpinBox->setValue(m_settingsManager->remoteControlPort());

    // Load PIN settings
    m_remoteControlPinEnabledCheckBox->setChecked(m_settingsManager->remoteControlPinEnabled());
    m_remoteControlPinEdit->setText(m_settingsManager->remoteControlPin());
    m_remoteControlPinEdit->setEnabled(m_settingsManager->remoteControlPinEnabled());

    // Load scripture formatting settings
    int refPosIndex = m_scriptureRefPositionCombo->findData(m_settingsManager->scriptureReferencePosition());
    if (refPosIndex >= 0) m_scriptureRefPositionCombo->setCurrentIndex(refPosIndex);

    // Load red letter settings
    m_redLettersEnabledCheckBox->setChecked(m_settingsManager->redLettersEnabled());
    m_redLetterColor = QColor(m_settingsManager->redLetterColor());
    updateColorButtonStyle(m_redLetterColorButton, m_redLetterColor);
    m_redLetterColorButton->setEnabled(m_settingsManager->redLettersEnabled());

    // Load Copyright & CCLI settings
    m_ccliLicenseNumberEdit->setText(m_settingsManager->ccliLicenseNumber());
    m_showCcliOnTitleSlidesCheckBox->setChecked(m_settingsManager->showCcliOnTitleSlides());
    m_showCopyrightSlideCheckBox->setChecked(m_settingsManager->showCopyrightSlide());

    // Load ESV API settings
    m_esvApiKeyEdit->setText(m_settingsManager->esvApiKey());
    int cachedVerses = m_settingsManager->esvCachedVerseCount();
    m_esvCacheStatusLabel->setText(tr("%1 / %2 verses cached").arg(cachedVerses).arg(500));

    // Load API.bible settings
    m_apiBibleApiKeyEdit->setText(m_settingsManager->apiBibleApiKey());
}

void SettingsDialog::saveSettings()
{
    if (!m_settingsManager) {
        qWarning() << "SettingsDialog: No settings manager provided";
        return;
    }

    // Save appearance (theme) setting
    m_settingsManager->setThemeMode(m_themeComboBox->currentData().toString());

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
    m_settingsManager->setShowAllSlidesInGrid(m_showAllSlidesInGridCheckBox->isChecked());
    m_settingsManager->setAutoSyncLibraryGroups(m_autoSyncLibraryGroupsCheckBox->isChecked());
    m_settingsManager->setSlidePreviewSize(m_slidePreviewSizeComboBox->currentData().toString());

    // Save cascading background settings
    m_settingsManager->setCascadingBackgrounds(m_cascadingBackgroundsCheckBox->isChecked());
    m_settingsManager->setScriptureThemeOverride(m_scriptureThemeOverrideCheckBox->isChecked());
    if (m_scriptureThemeOverrideCombo->currentIndex() >= 0) {
        m_settingsManager->setScriptureThemeOverrideName(
            m_scriptureThemeOverrideCombo->currentData().toString());
    }

    // Save language setting
    QString languageCode = m_languageComboBox->currentData().toString();
    m_settingsManager->setLanguage(languageCode);

    // Save remote control settings
    m_settingsManager->setRemoteControlEnabled(m_remoteControlEnabledCheckBox->isChecked());
    m_settingsManager->setRemoteControlPort(static_cast<quint16>(m_remoteControlPortSpinBox->value()));

    // Save PIN settings
    m_settingsManager->setRemoteControlPinEnabled(m_remoteControlPinEnabledCheckBox->isChecked());
    m_settingsManager->setRemoteControlPin(m_remoteControlPinEdit->text());

    // Save Bible translation settings
    QString preferredTranslation = m_preferredTranslationComboBox->currentData().toString();
    if (!preferredTranslation.isEmpty()) {
        m_settingsManager->setPreferredBibleTranslation(preferredTranslation);
    }
    m_settingsManager->setRememberLastBibleTranslation(m_rememberLastTranslationCheckBox->isChecked());

    // Save scripture formatting settings
    m_settingsManager->setScriptureReferencePosition(
        m_scriptureRefPositionCombo->currentData().toString());

    // Save red letter settings
    m_settingsManager->setRedLettersEnabled(m_redLettersEnabledCheckBox->isChecked());
    m_settingsManager->setRedLetterColor(m_redLetterColor.name());

    // Save Copyright & CCLI settings
    m_settingsManager->setCcliLicenseNumber(m_ccliLicenseNumberEdit->text());
    m_settingsManager->setShowCcliOnTitleSlides(m_showCcliOnTitleSlidesCheckBox->isChecked());
    m_settingsManager->setShowCopyrightSlide(m_showCopyrightSlideCheckBox->isChecked());

    // Save update check settings
    m_settingsManager->setAutoCheckForUpdates(m_autoCheckUpdatesCheckbox->isChecked());
    m_settingsManager->setIncludeBetaUpdates(m_includeBetaCheckbox->isChecked());

    // Save ESV API settings
    m_settingsManager->setEsvApiKey(m_esvApiKeyEdit->text());

    // Save API.bible settings
    m_settingsManager->setApiBibleApiKey(m_apiBibleApiKeyEdit->text());
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
    QColor color = QColorDialog::getColor(m_confidenceTextColor, this, tr("Select Text Color"));
    if (color.isValid()) {
        m_confidenceTextColor = color;
        updateColorButtonStyle(m_confidenceTextColorButton, color);
    }
}

void SettingsDialog::onConfidenceBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(m_confidenceBackgroundColor, this, tr("Select Background Color"));
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

void SettingsDialog::onRedLetterColorClicked()
{
    QColor color = QColorDialog::getColor(m_redLetterColor, this, tr("Select Red Letter Color"));
    if (color.isValid()) {
        m_redLetterColor = color;
        updateColorButtonStyle(m_redLetterColorButton, color);
    }
}

void SettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // Launch the settings tour the first time this dialog is shown.
    // Mark immediately to prevent duplicate tours if the dialog is closed and reopened.
    if (m_settingsManager && !m_settingsManager->hasCompletedSettingsTour()) {
        m_settingsManager->setHasCompletedSettingsTour(true);
        QTimer::singleShot(300, this, &SettingsDialog::startSettingsTour);
    }
}

void SettingsDialog::startSettingsTour()
{
    // Category indices match the order in setupUI:
    //   0 = General, 1 = Display, 2 = Bible, 3 = Copyright, 4 = Remote Control
    QList<TourOverlay::Step> steps;

    steps.append({
        m_categoryList,
        tr("Settings Categories"),
        tr("Click any category in this list to jump to that group of settings. Let's walk through the most important ones."),
        [this]() { m_categoryList->setCurrentRow(0); }
    });

    steps.append({
        m_pageStack,
        tr("Display Settings"),
        tr("Choose which monitor to use for your output display and confidence monitor. You can also set transitions and preview sizes here."),
        [this]() { m_categoryList->setCurrentRow(1); }
    });

    steps.append({
        m_pageStack,
        tr("Bible Settings"),
        tr("Import Bible translations and choose your preferred version for scripture slides. Red-letter edition and scripture reference position are also configured here."),
        [this]() { m_categoryList->setCurrentRow(2); }
    });

    steps.append({
        m_pageStack,
        tr("Remote Control"),
        tr("Enable the built-in web remote to control slides from your phone or tablet over your local network. No extra software needed."),
        [this]() { m_categoryList->setCurrentRow(4); }
    });

    steps.append({
        nullptr,
        tr("You're All Set!"),
        tr("That covers the key settings. Make any changes you need, then click OK to save. You can always replay this tour from the General tab."),
        [this]() { m_categoryList->setCurrentRow(0); }
    });

    auto* tour = new TourOverlay(this, steps);
    connect(tour, &TourOverlay::completed, this, [this]() {
        if (m_settingsManager)
            m_settingsManager->setHasCompletedSettingsTour(true);
    });
    connect(tour, &TourOverlay::skipped, this, [this]() {
        if (m_settingsManager)
            m_settingsManager->setHasCompletedSettingsTour(true);
    });
    tour->start();
}

} // namespace Clarity
