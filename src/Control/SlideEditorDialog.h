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
#include <QCheckBox>

namespace Clarity {

class SettingsManager;
class MediaLibrary;
class VideoThumbnailGenerator;
class GradientEditorWidget;

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
    explicit SlideEditorDialog(SettingsManager* settings, MediaLibrary* mediaLibrary,
                               VideoThumbnailGenerator* thumbnailGen, QWidget* parent = nullptr);

    // Set/get the slide being edited
    void setSlide(const Slide& slide);
    Slide slide() const;

private slots:
    void onBackgroundTypeChanged(int index);
    void onChooseBackgroundColor();
    void onChooseTextColor();
    void onChooseBackgroundImage();
    void onBrowseImageLibrary();
    void onChooseBackgroundVideo();
    void onBrowseVideoLibrary();
    void onChooseDropShadowColor();
    void onChooseOverlayColor();
    void onChooseTextContainerColor();
    void onChooseTextBandColor();

private:
    void setupUI();
    void updateBackgroundControls();
    void updateColorButton(QPushButton* button, const QColor& color);
    void installWheelFilter(QWidget* widget);
    void setImageFromPath(const QString& path);
    void setVideoFromPath(const QString& path);

    SettingsManager* m_settings;
    MediaLibrary* m_mediaLibrary;
    VideoThumbnailGenerator* m_thumbnailGen;

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
    GradientEditorWidget* m_gradientEditor;

    // Image controls
    QLineEdit* m_imagePathEdit;
    QPushButton* m_choosImageButton;
    QPushButton* m_imageLibraryButton;
    QLabel* m_imagePreviewLabel;

    // Video controls
    QLineEdit* m_videoPathEdit;
    QPushButton* m_chooseVideoButton;
    QPushButton* m_videoLibraryButton;
    QCheckBox* m_videoLoopCheck;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Transition override controls
    QComboBox* m_transitionTypeCombo;
    QComboBox* m_transitionDurationCombo;

    // Auto-advance timer control
    QSpinBox* m_autoAdvanceSpinBox;

    // Presenter notes
    QTextEdit* m_notesEdit;

    // Text legibility: Drop shadow controls
    QCheckBox* m_dropShadowEnabledCheck;
    QPushButton* m_dropShadowColorButton;
    QSpinBox* m_dropShadowOffsetXSpinBox;
    QSpinBox* m_dropShadowOffsetYSpinBox;
    QSpinBox* m_dropShadowBlurSpinBox;

    // Text legibility: Overlay controls
    QCheckBox* m_overlayEnabledCheck;
    QPushButton* m_overlayColorButton;
    QSpinBox* m_overlayBlurSpinBox;

    // Text legibility: Text container controls
    QCheckBox* m_textContainerEnabledCheck;
    QPushButton* m_textContainerColorButton;
    QSpinBox* m_textContainerPaddingSpinBox;
    QSpinBox* m_textContainerRadiusSpinBox;
    QSpinBox* m_textContainerBlurSpinBox;

    // Text legibility: Text band controls
    QCheckBox* m_textBandEnabledCheck;
    QPushButton* m_textBandColorButton;
    QSpinBox* m_textBandBlurSpinBox;

    // Current slide data
    Slide m_slide;
};

} // namespace Clarity
