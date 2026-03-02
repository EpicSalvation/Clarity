// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SlideEditorDialog.h"
#include "SlideCanvasWidget.h"
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
#include <QButtonGroup>
#include <QSplitter>

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
    resize(1200, 750);
    setMinimumSize(900, 500);
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
    mainLayout->setSpacing(4);

    setupToolbar(mainLayout);
    setupCenterAndPanels(mainLayout);

    // OK / Cancel buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_okButton);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
}

void SlideEditorDialog::setupToolbar(QVBoxLayout* mainLayout)
{
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    // Template selector
    toolbar->addWidget(new QLabel(tr("Template:"), this));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(tr("Blank"), static_cast<int>(SlideTemplate::Blank));
    m_templateCombo->addItem(tr("Title"), static_cast<int>(SlideTemplate::Title));
    m_templateCombo->addItem(tr("Title & Body"), static_cast<int>(SlideTemplate::TitleBody));
    m_templateCombo->addItem(tr("Scripture"), static_cast<int>(SlideTemplate::Scripture));
    installWheelFilter(m_templateCombo);
    toolbar->addWidget(m_templateCombo);

    QFrame* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    toolbar->addWidget(sep1);

    // Font family
    m_fontFamilyCombo = new QComboBox(this);
    m_fontFamilyCombo->addItems({"Arial", "Helvetica", "Georgia", "Verdana", "Times New Roman"});
    m_fontFamilyCombo->setMinimumWidth(120);
    installWheelFilter(m_fontFamilyCombo);
    toolbar->addWidget(m_fontFamilyCombo);

    // Font size
    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(tr(" pt"));
    m_fontSizeSpinBox->setFixedWidth(80);
    installWheelFilter(m_fontSizeSpinBox);
    toolbar->addWidget(m_fontSizeSpinBox);

    // Font size decrease / increase buttons
    QToolButton* sizeDecButton = new QToolButton(this);
    sizeDecButton->setText(QStringLiteral("\u2212")); // minus sign
    sizeDecButton->setToolTip(tr("Decrease Font Size"));
    sizeDecButton->setFixedSize(24, 28);
    sizeDecButton->setAutoRepeat(true);
    connect(sizeDecButton, &QToolButton::clicked, this, [this]() {
        int val = m_fontSizeSpinBox->value() - 2;
        m_fontSizeSpinBox->setValue(qMax(m_fontSizeSpinBox->minimum(), val));
    });
    toolbar->addWidget(sizeDecButton);

    QToolButton* sizeIncButton = new QToolButton(this);
    sizeIncButton->setText(QStringLiteral("+"));
    sizeIncButton->setToolTip(tr("Increase Font Size"));
    sizeIncButton->setFixedSize(24, 28);
    sizeIncButton->setAutoRepeat(true);
    connect(sizeIncButton, &QToolButton::clicked, this, [this]() {
        int val = m_fontSizeSpinBox->value() + 2;
        m_fontSizeSpinBox->setValue(qMin(m_fontSizeSpinBox->maximum(), val));
    });
    toolbar->addWidget(sizeIncButton);

    // Text color button
    m_textColorButton = new QPushButton(this);
    m_textColorButton->setFixedSize(28, 28);
    m_textColorButton->setToolTip(tr("Text Color"));
    connect(m_textColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextColor);
    toolbar->addWidget(m_textColorButton);

    QFrame* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFrameShadow(QFrame::Sunken);
    toolbar->addWidget(sep2);

    // Alignment buttons
    m_alignLeftButton = new QToolButton(this);
    m_alignLeftButton->setText(tr("L"));
    m_alignLeftButton->setToolTip(tr("Align Left"));
    m_alignLeftButton->setCheckable(true);
    m_alignLeftButton->setFixedSize(28, 28);

    m_alignCenterButton = new QToolButton(this);
    m_alignCenterButton->setText(tr("C"));
    m_alignCenterButton->setToolTip(tr("Align Center"));
    m_alignCenterButton->setCheckable(true);
    m_alignCenterButton->setChecked(true);
    m_alignCenterButton->setFixedSize(28, 28);

    m_alignRightButton = new QToolButton(this);
    m_alignRightButton->setText(tr("R"));
    m_alignRightButton->setToolTip(tr("Align Right"));
    m_alignRightButton->setCheckable(true);
    m_alignRightButton->setFixedSize(28, 28);

    QButtonGroup* alignGroup = new QButtonGroup(this);
    alignGroup->setExclusive(true);
    alignGroup->addButton(m_alignLeftButton, 0);
    alignGroup->addButton(m_alignCenterButton, 1);
    alignGroup->addButton(m_alignRightButton, 2);

    toolbar->addWidget(m_alignLeftButton);
    toolbar->addWidget(m_alignCenterButton);
    toolbar->addWidget(m_alignRightButton);

    QFrame* sep3 = new QFrame(this);
    sep3->setFrameShape(QFrame::VLine);
    sep3->setFrameShadow(QFrame::Sunken);
    toolbar->addWidget(sep3);

    // List buttons
    m_bulletListButton = new QToolButton(this);
    m_bulletListButton->setText(QStringLiteral("\u2022")); // bullet character
    m_bulletListButton->setToolTip(tr("Bullet List"));
    m_bulletListButton->setCheckable(true);
    m_bulletListButton->setFixedSize(28, 28);
    toolbar->addWidget(m_bulletListButton);

    m_numberedListButton = new QToolButton(this);
    m_numberedListButton->setText(QStringLiteral("1."));
    m_numberedListButton->setToolTip(tr("Numbered List"));
    m_numberedListButton->setCheckable(true);
    m_numberedListButton->setFixedSize(28, 28);
    toolbar->addWidget(m_numberedListButton);

    toolbar->addStretch();

    mainLayout->addLayout(toolbar);

    // Wire toolbar → canvas
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorDialog::onTemplateChanged);
    connect(m_fontFamilyCombo, &QComboBox::currentTextChanged, this, [this](const QString& family) {
        if (!m_updatingFromZone)
            m_canvas->setZoneFontFamily(family);
    });
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int size) {
        if (!m_updatingFromZone)
            m_canvas->setZoneFontSize(size);
    });
    connect(alignGroup, &QButtonGroup::idClicked, this, [this](int id) {
        if (!m_updatingFromZone)
            m_canvas->setZoneHorizontalAlignment(id);
    });

    // List button connections
    connect(m_bulletListButton, &QToolButton::clicked, this, [this]() {
        if (!m_updatingFromZone)
            m_canvas->toggleBulletList();
    });
    connect(m_numberedListButton, &QToolButton::clicked, this, [this]() {
        if (!m_updatingFromZone)
            m_canvas->toggleNumberedList();
    });
}

