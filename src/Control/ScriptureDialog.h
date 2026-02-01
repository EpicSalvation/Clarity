#pragma once

#include "Core/Slide.h"
#include "Core/Song.h"  // For SlideStyle
#include "Core/BibleDatabase.h"
#include "Core/ThemeManager.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>

namespace Clarity {

class SettingsManager;

/**
 * @brief Dialog for searching and inserting Bible scripture
 *
 * Provides UI for:
 * - Searching by reference (John 3:16) or keyword
 * - Translation selection
 * - Verse preview with formatting options
 * - Insert as slides with configurable options
 */
class ScriptureDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct scripture dialog
     * @param bible Pointer to BibleDatabase (must remain valid during dialog lifetime)
     * @param settings Pointer to SettingsManager for remembering translation preference
     * @param themeManager Pointer to ThemeManager for theme selection
     * @param parent Parent widget
     */
    explicit ScriptureDialog(BibleDatabase* bible, SettingsManager* settings = nullptr,
                            ThemeManager* themeManager = nullptr, QWidget* parent = nullptr);

    /**
     * @brief Get the slides created from selected scripture
     * @return List of slides (empty if cancelled or no selection)
     */
    QList<Slide> getSlides() const;

    /**
     * @brief Get the scripture reference string (e.g., "John 3:16-17")
     */
    QString reference() const;

    /**
     * @brief Get the translation identifier (e.g., "KJV")
     */
    QString translation() const;

    /**
     * @brief Get whether one verse per slide is selected
     */
    bool oneVersePerSlide() const;

    /**
     * @brief Get whether to include verse references on slides
     */
    bool includeVerseReferences() const;

    /**
     * @brief Get the slide style settings
     */
    SlideStyle slideStyle() const;

    /**
     * @brief Set default slide style for generated slides
     */
    void setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                         const QString& fontFamily, int fontSize);

private slots:
    void onSearch();
    void onReferenceSearch();
    void onKeywordSearch();
    void onResultSelectionChanged();
    void onTranslationChanged(int index);
    void onThemeChanged(int index);
    void onIncludeReferenceChanged(Qt::CheckState state);
    void onOnePerSlideChanged(Qt::CheckState state);
    void updatePreview();

private:
    void setupUI();
    void populateTranslations();
    void populateThemes();
    void applyTheme(const Theme& theme);
    QList<Slide> createSlidesFromVerses() const;
    QString formatVerseText(const BibleVerse& verse, bool includeReference) const;

    // Database and settings references
    BibleDatabase* m_bible;
    SettingsManager* m_settings;
    ThemeManager* m_themeManager;

    // Search controls
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    QComboBox* m_searchTypeCombo;  // Reference or Keyword search
    QComboBox* m_translationCombo;
    QComboBox* m_themeCombo;

    // Results display
    QListWidget* m_resultsList;
    QLabel* m_resultsCountLabel;

    // Preview
    QTextEdit* m_previewEdit;

    // Options
    QCheckBox* m_includeReferenceCheck;
    QCheckBox* m_onePerSlideCheck;
    QSpinBox* m_fontSizeSpinBox;

    // Dialog buttons
    QPushButton* m_insertButton;
    QPushButton* m_cancelButton;

    // Selected verses
    QList<BibleVerse> m_searchResults;
    QList<BibleVerse> m_selectedVerses;

    // Style settings for generated slides
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;

    // Selected theme (if any)
    Theme m_selectedTheme;
    bool m_useTheme = false;
};

} // namespace Clarity
