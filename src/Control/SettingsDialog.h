// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/SettingsManager.h"
#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QPushButton>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

namespace Clarity {

class BibleDatabase;
class SongLibrary;
class ThemeManager;

/**
 * @brief Settings dialog with category sidebar
 *
 * Layout:
 * - Left side: Category list
 * - Right side: Settings pages (stacked)
 * - Bottom: OK/Cancel buttons
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(SettingsManager* settingsManager, QWidget* parent = nullptr);
    ~SettingsDialog();

    /** Launch the settings tour immediately. Call this to replay it. */
    void startSettingsTour();

    /**
     * @brief Set the Bible database for translation management
     */
    void setBibleDatabase(BibleDatabase* database);

    /**
     * @brief Set the theme manager for scripture theme override combo
     */
    void setThemeManager(ThemeManager* themeManager);

    /**
     * @brief Set the song library for CCLI report generation
     */
    void setSongLibrary(SongLibrary* library);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onCategoryChanged(int row);
    void onOkClicked();
    void onCancelClicked();
    void onConfidenceTextColorClicked();
    void onConfidenceBackgroundColorClicked();
    void onImportTranslationClicked();
    void onDeleteTranslationClicked();
    void onRedLetterColorClicked();

private:
    void setupUI();
    void createGeneralPage();
    void createDisplayPage();
    void createRemoteControlPage();
    void createBiblePage();
    void createCopyrightPage();
    void loadSettings();
    void saveSettings();
    void updateColorButtonStyle(QPushButton* button, const QColor& color);
    void refreshTranslationsList();

    // UI components
    QListWidget* m_categoryList;
    QStackedWidget* m_pageStack;

    // Display page widgets
    QComboBox* m_outputScreenComboBox;
    QComboBox* m_confidenceScreenComboBox;

    // Confidence monitor display settings
    QFontComboBox* m_confidenceFontComboBox;
    QSpinBox* m_confidenceFontSizeSpinBox;
    QPushButton* m_confidenceTextColorButton;
    QPushButton* m_confidenceBackgroundColorButton;
    QColor m_confidenceTextColor;
    QColor m_confidenceBackgroundColor;

    // Transition settings
    QComboBox* m_transitionTypeComboBox;
    QComboBox* m_transitionDurationComboBox;

    // UI behavior settings
    QCheckBox* m_scrollWheelChangesInputsCheckBox;
    QCheckBox* m_showAllSlidesInGridCheckBox;
    QComboBox* m_slidePreviewSizeComboBox;

    // Appearance settings
    QComboBox* m_themeComboBox;

    // Language settings
    QComboBox* m_languageComboBox;

    // Remote control settings
    QCheckBox* m_remoteControlEnabledCheckBox;
    QSpinBox* m_remoteControlPortSpinBox;
    QCheckBox* m_remoteControlPinEnabledCheckBox;
    QLineEdit* m_remoteControlPinEdit;

    // Bible translation management
    QListWidget* m_translationsListWidget;
    QPushButton* m_importTranslationButton;
    QPushButton* m_deleteTranslationButton;
    QComboBox* m_preferredTranslationComboBox;
    QCheckBox* m_rememberLastTranslationCheckBox;
    BibleDatabase* m_bibleDatabase;

    // Cascading background settings
    QCheckBox* m_cascadingBackgroundsCheckBox;
    QCheckBox* m_scriptureThemeOverrideCheckBox;
    QComboBox* m_scriptureThemeOverrideCombo;
    ThemeManager* m_themeManager;
    SongLibrary* m_songLibrary;

    // Library settings
    QCheckBox* m_autoSyncLibraryGroupsCheckBox;

    // Scripture formatting settings
    QComboBox* m_scriptureRefPositionCombo;

    // Red Letter Edition settings
    QCheckBox* m_redLettersEnabledCheckBox;
    QPushButton* m_redLetterColorButton;
    QColor m_redLetterColor;

    // Copyright & CCLI settings
    QLineEdit* m_ccliLicenseNumberEdit;
    QCheckBox* m_showCcliOnTitleSlidesCheckBox;
    QCheckBox* m_showCopyrightSlideCheckBox;

    // ESV API settings
    QLineEdit* m_esvApiKeyEdit;
    QLabel* m_esvCacheStatusLabel;

    // API.bible settings
    QLineEdit* m_apiBibleApiKeyEdit;

    // Settings manager
    SettingsManager* m_settingsManager;

    // Replay tour button (in General page)
    QPushButton* m_replaySettingsTourButton;

    // Update check (in General page)
    QCheckBox* m_autoCheckUpdatesCheckbox = nullptr;
    QCheckBox* m_includeBetaCheckbox = nullptr;
};

} // namespace Clarity
