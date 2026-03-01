// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SlideEditorDialog.h"
#include "GradientEditorWidget.h"
#include "MediaLibraryDialog.h"
#include "Core/SettingsManager.h"
#include "Core/MediaLibrary.h"
#include "Core/VideoThumbnailGenerator.h"
#include "Core/WheelEventFilter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QBuffer>
#include <QDebug>
#include <QScrollArea>
#include <QFrame>

namespace Clarity {

SlideEditorDialog::SlideEditorDialog(SettingsManager* settings, MediaLibrary* mediaLibrary,
                                     VideoThumbnailGenerator* thumbnailGen, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_mediaLibrary(mediaLibrary)
    , m_thumbnailGen(thumbnailGen)
{
    setupUI();
    setWindowTitle(tr("Edit Slide"));
    resize(900, 600);
    setMinimumSize(700, 400);  // Allow resizing but set minimum for two columns
}

void SlideEditorDialog::installWheelFilter(QWidget* widget)
{
    if (m_settings) {
        widget->installEventFilter(new WheelEventFilter(m_settings, widget));
        widget->setFocusPolicy(Qt::StrongFocus);
    }
}

void SlideEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    // Create scroll area for all content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Content widget that goes inside scroll area
    QWidget* contentWidget = new QWidget();
    QHBoxLayout* columnsLayout = new QHBoxLayout(contentWidget);
    columnsLayout->setContentsMargins(0, 0, 6, 0);  // Right margin for scrollbar

    // Left column - main slide content
    QWidget* leftColumn = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Right column - text legibility options
    QWidget* rightColumn = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // === LEFT COLUMN ===

    // Template selector
    QHBoxLayout* templateLayout = new QHBoxLayout();
    templateLayout->addWidget(new QLabel(tr("Template:"), leftColumn));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(tr("Blank"), static_cast<int>(SlideTemplate::Blank));
    m_templateCombo->addItem(tr("Title"), static_cast<int>(SlideTemplate::Title));
    m_templateCombo->addItem(tr("Title & Body"), static_cast<int>(SlideTemplate::TitleBody));
    m_templateCombo->addItem(tr("Scripture"), static_cast<int>(SlideTemplate::Scripture));
    installWheelFilter(m_templateCombo);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorDialog::onTemplateChanged);
    templateLayout->addWidget(m_templateCombo, 1);
    leftLayout->addLayout(templateLayout);

    // Stacked widget: page 0 = single text + style, page 1 = zone tabs
    m_textStack = new QStackedWidget(leftColumn);

    // Page 0: Blank template — single text content + text style
    QWidget* blankPage = new QWidget();
    QVBoxLayout* blankLayout = new QVBoxLayout(blankPage);
    blankLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* textGroup = new QGroupBox(tr("Slide Content"), blankPage);
    QVBoxLayout* textLayout = new QVBoxLayout(textGroup);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setPlaceholderText(tr("Enter slide text here..."));
    m_textEdit->setMinimumHeight(120);
    textLayout->addWidget(m_textEdit);
    blankLayout->addWidget(textGroup);

    QGroupBox* textStyleGroup = new QGroupBox(tr("Text Style"), blankPage);
    QFormLayout* textStyleLayout = new QFormLayout(textStyleGroup);

    m_textColorButton = new QPushButton(tr("Choose Color"), this);
    connect(m_textColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextColor);
    textStyleLayout->addRow(tr("Text Color:"), m_textColorButton);

    m_fontFamilyCombo = new QComboBox(this);
    m_fontFamilyCombo->addItems({"Arial", "Helvetica", "Georgia", "Verdana", "Times New Roman"});
    installWheelFilter(m_fontFamilyCombo);
    textStyleLayout->addRow(tr("Font Family:"), m_fontFamilyCombo);

    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(tr(" pt"));
    installWheelFilter(m_fontSizeSpinBox);
    textStyleLayout->addRow(tr("Font Size:"), m_fontSizeSpinBox);

    blankLayout->addWidget(textStyleGroup);
    m_textStack->addWidget(blankPage);

    // Page 1: Multi-zone templates — tab widget with per-zone controls
    m_zoneTabWidget = new QTabWidget(this);
    m_textStack->addWidget(m_zoneTabWidget);

    leftLayout->addWidget(m_textStack);

    // Background section
    m_backgroundGroup = new QGroupBox(tr("Background"), leftColumn);
    QVBoxLayout* backgroundLayout = new QVBoxLayout(m_backgroundGroup);

    // "Use own background" checkbox for cascading background control
    m_useOwnBackgroundCheck = new QCheckBox(tr("Use own background"), m_backgroundGroup);
    m_useOwnBackgroundCheck->setChecked(true);
    m_useOwnBackgroundCheck->setToolTip(
        tr("When unchecked, this slide inherits the background from the previous slide "
           "that has its own background (cascading backgrounds)."));
    backgroundLayout->addWidget(m_useOwnBackgroundCheck);

    m_backgroundTypeCombo = new QComboBox(this);
    m_backgroundTypeCombo->addItem(tr("Solid Color"), "solidColor");
    m_backgroundTypeCombo->addItem(tr("Gradient"), "gradient");
    m_backgroundTypeCombo->addItem(tr("Image"), "image");
    m_backgroundTypeCombo->addItem(tr("Video"), "video");
    installWheelFilter(m_backgroundTypeCombo);
    connect(m_backgroundTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorDialog::onBackgroundTypeChanged);
    backgroundLayout->addWidget(m_backgroundTypeCombo);

    // Stacked widget for different background types
    m_backgroundStack = new QStackedWidget(this);

