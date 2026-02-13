#include "ThemeEditorDialog.h"
#include "GradientEditorWidget.h"
#include "Core/SettingsManager.h"
#include "Core/WheelEventFilter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QScrollArea>

namespace Clarity {

ThemeEditorDialog::ThemeEditorDialog(SettingsManager* settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_backgroundColorButton(nullptr)
    , m_textColorButton(nullptr)
    , m_accentColorButton(nullptr)
    , m_fontFamilyCombo(nullptr)
    , m_titleFontSizeSpinBox(nullptr)
    , m_bodyFontSizeSpinBox(nullptr)
    , m_backgroundTypeCombo(nullptr)
    , m_backgroundStack(nullptr)
    , m_gradientEditor(nullptr)
    , m_previewLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isNewTheme(true)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_accentColor(QColor("#fbbf24"))
{
    setupUI();
    updatePreview();
}

ThemeEditorDialog::ThemeEditorDialog(SettingsManager* settings, const Theme& theme, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_backgroundColorButton(nullptr)
    , m_textColorButton(nullptr)
    , m_accentColorButton(nullptr)
    , m_fontFamilyCombo(nullptr)
    , m_titleFontSizeSpinBox(nullptr)
    , m_bodyFontSizeSpinBox(nullptr)
    , m_backgroundTypeCombo(nullptr)
    , m_backgroundStack(nullptr)
    , m_gradientEditor(nullptr)
    , m_previewLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isNewTheme(false)
    , m_originalName(theme.name())
    , m_backgroundColor(theme.backgroundColor())
    , m_textColor(theme.textColor())
    , m_accentColor(theme.accentColor())
{
    setupUI();
    loadTheme(theme);
    updatePreview();
}

void ThemeEditorDialog::installWheelFilter(QWidget* widget)
{
    if (m_settings) {
        widget->installEventFilter(new WheelEventFilter(m_settings, widget));
        widget->setFocusPolicy(Qt::StrongFocus);
    }
}

void ThemeEditorDialog::setupUI()
{
    setWindowTitle(m_isNewTheme ? tr("Create New Theme") : tr("Edit Theme"));
    setMinimumSize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Scroll area for the content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Container widget for scroll area
    QWidget* scrollContent = new QWidget();
    scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);

    // Basic info group
    QGroupBox* infoGroup = new QGroupBox("Theme Info", scrollContent);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);

    m_nameEdit = new QLineEdit(scrollContent);
    m_nameEdit->setPlaceholderText("Enter theme name...");
    infoLayout->addRow("Name:", m_nameEdit);

    m_descriptionEdit = new QTextEdit(scrollContent);
    m_descriptionEdit->setPlaceholderText("Enter theme description...");
    m_descriptionEdit->setMaximumHeight(60);
    infoLayout->addRow("Description:", m_descriptionEdit);

    contentLayout->addWidget(infoGroup);

    // Colors group
    QGroupBox* colorsGroup = new QGroupBox("Colors", scrollContent);
    QFormLayout* colorsLayout = new QFormLayout(colorsGroup);

    m_textColorButton = new QPushButton(scrollContent);
    m_textColorButton->setMinimumWidth(120);
    connect(m_textColorButton, &QPushButton::clicked, this, &ThemeEditorDialog::onChooseTextColor);
    updateColorButton(m_textColorButton, m_textColor);
    colorsLayout->addRow("Text Color:", m_textColorButton);

    m_accentColorButton = new QPushButton(scrollContent);
    m_accentColorButton->setMinimumWidth(120);
    connect(m_accentColorButton, &QPushButton::clicked, this, &ThemeEditorDialog::onChooseAccentColor);
    updateColorButton(m_accentColorButton, m_accentColor);
    colorsLayout->addRow("Accent Color:", m_accentColorButton);

    contentLayout->addWidget(colorsGroup);

    // Typography group
    QGroupBox* typographyGroup = new QGroupBox("Typography", scrollContent);
    QFormLayout* typographyLayout = new QFormLayout(typographyGroup);

    m_fontFamilyCombo = new QComboBox(scrollContent);
    QFontDatabase fontDb;
    m_fontFamilyCombo->addItems(fontDb.families());
    m_fontFamilyCombo->setCurrentText("Arial");
    connect(m_fontFamilyCombo, &QComboBox::currentTextChanged, this, &ThemeEditorDialog::updatePreview);
    installWheelFilter(m_fontFamilyCombo);
    typographyLayout->addRow("Font Family:", m_fontFamilyCombo);

    m_titleFontSizeSpinBox = new QSpinBox(scrollContent);
    m_titleFontSizeSpinBox->setRange(24, 120);
    m_titleFontSizeSpinBox->setValue(72);
    m_titleFontSizeSpinBox->setSuffix(" pt");
    connect(m_titleFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeEditorDialog::updatePreview);
    installWheelFilter(m_titleFontSizeSpinBox);
    typographyLayout->addRow("Title Size:", m_titleFontSizeSpinBox);

    m_bodyFontSizeSpinBox = new QSpinBox(scrollContent);
    m_bodyFontSizeSpinBox->setRange(12, 96);
    m_bodyFontSizeSpinBox->setValue(48);
    m_bodyFontSizeSpinBox->setSuffix(" pt");
    connect(m_bodyFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeEditorDialog::updatePreview);
    installWheelFilter(m_bodyFontSizeSpinBox);
    typographyLayout->addRow("Body Size:", m_bodyFontSizeSpinBox);

    contentLayout->addWidget(typographyGroup);

    // Background group
    QGroupBox* backgroundGroup = new QGroupBox("Background", scrollContent);
    QVBoxLayout* backgroundLayout = new QVBoxLayout(backgroundGroup);

    QHBoxLayout* typeLayout = new QHBoxLayout();
    QLabel* typeLabel = new QLabel("Type:", scrollContent);
    m_backgroundTypeCombo = new QComboBox(scrollContent);
    m_backgroundTypeCombo->addItem("Solid Color", static_cast<int>(Slide::SolidColor));
    m_backgroundTypeCombo->addItem("Gradient", static_cast<int>(Slide::Gradient));
    connect(m_backgroundTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThemeEditorDialog::onBackgroundTypeChanged);
    installWheelFilter(m_backgroundTypeCombo);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_backgroundTypeCombo);
    typeLayout->addStretch();
    backgroundLayout->addLayout(typeLayout);

