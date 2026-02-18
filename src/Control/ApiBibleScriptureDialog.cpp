#include "ApiBibleScriptureDialog.h"
#include "Core/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDebug>

namespace Clarity {

ApiBibleScriptureDialog::ApiBibleScriptureDialog(ApiBibleClient* client, SettingsManager* settings,
                                                   ThemeManager* themeManager, QWidget* parent)
    : QDialog(parent)
    , m_client(client)
    , m_settings(settings)
    , m_themeManager(themeManager)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_fontFamily("Arial")
    , m_fontSize(48)
{
    setupUI();
    populateThemes();

    setWindowTitle(tr("Insert API.bible Scripture"));
    resize(700, 550);

    // Connect to API client signals
    connect(m_client, &ApiBibleClient::biblesLoaded, this, &ApiBibleScriptureDialog::onBiblesLoaded);
    connect(m_client, &ApiBibleClient::passageFetched, this, &ApiBibleScriptureDialog::onPassageFetched);
    connect(m_client, &ApiBibleClient::fetchError, this, &ApiBibleScriptureDialog::onFetchError);

    // Load available Bibles
    loadBibles();

    m_searchEdit->setFocus();
}

void ApiBibleScriptureDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Bible version selection
    QGroupBox* bibleGroup = new QGroupBox(tr("Bible Version"), this);
    QHBoxLayout* bibleLayout = new QHBoxLayout(bibleGroup);

    m_bibleCombo = new QComboBox(this);
    m_bibleCombo->setMinimumWidth(300);
    m_bibleCombo->addItem(tr("Loading Bible versions..."), "");
    bibleLayout->addWidget(m_bibleCombo, 1);

    m_refreshBiblesButton = new QPushButton(tr("Refresh"), this);
    connect(m_refreshBiblesButton, &QPushButton::clicked, this, [this]() {
        m_bibleCombo->clear();
        m_bibleCombo->addItem(tr("Loading Bible versions..."), "");
        m_client->fetchBibles("eng");
    });
    bibleLayout->addWidget(m_refreshBiblesButton);

    mainLayout->addWidget(bibleGroup);

    // Search section
    QGroupBox* searchGroup = new QGroupBox(tr("Scripture Reference"), this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

    // Reference format help
    QLabel* formatHelpLabel = new QLabel(
        tr("Enter a reference (e.g., John 3:16, Romans 8:28-30). "
           "The search will try to find the passage in the selected Bible version."),
        this);
    formatHelpLabel->setWordWrap(true);
    formatHelpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    searchLayout->addWidget(formatHelpLabel);

    // Search input row
    QHBoxLayout* searchInputLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Enter Bible reference (e.g., John 3:16, Romans 8:28-30)..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ApiBibleScriptureDialog::onSearch);
    searchInputLayout->addWidget(m_searchEdit);

    m_searchButton = new QPushButton(tr("Fetch"), this);
    m_searchButton->setDefault(false);
    connect(m_searchButton, &QPushButton::clicked, this, &ApiBibleScriptureDialog::onSearch);
    searchInputLayout->addWidget(m_searchButton);

    searchLayout->addLayout(searchInputLayout);

    // Status label for API feedback
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    searchLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(searchGroup);

    // Preview
    QLabel* previewLabel = new QLabel(tr("Preview:"), this);
    mainLayout->addWidget(previewLabel);

    m_previewEdit = new QTextEdit(this);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setStyleSheet(
        "QTextEdit { background-color: #1e3a8a; color: white; font-size: 14pt; padding: 10px; }"
    );
    mainLayout->addWidget(m_previewEdit, 1);

    // Copyright label
    m_copyrightLabel = new QLabel(this);
    m_copyrightLabel->setWordWrap(true);
    m_copyrightLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    mainLayout->addWidget(m_copyrightLabel);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsMainLayout = new QVBoxLayout(optionsGroup);

    // First row: checkboxes
    QHBoxLayout* optionsRow1 = new QHBoxLayout();

