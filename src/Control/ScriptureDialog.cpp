#include "ScriptureDialog.h"
#include "Core/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QDebug>

namespace Clarity {

ScriptureDialog::ScriptureDialog(BibleDatabase* bible, SettingsManager* settings,
                                   ThemeManager* themeManager, QWidget* parent)
    : QDialog(parent)
    , m_bible(bible)
    , m_settings(settings)
    , m_themeManager(themeManager)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_fontFamily("Arial")
    , m_fontSize(48)
{
    setupUI();
    populateTranslations();
    populateThemes();

    setWindowTitle(tr("Insert Scripture"));
    resize(700, 550);

    // Set focus to search field
    m_searchEdit->setFocus();
}

void ScriptureDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search section
    QGroupBox* searchGroup = new QGroupBox(tr("Search Scripture"), this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

    // Search type and translation row
    QHBoxLayout* searchOptionsLayout = new QHBoxLayout();

    m_searchTypeCombo = new QComboBox(this);
    m_searchTypeCombo->addItem("Reference (e.g., John 3:16)", "reference");
    m_searchTypeCombo->addItem("Keyword Search", "keyword");
    m_searchTypeCombo->setToolTip("Choose search type: reference lookup or keyword search");
    searchOptionsLayout->addWidget(new QLabel("Search by:"));
    searchOptionsLayout->addWidget(m_searchTypeCombo);

    searchOptionsLayout->addSpacing(20);

    m_translationCombo = new QComboBox(this);
    m_translationCombo->setMinimumWidth(80);
    connect(m_translationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScriptureDialog::onTranslationChanged);
    searchOptionsLayout->addWidget(new QLabel("Translation:"));
    searchOptionsLayout->addWidget(m_translationCombo);
    searchOptionsLayout->addStretch();

    searchLayout->addLayout(searchOptionsLayout);

    // Search input row
    QHBoxLayout* searchInputLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Enter Bible reference or search keyword...");
    m_searchEdit->setClearButtonEnabled(true);
    // Connect Enter key to search
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ScriptureDialog::onSearch);
    searchInputLayout->addWidget(m_searchEdit);

    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setDefault(false);  // Don't make it default to avoid conflicts
    connect(m_searchButton, &QPushButton::clicked, this, &ScriptureDialog::onSearch);
    searchInputLayout->addWidget(m_searchButton);

    searchLayout->addLayout(searchInputLayout);

    mainLayout->addWidget(searchGroup);

