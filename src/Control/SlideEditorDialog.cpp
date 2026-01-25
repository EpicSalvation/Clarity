#include "SlideEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QPixmap>
#include <QBuffer>
#include <QDebug>

namespace Clarity {

SlideEditorDialog::SlideEditorDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("Edit Slide");
    resize(600, 500);
}

void SlideEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Text content section
    QGroupBox* textGroup = new QGroupBox("Slide Content", this);
    QVBoxLayout* textLayout = new QVBoxLayout(textGroup);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setPlaceholderText("Enter slide text here...");
    m_textEdit->setMinimumHeight(150);
    textLayout->addWidget(m_textEdit);

    mainLayout->addWidget(textGroup);

    // Text styling section
    QGroupBox* textStyleGroup = new QGroupBox("Text Style", this);
    QFormLayout* textStyleLayout = new QFormLayout(textStyleGroup);

    m_textColorButton = new QPushButton("Choose Color", this);
    connect(m_textColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseTextColor);
    textStyleLayout->addRow("Text Color:", m_textColorButton);

    m_fontFamilyCombo = new QComboBox(this);
    m_fontFamilyCombo->addItems({"Arial", "Helvetica", "Georgia", "Verdana", "Times New Roman"});
    textStyleLayout->addRow("Font Family:", m_fontFamilyCombo);

    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(" pt");
    textStyleLayout->addRow("Font Size:", m_fontSizeSpinBox);

    mainLayout->addWidget(textStyleGroup);

    // Background section
    QGroupBox* backgroundGroup = new QGroupBox("Background", this);
    QVBoxLayout* backgroundLayout = new QVBoxLayout(backgroundGroup);

    m_backgroundTypeCombo = new QComboBox(this);
    m_backgroundTypeCombo->addItem("Solid Color", "solidColor");
    m_backgroundTypeCombo->addItem("Gradient", "gradient");
    m_backgroundTypeCombo->addItem("Image", "image");
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

    // Page 1: Gradient
    QWidget* gradientPage = new QWidget(this);
    QFormLayout* gradientLayout = new QFormLayout(gradientPage);

    m_gradientStartColorButton = new QPushButton("Choose Color", this);
    connect(m_gradientStartColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseGradientStartColor);
    gradientLayout->addRow("Start Color:", m_gradientStartColorButton);

    m_gradientEndColorButton = new QPushButton("Choose Color", this);
    connect(m_gradientEndColorButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseGradientEndColor);
    gradientLayout->addRow("End Color:", m_gradientEndColorButton);

    m_gradientAngleSpinBox = new QSpinBox(this);
    m_gradientAngleSpinBox->setRange(0, 359);
    m_gradientAngleSpinBox->setValue(135);
    m_gradientAngleSpinBox->setSuffix("°");
    m_gradientAngleSpinBox->setToolTip("0° = top to bottom, 90° = left to right, 180° = bottom to top, 270° = right to left");
    gradientLayout->addRow("Gradient Angle:", m_gradientAngleSpinBox);

    m_backgroundStack->addWidget(gradientPage);

    // Page 2: Image
    QWidget* imagePage = new QWidget(this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imagePage);

    QHBoxLayout* imagePathLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit(this);
    m_imagePathEdit->setPlaceholderText("No image selected");
    m_imagePathEdit->setReadOnly(true);
    imagePathLayout->addWidget(m_imagePathEdit);

    m_choosImageButton = new QPushButton("Browse...", this);
    connect(m_choosImageButton, &QPushButton::clicked, this, &SlideEditorDialog::onChooseBackgroundImage);
    imagePathLayout->addWidget(m_choosImageButton);
    imageLayout->addLayout(imagePathLayout);

    m_imagePreviewLabel = new QLabel(this);
    m_imagePreviewLabel->setMinimumHeight(100);
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f5f5f5; }");
    m_imagePreviewLabel->setText("No image preview");
    imageLayout->addWidget(m_imagePreviewLabel);

    m_backgroundStack->addWidget(imagePage);

    backgroundLayout->addWidget(m_backgroundStack);
    mainLayout->addWidget(backgroundGroup);

    // Transition override section
    QGroupBox* transitionGroup = new QGroupBox("Transition Override", this);
    QFormLayout* transitionLayout = new QFormLayout(transitionGroup);

    m_transitionTypeCombo = new QComboBox(this);
    m_transitionTypeCombo->addItem("Use Default", "");  // Empty string = use default
    m_transitionTypeCombo->addItem("Cut (Instant)", "cut");
    m_transitionTypeCombo->addItem("Fade", "fade");
    m_transitionTypeCombo->addItem("Slide Left", "slideLeft");
    m_transitionTypeCombo->addItem("Slide Right", "slideRight");
    m_transitionTypeCombo->addItem("Slide Up", "slideUp");
    m_transitionTypeCombo->addItem("Slide Down", "slideDown");
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
    transitionLayout->addRow("Transition Duration:", m_transitionDurationCombo);

    mainLayout->addWidget(transitionGroup);

    // Dialog buttons
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

    // Update UI with slide data
    m_textEdit->setPlainText(slide.text());
    m_fontFamilyCombo->setCurrentText(slide.fontFamily());
    m_fontSizeSpinBox->setValue(slide.fontSize());

    updateColorButton(m_textColorButton, slide.textColor());
    updateColorButton(m_backgroundColorButton, slide.backgroundColor());
    updateColorButton(m_gradientStartColorButton, slide.gradientStartColor());
    updateColorButton(m_gradientEndColorButton, slide.gradientEndColor());
    m_gradientAngleSpinBox->setValue(slide.gradientAngle());

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
    }

    updateBackgroundControls();

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
}