    // Page 0: Solid color
    QWidget* solidColorPage = new QWidget(this);
    QFormLayout* solidColorLayout = new QFormLayout(solidColorPage);
    m_backgroundColorButton = new QPushButton("Choose Color", this);
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundColor);
    solidColorLayout->addRow("Background Color:", m_backgroundColorButton);
    m_backgroundStack->addWidget(solidColorPage);

    // Page 1: Gradient (multi-stop editor)
    m_gradientEditor = new GradientEditorWidget(m_settings, this);
    m_backgroundStack->addWidget(m_gradientEditor);

    // Page 2: Image
    QWidget* imagePage = new QWidget(this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imagePage);

    QHBoxLayout* imagePathLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit(this);
    m_imagePathEdit->setPlaceholderText("No image selected");
    m_imagePathEdit->setReadOnly(true);
    imagePathLayout->addWidget(m_imagePathEdit);

    m_choosImageButton = new QPushButton("Import...", this);
    m_choosImageButton->setToolTip("Import a new image file");
    connect(m_choosImageButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundImage);
    imagePathLayout->addWidget(m_choosImageButton);

    m_imageLibraryButton = new QPushButton("Library...", this);
    m_imageLibraryButton->setToolTip("Browse images in your media library");
    connect(m_imageLibraryButton, &QPushButton::clicked, this, &SlideEditorDialog::onBrowseImageLibrary);
    imagePathLayout->addWidget(m_imageLibraryButton);

    imageLayout->addLayout(imagePathLayout);

    m_imagePreviewLabel = new QLabel(this);
    m_imagePreviewLabel->setMinimumHeight(100);
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f5f5f5; }");
    m_imagePreviewLabel->setText("No image preview");
    imageLayout->addWidget(m_imagePreviewLabel);

    m_backgroundStack->addWidget(imagePage);

    // Page 3: Video
    QWidget* videoPage = new QWidget(this);
    QVBoxLayout* videoLayout = new QVBoxLayout(videoPage);

    QHBoxLayout* videoPathLayout = new QHBoxLayout();
    m_videoPathEdit = new QLineEdit(this);
    m_videoPathEdit->setPlaceholderText("No video selected");
    m_videoPathEdit->setReadOnly(true);
    videoPathLayout->addWidget(m_videoPathEdit);

    m_chooseVideoButton = new QPushButton("Import...", this);
    m_chooseVideoButton->setToolTip("Import a new video file");
    connect(m_chooseVideoButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundVideo);
    videoPathLayout->addWidget(m_chooseVideoButton);

    m_videoLibraryButton = new QPushButton("Library...", this);
    m_videoLibraryButton->setToolTip("Browse videos in your media library");
    connect(m_videoLibraryButton, &QPushButton::clicked, this, &SlideEditorDialog::onBrowseVideoLibrary);
    videoPathLayout->addWidget(m_videoLibraryButton);

    videoLayout->addLayout(videoPathLayout);

    m_videoLoopCheck = new QCheckBox("Loop video", this);
    m_videoLoopCheck->setChecked(true);
    m_videoLoopCheck->setToolTip("When enabled, video will repeat continuously");
    videoLayout->addWidget(m_videoLoopCheck);

    QLabel* videoNote = new QLabel("Note: Videos are always muted (background only)", this);
    videoNote->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    videoLayout->addWidget(videoNote);

    videoLayout->addStretch();
    m_backgroundStack->addWidget(videoPage);

    backgroundLayout->addWidget(m_backgroundStack);

    // Background Blur (part of background section, independent of overlay)
    QFormLayout* blurLayout = new QFormLayout();

    m_overlayBlurSpinBox = new QSpinBox(this);
    m_overlayBlurSpinBox->setRange(0, 50);
    m_overlayBlurSpinBox->setValue(0);
    m_overlayBlurSpinBox->setSuffix(" px");
    m_overlayBlurSpinBox->setToolTip("Blur the background (0 = no blur, works with or without overlay)");
    installWheelFilter(m_overlayBlurSpinBox);
    blurLayout->addRow("Background Blur:", m_overlayBlurSpinBox);

    backgroundLayout->addLayout(blurLayout);

    // Wire "Use own background" checkbox to enable/disable all background controls
    connect(m_useOwnBackgroundCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_backgroundTypeCombo->setEnabled(checked);
        m_backgroundStack->setEnabled(checked);
        m_overlayEnabledCheck->setEnabled(checked);
        m_overlayColorButton->setEnabled(checked && m_overlayEnabledCheck->isChecked());
        m_overlayBlurSpinBox->setEnabled(checked);
    });

    leftLayout->addWidget(m_backgroundGroup);

    // === RIGHT COLUMN ===

    // Text Legibility section
    QGroupBox* legibilityGroup = new QGroupBox("Text Legibility", rightColumn);
    QVBoxLayout* legibilityLayout = new QVBoxLayout(legibilityGroup);

    // Drop Shadow subsection (hidden for template slides — per-zone controls used instead)
    m_shadowGroup = new QGroupBox("Drop Shadow", this);
    QGroupBox* shadowGroup = m_shadowGroup;
    QFormLayout* shadowLayout = new QFormLayout(shadowGroup);

    m_dropShadowEnabledCheck = new QCheckBox("Enable drop shadow", this);
    m_dropShadowEnabledCheck->setChecked(true);
    shadowLayout->addRow(m_dropShadowEnabledCheck);

    m_dropShadowColorButton = new QPushButton("Choose Color", this);
    connect(m_dropShadowColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseDropShadowColor);
    shadowLayout->addRow("Shadow Color:", m_dropShadowColorButton);

    QHBoxLayout* shadowOffsetLayout = new QHBoxLayout();
    m_dropShadowOffsetXSpinBox = new QSpinBox(this);
    m_dropShadowOffsetXSpinBox->setRange(-20, 20);
    m_dropShadowOffsetXSpinBox->setValue(2);
    m_dropShadowOffsetXSpinBox->setSuffix(" px");
    installWheelFilter(m_dropShadowOffsetXSpinBox);
    shadowOffsetLayout->addWidget(new QLabel("X:", this));
    shadowOffsetLayout->addWidget(m_dropShadowOffsetXSpinBox);

    m_dropShadowOffsetYSpinBox = new QSpinBox(this);
    m_dropShadowOffsetYSpinBox->setRange(-20, 20);
    m_dropShadowOffsetYSpinBox->setValue(2);
    m_dropShadowOffsetYSpinBox->setSuffix(" px");
    installWheelFilter(m_dropShadowOffsetYSpinBox);
    shadowOffsetLayout->addWidget(new QLabel("Y:", this));
    shadowOffsetLayout->addWidget(m_dropShadowOffsetYSpinBox);
    shadowLayout->addRow("Offset:", shadowOffsetLayout);

    m_dropShadowBlurSpinBox = new QSpinBox(this);
    m_dropShadowBlurSpinBox->setRange(0, 50);
    m_dropShadowBlurSpinBox->setValue(0);
    m_dropShadowBlurSpinBox->setSuffix(" px");
    installWheelFilter(m_dropShadowBlurSpinBox);
    shadowLayout->addRow("Blur Radius:", m_dropShadowBlurSpinBox);

    legibilityLayout->addWidget(shadowGroup);

    // Background Overlay subsection (color tint over background)
    QGroupBox* overlayGroup = new QGroupBox("Background Overlay", this);
    QFormLayout* overlayLayout = new QFormLayout(overlayGroup);

    m_overlayEnabledCheck = new QCheckBox("Enable background color overlay", this);
    overlayLayout->addRow(m_overlayEnabledCheck);

    m_overlayColorButton = new QPushButton("Choose Color", this);
    connect(m_overlayColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseOverlayColor);
    overlayLayout->addRow("Overlay Color:", m_overlayColorButton);

    legibilityLayout->addWidget(overlayGroup);

    // Text Container subsection (hidden for template slides — per-zone controls used instead)
    m_containerGroup = new QGroupBox("Text Container", this);
    QGroupBox* containerGroup = m_containerGroup;
    QFormLayout* containerLayout = new QFormLayout(containerGroup);

    m_textContainerEnabledCheck = new QCheckBox("Enable text container box", this);
    containerLayout->addRow(m_textContainerEnabledCheck);

    m_textContainerColorButton = new QPushButton("Choose Color", this);
    connect(m_textContainerColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextContainerColor);
    containerLayout->addRow("Container Color:", m_textContainerColorButton);

    m_textContainerPaddingSpinBox = new QSpinBox(this);
    m_textContainerPaddingSpinBox->setRange(0, 100);
    m_textContainerPaddingSpinBox->setValue(20);
    m_textContainerPaddingSpinBox->setSuffix(" px");
    installWheelFilter(m_textContainerPaddingSpinBox);
    containerLayout->addRow("Padding:", m_textContainerPaddingSpinBox);

    m_textContainerRadiusSpinBox = new QSpinBox(this);
    m_textContainerRadiusSpinBox->setRange(0, 50);
    m_textContainerRadiusSpinBox->setValue(8);
    m_textContainerRadiusSpinBox->setSuffix(" px");
    installWheelFilter(m_textContainerRadiusSpinBox);
    containerLayout->addRow("Corner Radius:", m_textContainerRadiusSpinBox);

    m_textContainerBlurSpinBox = new QSpinBox(this);
    m_textContainerBlurSpinBox->setRange(0, 50);
    m_textContainerBlurSpinBox->setValue(0);
    m_textContainerBlurSpinBox->setSuffix(" px");
    m_textContainerBlurSpinBox->setToolTip("Blur the background under the container (0 = no blur)");
    installWheelFilter(m_textContainerBlurSpinBox);
    containerLayout->addRow("Background Blur:", m_textContainerBlurSpinBox);

    legibilityLayout->addWidget(containerGroup);

    // Text Band subsection (hidden for template slides — per-zone controls used instead)
    m_bandGroup = new QGroupBox("Text Band", this);
    QGroupBox* bandGroup = m_bandGroup;
    QFormLayout* bandLayout = new QFormLayout(bandGroup);

    m_textBandEnabledCheck = new QCheckBox("Enable horizontal text band", this);
    bandLayout->addRow(m_textBandEnabledCheck);

    m_textBandColorButton = new QPushButton("Choose Color", this);
    connect(m_textBandColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextBandColor);
    bandLayout->addRow("Band Color:", m_textBandColorButton);

    m_textBandBlurSpinBox = new QSpinBox(this);
    m_textBandBlurSpinBox->setRange(0, 50);
    m_textBandBlurSpinBox->setValue(0);
    m_textBandBlurSpinBox->setSuffix(" px");
    m_textBandBlurSpinBox->setToolTip("Blur the background under the band (0 = no blur)");
    installWheelFilter(m_textBandBlurSpinBox);
    bandLayout->addRow("Background Blur:", m_textBandBlurSpinBox);

    legibilityLayout->addWidget(bandGroup);
    rightLayout->addWidget(legibilityGroup);

    // Add stretch to right column to push content to top
    rightLayout->addStretch();

    // === BACK TO LEFT COLUMN ===

    // Transition override section
    QGroupBox* transitionGroup = new QGroupBox("Transition Override", leftColumn);
    QFormLayout* transitionLayout = new QFormLayout(transitionGroup);

    m_transitionTypeCombo = new QComboBox(this);
    m_transitionTypeCombo->addItem("Use Default", "");  // Empty string = use default
    m_transitionTypeCombo->addItem("Cut (Instant)", "cut");
    m_transitionTypeCombo->addItem("Fade", "fade");
    m_transitionTypeCombo->addItem("Slide Left", "slideLeft");
    m_transitionTypeCombo->addItem("Slide Right", "slideRight");
    m_transitionTypeCombo->addItem("Slide Up", "slideUp");
    m_transitionTypeCombo->addItem("Slide Down", "slideDown");
    installWheelFilter(m_transitionTypeCombo);
    transitionLayout->addRow("Transition Type:", m_transitionTypeCombo);

    m_transitionDurationCombo = new QComboBox(this);
    m_transitionDurationCombo->addItem("Use Default", -1);  // -1 = use default
    m_transitionDurationCombo->addItem("Instant (0 ms)", 0);
    m_transitionDurationCombo->addItem("Very Fast (250 ms)", 250);
    m_transitionDurationCombo->addItem("Fast (500 ms)", 500);
    m_transitionDurationCombo->addItem("Medium (750 ms)", 750);
    m_transitionDurationCombo->addItem("Slow (1000 ms)", 1000);
    m_transitionDurationCombo->addItem("Very Slow (1500 ms)", 1500);
    m_transitionDurationCombo->addItem("Extra Slow (2000 ms)", 2000);
    installWheelFilter(m_transitionDurationCombo);
    transitionLayout->addRow("Transition Duration:", m_transitionDurationCombo);

    // Auto-advance timer (0 = disabled, positive = seconds)
    m_autoAdvanceSpinBox = new QSpinBox(this);
    m_autoAdvanceSpinBox->setRange(0, 300);
    m_autoAdvanceSpinBox->setSuffix(" sec");
    m_autoAdvanceSpinBox->setSpecialValueText("Disabled");
    m_autoAdvanceSpinBox->setToolTip("Auto-advance to next slide after this many seconds (0 = disabled)");
    installWheelFilter(m_autoAdvanceSpinBox);
    transitionLayout->addRow("Auto-Advance:", m_autoAdvanceSpinBox);

    leftLayout->addWidget(transitionGroup);

    // Presenter notes section
    QGroupBox* notesGroup = new QGroupBox("Presenter Notes", leftColumn);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setPlaceholderText("Notes for the presenter (shown only on confidence monitor, not on output display)...");
    m_notesEdit->setMaximumHeight(80);
    notesLayout->addWidget(m_notesEdit);

    leftLayout->addWidget(notesGroup);

    // Add stretch to left column to push content to top
    leftLayout->addStretch();

    // Add columns to the main content layout
    columnsLayout->addWidget(leftColumn, 1);   // Left column gets stretch factor 1
    columnsLayout->addWidget(rightColumn, 1);  // Right column gets stretch factor 1

    // Set the content widget in scroll area and add to main layout
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);  // Give scroll area stretch factor

    // Dialog buttons (outside scroll area, fixed at bottom)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton("OK", this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void SlideEditorDialog::setSlide(const Slide& slide)
{
    m_slide = slide;

    // Set template combo (block signal to avoid triggering onTemplateChanged)
    m_templateCombo->blockSignals(true);
    int tmplIndex = m_templateCombo->findData(static_cast<int>(slide.slideTemplate()));
    m_templateCombo->setCurrentIndex(tmplIndex >= 0 ? tmplIndex : 0);
    m_templateCombo->blockSignals(false);

    bool isTemplate = slide.hasTextZones();
    if (isTemplate) {
        // Multi-zone template: populate zone tabs
        rebuildZoneTabs(slide.textZones());
        m_textStack->setCurrentIndex(1);
    } else {
        // Blank template: single text path
        m_textStack->setCurrentIndex(0);
    }

    // Hide slide-level legibility groups for template slides (per-zone controls used instead)
    m_shadowGroup->setVisible(!isTemplate);
    m_containerGroup->setVisible(!isTemplate);
    m_bandGroup->setVisible(!isTemplate);

    // Update UI with slide data (single-text controls, used for Blank template)
    m_textEdit->setPlainText(slide.text());
    m_fontFamilyCombo->setCurrentText(slide.fontFamily());
    m_fontSizeSpinBox->setValue(slide.fontSize());

    updateColorButton(m_textColorButton, slide.textColor());
    updateColorButton(m_backgroundColorButton, slide.backgroundColor());
    m_gradientEditor->setGradientStops(slide.gradientStops());
    m_gradientEditor->setGradientType(slide.gradientType());
    m_gradientEditor->setGradientAngle(slide.gradientAngle());
    m_gradientEditor->setRadialCenterX(slide.radialCenterX());
    m_gradientEditor->setRadialCenterY(slide.radialCenterY());
    m_gradientEditor->setRadialRadius(slide.radialRadius());

    // Set background type
    switch (slide.backgroundType()) {
        case Slide::SolidColor:
            m_backgroundTypeCombo->setCurrentIndex(0);
            break;
        case Slide::Gradient:
            m_backgroundTypeCombo->setCurrentIndex(1);
            break;
        case Slide::Image:
            m_backgroundTypeCombo->setCurrentIndex(2);
            m_imagePathEdit->setText(slide.backgroundImagePath());

            // Show image preview if available
            if (!slide.backgroundImageData().isEmpty()) {
                QPixmap pixmap;
                if (pixmap.loadFromData(slide.backgroundImageData())) {
                    m_imagePreviewLabel->setPixmap(pixmap.scaled(
                        m_imagePreviewLabel->size(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                    ));
                }
            }
            break;
        case Slide::Video:
            m_backgroundTypeCombo->setCurrentIndex(3);
            m_videoPathEdit->setText(slide.backgroundVideoPath());
            m_videoLoopCheck->setChecked(slide.videoLoop());
            break;
    }

    updateBackgroundControls();

    // Set "Use own background" checkbox based on hasExplicitBackground flag
    m_useOwnBackgroundCheck->setChecked(slide.hasExplicitBackground());
    // Manually trigger the toggled logic for initial state
    m_backgroundTypeCombo->setEnabled(slide.hasExplicitBackground());
    m_backgroundStack->setEnabled(slide.hasExplicitBackground());

    // Set transition override settings
    // Find matching transition type in combo box
    QString transitionType = slide.transitionType();
    int typeIndex = m_transitionTypeCombo->findData(transitionType);
    if (typeIndex >= 0) {
        m_transitionTypeCombo->setCurrentIndex(typeIndex);
    } else {
        m_transitionTypeCombo->setCurrentIndex(0);  // Default to "Use Default"
    }

    // Find matching transition duration in combo box
    int transitionDuration = slide.transitionDuration();
    int durationIndex = m_transitionDurationCombo->findData(transitionDuration);
    if (durationIndex >= 0) {
        m_transitionDurationCombo->setCurrentIndex(durationIndex);
    } else {
        m_transitionDurationCombo->setCurrentIndex(0);  // Default to "Use Default"
    }

    // Load auto-advance timer
    m_autoAdvanceSpinBox->setValue(slide.autoAdvanceDuration());

    // Load presenter notes
    m_notesEdit->setPlainText(slide.notes());

    // Load text legibility settings - Drop shadow
    m_dropShadowEnabledCheck->setChecked(slide.dropShadowEnabled());
    updateColorButton(m_dropShadowColorButton, slide.dropShadowColor());
    m_dropShadowOffsetXSpinBox->setValue(slide.dropShadowOffsetX());
    m_dropShadowOffsetYSpinBox->setValue(slide.dropShadowOffsetY());
    m_dropShadowBlurSpinBox->setValue(slide.dropShadowBlur());

    // Load text legibility settings - Overlay
    m_overlayEnabledCheck->setChecked(slide.overlayEnabled());
    updateColorButton(m_overlayColorButton, slide.overlayColor());
    m_overlayBlurSpinBox->setValue(slide.overlayBlur());

    // Load text legibility settings - Text container
    m_textContainerEnabledCheck->setChecked(slide.textContainerEnabled());
    updateColorButton(m_textContainerColorButton, slide.textContainerColor());
    m_textContainerPaddingSpinBox->setValue(slide.textContainerPadding());
    m_textContainerRadiusSpinBox->setValue(slide.textContainerRadius());
    m_textContainerBlurSpinBox->setValue(slide.textContainerBlur());

    // Load text legibility settings - Text band
    m_textBandEnabledCheck->setChecked(slide.textBandEnabled());
    updateColorButton(m_textBandColorButton, slide.textBandColor());
    m_textBandBlurSpinBox->setValue(slide.textBandBlur());
}

Slide SlideEditorDialog::slide() const
{
    Slide slide = m_slide;

    // Template type
    auto tmpl = static_cast<SlideTemplate>(m_templateCombo->currentData().toInt());
    slide.setSlideTemplate(tmpl);

    if (tmpl != SlideTemplate::Blank && !m_zoneTabs.isEmpty()) {
        // Collect zone data from tabs
        bool refAtBottom = (tmpl == SlideTemplate::Scripture && m_settings
                            && m_settings->scriptureReferencePosition() == "bottom");
        auto zones = Slide::createTemplateZones(tmpl, refAtBottom);
        for (int i = 0; i < zones.size() && i < m_zoneTabs.size(); ++i) {
            zones[i].text = m_zoneTabs[i].textEdit->toPlainText();
            zones[i].fontFamily = m_zoneTabs[i].fontFamilyCombo->currentText();
            zones[i].fontSize = m_zoneTabs[i].fontSizeSpinBox->value();
            zones[i].textColor = m_zoneTabs[i].textColor;
            // Per-zone legibility
            zones[i].dropShadowEnabled = m_zoneTabs[i].dropShadowEnabledCheck->isChecked();
            zones[i].dropShadowColor = m_zoneTabs[i].dropShadowColor;
            zones[i].dropShadowOffsetX = m_zoneTabs[i].dropShadowOffsetXSpinBox->value();
            zones[i].dropShadowOffsetY = m_zoneTabs[i].dropShadowOffsetYSpinBox->value();
            zones[i].dropShadowBlur = m_zoneTabs[i].dropShadowBlurSpinBox->value();
            zones[i].textContainerEnabled = m_zoneTabs[i].textContainerEnabledCheck->isChecked();
            zones[i].textContainerColor = m_zoneTabs[i].textContainerColor;
            zones[i].textContainerPadding = m_zoneTabs[i].textContainerPaddingSpinBox->value();
            zones[i].textContainerRadius = m_zoneTabs[i].textContainerRadiusSpinBox->value();
            zones[i].textBandEnabled = m_zoneTabs[i].textBandEnabledCheck->isChecked();
            zones[i].textBandColor = m_zoneTabs[i].textBandColor;
        }
        slide.setTextZones(zones);
        // Clear legacy single-text field for cleanliness
        slide.setText(QString());
    } else {
        // Blank template: use single-text path
        slide.setTextZones({});
    }

    // Update slide with current UI values (single-text controls)
    if (tmpl == SlideTemplate::Blank) {
        slide.setText(m_textEdit->toPlainText());
    }
    slide.setFontFamily(m_fontFamilyCombo->currentText());
    slide.setFontSize(m_fontSizeSpinBox->value());

    // Cascading background flag
    slide.setHasExplicitBackground(m_useOwnBackgroundCheck->isChecked());

    // Background type
    int bgTypeIndex = m_backgroundTypeCombo->currentIndex();
    switch (bgTypeIndex) {
        case 0: // Solid color
            slide.setBackgroundType(Slide::SolidColor);
            break;
        case 1: // Gradient
            slide.setBackgroundType(Slide::Gradient);
            slide.setGradientStops(m_gradientEditor->gradientStops());
            slide.setGradientType(m_gradientEditor->gradientType());
            slide.setGradientAngle(m_gradientEditor->gradientAngle());
            slide.setRadialCenterX(m_gradientEditor->radialCenterX());
            slide.setRadialCenterY(m_gradientEditor->radialCenterY());
            slide.setRadialRadius(m_gradientEditor->radialRadius());
            break;
        case 2: // Image
            slide.setBackgroundType(Slide::Image);
            break;
        case 3: // Video
            slide.setBackgroundType(Slide::Video);
            slide.setVideoLoop(m_videoLoopCheck->isChecked());
            break;
    }

    // Transition override settings
    QString transitionType = m_transitionTypeCombo->currentData().toString();
    int transitionDuration = m_transitionDurationCombo->currentData().toInt();

    slide.setTransitionType(transitionType);
    slide.setTransitionDuration(transitionDuration);

    // Auto-advance timer
    slide.setAutoAdvanceDuration(m_autoAdvanceSpinBox->value());

    // Presenter notes
    slide.setNotes(m_notesEdit->toPlainText());

    // Text legibility - Drop shadow
    slide.setDropShadowEnabled(m_dropShadowEnabledCheck->isChecked());
    slide.setDropShadowOffsetX(m_dropShadowOffsetXSpinBox->value());
    slide.setDropShadowOffsetY(m_dropShadowOffsetYSpinBox->value());
    slide.setDropShadowBlur(m_dropShadowBlurSpinBox->value());

    // Text legibility - Overlay
    slide.setOverlayEnabled(m_overlayEnabledCheck->isChecked());
    slide.setOverlayBlur(m_overlayBlurSpinBox->value());

    // Text legibility - Text container
    slide.setTextContainerEnabled(m_textContainerEnabledCheck->isChecked());
    slide.setTextContainerPadding(m_textContainerPaddingSpinBox->value());
    slide.setTextContainerRadius(m_textContainerRadiusSpinBox->value());
    slide.setTextContainerBlur(m_textContainerBlurSpinBox->value());

    // Text legibility - Text band
    slide.setTextBandEnabled(m_textBandEnabledCheck->isChecked());
    slide.setTextBandBlur(m_textBandBlurSpinBox->value());

    return slide;
}

void SlideEditorDialog::onTemplateChanged(int index)
{
    auto tmpl = static_cast<SlideTemplate>(m_templateCombo->itemData(index).toInt());
    bool isTemplate = (tmpl != SlideTemplate::Blank);

    if (!isTemplate) {
        m_textStack->setCurrentIndex(0);
    } else {
        // Create default zones for the selected template
        bool refAtBottom = (tmpl == SlideTemplate::Scripture && m_settings
                            && m_settings->scriptureReferencePosition() == "bottom");
        auto zones = Slide::createTemplateZones(tmpl, refAtBottom);
        rebuildZoneTabs(zones);
        m_textStack->setCurrentIndex(1);
    }

    // Hide slide-level legibility groups for template slides (per-zone controls used instead)
    m_shadowGroup->setVisible(!isTemplate);
    m_containerGroup->setVisible(!isTemplate);
    m_bandGroup->setVisible(!isTemplate);
}

void SlideEditorDialog::rebuildZoneTabs(const QList<TextZone>& zones)
{
    // Clear existing tabs
    m_zoneTabWidget->clear();
    m_zoneTabs.clear();

    for (int i = 0; i < zones.size(); ++i) {
        const auto& zone = zones[i];

        QWidget* tab = new QWidget();
        QVBoxLayout* tabLayout = new QVBoxLayout(tab);

        QTextEdit* textEdit = new QTextEdit(tab);
        textEdit->setPlaceholderText(tr("Enter %1 text...").arg(zone.id));
        textEdit->setPlainText(zone.text);
        textEdit->setMinimumHeight(80);
        tabLayout->addWidget(textEdit);

        QFormLayout* styleLayout = new QFormLayout();

        QPushButton* colorButton = new QPushButton(tr("Choose Color"), tab);
        updateColorButton(colorButton, zone.textColor);
        int capturedIndex = i;
        connect(colorButton, &QPushButton::clicked, this, [this, capturedIndex]() {
            onChooseZoneTextColor(capturedIndex);
        });
        styleLayout->addRow(tr("Text Color:"), colorButton);

        QComboBox* fontCombo = new QComboBox(tab);
        fontCombo->addItems({"Arial", "Helvetica", "Georgia", "Verdana", "Times New Roman"});
        fontCombo->setCurrentText(zone.fontFamily);
        installWheelFilter(fontCombo);
        styleLayout->addRow(tr("Font:"), fontCombo);

        QSpinBox* sizeSpin = new QSpinBox(tab);
        sizeSpin->setRange(12, 144);
        sizeSpin->setValue(zone.fontSize);
        sizeSpin->setSuffix(tr(" pt"));
        installWheelFilter(sizeSpin);
        styleLayout->addRow(tr("Size:"), sizeSpin);

        tabLayout->addLayout(styleLayout);

        // Per-zone legibility controls (collapsible group box)
        QGroupBox* legGroup = new QGroupBox(tr("Legibility"), tab);
        legGroup->setCheckable(false);
        QVBoxLayout* legLayout = new QVBoxLayout(legGroup);

        // Drop shadow
        QGroupBox* dsSub = new QGroupBox(tr("Drop Shadow"), legGroup);
        QFormLayout* dsLayout = new QFormLayout(dsSub);
        QCheckBox* dsCheck = new QCheckBox(tr("Enable"), dsSub);
        dsCheck->setChecked(zone.dropShadowEnabled);
        dsLayout->addRow(dsCheck);
        QPushButton* dsColorBtn = new QPushButton(tr("Choose Color"), dsSub);
        updateColorButton(dsColorBtn, zone.dropShadowColor);
        int ci = i;
        connect(dsColorBtn, &QPushButton::clicked, this, [this, ci]() { onChooseZoneDropShadowColor(ci); });
        dsLayout->addRow(tr("Color:"), dsColorBtn);
        QHBoxLayout* dsOffLayout = new QHBoxLayout();
        QSpinBox* dsOffX = new QSpinBox(dsSub);
        dsOffX->setRange(-20, 20); dsOffX->setValue(zone.dropShadowOffsetX); dsOffX->setSuffix(tr(" px"));
        installWheelFilter(dsOffX);
        dsOffLayout->addWidget(new QLabel(tr("X:"), dsSub)); dsOffLayout->addWidget(dsOffX);
        QSpinBox* dsOffY = new QSpinBox(dsSub);
        dsOffY->setRange(-20, 20); dsOffY->setValue(zone.dropShadowOffsetY); dsOffY->setSuffix(tr(" px"));
        installWheelFilter(dsOffY);
        dsOffLayout->addWidget(new QLabel(tr("Y:"), dsSub)); dsOffLayout->addWidget(dsOffY);
        dsLayout->addRow(tr("Offset:"), dsOffLayout);
        QSpinBox* dsBlur = new QSpinBox(dsSub);
        dsBlur->setRange(0, 50); dsBlur->setValue(zone.dropShadowBlur); dsBlur->setSuffix(tr(" px"));
        installWheelFilter(dsBlur);
        dsLayout->addRow(tr("Blur:"), dsBlur);
        legLayout->addWidget(dsSub);

        // Text container
        QGroupBox* tcSub = new QGroupBox(tr("Text Container"), legGroup);
        QFormLayout* tcLayout = new QFormLayout(tcSub);
        QCheckBox* tcCheck = new QCheckBox(tr("Enable"), tcSub);
        tcCheck->setChecked(zone.textContainerEnabled);
        tcLayout->addRow(tcCheck);
        QPushButton* tcColorBtn = new QPushButton(tr("Choose Color"), tcSub);
        updateColorButton(tcColorBtn, zone.textContainerColor);
        connect(tcColorBtn, &QPushButton::clicked, this, [this, ci]() { onChooseZoneTextContainerColor(ci); });
        tcLayout->addRow(tr("Color:"), tcColorBtn);
        QSpinBox* tcPad = new QSpinBox(tcSub);
        tcPad->setRange(0, 100); tcPad->setValue(zone.textContainerPadding); tcPad->setSuffix(tr(" px"));
        installWheelFilter(tcPad);
        tcLayout->addRow(tr("Padding:"), tcPad);
        QSpinBox* tcRad = new QSpinBox(tcSub);
        tcRad->setRange(0, 50); tcRad->setValue(zone.textContainerRadius); tcRad->setSuffix(tr(" px"));
        installWheelFilter(tcRad);
        tcLayout->addRow(tr("Radius:"), tcRad);
        legLayout->addWidget(tcSub);

        // Text band
        QGroupBox* tbSub = new QGroupBox(tr("Text Band"), legGroup);
        QFormLayout* tbLayout = new QFormLayout(tbSub);
        QCheckBox* tbCheck = new QCheckBox(tr("Enable"), tbSub);
        tbCheck->setChecked(zone.textBandEnabled);
        tbLayout->addRow(tbCheck);
        QPushButton* tbColorBtn = new QPushButton(tr("Choose Color"), tbSub);
        updateColorButton(tbColorBtn, zone.textBandColor);
        connect(tbColorBtn, &QPushButton::clicked, this, [this, ci]() { onChooseZoneTextBandColor(ci); });
        tbLayout->addRow(tr("Color:"), tbColorBtn);
        legLayout->addWidget(tbSub);

        tabLayout->addWidget(legGroup);
        tabLayout->addStretch();

        // Capitalize first letter for tab label
        QString label = zone.id;
        if (!label.isEmpty())
            label[0] = label[0].toUpper();
        m_zoneTabWidget->addTab(tab, label);

        ZoneTab zt;
        zt.textEdit = textEdit;
        zt.fontFamilyCombo = fontCombo;
        zt.fontSizeSpinBox = sizeSpin;
        zt.textColorButton = colorButton;
        zt.textColor = zone.textColor;
        zt.dropShadowEnabledCheck = dsCheck;
        zt.dropShadowColorButton = dsColorBtn;
        zt.dropShadowColor = zone.dropShadowColor;
        zt.dropShadowOffsetXSpinBox = dsOffX;
        zt.dropShadowOffsetYSpinBox = dsOffY;
        zt.dropShadowBlurSpinBox = dsBlur;
        zt.textContainerEnabledCheck = tcCheck;
        zt.textContainerColorButton = tcColorBtn;
        zt.textContainerColor = zone.textContainerColor;
        zt.textContainerPaddingSpinBox = tcPad;
        zt.textContainerRadiusSpinBox = tcRad;
        zt.textBandEnabledCheck = tbCheck;
        zt.textBandColorButton = tbColorBtn;
        zt.textBandColor = zone.textBandColor;
        m_zoneTabs.append(zt);
    }
}

void SlideEditorDialog::onChooseZoneTextColor(int zoneIndex)
{
    if (zoneIndex < 0 || zoneIndex >= m_zoneTabs.size())
        return;

    QColor current = m_zoneTabs[zoneIndex].textColor;
    QColor color = QColorDialog::getColor(current, this, tr("Choose Text Color"));
    if (color.isValid()) {
        m_zoneTabs[zoneIndex].textColor = color;
        updateColorButton(m_zoneTabs[zoneIndex].textColorButton, color);
    }
}

void SlideEditorDialog::onChooseZoneDropShadowColor(int zoneIndex)
{
    if (zoneIndex < 0 || zoneIndex >= m_zoneTabs.size())
        return;

    QColor current = m_zoneTabs[zoneIndex].dropShadowColor;
    QColor color = QColorDialog::getColor(current, this, tr("Choose Drop Shadow Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_zoneTabs[zoneIndex].dropShadowColor = color;
        updateColorButton(m_zoneTabs[zoneIndex].dropShadowColorButton, color);
    }
}

void SlideEditorDialog::onChooseZoneTextContainerColor(int zoneIndex)
{
    if (zoneIndex < 0 || zoneIndex >= m_zoneTabs.size())
        return;

    QColor current = m_zoneTabs[zoneIndex].textContainerColor;
    QColor color = QColorDialog::getColor(current, this, tr("Choose Container Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_zoneTabs[zoneIndex].textContainerColor = color;
        updateColorButton(m_zoneTabs[zoneIndex].textContainerColorButton, color);
    }
}

void SlideEditorDialog::onChooseZoneTextBandColor(int zoneIndex)
{
    if (zoneIndex < 0 || zoneIndex >= m_zoneTabs.size())
        return;

    QColor current = m_zoneTabs[zoneIndex].textBandColor;
    QColor color = QColorDialog::getColor(current, this, tr("Choose Band Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_zoneTabs[zoneIndex].textBandColor = color;
        updateColorButton(m_zoneTabs[zoneIndex].textBandColorButton, color);
    }
}

void SlideEditorDialog::onBackgroundTypeChanged(int index)
{
    m_backgroundStack->setCurrentIndex(index);
}

void SlideEditorDialog::onChooseBackgroundColor()
{
    QColor currentColor = m_slide.backgroundColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Background Color");
    if (color.isValid()) {
        m_slide.setBackgroundColor(color);
        updateColorButton(m_backgroundColorButton, color);
    }
}

void SlideEditorDialog::onChooseTextColor()
{
    QColor currentColor = m_slide.textColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Text Color");
    if (color.isValid()) {
        m_slide.setTextColor(color);
        updateColorButton(m_textColorButton, color);
    }
}

void SlideEditorDialog::onChooseBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Import Background Image",
        QString(),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        // Add to media library (this copies the file to the library)
        QString libraryPath = fileName;
        if (m_mediaLibrary) {
            libraryPath = m_mediaLibrary->addImage(fileName);
            if (libraryPath.isEmpty()) {
                qWarning() << "Failed to add image to library, using original path";
                libraryPath = fileName;
            }
        }

        setImageFromPath(libraryPath);
    }
}

void SlideEditorDialog::onBrowseImageLibrary()
{
    if (!m_mediaLibrary) {
        return;
    }

    MediaLibraryDialog dialog(m_mediaLibrary, MediaLibrary::Image, m_thumbnailGen, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedPath = dialog.selectedPath();
        if (!selectedPath.isEmpty()) {
            setImageFromPath(selectedPath);
        }
    }
}

void SlideEditorDialog::setImageFromPath(const QString& path)
{
    // Read raw file bytes directly — avoids expensive decode→re-encode cycle
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read image:" << path;
        return;
    }
    QByteArray imageData = file.readAll();
    if (imageData.isEmpty()) {
        qWarning() << "Image file is empty:" << path;
        return;
    }

    // Update slide
    m_slide.setBackgroundImagePath(path);
    m_slide.setBackgroundImageData(imageData);

    // Update UI preview - load pixmap for the small preview widget
    QPixmap pixmap(path);
    QFileInfo fileInfo(path);
    m_imagePathEdit->setText(fileInfo.fileName());
    m_imagePathEdit->setToolTip(path);
    if (!pixmap.isNull()) {
        m_imagePreviewLabel->setPixmap(pixmap.scaled(
            m_imagePreviewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }

    qDebug() << "Loaded background image:" << path << "size:" << imageData.size() << "bytes";
}

void SlideEditorDialog::onChooseBackgroundVideo()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Import Background Video",
        QString(),
        "Video Files (*.mp4 *.webm *.avi *.mov *.mkv);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        // Add to media library (this copies the file to the library)
        QString libraryPath = fileName;
        if (m_mediaLibrary) {
            libraryPath = m_mediaLibrary->addVideo(fileName);
            if (libraryPath.isEmpty()) {
                qWarning() << "Failed to add video to library, using original path";
                libraryPath = fileName;
            }
        }

        setVideoFromPath(libraryPath);
    }
}

void SlideEditorDialog::onBrowseVideoLibrary()
{
    if (!m_mediaLibrary) {
        return;
    }

    MediaLibraryDialog dialog(m_mediaLibrary, MediaLibrary::Video, m_thumbnailGen, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedPath = dialog.selectedPath();
        if (!selectedPath.isEmpty()) {
            setVideoFromPath(selectedPath);
        }
    }
}

void SlideEditorDialog::setVideoFromPath(const QString& path)
{
    // Verify file exists
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qWarning() << "Video file does not exist:" << path;
        return;
    }

    // Update slide with video path (not embedded - videos are too large)
    m_slide.setBackgroundVideoPath(path);
    m_slide.setVideoLoop(m_videoLoopCheck->isChecked());

    // Update UI - show just the filename for cleaner display
    m_videoPathEdit->setText(fileInfo.fileName());
    m_videoPathEdit->setToolTip(path);

    qDebug() << "Selected background video:" << path;
}