    m_includeVerseNumbersCheck = new QCheckBox(tr("Include verse numbers"), this);
    m_includeVerseNumbersCheck->setChecked(true);
    connect(m_includeVerseNumbersCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState) {
        updatePreview();
    });
    optionsRow1->addWidget(m_includeVerseNumbersCheck);

    m_onePerSlideCheck = new QCheckBox(tr("One verse per slide"), this);
    m_onePerSlideCheck->setChecked(m_settings ? m_settings->scriptureOneVersePerSlide() : false);
    optionsRow1->addWidget(m_onePerSlideCheck);

    optionsRow1->addStretch();
    optionsMainLayout->addLayout(optionsRow1);

    // Second row: theme and font size
    QHBoxLayout* optionsRow2 = new QHBoxLayout();

    optionsRow2->addWidget(new QLabel(tr("Theme:")));
    m_themeCombo = new QComboBox(this);
    m_themeCombo->setMinimumWidth(150);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiBibleScriptureDialog::onThemeChanged);
    optionsRow2->addWidget(m_themeCombo);

    optionsRow2->addSpacing(20);

    optionsRow2->addWidget(new QLabel(tr("Font size:")));
    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(tr(" pt"));
    optionsRow2->addWidget(m_fontSizeSpinBox);

    optionsRow2->addStretch();
    optionsMainLayout->addLayout(optionsRow2);

    mainLayout->addWidget(optionsGroup);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_insertButton = new QPushButton(tr("Insert"), this);
    m_insertButton->setEnabled(false);
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_insertButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void ApiBibleScriptureDialog::populateThemes()
{
    if (!m_themeManager) {
        m_themeCombo->addItem(tr("Default"), "");
        return;
    }

    m_themeCombo->blockSignals(true);

    m_themeCombo->addItem(tr("Current Style"), "_current_");
    m_themeCombo->insertSeparator(m_themeCombo->count());

    // Add scripture-specific themes first
    QList<Theme> allThemes = m_themeManager->allThemes();
    int firstScriptureIndex = -1;

    for (const Theme& theme : allThemes) {
        if (theme.name().startsWith("Scripture")) {
            if (firstScriptureIndex < 0) {
                firstScriptureIndex = m_themeCombo->count();
            }
            m_themeCombo->addItem(theme.name(), theme.name());
        }
    }

    if (firstScriptureIndex >= 0) {
        m_themeCombo->insertSeparator(m_themeCombo->count());
    }

    // Add all other themes
    for (const Theme& theme : allThemes) {
        if (!theme.name().startsWith("Scripture")) {
            m_themeCombo->addItem(theme.name(), theme.name());
        }
    }

    // Default to first scripture theme if available
    if (firstScriptureIndex >= 0) {
        m_themeCombo->setCurrentIndex(firstScriptureIndex);
        QString themeName = m_themeCombo->currentData().toString();
        Theme theme = m_themeManager->getTheme(themeName);
        applyTheme(theme);
    }

    m_themeCombo->blockSignals(false);
}

void ApiBibleScriptureDialog::applyTheme(const Theme& theme)
{
    m_selectedTheme = theme;
    m_useTheme = true;

    m_backgroundColor = theme.backgroundType() == Slide::SolidColor
        ? theme.backgroundColor()
        : theme.gradientStartColor();
    m_textColor = theme.textColor();
    m_fontFamily = theme.fontFamily();
    m_fontSize = theme.bodyFontSize();
    m_fontSizeSpinBox->setValue(m_fontSize);

    QString styleSheet = QString(
        "QTextEdit { background-color: %1; color: %2; font-family: %3; font-size: 14pt; padding: 10px; }"
    ).arg(m_backgroundColor.name(), m_textColor.name(), m_fontFamily);
    m_previewEdit->setStyleSheet(styleSheet);

    updatePreview();
}

void ApiBibleScriptureDialog::onThemeChanged(int index)
{
    Q_UNUSED(index)

    QString themeName = m_themeCombo->currentData().toString();
    if (themeName == "_current_" || themeName.isEmpty()) {
        m_useTheme = false;
        return;
    }

    if (!m_themeManager) {
        return;
    }

    Theme theme = m_themeManager->getTheme(themeName);
    if (!theme.name().isEmpty()) {
        applyTheme(theme);
    }
}

void ApiBibleScriptureDialog::loadBibles()
{
    if (m_client->hasCachedBibles()) {
        // Use previously fetched list
        onBiblesLoaded(m_client->cachedBibles());
    } else {
        // Fetch from API
        m_client->fetchBibles("eng");
    }
}

void ApiBibleScriptureDialog::onBiblesLoaded(const QList<ApiBibleVersion>& bibles)
{
    m_bibleCombo->clear();

    if (bibles.isEmpty()) {
        m_bibleCombo->addItem(tr("No Bible versions available"), "");
        return;
    }

    // Populate combo box, sorted by abbreviation
    for (const ApiBibleVersion& bible : bibles) {
        QString displayText = QStringLiteral("%1 - %2").arg(bible.abbreviation, bible.name);
        m_bibleCombo->addItem(displayText, bible.id);
    }

    // Try to select a saved preference or default to first item
    if (m_settings) {
        QString lastBibleId = m_settings->apiBibleLastBibleId();
        if (!lastBibleId.isEmpty()) {
            int idx = m_bibleCombo->findData(lastBibleId);
            if (idx >= 0) {
                m_bibleCombo->setCurrentIndex(idx);
            }
        }
    }
}

void ApiBibleScriptureDialog::onSearch()
{
    QString reference = m_searchEdit->text().trimmed();
    if (reference.isEmpty()) {
        return;
    }

    QString bibleId = m_bibleCombo->currentData().toString();
    if (bibleId.isEmpty()) {
        QMessageBox::warning(this, tr("No Bible Selected"),
            tr("Please select a Bible version before searching."));
        return;
    }

    m_statusLabel->setText(tr("Fetching from API.bible..."));
    m_statusLabel->setStyleSheet("");
    m_searchButton->setEnabled(false);
    m_insertButton->setEnabled(false);
    m_passage = ApiBiblePassage();  // Clear previous

    // Use the search endpoint which handles human-readable references well
    m_client->searchPassage(bibleId, reference);
}

