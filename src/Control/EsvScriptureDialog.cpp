#include "EsvScriptureDialog.h"
#include "Core/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QDebug>

namespace Clarity {

EsvScriptureDialog::EsvScriptureDialog(EsvApiClient* esvClient, SettingsManager* settings,
                                       ThemeManager* themeManager, QWidget* parent)
    : QDialog(parent)
    , m_esvClient(esvClient)
    , m_settings(settings)
    , m_themeManager(themeManager)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_fontFamily("Arial")
    , m_fontSize(48)
{
    setupUI();
    populateThemes();
    updateCacheStatus();

    setWindowTitle(tr("Insert ESV Scripture"));
    resize(650, 500);

    // Connect to ESV API client signals
    connect(m_esvClient, &EsvApiClient::passageFetched, this, &EsvScriptureDialog::onPassageFetched);
    connect(m_esvClient, &EsvApiClient::fetchError, this, &EsvScriptureDialog::onFetchError);

    m_searchEdit->setFocus();
}

void EsvScriptureDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search section
    QGroupBox* searchGroup = new QGroupBox(tr("ESV Scripture Reference"), this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

    // Search input row
    QHBoxLayout* searchInputLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Enter Bible reference (e.g., John 3:16, Romans 8:28-30)..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &EsvScriptureDialog::onSearch);
    searchInputLayout->addWidget(m_searchEdit);

    m_searchButton = new QPushButton(tr("Fetch"), this);
    m_searchButton->setDefault(false);
    connect(m_searchButton, &QPushButton::clicked, this, &EsvScriptureDialog::onSearch);
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

    // Cache warning label
    m_cacheLabel = new QLabel(this);
    m_cacheLabel->setWordWrap(true);
    m_cacheLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    mainLayout->addWidget(m_cacheLabel);

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
            this, &EsvScriptureDialog::onThemeChanged);
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

void EsvScriptureDialog::populateThemes()
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

void EsvScriptureDialog::applyTheme(const Theme& theme)
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

void EsvScriptureDialog::onThemeChanged(int index)
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

void EsvScriptureDialog::onSearch()
{
    QString reference = m_searchEdit->text().trimmed();
    if (reference.isEmpty()) {
        return;
    }

    if (!m_esvClient || !m_esvClient->hasApiKey()) {
        QMessageBox::warning(this, tr("ESV API Key Required"),
            tr("No ESV API key is configured.\n\n"
               "Please go to Settings > Bible and enter your ESV API key.\n"
               "You can get a key at api.esv.org."));
        return;
    }

    m_statusLabel->setText(tr("Fetching from ESV API..."));
    m_statusLabel->setStyleSheet("");
    m_searchButton->setEnabled(false);
    m_insertButton->setEnabled(false);
    m_passage = EsvPassage();  // Clear previous

    m_esvClient->fetchPassage(reference);
}

void EsvScriptureDialog::onPassageFetched(const EsvPassage& passage)
{
    m_searchButton->setEnabled(true);
    m_passage = passage;

    if (!passage.isValid()) {
        m_statusLabel->setText(tr("No passage found for the given reference."));
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        m_insertButton->setEnabled(false);
        m_previewEdit->clear();
        return;
    }

    // Check cache limit
    if (m_esvClient->wouldExceedCacheLimit(passage.verseCount)) {
        m_statusLabel->setText(tr("Warning: Inserting %1 verses would exceed the 500-verse ESV cache limit. "
                                  "Current cache: %2 verses.")
                                  .arg(passage.verseCount)
                                  .arg(m_esvClient->cachedVerseCount()));
        m_statusLabel->setStyleSheet("QLabel { color: orange; }");
    } else {
        m_statusLabel->setText(tr("Found: %1 (%2 verses)")
                                  .arg(passage.canonical)
                                  .arg(passage.verseCount));
        m_statusLabel->setStyleSheet("QLabel { color: green; }");
    }

    m_insertButton->setEnabled(true);
    updatePreview();
    updateCacheStatus();
}

void EsvScriptureDialog::onFetchError(const QString& error)
{
    m_searchButton->setEnabled(true);
    m_statusLabel->setText(error);
    m_statusLabel->setStyleSheet("QLabel { color: red; }");
    m_insertButton->setEnabled(false);
    m_previewEdit->clear();
}

void EsvScriptureDialog::updatePreview()
{
    if (!m_passage.isValid()) {
        return;
    }

    bool includeNumbers = m_includeVerseNumbersCheck->isChecked();
    bool onePerSlide = m_onePerSlideCheck->isChecked();

    QString previewText;

    if (onePerSlide && !m_passage.verses.isEmpty()) {
        // Show first verse as preview
        const EsvVerse& verse = m_passage.verses.first();
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
        for (const EsvVerse& verse : m_passage.verses) {
            if (includeNumbers) {
                parts.append(QStringLiteral("%1 %2").arg(verse.number).arg(verse.text));
            } else {
                parts.append(verse.text);
            }
        }
        previewText = parts.join(" ");
    }

    // Add reference and copyright footer
    previewText += QStringLiteral("\n\n- %1 (ESV)").arg(m_passage.canonical);

    m_previewEdit->setPlainText(previewText);
}

void EsvScriptureDialog::updateCacheStatus()
{
    if (!m_esvClient) {
        return;
    }

    int cached = m_esvClient->cachedVerseCount();
    int pending = m_passage.isValid() ? m_passage.verseCount : 0;
    int total = cached + pending;

    QString text = tr("ESV verse cache: %1 / %2").arg(cached).arg(EsvApiClient::MAX_CACHED_VERSES);
    if (pending > 0) {
        text += tr(" (+%1 pending)").arg(pending);
    }
    if (total > EsvApiClient::MAX_CACHED_VERSES) {
        text += tr(" - EXCEEDS LIMIT");
        m_cacheLabel->setStyleSheet("QLabel { color: red; font-size: 10pt; }");
    } else {
        m_cacheLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    }

    m_cacheLabel->setText(text);
}

void EsvScriptureDialog::setDefaultStyle(const QColor& bgColor, const QColor& textColor,
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

bool EsvScriptureDialog::oneVersePerSlide() const
{
    return m_onePerSlideCheck->isChecked();
}

bool EsvScriptureDialog::includeVerseNumbers() const
{
    return m_includeVerseNumbersCheck->isChecked();
}

SlideStyle EsvScriptureDialog::slideStyle() const
{
    if (m_useTheme) {
        SlideStyle style = m_selectedTheme.toSlideStyle();
        style.fontSize = m_fontSizeSpinBox->value();
        return style;
    }
    return SlideStyle(m_backgroundColor, m_textColor, m_fontFamily, m_fontSizeSpinBox->value());
}

} // namespace Clarity