Slide SlideEditorDialog::slide() const
{
    Slide slide = m_slide;

    // Update slide with current UI values
    slide.setText(m_textEdit->toPlainText());
    slide.setFontFamily(m_fontFamilyCombo->currentText());
    slide.setFontSize(m_fontSizeSpinBox->value());

    // Background type
    int bgTypeIndex = m_backgroundTypeCombo->currentIndex();
    switch (bgTypeIndex) {
        case 0: // Solid color
            slide.setBackgroundType(Slide::SolidColor);
            break;
        case 1: // Gradient
            slide.setBackgroundType(Slide::Gradient);
            slide.setGradientAngle(m_gradientAngleSpinBox->value());
            break;
        case 2: // Image
            slide.setBackgroundType(Slide::Image);
            break;
    }

    // Transition override settings
    QString transitionType = m_transitionTypeCombo->currentData().toString();
    int transitionDuration = m_transitionDurationCombo->currentData().toInt();

    slide.setTransitionType(transitionType);
    slide.setTransitionDuration(transitionDuration);

    return slide;
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

void SlideEditorDialog::onChooseGradientStartColor()
{
    QColor currentColor = m_slide.gradientStartColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Gradient Start Color");
    if (color.isValid()) {
        m_slide.setGradientStartColor(color);
        updateColorButton(m_gradientStartColorButton, color);
    }
}

void SlideEditorDialog::onChooseGradientEndColor()
{
    QColor currentColor = m_slide.gradientEndColor();
    QColor color = QColorDialog::getColor(currentColor, this, "Choose Gradient End Color");
    if (color.isValid()) {
        m_slide.setGradientEndColor(color);
        updateColorButton(m_gradientEndColorButton, color);
    }
}

void SlideEditorDialog::onChooseBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Choose Background Image",
        QString(),
        "Image Files (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        // Load image file
        QPixmap pixmap(fileName);
        if (pixmap.isNull()) {
            qWarning() << "Failed to load image:" << fileName;
            return;
        }

        // Convert to byte array
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");

        // Update slide
        m_slide.setBackgroundImagePath(fileName);
        m_slide.setBackgroundImageData(imageData);

        // Update UI
        m_imagePathEdit->setText(fileName);
        m_imagePreviewLabel->setPixmap(pixmap.scaled(
            m_imagePreviewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));

        qDebug() << "Loaded background image:" << fileName << "size:" << imageData.size() << "bytes";
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