void SlideEditorDialog::onChooseDropShadowColor()
{
    QColor currentColor = m_slide.dropShadowColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Drop Shadow Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_slide.setDropShadowColor(color);
        updateColorButton(m_dropShadowColorButton, color);
    }
}

void SlideEditorDialog::onChooseOverlayColor()
{
    QColor currentColor = m_slide.overlayColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Overlay Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_slide.setOverlayColor(color);
        updateColorButton(m_overlayColorButton, color);
    }
}

void SlideEditorDialog::onChooseTextContainerColor()
{
    QColor currentColor = m_slide.textContainerColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Text Container Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_slide.setTextContainerColor(color);
        updateColorButton(m_textContainerColorButton, color);
    }
}

void SlideEditorDialog::onChooseTextBandColor()
{
    QColor currentColor = m_slide.textBandColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Text Band Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_slide.setTextBandColor(color);
        updateColorButton(m_textBandColorButton, color);
    }
}

void SlideEditorDialog::updateBackgroundControls()
{
    int index = m_backgroundTypeCombo->currentIndex();
    m_backgroundStack->setCurrentIndex(index);
}

void SlideEditorDialog::updateColorButton(QPushButton* button, const QColor& color)
{
    // Set button background color to show the selected color
    QString styleSheet = QString(
        "QPushButton { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid #999; "
        "  padding: 5px; "
        "  min-width: 80px; "
        "}"
    ).arg(color.name(), color.lightness() > 128 ? "#000000" : "#ffffff");

    button->setStyleSheet(styleSheet);
    button->setText(color.name().toUpper());
}

} // namespace Clarity