    // Stacked widget for different background type controls
    m_backgroundStack = new QStackedWidget(scrollContent);

    // Solid color page
    QWidget* solidPage = new QWidget(scrollContent);
    QFormLayout* solidLayout = new QFormLayout(solidPage);
    m_backgroundColorButton = new QPushButton(scrollContent);
    m_backgroundColorButton->setMinimumWidth(120);
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &ThemeEditorDialog::onChooseBackgroundColor);
    updateColorButton(m_backgroundColorButton, m_backgroundColor);
    solidLayout->addRow("Background:", m_backgroundColorButton);
    m_backgroundStack->addWidget(solidPage);

    // Gradient page (multi-stop editor)
    m_gradientEditor = new GradientEditorWidget(m_settings, scrollContent);
    connect(m_gradientEditor, &GradientEditorWidget::gradientChanged, this, &ThemeEditorDialog::updatePreview);
    m_backgroundStack->addWidget(m_gradientEditor);

    backgroundLayout->addWidget(m_backgroundStack);
    contentLayout->addWidget(backgroundGroup);

    // Preview group
    QGroupBox* previewGroup = new QGroupBox("Preview", scrollContent);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel(scrollContent);
    m_previewLabel->setMinimumSize(300, 150);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    previewLayout->addWidget(m_previewLabel);

    contentLayout->addWidget(previewGroup);

    // Set scroll area widget
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Dialog buttons (outside scroll area)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton(m_isNewTheme ? "Create" : "Save", this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &ThemeEditorDialog::validateAndAccept);
    buttonLayout->addWidget(m_okButton);

    mainLayout->addLayout(buttonLayout);
}

void ThemeEditorDialog::loadTheme(const Theme& theme)
{
    m_nameEdit->setText(theme.name());
    m_descriptionEdit->setPlainText(theme.description());

    // Colors
    m_backgroundColor = theme.backgroundColor();
    m_textColor = theme.textColor();
    m_accentColor = theme.accentColor();

    updateColorButton(m_backgroundColorButton, m_backgroundColor);
    updateColorButton(m_textColorButton, m_textColor);
    updateColorButton(m_accentColorButton, m_accentColor);

    // Gradient editor
    m_gradientEditor->setGradientStops(theme.gradientStops());
    m_gradientEditor->setGradientType(theme.gradientType());
    m_gradientEditor->setGradientAngle(theme.gradientAngle());
    m_gradientEditor->setRadialCenterX(theme.radialCenterX());
    m_gradientEditor->setRadialCenterY(theme.radialCenterY());
    m_gradientEditor->setRadialRadius(theme.radialRadius());

    // Typography
    m_fontFamilyCombo->setCurrentText(theme.fontFamily());
    m_titleFontSizeSpinBox->setValue(theme.titleFontSize());
    m_bodyFontSizeSpinBox->setValue(theme.bodyFontSize());

    // Background type
    int typeIndex = (theme.backgroundType() == Slide::Gradient) ? 1 : 0;
    m_backgroundTypeCombo->setCurrentIndex(typeIndex);
    m_backgroundStack->setCurrentIndex(typeIndex);

    // Disable name editing for built-in themes
    if (theme.isBuiltIn()) {
        m_nameEdit->setEnabled(false);
    }
}

