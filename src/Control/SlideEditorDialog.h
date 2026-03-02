// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

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
#include <QGroupBox>
#include <QToolButton>

class QVBoxLayout;

namespace Clarity {

class SettingsManager;
class MediaLibrary;
class VideoThumbnailGenerator;
class GradientEditorWidget;
class SlideCanvasWidget;

/**
 * @brief WYSIWYG slide editor dialog
 *
 * Layout: toolbar across the top, canvas on the left, scrollable property
 * panels on the right, OK/Cancel at the bottom.
 */
class SlideEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SlideEditorDialog(SettingsManager* settings, MediaLibrary* mediaLibrary,
                               VideoThumbnailGenerator* thumbnailGen, QWidget* parent = nullptr);

    void setSlide(const Slide& slide);
    Slide slide() const;

private slots:
    void onTemplateChanged(int index);
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
    void onZoneSelected(int index);

private:
    void setupUI();
    void setupToolbar(QVBoxLayout* mainLayout);
    void setupCenterAndPanels(QVBoxLayout* mainLayout);
    QWidget* buildRightPanel();
    QWidget* buildBackgroundSection();
    QWidget* buildTextEffectsSection();
    QWidget* buildTransitionSection();
    QWidget* buildNotesSection();
    void updateBackgroundControls();
    void updateColorButton(QPushButton* button, const QColor& color);
    void installWheelFilter(QWidget* widget);
    void setImageFromPath(const QString& path);
    void setVideoFromPath(const QString& path);
    void syncToolbarFromZone(int zoneIndex);
    void syncTextEffectsFromZone(int zoneIndex);

    SettingsManager* m_settings;
    MediaLibrary* m_mediaLibrary;
    VideoThumbnailGenerator* m_thumbnailGen;

    // Toolbar controls
    QComboBox* m_templateCombo;
    QComboBox* m_fontFamilyCombo;
    QSpinBox* m_fontSizeSpinBox;
    QPushButton* m_textColorButton;
    QToolButton* m_alignLeftButton;
    QToolButton* m_alignCenterButton;
    QToolButton* m_alignRightButton;
    QToolButton* m_bulletListButton;
    QToolButton* m_numberedListButton;

    // WYSIWYG canvas
    SlideCanvasWidget* m_canvas;

    // Background section controls
    QCheckBox* m_useOwnBackgroundCheck;
    QComboBox* m_backgroundTypeCombo;
    QStackedWidget* m_backgroundStack;
    QPushButton* m_backgroundColorButton;
    GradientEditorWidget* m_gradientEditor;
    QLineEdit* m_imagePathEdit;
    QPushButton* m_choosImageButton;
    QPushButton* m_imageLibraryButton;
    QLabel* m_imagePreviewLabel;
    QLineEdit* m_videoPathEdit;
    QPushButton* m_chooseVideoButton;
    QPushButton* m_videoLibraryButton;
    QCheckBox* m_videoLoopCheck;
    QSpinBox* m_overlayBlurSpinBox;

    // Text effects section controls
    QGroupBox* m_shadowGroup;
    QCheckBox* m_dropShadowEnabledCheck;
    QPushButton* m_dropShadowColorButton;
    QSpinBox* m_dropShadowOffsetXSpinBox;
    QSpinBox* m_dropShadowOffsetYSpinBox;
    QSpinBox* m_dropShadowBlurSpinBox;
    QGroupBox* m_overlayGroup;
    QCheckBox* m_overlayEnabledCheck;
    QPushButton* m_overlayColorButton;
    QGroupBox* m_containerGroup;
    QCheckBox* m_textContainerEnabledCheck;
    QPushButton* m_textContainerColorButton;
    QSpinBox* m_textContainerPaddingSpinBox;
    QSpinBox* m_textContainerRadiusSpinBox;
    QSpinBox* m_textContainerBlurSpinBox;
    QGroupBox* m_bandGroup;
    QCheckBox* m_textBandEnabledCheck;
    QPushButton* m_textBandColorButton;
    QSpinBox* m_textBandBlurSpinBox;

    // Transition section controls
    QComboBox* m_transitionTypeCombo;
    QComboBox* m_transitionDurationCombo;
    QSpinBox* m_autoAdvanceSpinBox;

    // Notes section controls
    QTextEdit* m_notesEdit;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Current slide data
    Slide m_slide;

    // Guard against feedback loops when syncing toolbar from zone
    bool m_updatingFromZone = false;
};

} // namespace Clarity
