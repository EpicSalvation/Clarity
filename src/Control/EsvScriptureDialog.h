// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Slide.h"
#include "Core/Song.h"  // For SlideStyle
#include "Core/EsvApiClient.h"
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
class EsvApiClient;

/**
 * @brief Dialog for searching and inserting ESV Bible scripture via the ESV API
 *
 * Provides UI for:
 * - Entering a scripture reference
 * - Fetching from the ESV API
 * - Preview with formatting options
 * - Inserting as slides
 * - Cache limit awareness (500 verse max per ESV terms)
 */
class EsvScriptureDialog : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct ESV scripture widget (embedded in ScriptureInsertDialog)
     * @param esvClient ESV API client for fetching passages
     * @param settings Settings manager for preferences
     * @param themeManager Theme manager for theme selection
     * @param parent Parent widget
     */
    explicit EsvScriptureDialog(EsvApiClient* esvClient, SettingsManager* settings = nullptr,
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
    EsvPassage passage() const { return m_passage; }

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
    void onPassageFetched(const EsvPassage& passage);
    void onFetchError(const QString& error);
    void onThemeChanged(int index);
    void updatePreview();

private:
    void setupUI();
    void populateThemes();
    void applyTheme(const Theme& theme);
    void updateCacheStatus();

    // References
    EsvApiClient* m_esvClient;
    SettingsManager* m_settings;
    ThemeManager* m_themeManager;

    // Search controls
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    QComboBox* m_themeCombo;

    // Results/preview
    QTextEdit* m_previewEdit;
    QLabel* m_statusLabel;
    QLabel* m_cacheLabel;

    // Options
    QCheckBox* m_includeVerseNumbersCheck;
    QCheckBox* m_onePerSlideCheck;
    QSpinBox* m_fontSizeSpinBox;

    // Fetched data
    EsvPassage m_passage;

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