Theme ThemeEditorDialog::theme() const
{
    Theme theme;

    theme.setName(m_nameEdit->text().trimmed());
    theme.setDescription(m_descriptionEdit->toPlainText().trimmed());
    theme.setBuiltIn(false);

    // Colors
    theme.setBackgroundColor(m_backgroundColor);
    theme.setTextColor(m_textColor);
    theme.setAccentColor(m_accentColor);

    // Typography
    theme.setFontFamily(m_fontFamilyCombo->currentText());
    theme.setTitleFontSize(m_titleFontSizeSpinBox->value());
    theme.setBodyFontSize(m_bodyFontSizeSpinBox->value());

    // Background type
    int typeIndex = m_backgroundTypeCombo->currentIndex();
    if (typeIndex == 1) {
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops(m_gradientEditor->gradientStops());
        theme.setGradientType(m_gradientEditor->gradientType());
        theme.setGradientAngle(m_gradientEditor->gradientAngle());
        theme.setRadialCenterX(m_gradientEditor->radialCenterX());
        theme.setRadialCenterY(m_gradientEditor->radialCenterY());
        theme.setRadialRadius(m_gradientEditor->radialRadius());
    } else {
        theme.setBackgroundType(Slide::SolidColor);
    }

    return theme;
}

void ThemeEditorDialog::onBackgroundTypeChanged(int index)
{
    m_backgroundStack->setCurrentIndex(index);
    updatePreview();
}

void ThemeEditorDialog::onChooseBackgroundColor()
{
    QColor color = QColorDialog::getColor(m_backgroundColor, this, "Choose Background Color");
    if (color.isValid()) {
        m_backgroundColor = color;
        updateColorButton(m_backgroundColorButton, color);
        updatePreview();
    }
}

void ThemeEditorDialog::onChooseTextColor()
{
    QColor color = QColorDialog::getColor(m_textColor, this, "Choose Text Color");
    if (color.isValid()) {
        m_textColor = color;
        updateColorButton(m_textColorButton, color);
        updatePreview();
    }
}

void ThemeEditorDialog::onChooseAccentColor()
{
    QColor color = QColorDialog::getColor(m_accentColor, this, "Choose Accent Color");
    if (color.isValid()) {
        m_accentColor = color;
        updateColorButton(m_accentColorButton, color);
        updatePreview();
    }
}

void ThemeEditorDialog::updateColorButton(QPushButton* button, const QColor& color)
{
    // Set button background to the color
    QString textColor = (color.lightness() > 128) ? "#000000" : "#ffffff";
    button->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border: 1px solid #888; padding: 4px; }")
            .arg(color.name(), textColor)
    );
    button->setText(color.name().toUpper());
}

void ThemeEditorDialog::updatePreview()
{
    int width = m_previewLabel->width() - 4;  // Account for frame
    int height = m_previewLabel->height() - 4;
    if (width < 10) width = 300;
    if (height < 10) height = 150;

    QPixmap pixmap(width, height);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    int typeIndex = m_backgroundTypeCombo->currentIndex();
    if (typeIndex == 1 && m_gradientEditor) {  // Gradient
        QList<GradientStop> stops = m_gradientEditor->gradientStops();

        if (m_gradientEditor->gradientType() == RadialGradient) {
            double cx = m_gradientEditor->radialCenterX() * width;
            double cy = m_gradientEditor->radialCenterY() * height;
            double r = m_gradientEditor->radialRadius() * qMax(width, height);
            QRadialGradient gradient(QPointF(cx, cy), r);
            for (const auto& stop : stops)
                gradient.setColorAt(stop.position, stop.color);
            painter.fillRect(pixmap.rect(), gradient);
        } else {
            int angle = m_gradientEditor->gradientAngle();
            double radians = angle * M_PI / 180.0;
            double centerX = width / 2.0;
            double centerY = height / 2.0;
            double diagonal = qSqrt(width * width + height * height) / 2.0;
            QPointF start(centerX + diagonal * qSin(radians), centerY - diagonal * qCos(radians));
            QPointF end(centerX - diagonal * qSin(radians), centerY + diagonal * qCos(radians));
            QLinearGradient gradient(start, end);
            for (const auto& stop : stops)
                gradient.setColorAt(stop.position, stop.color);
            painter.fillRect(pixmap.rect(), gradient);
        }
    } else {  // Solid color
        painter.fillRect(pixmap.rect(), m_backgroundColor);
    }

    // Draw sample text
    QFont font(m_fontFamilyCombo->currentText());

    // Scale font size for preview (preview is much smaller than actual slide)
    int previewFontSize = qMax(12, m_bodyFontSizeSpinBox->value() / 4);
    font.setPixelSize(previewFontSize);

    painter.setFont(font);
    painter.setPen(m_textColor);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "Sample Text\nPreview");

    painter.end();

    m_previewLabel->setPixmap(pixmap);
}

void ThemeEditorDialog::validateAndAccept()
{
    QString name = m_nameEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name", "Please enter a theme name.");
        m_nameEdit->setFocus();
        return;
    }

    accept();
}

} // namespace Clarity
