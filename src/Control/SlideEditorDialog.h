#pragma once

#include "Core/Slide.h"
#include <QDialog>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QStackedWidget>
#include <QLineEdit>

namespace Clarity {

/**
 * @brief Dialog for editing slide properties
 *
 * Provides UI for:
 * - Text content editing
 * - Background type selection (solid color, gradient, image)
 * - Background configuration (colors, gradient angle, image path)
 * - Text styling (color, font, size)
 */
class SlideEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SlideEditorDialog(QWidget* parent = nullptr);

    // Set/get the slide being edited
    void setSlide(const Slide& slide);
    Slide slide() const;

private slots:
    void onBackgroundTypeChanged(int index);
    void onChooseBackgroundColor();
    void onChooseTextColor();
    void onChooseGradientStartColor();
    void onChooseGradientEndColor();
    void onChooseBackgroundImage();

private:
    void setupUI();
    void updateBackgroundControls();
    void updateColorButton(QPushButton* button, const QColor& color);

    // Text controls
    QTextEdit* m_textEdit;
    QPushButton* m_textColorButton;
    QComboBox* m_fontFamilyCombo;
    QSpinBox* m_fontSizeSpinBox;

    // Background type controls
    QComboBox* m_backgroundTypeCombo;
    QStackedWidget* m_backgroundStack;

    // Solid color controls
    QPushButton* m_backgroundColorButton;

    // Gradient controls
    QPushButton* m_gradientStartColorButton;
    QPushButton* m_gradientEndColorButton;
    QSpinBox* m_gradientAngleSpinBox;

    // Image controls
    QLineEdit* m_imagePathEdit;
    QPushButton* m_choosImageButton;
    QLabel* m_imagePreviewLabel;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Transition override controls
    QComboBox* m_transitionTypeCombo;
    QComboBox* m_transitionDurationCombo;

    // Presenter notes
    QTextEdit* m_notesEdit;

    // Current slide data
    Slide m_slide;
};

} // namespace Clarity