void SlideEditorDialog::setupCenterAndPanels(QVBoxLayout* mainLayout)
{
    // Splitter: canvas on left, property panels on right
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: canvas
    m_canvas = new SlideCanvasWidget(m_settings, this);
    splitter->addWidget(m_canvas);

    connect(m_canvas, &SlideCanvasWidget::zoneSelected, this, &SlideEditorDialog::onZoneSelected);
    connect(m_canvas, &SlideCanvasWidget::listStyleChanged, this, [this](int style) {
        m_bulletListButton->setChecked(style == 1);
        m_numberedListButton->setChecked(style == 2);
    });

    // Right side: scrollable property panels
    QWidget* rightPanel = buildRightPanel();
    splitter->addWidget(rightPanel);

    // Give canvas ~65% of width, panels ~35%
    splitter->setStretchFactor(0, 65);
    splitter->setStretchFactor(1, 35);
    splitter->setSizes({700, 400});

    mainLayout->addWidget(splitter, 1);
}

QWidget* SlideEditorDialog::buildRightPanel()
{
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMinimumWidth(280);
    scroll->setMaximumWidth(450);

    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(4);

    layout->addWidget(buildBackgroundSection());
    layout->addWidget(buildTextEffectsSection());
    layout->addWidget(buildTransitionSection());
    layout->addWidget(buildNotesSection());
    layout->addStretch();

    scroll->setWidget(container);
    return scroll;
}

