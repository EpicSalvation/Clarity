#pragma once

#include "Core/Theme.h"
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>

namespace Clarity {

class SettingsManager;

/**
 * @brief Dialog for creating and editing themes
 *
 * Provides UI for configuring all theme properties:
 * - Name and description
 * - Colors (background, text, accent)
 * - Typography (font family, sizes)
 * - Background type and settings (solid, gradient, image)
 */
class ThemeEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ThemeEditorDialog(SettingsManager* settings, QWidget* parent = nullptr);
    ThemeEditorDialog(SettingsManager* settings, const Theme& theme, QWidget* parent = nullptr);

    // Get the configured theme
    Theme theme() const;

    // Check if this is a new theme or editing existing
    bool isNewTheme() const { return m_isNewTheme; }

private slots:
    void onBackgroundTypeChanged(int index);
    void onChooseBackgroundColor();
    void onChooseTextColor();
    void onChooseAccentColor();
    void onChooseGradientStartColor();
    void onChooseGradientEndColor();
    void updatePreview();
    void validateAndAccept();

private:
    void setupUI();
    void loadTheme(const Theme& theme);
    void updateColorButton(QPushButton* button, const QColor& color);
    void installWheelFilter(QWidget* widget);

    SettingsManager* m_settings;

    // Basic info
    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;

    // Colors
    QPushButton* m_backgroundColorButton;
    QPushButton* m_textColorButton;
    QPushButton* m_accentColorButton;

    // Typography
    QComboBox* m_fontFamilyCombo;
    QSpinBox* m_titleFontSizeSpinBox;
    QSpinBox* m_bodyFontSizeSpinBox;

    // Background type
    QComboBox* m_backgroundTypeCombo;
    QStackedWidget* m_backgroundStack;

    // Gradient controls (in stack page)
    QPushButton* m_gradientStartColorButton;
    QPushButton* m_gradientEndColorButton;
    QSpinBox* m_gradientAngleSpinBox;

    // Preview
    QLabel* m_previewLabel;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // State
    bool m_isNewTheme;
    QString m_originalName;

    // Current values for color buttons
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_accentColor;
    QColor m_gradientStartColor;
    QColor m_gradientEndColor;
};

} // namespace Clarity