    // Results and preview in a splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Results section
    QWidget* resultsWidget = new QWidget(this);
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsWidget);
    resultsLayout->setContentsMargins(0, 0, 0, 0);

    m_resultsCountLabel = new QLabel("Results: 0", this);
    resultsLayout->addWidget(m_resultsCountLabel);

    m_resultsList = new QListWidget(this);
    m_resultsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_resultsList->setToolTip("Click to select verses. Ctrl+Click for multiple selection.");
    connect(m_resultsList, &QListWidget::itemSelectionChanged,
            this, &ScriptureDialog::onResultSelectionChanged);
    resultsLayout->addWidget(m_resultsList);

    splitter->addWidget(resultsWidget);

    // Preview section
    QWidget* previewWidget = new QWidget(this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* previewLabel = new QLabel("Preview:", this);
    previewLayout->addWidget(previewLabel);

    m_previewEdit = new QTextEdit(this);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setStyleSheet(
        "QTextEdit { background-color: #1e3a8a; color: white; font-size: 14pt; padding: 10px; }"
    );
    previewLayout->addWidget(m_previewEdit);

    splitter->addWidget(previewWidget);
    splitter->setSizes({300, 400});

    mainLayout->addWidget(splitter, 1);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsMainLayout = new QVBoxLayout(optionsGroup);

    // First row: checkboxes
    QHBoxLayout* optionsRow1 = new QHBoxLayout();

    m_includeReferenceCheck = new QCheckBox("Include reference on slide", this);
    m_includeReferenceCheck->setChecked(true);
    m_includeReferenceCheck->setToolTip("Add the Bible reference (e.g., John 3:16) to each slide");
    connect(m_includeReferenceCheck, &QCheckBox::checkStateChanged,
            this, &ScriptureDialog::onIncludeReferenceChanged);
    optionsRow1->addWidget(m_includeReferenceCheck);

    m_onePerSlideCheck = new QCheckBox("One verse per slide", this);
    m_onePerSlideCheck->setChecked(m_settings ? m_settings->scriptureOneVersePerSlide() : false);
    m_onePerSlideCheck->setToolTip("Create separate slides for each verse");
    connect(m_onePerSlideCheck, &QCheckBox::checkStateChanged,
            this, &ScriptureDialog::onOnePerSlideChanged);
    optionsRow1->addWidget(m_onePerSlideCheck);

    optionsRow1->addStretch();
    optionsMainLayout->addLayout(optionsRow1);

    // Second row: theme and font size
    QHBoxLayout* optionsRow2 = new QHBoxLayout();

    optionsRow2->addWidget(new QLabel("Theme:"));
    m_themeCombo = new QComboBox(this);
    m_themeCombo->setMinimumWidth(150);
    m_themeCombo->setToolTip("Select a theme for scripture slides (Scripture themes are designed for Bible text)");
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScriptureDialog::onThemeChanged);
    optionsRow2->addWidget(m_themeCombo);

    optionsRow2->addSpacing(20);

    optionsRow2->addWidget(new QLabel("Font size:"));
    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(" pt");
    optionsRow2->addWidget(m_fontSizeSpinBox);

    optionsRow2->addStretch();
    optionsMainLayout->addLayout(optionsRow2);

    mainLayout->addWidget(optionsGroup);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_insertButton = new QPushButton("Insert", this);
    m_insertButton->setEnabled(false);  // Disabled until selection
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_insertButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void ScriptureDialog::populateTranslations()
{
    if (!m_bible || !m_bible->isValid()) {
        m_translationCombo->addItem("(No Bible database)", "");
        m_searchButton->setEnabled(false);
        m_searchEdit->setEnabled(false);
        return;
    }

    QStringList translations = m_bible->availableTranslations();
    if (translations.isEmpty()) {
        m_translationCombo->addItem("(No translations found)", "");
        return;
    }

    // Block signals while populating to avoid saving the wrong "last used" translation
    m_translationCombo->blockSignals(true);

    for (const QString& translation : translations) {
        m_translationCombo->addItem(translation, translation);
    }

    // Select translation based on settings (or fallback to database default)
    QString preferredTranslation;
    if (m_settings) {
        preferredTranslation = m_settings->effectiveBibleTranslation();
    } else {
        preferredTranslation = m_bible->defaultTranslation();
    }

    int defaultIndex = m_translationCombo->findText(preferredTranslation);
    if (defaultIndex >= 0) {
        m_translationCombo->setCurrentIndex(defaultIndex);
        m_bible->setDefaultTranslation(preferredTranslation);
    } else if (m_translationCombo->count() > 0) {
        // Fallback to first available translation
        m_translationCombo->setCurrentIndex(0);
        m_bible->setDefaultTranslation(m_translationCombo->currentData().toString());
    }

    m_translationCombo->blockSignals(false);
}

