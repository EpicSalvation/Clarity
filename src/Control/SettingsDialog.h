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
    void onConfidenceTextColorClicked();
    void onConfidenceBackgroundColorClicked();

private:
    void setupUI();
    void createGeneralPage();
    void createDisplayPage();
    void createRemoteControlPage();
    void loadSettings();
    void saveSettings();
    void updateColorButtonStyle(QPushButton* button, const QColor& color);

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

    // Language settings
    QComboBox* m_languageComboBox;

    // Remote control settings
    QCheckBox* m_remoteControlEnabledCheckBox;
    QSpinBox* m_remoteControlPortSpinBox;
    QCheckBox* m_remoteControlPinEnabledCheckBox;
    QLineEdit* m_remoteControlPinEdit;

    // Settings manager
    SettingsManager* m_settingsManager;
};

} // namespace Clarity
