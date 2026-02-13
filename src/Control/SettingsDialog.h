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

namespace Clarity {

class BibleDatabase;

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

    /**
     * @brief Set the Bible database for translation management
     */
    void setBibleDatabase(BibleDatabase* database);

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

    // Library settings
    QCheckBox* m_autoSyncLibraryGroupsCheckBox;

    // Red Letter Edition settings
    QCheckBox* m_redLettersEnabledCheckBox;
    QPushButton* m_redLetterColorButton;
    QColor m_redLetterColor;

    // Settings manager
    SettingsManager* m_settingsManager;
};

} // namespace Clarity