QWidget* SlideEditorDialog::buildBackgroundSection()
{
    QGroupBox* group = new QGroupBox(tr("Background"), this);
    QVBoxLayout* bgLayout = new QVBoxLayout(group);
    bgLayout->setContentsMargins(6, 6, 6, 6);
    bgLayout->setSpacing(4);

    // "Use own background" checkbox
    m_useOwnBackgroundCheck = new QCheckBox(tr("Use own background"), group);
    m_useOwnBackgroundCheck->setChecked(true);
    m_useOwnBackgroundCheck->setToolTip(
        tr("When unchecked, this slide inherits the background from the previous slide."));
    bgLayout->addWidget(m_useOwnBackgroundCheck);

    // Background type combo
    m_backgroundTypeCombo = new QComboBox(group);
    m_backgroundTypeCombo->addItem(tr("Solid Color"), "solidColor");
    m_backgroundTypeCombo->addItem(tr("Gradient"), "gradient");
    m_backgroundTypeCombo->addItem(tr("Image"), "image");
    m_backgroundTypeCombo->addItem(tr("Video"), "video");
    installWheelFilter(m_backgroundTypeCombo);
    connect(m_backgroundTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorDialog::onBackgroundTypeChanged);
    bgLayout->addWidget(m_backgroundTypeCombo);

    // Stacked widget for type-specific controls
    m_backgroundStack = new QStackedWidget(group);

    // Page 0: Solid color
    QWidget* solidPage = new QWidget();
    QFormLayout* solidLayout = new QFormLayout(solidPage);
    solidLayout->setContentsMargins(0, 0, 0, 0);
    m_backgroundColorButton = new QPushButton(tr("Choose Color"), solidPage);
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundColor);
    solidLayout->addRow(tr("Color:"), m_backgroundColorButton);
    m_backgroundStack->addWidget(solidPage);

    // Page 1: Gradient
    m_gradientEditor = new GradientEditorWidget(m_settings, group);
    m_backgroundStack->addWidget(m_gradientEditor);

    // Page 2: Image
    QWidget* imagePage = new QWidget();
    QVBoxLayout* imageLayout = new QVBoxLayout(imagePage);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* imagePathLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit(imagePage);
    m_imagePathEdit->setPlaceholderText(tr("No image selected"));
    m_imagePathEdit->setReadOnly(true);
    imagePathLayout->addWidget(m_imagePathEdit);
    m_choosImageButton = new QPushButton(tr("Import..."), imagePage);
    connect(m_choosImageButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundImage);
    imagePathLayout->addWidget(m_choosImageButton);
    m_imageLibraryButton = new QPushButton(tr("Library..."), imagePage);
    connect(m_imageLibraryButton, &QPushButton::clicked, this, &SlideEditorDialog::onBrowseImageLibrary);
    imagePathLayout->addWidget(m_imageLibraryButton);
    imageLayout->addLayout(imagePathLayout);
    m_imagePreviewLabel = new QLabel(imagePage);
    m_imagePreviewLabel->setMinimumHeight(50);
    m_imagePreviewLabel->setMaximumHeight(70);
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f5f5f5; }");
    m_imagePreviewLabel->setText(tr("No image preview"));
    imageLayout->addWidget(m_imagePreviewLabel);
    m_backgroundStack->addWidget(imagePage);

    // Page 3: Video
    QWidget* videoPage = new QWidget();
    QVBoxLayout* videoLayout = new QVBoxLayout(videoPage);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* videoPathLayout = new QHBoxLayout();
    m_videoPathEdit = new QLineEdit(videoPage);
    m_videoPathEdit->setPlaceholderText(tr("No video selected"));
    m_videoPathEdit->setReadOnly(true);
    videoPathLayout->addWidget(m_videoPathEdit);
    m_chooseVideoButton = new QPushButton(tr("Import..."), videoPage);
    connect(m_chooseVideoButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundVideo);
    videoPathLayout->addWidget(m_chooseVideoButton);
    m_videoLibraryButton = new QPushButton(tr("Library..."), videoPage);
    connect(m_videoLibraryButton, &QPushButton::clicked, this, &SlideEditorDialog::onBrowseVideoLibrary);
    videoPathLayout->addWidget(m_videoLibraryButton);
    videoLayout->addLayout(videoPathLayout);
    m_videoLoopCheck = new QCheckBox(tr("Loop video"), videoPage);
    m_videoLoopCheck->setChecked(true);
    videoLayout->addWidget(m_videoLoopCheck);
    videoLayout->addStretch();
    m_backgroundStack->addWidget(videoPage);

    bgLayout->addWidget(m_backgroundStack);

    // Background blur
    QFormLayout* blurLayout = new QFormLayout();
    blurLayout->setContentsMargins(0, 0, 0, 0);
    m_overlayBlurSpinBox = new QSpinBox(group);
    m_overlayBlurSpinBox->setRange(0, 50);
    m_overlayBlurSpinBox->setValue(0);
    m_overlayBlurSpinBox->setSuffix(tr(" px"));
    m_overlayBlurSpinBox->setToolTip(tr("Blur the background (0 = no blur)"));
    installWheelFilter(m_overlayBlurSpinBox);
    blurLayout->addRow(tr("Blur:"), m_overlayBlurSpinBox);
    bgLayout->addLayout(blurLayout);

    // Wire controls to canvas
    connect(m_useOwnBackgroundCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_backgroundTypeCombo->setEnabled(checked);
        m_backgroundStack->setEnabled(checked);
        m_overlayBlurSpinBox->setEnabled(checked);
        m_canvas->setHasExplicitBackground(checked);
    });
    connect(m_gradientEditor, &GradientEditorWidget::gradientChanged, this, [this]() {
        m_canvas->setGradientStops(m_gradientEditor->gradientStops());
        m_canvas->setGradientType(m_gradientEditor->gradientType());
        m_canvas->setGradientAngle(m_gradientEditor->gradientAngle());
        m_canvas->setRadialCenterX(m_gradientEditor->radialCenterX());
        m_canvas->setRadialCenterY(m_gradientEditor->radialCenterY());
        m_canvas->setRadialRadius(m_gradientEditor->radialRadius());
    });
    connect(m_overlayBlurSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        m_canvas->setOverlayBlur(val);
    });
    connect(m_videoLoopCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_canvas->setVideoLoop(checked);
    });

    return group;
}