void ScriptureDialog::populateThemes()
{
    if (!m_themeManager) {
        m_themeCombo->addItem("Default", "");
        return;
    }

    m_themeCombo->blockSignals(true);

    // Add a "Current Style" option to keep whatever was set via setDefaultStyle()
    m_themeCombo->addItem("Current Style", "_current_");

    // Add separator before scripture themes
    m_themeCombo->insertSeparator(m_themeCombo->count());

    // Add scripture-specific themes first (themes starting with "Scripture")
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

    // Add separator before other themes
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

void ScriptureDialog::applyTheme(const Theme& theme)
{
    // Store the theme for slide creation
    m_selectedTheme = theme;
    m_useTheme = true;

    // Extract colors for preview (use gradient start color for gradient backgrounds)
    m_backgroundColor = theme.backgroundType() == Slide::SolidColor
        ? theme.backgroundColor()
        : theme.gradientStartColor();
    m_textColor = theme.textColor();
    m_fontFamily = theme.fontFamily();
    m_fontSize = theme.bodyFontSize();
    m_fontSizeSpinBox->setValue(m_fontSize);

    // Update preview style
    QString bgColor = m_backgroundColor.name();
    QString textColor = m_textColor.name();

    QString styleSheet = QString(
        "QTextEdit { background-color: %1; color: %2; font-family: %3; font-size: 14pt; padding: 10px; }"
    ).arg(bgColor, textColor, m_fontFamily);
    m_previewEdit->setStyleSheet(styleSheet);

    updatePreview();
}

void ScriptureDialog::onThemeChanged(int index)
{
    Q_UNUSED(index)

    QString themeName = m_themeCombo->currentData().toString();

    // "Current Style" keeps the existing settings (set via setDefaultStyle)
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

void ScriptureDialog::onSearch()
{
    QString searchType = m_searchTypeCombo->currentData().toString();

    if (searchType == "reference") {
        onReferenceSearch();
    } else {
        onKeywordSearch();
    }
}

void ScriptureDialog::onReferenceSearch()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }

    if (!m_bible || !m_bible->isValid()) {
        QMessageBox::warning(this, "Error", "Bible database is not available.");
        return;
    }

    m_resultsList->clear();
    m_searchResults.clear();
    m_selectedVerses.clear();

    // Look up reference
    QList<BibleVerse> verses = m_bible->lookupReference(searchText);

    if (verses.isEmpty()) {
        m_resultsCountLabel->setText("Results: 0 - Reference not found");
        m_previewEdit->clear();
        m_insertButton->setEnabled(false);
        return;
    }

    m_searchResults = verses;

    // Populate results list
    for (const BibleVerse& verse : verses) {
        QString itemText = QString("%1 %2:%3 - %4")
            .arg(verse.book)
            .arg(verse.chapter)
            .arg(verse.verse)
            .arg(verse.text.left(60) + (verse.text.length() > 60 ? "..." : ""));

        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, m_resultsList->count());  // Store index
        m_resultsList->addItem(item);
    }

    m_resultsCountLabel->setText(QString("Results: %1 verse(s)").arg(verses.count()));

    // Select all results by default for reference lookup
    m_resultsList->selectAll();
}

void ScriptureDialog::onKeywordSearch()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }

    if (!m_bible || !m_bible->isValid()) {
        QMessageBox::warning(this, "Error", "Bible database is not available.");
        return;
    }

    m_resultsList->clear();
    m_searchResults.clear();
    m_selectedVerses.clear();

    // Search by keyword
    QList<BibleVerse> verses = m_bible->searchKeyword(searchText, 100);

    if (verses.isEmpty()) {
        m_resultsCountLabel->setText("Results: 0 - No matching verses found");
        m_previewEdit->clear();
        m_insertButton->setEnabled(false);
        return;
    }

    m_searchResults = verses;

    // Populate results list
    for (const BibleVerse& verse : verses) {
        QString itemText = QString("%1 %2:%3 - %4")
            .arg(verse.book)
            .arg(verse.chapter)
            .arg(verse.verse)
            .arg(verse.text.left(60) + (verse.text.length() > 60 ? "..." : ""));

        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, m_resultsList->count());  // Store index
        m_resultsList->addItem(item);
    }

    m_resultsCountLabel->setText(QString("Results: %1 verse(s)").arg(verses.count()));
}

void ScriptureDialog::onResultSelectionChanged()
{
    m_selectedVerses.clear();

    QList<QListWidgetItem*> selectedItems = m_resultsList->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        int index = item->data(Qt::UserRole).toInt();
        if (index >= 0 && index < m_searchResults.count()) {
            m_selectedVerses.append(m_searchResults[index]);
        }
    }

    m_insertButton->setEnabled(!m_selectedVerses.isEmpty());
    updatePreview();
}

