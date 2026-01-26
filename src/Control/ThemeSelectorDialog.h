#pragma once

#include "Core/Theme.h"
#include "Core/ThemeManager.h"
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

namespace Clarity {

class SettingsManager;

/**
 * @brief Dialog for selecting and managing themes
 *
 * Provides UI for:
 * - Browsing available themes (built-in and custom)
 * - Previewing themes
 * - Creating, editing, and deleting custom themes
 * - Applying themes to current slide or all slides
 */
class ThemeSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ThemeSelectorDialog(ThemeManager* themeManager, SettingsManager* settings, QWidget* parent = nullptr);

    // Get the selected theme (if any)
    Theme selectedTheme() const;
    bool hasSelection() const;

    // Get apply option
    bool applyToAllSlides() const;

private slots:
    void onThemeSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onThemeDoubleClicked(QListWidgetItem* item);
    void onNewTheme();
    void onEditTheme();
    void onDeleteTheme();
    void onDuplicateTheme();
    void updateButtons();

private:
    void setupUI();
    void populateThemeList();
    void updatePreview(const Theme& theme);

    ThemeManager* m_themeManager;
    SettingsManager* m_settings;

    // UI components
    QListWidget* m_themeList;
    QLabel* m_previewLabel;
    QLabel* m_descriptionLabel;

    // Theme management buttons
    QPushButton* m_newButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_duplicateButton;

    // Apply options
    QCheckBox* m_applyToAllCheck;

    // Dialog buttons
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
};

} // namespace Clarity
