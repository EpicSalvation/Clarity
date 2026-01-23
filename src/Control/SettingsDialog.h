#pragma once

#include "Core/SettingsManager.h"
#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QPushButton>

namespace Clarity {

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

private slots:
    void onCategoryChanged(int row);
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUI();
    void createDisplayPage();
    void loadSettings();
    void saveSettings();

    // UI components
    QListWidget* m_categoryList;
    QStackedWidget* m_pageStack;

    // Display page widgets
    QComboBox* m_screenComboBox;

    // Settings manager
    SettingsManager* m_settingsManager;
};

} // namespace Clarity