void ScriptureDialog::onTranslationChanged(int index)
{
    Q_UNUSED(index)

    if (!m_bible) return;

    QString translation = m_translationCombo->currentData().toString();
    if (!translation.isEmpty()) {
        m_bible->setDefaultTranslation(translation);

        // Save the last used translation
        if (m_settings) {
            m_settings->setLastBibleTranslation(translation);
        }
    }

    // Re-run search if there was one
    if (!m_searchEdit->text().isEmpty()) {
        onSearch();
    }
}

void ScriptureDialog::onIncludeReferenceChanged(Qt::CheckState state)
{
    Q_UNUSED(state)
    updatePreview();
}

void ScriptureDialog::onOnePerSlideChanged(Qt::CheckState state)
{
    // Save the preference
    if (m_settings) {
        m_settings->setScriptureOneVersePerSlide(state == Qt::Checked);
    }
    updatePreview();
}

void ScriptureDialog::updatePreview()
{
    if (m_selectedVerses.isEmpty()) {
        m_previewEdit->clear();
        return;
    }

    bool includeRef = m_includeReferenceCheck->isChecked();
    bool onePerSlide = m_onePerSlideCheck->isChecked();

    QString previewText;

    if (onePerSlide) {
        // Show first verse as preview
        const BibleVerse& verse = m_selectedVerses.first();
        previewText = formatVerseText(verse, includeRef);
        if (m_selectedVerses.count() > 1) {
            previewText += QString("\n\n(+%1 more slides)").arg(m_selectedVerses.count() - 1);
        }
    } else {
        // Combine all verses
        QStringList texts;
        for (const BibleVerse& verse : m_selectedVerses) {
            texts.append(verse.text);
        }

        previewText = texts.join(" ");

        if (includeRef && !m_selectedVerses.isEmpty()) {
            // Build reference string for range
            const BibleVerse& first = m_selectedVerses.first();
            const BibleVerse& last = m_selectedVerses.last();

            QString refStr;
            if (m_selectedVerses.count() == 1) {
                refStr = first.reference();
            } else if (first.book == last.book && first.chapter == last.chapter) {
                refStr = QString("%1 %2:%3-%4")
                    .arg(first.book).arg(first.chapter).arg(first.verse).arg(last.verse);
            } else if (first.book == last.book) {
                refStr = QString("%1 %2:%3-%4:%5")
                    .arg(first.book).arg(first.chapter).arg(first.verse)
                    .arg(last.chapter).arg(last.verse);
            } else {
                refStr = QString("%1 - %2").arg(first.reference(), last.reference());
            }

            previewText += QString("\n\n- %1 (%2)").arg(refStr, first.translation);
        }
    }

    m_previewEdit->setPlainText(previewText);
}

QString ScriptureDialog::formatVerseText(const BibleVerse& verse, bool includeReference) const
{
    QString text = verse.text;

    if (includeReference) {
        text += QString("\n\n- %1 (%2)").arg(verse.reference(), verse.translation);
    }

    return text;
}

void ScriptureDialog::setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                                       const QString& fontFamily, int fontSize)
{
    m_backgroundColor = bgColor;
    m_textColor = textColor;
    m_fontFamily = fontFamily;
    m_fontSize = fontSize;
    m_fontSizeSpinBox->setValue(fontSize);

    // Update preview style
    QString styleSheet = QString(
        "QTextEdit { background-color: %1; color: %2; font-size: 14pt; padding: 10px; }"
    ).arg(bgColor.name(), textColor.name());
    m_previewEdit->setStyleSheet(styleSheet);
}

QList<Slide> ScriptureDialog::getSlides() const
{
    return createSlidesFromVerses();
}

