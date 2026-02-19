#pragma once

#include "Core/Slide.h"
#include "Core/Song.h"  // For SlideStyle
#include "Core/ApiBibleClient.h"
#include "Core/ThemeManager.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>

namespace Clarity {

class SettingsManager;

/**
 * @brief Dialog for searching and inserting scripture from API.bible
 *
 * Provides UI for:
 * - Selecting a Bible version from API.bible's catalog
 * - Entering a scripture reference
 * - Fetching from the API.bible service
 * - Preview with formatting options
 * - Inserting as slides
 */
class ApiBibleScriptureDialog : public QWidget {
    Q_OBJECT

public:
    explicit ApiBibleScriptureDialog(ApiBibleClient* client, SettingsManager* settings = nullptr,
                                     ThemeManager* themeManager = nullptr, QWidget* parent = nullptr);

    /**
     * @brief Whether valid content is available for insertion
     */
    bool hasValidContent() const;

signals:
    /**
     * @brief Emitted when content readiness changes
     */
    void contentReadyChanged(bool ready);

public:
    /**
     * @brief Get the fetched passage data
     */
    ApiBiblePassage passage() const { return m_passage; }

    /**
     * @brief Get whether one verse per slide is selected
     */
    bool oneVersePerSlide() const;

    /**
     * @brief Get whether to include verse numbers on slides
     */
    bool includeVerseNumbers() const;

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
    void onBiblesLoaded(const QList<ApiBibleVersion>& bibles);
    void onPassageFetched(const ApiBiblePassage& passage);
    void onFetchError(const QString& error);
    void onThemeChanged(int index);
    void updatePreview();

private:
    void setupUI();
    void populateThemes();
    void applyTheme(const Theme& theme);
    void loadBibles();
    void onLanguageChanged(int index);
    void onBibleVersionChanged(int index);
    void filterBiblesByLanguage(const QString& languageId);
    void populateEditions(const QString& abbreviation);
    QString selectedBibleId() const;

    // References
    ApiBibleClient* m_client;
    SettingsManager* m_settings;
    ThemeManager* m_themeManager;

    // Bible version selector
    QComboBox* m_languageCombo;
    QComboBox* m_bibleCombo;
    QComboBox* m_editionCombo;
    QLabel* m_editionLabel;
    QPushButton* m_refreshBiblesButton;

    // Cached full bible list for client-side language filtering
    QList<ApiBibleVersion> m_allBibles;

    // Search controls
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    QComboBox* m_themeCombo;

    // Results/preview
    QTextEdit* m_previewEdit;
    QLabel* m_statusLabel;
    QLabel* m_copyrightLabel;

    // Options
    QCheckBox* m_includeVerseNumbersCheck;
    QCheckBox* m_onePerSlideCheck;
    QSpinBox* m_fontSizeSpinBox;

    // Fetched data
    ApiBiblePassage m_passage;

    // Style settings
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;

    // Selected theme
    Theme m_selectedTheme;
    bool m_useTheme = false;
};

} // namespace Clarity