QWidget* SlideEditorDialog::buildTextEffectsSection()
{
    QGroupBox* group = new QGroupBox(tr("Text Effects"), this);
    QVBoxLayout* teLayout = new QVBoxLayout(group);
    teLayout->setContentsMargins(6, 6, 6, 6);
    teLayout->setSpacing(4);

    // Drop Shadow
    m_shadowGroup = new QGroupBox(tr("Drop Shadow"), group);
    QFormLayout* shadowLayout = new QFormLayout(m_shadowGroup);
    shadowLayout->setContentsMargins(4, 4, 4, 4);
    m_dropShadowEnabledCheck = new QCheckBox(tr("Enabled"), m_shadowGroup);
    m_dropShadowEnabledCheck->setChecked(true);
    shadowLayout->addRow(m_dropShadowEnabledCheck);
    m_dropShadowColorButton = new QPushButton(tr("Color"), m_shadowGroup);
    connect(m_dropShadowColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseDropShadowColor);
    shadowLayout->addRow(tr("Color:"), m_dropShadowColorButton);
    QHBoxLayout* shadowOffsetLayout = new QHBoxLayout();
    m_dropShadowOffsetXSpinBox = new QSpinBox(m_shadowGroup);
    m_dropShadowOffsetXSpinBox->setRange(-20, 20);
    m_dropShadowOffsetXSpinBox->setValue(2);
    m_dropShadowOffsetXSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_dropShadowOffsetXSpinBox);
    shadowOffsetLayout->addWidget(new QLabel(tr("X:"), m_shadowGroup));
    shadowOffsetLayout->addWidget(m_dropShadowOffsetXSpinBox);
    m_dropShadowOffsetYSpinBox = new QSpinBox(m_shadowGroup);
    m_dropShadowOffsetYSpinBox->setRange(-20, 20);
    m_dropShadowOffsetYSpinBox->setValue(2);
    m_dropShadowOffsetYSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_dropShadowOffsetYSpinBox);
    shadowOffsetLayout->addWidget(new QLabel(tr("Y:"), m_shadowGroup));
    shadowOffsetLayout->addWidget(m_dropShadowOffsetYSpinBox);
    shadowLayout->addRow(tr("Offset:"), shadowOffsetLayout);
    m_dropShadowBlurSpinBox = new QSpinBox(m_shadowGroup);
    m_dropShadowBlurSpinBox->setRange(0, 50);
    m_dropShadowBlurSpinBox->setValue(0);
    m_dropShadowBlurSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_dropShadowBlurSpinBox);
    shadowLayout->addRow(tr("Blur:"), m_dropShadowBlurSpinBox);
    teLayout->addWidget(m_shadowGroup);

    // Wire shadow controls
    connect(m_dropShadowEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneDropShadowEnabled(checked);
        else m_canvas->setDropShadowEnabled(checked);
    });
    connect(m_dropShadowOffsetXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneDropShadowOffsetX(val);
        else m_canvas->setDropShadowOffsetX(val);
    });
    connect(m_dropShadowOffsetYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneDropShadowOffsetY(val);
        else m_canvas->setDropShadowOffsetY(val);
    });
    connect(m_dropShadowBlurSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneDropShadowBlur(val);
        else m_canvas->setDropShadowBlur(val);
    });

    // Background Overlay
    m_overlayGroup = new QGroupBox(tr("Overlay"), group);
    QFormLayout* overlayLayout = new QFormLayout(m_overlayGroup);
    overlayLayout->setContentsMargins(4, 4, 4, 4);
    m_overlayEnabledCheck = new QCheckBox(tr("Enabled"), m_overlayGroup);
    overlayLayout->addRow(m_overlayEnabledCheck);
    m_overlayColorButton = new QPushButton(tr("Color"), m_overlayGroup);
    connect(m_overlayColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseOverlayColor);
    overlayLayout->addRow(tr("Color:"), m_overlayColorButton);
    teLayout->addWidget(m_overlayGroup);

    connect(m_overlayEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_updatingFromZone) m_canvas->setOverlayEnabled(checked);
    });

    // Text Container
    m_containerGroup = new QGroupBox(tr("Text Container"), group);
    QFormLayout* containerLayout = new QFormLayout(m_containerGroup);
    containerLayout->setContentsMargins(4, 4, 4, 4);
    m_textContainerEnabledCheck = new QCheckBox(tr("Enabled"), m_containerGroup);
    containerLayout->addRow(m_textContainerEnabledCheck);
    m_textContainerColorButton = new QPushButton(tr("Color"), m_containerGroup);
    connect(m_textContainerColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextContainerColor);
    containerLayout->addRow(tr("Color:"), m_textContainerColorButton);
    m_textContainerPaddingSpinBox = new QSpinBox(m_containerGroup);
    m_textContainerPaddingSpinBox->setRange(0, 100);
    m_textContainerPaddingSpinBox->setValue(20);
    m_textContainerPaddingSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_textContainerPaddingSpinBox);
    containerLayout->addRow(tr("Padding:"), m_textContainerPaddingSpinBox);
    m_textContainerRadiusSpinBox = new QSpinBox(m_containerGroup);
    m_textContainerRadiusSpinBox->setRange(0, 50);
    m_textContainerRadiusSpinBox->setValue(8);
    m_textContainerRadiusSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_textContainerRadiusSpinBox);
    containerLayout->addRow(tr("Radius:"), m_textContainerRadiusSpinBox);
    m_textContainerBlurSpinBox = new QSpinBox(m_containerGroup);
    m_textContainerBlurSpinBox->setRange(0, 50);
    m_textContainerBlurSpinBox->setValue(0);
    m_textContainerBlurSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_textContainerBlurSpinBox);
    containerLayout->addRow(tr("Blur:"), m_textContainerBlurSpinBox);
    teLayout->addWidget(m_containerGroup);

    connect(m_textContainerEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextContainerEnabled(checked);
        else m_canvas->setTextContainerEnabled(checked);
    });
    connect(m_textContainerPaddingSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextContainerPadding(val);
        else m_canvas->setTextContainerPadding(val);
    });
    connect(m_textContainerRadiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextContainerRadius(val);
        else m_canvas->setTextContainerRadius(val);
    });
    connect(m_textContainerBlurSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        m_canvas->setTextContainerBlur(val);
    });

    // Text Band
    m_bandGroup = new QGroupBox(tr("Text Band"), group);
    QFormLayout* bandLayout = new QFormLayout(m_bandGroup);
    bandLayout->setContentsMargins(4, 4, 4, 4);
    m_textBandEnabledCheck = new QCheckBox(tr("Enabled"), m_bandGroup);
    bandLayout->addRow(m_textBandEnabledCheck);
    m_textBandColorButton = new QPushButton(tr("Color"), m_bandGroup);
    connect(m_textBandColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextBandColor);
    bandLayout->addRow(tr("Color:"), m_textBandColorButton);
    m_textBandBlurSpinBox = new QSpinBox(m_bandGroup);
    m_textBandBlurSpinBox->setRange(0, 50);
    m_textBandBlurSpinBox->setValue(0);
    m_textBandBlurSpinBox->setSuffix(tr(" px"));
    installWheelFilter(m_textBandBlurSpinBox);
    bandLayout->addRow(tr("Blur:"), m_textBandBlurSpinBox);
    teLayout->addWidget(m_bandGroup);

    connect(m_textBandEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingFromZone) return;
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextBandEnabled(checked);
        else m_canvas->setTextBandEnabled(checked);
    });
    connect(m_textBandBlurSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_updatingFromZone) return;
        m_canvas->setTextBandBlur(val);
    });

    return group;
}