QString ScriptureDialog::reference() const
{
    // Build reference string from the selected verses
    if (m_selectedVerses.isEmpty()) {
        return m_searchEdit->text();
    }

    const BibleVerse& first = m_selectedVerses.first();
    const BibleVerse& last = m_selectedVerses.last();

    if (m_selectedVerses.count() == 1) {
        return first.reference();
    } else if (first.book == last.book && first.chapter == last.chapter) {
        return QString("%1 %2:%3-%4")
            .arg(first.book).arg(first.chapter).arg(first.verse).arg(last.verse);
    } else if (first.book == last.book) {
        return QString("%1 %2:%3-%4:%5")
            .arg(first.book).arg(first.chapter).arg(first.verse)
            .arg(last.chapter).arg(last.verse);
    } else {
        return QString("%1 - %2").arg(first.reference(), last.reference());
    }
}

QString ScriptureDialog::translation() const
{
    return m_translationCombo->currentText();
}

bool ScriptureDialog::oneVersePerSlide() const
{
    return m_onePerSlideCheck->isChecked();
}

bool ScriptureDialog::includeVerseReferences() const
{
    return m_includeReferenceCheck->isChecked();
}

SlideStyle ScriptureDialog::slideStyle() const
{
    if (m_useTheme) {
        // Use the full theme conversion to preserve gradient data
        SlideStyle style = m_selectedTheme.toSlideStyle();
        style.fontSize = m_fontSizeSpinBox->value();  // Override with user-selected font size
        return style;
    }
    return SlideStyle(m_backgroundColor, m_textColor, m_fontFamily, m_fontSizeSpinBox->value());
}

QList<Slide> ScriptureDialog::createSlidesFromVerses() const
{
    QList<Slide> slides;

    if (m_selectedVerses.isEmpty()) {
        return slides;
    }

    bool includeRef = m_includeReferenceCheck->isChecked();
    bool onePerSlide = m_onePerSlideCheck->isChecked();
    int fontSize = m_fontSizeSpinBox->value();

    if (onePerSlide) {
        // Create one slide per verse
        for (const BibleVerse& verse : m_selectedVerses) {
            Slide slide;
            if (m_useTheme) {
                // Use theme to create slide with proper background (including gradients)
                slide = m_selectedTheme.createSlide(formatVerseText(verse, includeRef));
                slide.setFontSize(fontSize);  // Override with user-selected font size
            } else {
                slide.setText(formatVerseText(verse, includeRef));
                slide.setBackgroundColor(m_backgroundColor);
                slide.setTextColor(m_textColor);
                slide.setFontFamily(m_fontFamily);
                slide.setFontSize(fontSize);
            }
            slides.append(slide);
        }
    } else {
        // Combine all verses into one slide
        QStringList texts;
        for (const BibleVerse& verse : m_selectedVerses) {
            texts.append(verse.text);
        }

        QString combinedText = texts.join(" ");

        if (includeRef && !m_selectedVerses.isEmpty()) {
            const BibleVerse& first = m_selectedVerses.first();
            const BibleVerse& last = m_selectedVerses.last();

            QString refStr;
            if (m_selectedVerses.count() == 1) {
                refStr = first.reference();
            } else if (first.book == last.book && first.chapter == last.chapter) {
                refStr = QString("%1 %2:%3-%4")
                    .arg(first.book).arg(first.chapter).arg(first.verse).arg(last.verse);
            } else if (first.book == last.book) {
                refStr = QString("%1 %2:%3-%4:%5")
                    .arg(first.book).arg(first.chapter).arg(first.verse)
                    .arg(last.chapter).arg(last.verse);
            } else {
                refStr = QString("%1 - %2").arg(first.reference(), last.reference());
            }

            combinedText += QString("\n\n- %1 (%2)").arg(refStr, first.translation);
        }

        Slide slide;
        if (m_useTheme) {
            // Use theme to create slide with proper background (including gradients)
            slide = m_selectedTheme.createSlide(combinedText);
            slide.setFontSize(fontSize);  // Override with user-selected font size
        } else {
            slide.setText(combinedText);
            slide.setBackgroundColor(m_backgroundColor);
            slide.setTextColor(m_textColor);
            slide.setFontFamily(m_fontFamily);
            slide.setFontSize(fontSize);
        }
        slides.append(slide);
    }

    return slides;
}

} // namespace Clarity