void ApiBibleScriptureDialog::onPassageFetched(const ApiBiblePassage& passage)
{
    m_searchButton->setEnabled(true);
    m_passage = passage;

    // Store the Bible abbreviation from the selected combo item
    int currentIdx = m_bibleCombo->currentIndex();
    if (currentIdx >= 0 && m_client->hasCachedBibles()) {
        const QList<ApiBibleVersion>& bibles = m_client->cachedBibles();
        QString selectedId = m_bibleCombo->currentData().toString();
        for (const ApiBibleVersion& bible : bibles) {
            if (bible.id == selectedId) {
                m_passage.bibleAbbreviation = bible.abbreviation;
                m_passage.copyright = bible.copyright;
                break;
            }
        }
    }

    if (!passage.isValid()) {
        m_statusLabel->setText(tr("No passage found for the given reference."));
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        m_insertButton->setEnabled(false);
        m_previewEdit->clear();
        m_copyrightLabel->clear();
        return;
    }

    m_statusLabel->setText(tr("Found: %1 (%2 verses)")
                              .arg(passage.reference)
                              .arg(passage.verseCount));
    m_statusLabel->setStyleSheet("QLabel { color: green; }");

    // Show copyright
    if (!m_passage.copyright.isEmpty()) {
        m_copyrightLabel->setText(m_passage.copyright);
    }

    // Save the selected Bible ID for next time
    if (m_settings) {
        m_settings->setApiBibleLastBibleId(m_bibleCombo->currentData().toString());
    }

    m_insertButton->setEnabled(true);
    updatePreview();
}

void ApiBibleScriptureDialog::onFetchError(const QString& error)
{
    m_searchButton->setEnabled(true);
    m_statusLabel->setText(error);
    m_statusLabel->setStyleSheet("QLabel { color: red; }");
    m_insertButton->setEnabled(false);
    m_previewEdit->clear();
}

void ApiBibleScriptureDialog::updatePreview()
{
    if (!m_passage.isValid()) {
        return;
    }

    bool includeNumbers = m_includeVerseNumbersCheck->isChecked();
    bool onePerSlide = m_onePerSlideCheck->isChecked();

    QString previewText;

    if (onePerSlide && !m_passage.verses.isEmpty()) {
        // Show first verse as preview
        const ApiBibleVerse& verse = m_passage.verses.first();
        if (includeNumbers) {
            previewText = QStringLiteral("%1 %2").arg(verse.number).arg(verse.text);
        } else {
            previewText = verse.text;
        }
        if (m_passage.verses.count() > 1) {
            previewText += QStringLiteral("\n\n(+%1 more slides)").arg(m_passage.verses.count() - 1);
        }
    } else {
        // Combine all verses
        QStringList parts;
        for (const ApiBibleVerse& verse : m_passage.verses) {
            if (includeNumbers) {
                parts.append(QStringLiteral("%1 %2").arg(verse.number).arg(verse.text));
            } else {
                parts.append(verse.text);
            }
        }
        previewText = parts.join(" ");
    }

    // Add reference footer
    QString abbrSuffix;
    if (!m_passage.bibleAbbreviation.isEmpty()) {
        abbrSuffix = QStringLiteral(" (%1)").arg(m_passage.bibleAbbreviation);
    }
    previewText += QStringLiteral("\n\n- %1%2").arg(m_passage.reference, abbrSuffix);

    m_previewEdit->setPlainText(previewText);
}

void ApiBibleScriptureDialog::setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                                                const QString& fontFamily, int fontSize)
{
    m_backgroundColor = bgColor;
    m_textColor = textColor;
    m_fontFamily = fontFamily;
    m_fontSize = fontSize;
    m_fontSizeSpinBox->setValue(fontSize);

    QString styleSheet = QString(
        "QTextEdit { background-color: %1; color: %2; font-size: 14pt; padding: 10px; }"
    ).arg(bgColor.name(), textColor.name());
    m_previewEdit->setStyleSheet(styleSheet);
}

bool ApiBibleScriptureDialog::oneVersePerSlide() const
{
    return m_onePerSlideCheck->isChecked();
}

bool ApiBibleScriptureDialog::includeVerseNumbers() const
{
    return m_includeVerseNumbersCheck->isChecked();
}

SlideStyle ApiBibleScriptureDialog::slideStyle() const
{
    if (m_useTheme) {
        SlideStyle style = m_selectedTheme.toSlideStyle();
        style.fontSize = m_fontSizeSpinBox->value();
        return style;
    }
    return SlideStyle(m_backgroundColor, m_textColor, m_fontFamily, m_fontSizeSpinBox->value());
}

} // namespace Clarity