QWidget* SlideEditorDialog::buildTransitionSection()
{
    QGroupBox* group = new QGroupBox(tr("Transition"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->setContentsMargins(6, 6, 6, 6);

    m_transitionTypeCombo = new QComboBox(group);
    m_transitionTypeCombo->addItem(tr("Use Default"), "");
    m_transitionTypeCombo->addItem(tr("Cut (Instant)"), "cut");
    m_transitionTypeCombo->addItem(tr("Fade"), "fade");
    m_transitionTypeCombo->addItem(tr("Slide Left"), "slideLeft");
    m_transitionTypeCombo->addItem(tr("Slide Right"), "slideRight");
    m_transitionTypeCombo->addItem(tr("Slide Up"), "slideUp");
    m_transitionTypeCombo->addItem(tr("Slide Down"), "slideDown");
    installWheelFilter(m_transitionTypeCombo);
    layout->addRow(tr("Type:"), m_transitionTypeCombo);

    m_transitionDurationCombo = new QComboBox(group);
    m_transitionDurationCombo->addItem(tr("Use Default"), -1);
    m_transitionDurationCombo->addItem(tr("Instant (0 ms)"), 0);
    m_transitionDurationCombo->addItem(tr("Very Fast (250 ms)"), 250);
    m_transitionDurationCombo->addItem(tr("Fast (500 ms)"), 500);
    m_transitionDurationCombo->addItem(tr("Medium (750 ms)"), 750);
    m_transitionDurationCombo->addItem(tr("Slow (1000 ms)"), 1000);
    m_transitionDurationCombo->addItem(tr("Very Slow (1500 ms)"), 1500);
    m_transitionDurationCombo->addItem(tr("Extra Slow (2000 ms)"), 2000);
    installWheelFilter(m_transitionDurationCombo);
    layout->addRow(tr("Duration:"), m_transitionDurationCombo);

    m_autoAdvanceSpinBox = new QSpinBox(group);
    m_autoAdvanceSpinBox->setRange(0, 300);
    m_autoAdvanceSpinBox->setSuffix(tr(" sec"));
    m_autoAdvanceSpinBox->setSpecialValueText(tr("Disabled"));
    m_autoAdvanceSpinBox->setToolTip(tr("Auto-advance after this many seconds (0 = disabled)"));
    installWheelFilter(m_autoAdvanceSpinBox);
    layout->addRow(tr("Auto-Advance:"), m_autoAdvanceSpinBox);

    return group;
}

QWidget* SlideEditorDialog::buildNotesSection()
{
    QGroupBox* group = new QGroupBox(tr("Presenter Notes"), this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setContentsMargins(6, 6, 6, 6);

    m_notesEdit = new QTextEdit(group);
    m_notesEdit->setPlaceholderText(tr("Notes shown only on confidence monitor..."));
    m_notesEdit->setMaximumHeight(80);
    layout->addWidget(m_notesEdit);

    return group;
}

// --- setSlide / slide ---

void SlideEditorDialog::setSlide(const Slide& slide)
{
    m_slide = slide;
    m_updatingFromZone = true;

    // Template combo
    m_templateCombo->blockSignals(true);
    int tmplIndex = m_templateCombo->findData(static_cast<int>(slide.slideTemplate()));
    m_templateCombo->setCurrentIndex(tmplIndex >= 0 ? tmplIndex : 0);
    m_templateCombo->blockSignals(false);

    // Load slide into canvas
    m_canvas->setSlide(slide);

    // Sync toolbar from first zone (or blank slide properties)
    if (slide.hasTextZones() && !slide.textZones().isEmpty()) {
        syncToolbarFromZone(0);
    } else {
        m_fontFamilyCombo->setCurrentText(slide.fontFamily());
        m_fontSizeSpinBox->setValue(slide.fontSize());
        updateColorButton(m_textColorButton, slide.textColor());
        m_alignCenterButton->setChecked(true);
    }

    // Background panel
    updateColorButton(m_backgroundColorButton, slide.backgroundColor());
    m_gradientEditor->setGradientStops(slide.gradientStops());
    m_gradientEditor->setGradientType(slide.gradientType());
    m_gradientEditor->setGradientAngle(slide.gradientAngle());
    m_gradientEditor->setRadialCenterX(slide.radialCenterX());
    m_gradientEditor->setRadialCenterY(slide.radialCenterY());
    m_gradientEditor->setRadialRadius(slide.radialRadius());

    switch (slide.backgroundType()) {
    case Slide::SolidColor: m_backgroundTypeCombo->setCurrentIndex(0); break;
    case Slide::Gradient:   m_backgroundTypeCombo->setCurrentIndex(1); break;
    case Slide::Image:
        m_backgroundTypeCombo->setCurrentIndex(2);
        m_imagePathEdit->setText(slide.backgroundImagePath());
        if (!slide.backgroundImageData().isEmpty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(slide.backgroundImageData())) {
                m_imagePreviewLabel->setPixmap(pixmap.scaled(
                    m_imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

    m_useOwnBackgroundCheck->setChecked(slide.hasExplicitBackground());
    m_backgroundTypeCombo->setEnabled(slide.hasExplicitBackground());
    m_backgroundStack->setEnabled(slide.hasExplicitBackground());
    m_overlayBlurSpinBox->setValue(slide.overlayBlur());

    // Text effects panel
    syncTextEffectsFromZone(m_canvas->selectedZoneIndex());

    // Transition panel
    int typeIndex = m_transitionTypeCombo->findData(slide.transitionType());
    m_transitionTypeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    int durationIndex = m_transitionDurationCombo->findData(slide.transitionDuration());
    m_transitionDurationCombo->setCurrentIndex(durationIndex >= 0 ? durationIndex : 0);
    m_autoAdvanceSpinBox->setValue(slide.autoAdvanceDuration());

    // Notes panel
    m_notesEdit->setPlainText(slide.notes());

    m_updatingFromZone = false;
}

Slide SlideEditorDialog::slide() const
{
    Slide s = m_canvas->slide();

    s.setHasExplicitBackground(m_useOwnBackgroundCheck->isChecked());

    int bgTypeIndex = m_backgroundTypeCombo->currentIndex();
    switch (bgTypeIndex) {
    case 0: s.setBackgroundType(Slide::SolidColor); break;
    case 1:
        s.setBackgroundType(Slide::Gradient);
        s.setGradientStops(m_gradientEditor->gradientStops());
        s.setGradientType(m_gradientEditor->gradientType());
        s.setGradientAngle(m_gradientEditor->gradientAngle());
        s.setRadialCenterX(m_gradientEditor->radialCenterX());
        s.setRadialCenterY(m_gradientEditor->radialCenterY());
        s.setRadialRadius(m_gradientEditor->radialRadius());
        break;
    case 2: s.setBackgroundType(Slide::Image); break;
    case 3:
        s.setBackgroundType(Slide::Video);
        s.setVideoLoop(m_videoLoopCheck->isChecked());
        break;
    }

    s.setTransitionType(m_transitionTypeCombo->currentData().toString());
    s.setTransitionDuration(m_transitionDurationCombo->currentData().toInt());
    s.setAutoAdvanceDuration(m_autoAdvanceSpinBox->value());
    s.setNotes(m_notesEdit->toPlainText());
    s.setOverlayBlur(m_overlayBlurSpinBox->value());

    return s;
}

// --- Slot implementations ---

void SlideEditorDialog::onTemplateChanged(int index)
{
    auto tmpl = static_cast<SlideTemplate>(m_templateCombo->itemData(index).toInt());
    m_canvas->setSlideTemplate(tmpl);
    if (m_canvas->zoneCount() > 0)
        syncToolbarFromZone(0);
}

void SlideEditorDialog::onZoneSelected(int index)
{
    syncToolbarFromZone(index);
    syncTextEffectsFromZone(index);
}

void SlideEditorDialog::syncToolbarFromZone(int zoneIndex)
{
    if (zoneIndex < 0 || zoneIndex >= m_canvas->zoneCount())
        return;

    m_updatingFromZone = true;

    Slide s = m_canvas->slide();
    if (s.hasTextZones() && zoneIndex < s.textZones().size()) {
        const auto& zone = s.textZones()[zoneIndex];
        m_fontFamilyCombo->setCurrentText(zone.fontFamily);
        m_fontSizeSpinBox->setValue(zone.fontSize);
        updateColorButton(m_textColorButton, zone.textColor);
        if (zone.horizontalAlignment == 0) m_alignLeftButton->setChecked(true);
        else if (zone.horizontalAlignment == 2) m_alignRightButton->setChecked(true);
        else m_alignCenterButton->setChecked(true);
    } else {
        m_fontFamilyCombo->setCurrentText(s.fontFamily());
        m_fontSizeSpinBox->setValue(s.fontSize());
        updateColorButton(m_textColorButton, s.textColor());
        m_alignCenterButton->setChecked(true);
    }

    // Sync list button state
    int listStyle = m_canvas->currentListStyle();
    m_bulletListButton->setChecked(listStyle == 1);
    m_numberedListButton->setChecked(listStyle == 2);

    m_updatingFromZone = false;
}

void SlideEditorDialog::syncTextEffectsFromZone(int zoneIndex)
{
    m_updatingFromZone = true;

    Slide s = m_canvas->slide();

    if (s.hasTextZones() && zoneIndex >= 0 && zoneIndex < s.textZones().size()) {
        const auto& zone = s.textZones()[zoneIndex];
        m_dropShadowEnabledCheck->setChecked(zone.dropShadowEnabled);
        updateColorButton(m_dropShadowColorButton, zone.dropShadowColor);
        m_dropShadowOffsetXSpinBox->setValue(zone.dropShadowOffsetX);
        m_dropShadowOffsetYSpinBox->setValue(zone.dropShadowOffsetY);
        m_dropShadowBlurSpinBox->setValue(zone.dropShadowBlur);
        m_textContainerEnabledCheck->setChecked(zone.textContainerEnabled);
        updateColorButton(m_textContainerColorButton, zone.textContainerColor);
        m_textContainerPaddingSpinBox->setValue(zone.textContainerPadding);
        m_textContainerRadiusSpinBox->setValue(zone.textContainerRadius);
        m_textBandEnabledCheck->setChecked(zone.textBandEnabled);
        updateColorButton(m_textBandColorButton, zone.textBandColor);
    } else {
        m_dropShadowEnabledCheck->setChecked(s.dropShadowEnabled());
        updateColorButton(m_dropShadowColorButton, s.dropShadowColor());
        m_dropShadowOffsetXSpinBox->setValue(s.dropShadowOffsetX());
        m_dropShadowOffsetYSpinBox->setValue(s.dropShadowOffsetY());
        m_dropShadowBlurSpinBox->setValue(s.dropShadowBlur());
        m_overlayEnabledCheck->setChecked(s.overlayEnabled());
        updateColorButton(m_overlayColorButton, s.overlayColor());
        m_textContainerEnabledCheck->setChecked(s.textContainerEnabled());
        updateColorButton(m_textContainerColorButton, s.textContainerColor());
        m_textContainerPaddingSpinBox->setValue(s.textContainerPadding());
        m_textContainerRadiusSpinBox->setValue(s.textContainerRadius());
        m_textContainerBlurSpinBox->setValue(s.textContainerBlur());
        m_textBandEnabledCheck->setChecked(s.textBandEnabled());
        updateColorButton(m_textBandColorButton, s.textBandColor());
        m_textBandBlurSpinBox->setValue(s.textBandBlur());
    }

    m_updatingFromZone = false;
}

void SlideEditorDialog::onBackgroundTypeChanged(int index)
{
    m_backgroundStack->setCurrentIndex(index);
    switch (index) {
    case 0: m_canvas->setBackgroundType(Slide::SolidColor); break;
    case 1: m_canvas->setBackgroundType(Slide::Gradient); break;
    case 2: m_canvas->setBackgroundType(Slide::Image); break;
    case 3: m_canvas->setBackgroundType(Slide::Video); break;
    }
}

void SlideEditorDialog::onChooseBackgroundColor()
{
    QColor current = m_slide.backgroundColor();
    QColor color = QColorDialog::getColor(current, this, tr("Choose Background Color"));
    if (color.isValid()) {
        m_slide.setBackgroundColor(color);
        updateColorButton(m_backgroundColorButton, color);
        m_canvas->setBackgroundColor(color);
    }
}

void SlideEditorDialog::onChooseTextColor()
{
    Slide s = m_canvas->slide();
    QColor current = s.textColor();
    if (s.hasTextZones() && m_canvas->selectedZoneIndex() >= 0
        && m_canvas->selectedZoneIndex() < s.textZones().size()) {
        current = s.textZones()[m_canvas->selectedZoneIndex()].textColor;
    }
    QColor color = QColorDialog::getColor(current, this, tr("Choose Text Color"));
    if (color.isValid()) {
        updateColorButton(m_textColorButton, color);
        m_canvas->setZoneTextColor(color);
    }
}

void SlideEditorDialog::onChooseBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Import Background Image"), QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QString libraryPath = fileName;
        if (m_mediaLibrary) {
            libraryPath = m_mediaLibrary->addImage(fileName);
            if (libraryPath.isEmpty()) libraryPath = fileName;
        }
        setImageFromPath(libraryPath);
    }
}

void SlideEditorDialog::onBrowseImageLibrary()
{
    if (!m_mediaLibrary) return;
    MediaLibraryDialog dialog(m_mediaLibrary, MediaLibrary::Image, m_thumbnailGen, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedPath = dialog.selectedPath();
        if (!selectedPath.isEmpty())
            setImageFromPath(selectedPath);
    }
}

void SlideEditorDialog::setImageFromPath(const QString& path)
{
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

    m_slide.setBackgroundImagePath(path);
    m_slide.setBackgroundImageData(imageData);
    m_canvas->setBackgroundImagePath(path);
    m_canvas->setBackgroundImageData(imageData);

    QPixmap pixmap(path);
    QFileInfo fileInfo(path);
    m_imagePathEdit->setText(fileInfo.fileName());
    m_imagePathEdit->setToolTip(path);
    if (!pixmap.isNull()) {
        m_imagePreviewLabel->setPixmap(pixmap.scaled(
            m_imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void SlideEditorDialog::onChooseBackgroundVideo()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Import Background Video"), QString(),
        tr("Video Files (*.mp4 *.webm *.avi *.mov *.mkv);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QString libraryPath = fileName;
        if (m_mediaLibrary) {
            libraryPath = m_mediaLibrary->addVideo(fileName);
            if (libraryPath.isEmpty()) libraryPath = fileName;
        }
        setVideoFromPath(libraryPath);
    }
}

void SlideEditorDialog::onBrowseVideoLibrary()
{
    if (!m_mediaLibrary) return;
    MediaLibraryDialog dialog(m_mediaLibrary, MediaLibrary::Video, m_thumbnailGen, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedPath = dialog.selectedPath();
        if (!selectedPath.isEmpty())
            setVideoFromPath(selectedPath);
    }
}

void SlideEditorDialog::setVideoFromPath(const QString& path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qWarning() << "Video file does not exist:" << path;
        return;
    }
    m_slide.setBackgroundVideoPath(path);
    m_slide.setVideoLoop(m_videoLoopCheck->isChecked());
    m_canvas->setBackgroundVideoPath(path);
    m_videoPathEdit->setText(fileInfo.fileName());
    m_videoPathEdit->setToolTip(path);
}

void SlideEditorDialog::onChooseDropShadowColor()
{
    Slide s = m_canvas->slide();
    QColor current = s.dropShadowColor();
    if (s.hasTextZones() && m_canvas->selectedZoneIndex() >= 0
        && m_canvas->selectedZoneIndex() < s.textZones().size()) {
        current = s.textZones()[m_canvas->selectedZoneIndex()].dropShadowColor;
    }
    QColor color = QColorDialog::getColor(current, this, tr("Choose Drop Shadow Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        updateColorButton(m_dropShadowColorButton, color);
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneDropShadowColor(color);
        else { m_slide.setDropShadowColor(color); m_canvas->setDropShadowColor(color); }
    }
}

void SlideEditorDialog::onChooseOverlayColor()
{
    QColor current = m_slide.overlayColor();
    QColor color = QColorDialog::getColor(current, this, tr("Choose Overlay Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_slide.setOverlayColor(color);
        updateColorButton(m_overlayColorButton, color);
        m_canvas->setOverlayColor(color);
    }
}

void SlideEditorDialog::onChooseTextContainerColor()
{
    Slide s = m_canvas->slide();
    QColor current = s.textContainerColor();
    if (s.hasTextZones() && m_canvas->selectedZoneIndex() >= 0
        && m_canvas->selectedZoneIndex() < s.textZones().size()) {
        current = s.textZones()[m_canvas->selectedZoneIndex()].textContainerColor;
    }
    QColor color = QColorDialog::getColor(current, this, tr("Choose Container Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        updateColorButton(m_textContainerColorButton, color);
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextContainerColor(color);
        else { m_slide.setTextContainerColor(color); m_canvas->setTextContainerColor(color); }
    }
}

void SlideEditorDialog::onChooseTextBandColor()
{
    Slide s = m_canvas->slide();
    QColor current = s.textBandColor();
    if (s.hasTextZones() && m_canvas->selectedZoneIndex() >= 0
        && m_canvas->selectedZoneIndex() < s.textZones().size()) {
        current = s.textZones()[m_canvas->selectedZoneIndex()].textBandColor;
    }
    QColor color = QColorDialog::getColor(current, this, tr("Choose Band Color"),
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        updateColorButton(m_textBandColorButton, color);
        if (m_canvas->zoneCount() > 1) m_canvas->setZoneTextBandColor(color);
        else { m_slide.setTextBandColor(color); m_canvas->setTextBandColor(color); }
    }
}

void SlideEditorDialog::updateBackgroundControls()
{
    m_backgroundStack->setCurrentIndex(m_backgroundTypeCombo->currentIndex());
}

void SlideEditorDialog::updateColorButton(QPushButton* button, const QColor& color)
{
    QString styleSheet = QString(
        "QPushButton { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid #999; "
        "  padding: 3px; "
        "  min-width: 28px; "
        "}"
    ).arg(color.name(), color.lightness() > 128 ? "#000000" : "#ffffff");

    button->setStyleSheet(styleSheet);
    button->setText(color.name().toUpper());
}

} // namespace Clarity
