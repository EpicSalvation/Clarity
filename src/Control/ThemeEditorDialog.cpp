#include "ThemeEditorDialog.h"
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
    , m_gradientStartColorButton(nullptr)
    , m_gradientEndColorButton(nullptr)
    , m_gradientAngleSpinBox(nullptr)
    , m_previewLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isNewTheme(true)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_accentColor(QColor("#fbbf24"))
    , m_gradientStartColor(QColor("#1e3a8a"))
    , m_gradientEndColor(QColor("#60a5fa"))
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
    , m_gradientStartColorButton(nullptr)
    , m_gradientEndColorButton(nullptr)
    , m_gradientAngleSpinBox(nullptr)
    , m_previewLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isNewTheme(false)
    , m_originalName(theme.name())
    , m_backgroundColor(theme.backgroundColor())
    , m_textColor(theme.textColor())
    , m_accentColor(theme.accentColor())
    , m_gradientStartColor(theme.gradientStartColor())
    , m_gradientEndColor(theme.gradientEndColor())
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

    // Gradient page
    QWidget* gradientPage = new QWidget(scrollContent);
    QFormLayout* gradientLayout = new QFormLayout(gradientPage);

    m_gradientStartColorButton = new QPushButton(scrollContent);
    m_gradientStartColorButton->setMinimumWidth(120);
    connect(m_gradientStartColorButton, &QPushButton::clicked, this, &ThemeEditorDialog::onChooseGradientStartColor);
    updateColorButton(m_gradientStartColorButton, m_gradientStartColor);
    gradientLayout->addRow("Start Color:", m_gradientStartColorButton);

    m_gradientEndColorButton = new QPushButton(scrollContent);
    m_gradientEndColorButton->setMinimumWidth(120);
    connect(m_gradientEndColorButton, &QPushButton::clicked, this, &ThemeEditorDialog::onChooseGradientEndColor);
    updateColorButton(m_gradientEndColorButton, m_gradientEndColor);
    gradientLayout->addRow("End Color:", m_gradientEndColorButton);

    m_gradientAngleSpinBox = new QSpinBox(scrollContent);
    m_gradientAngleSpinBox->setRange(0, 359);
    m_gradientAngleSpinBox->setValue(135);
    m_gradientAngleSpinBox->setSuffix(QString::fromUtf8("\u00B0"));  // degree symbol
    connect(m_gradientAngleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeEditorDialog::updatePreview);
    installWheelFilter(m_gradientAngleSpinBox);
    gradientLayout->addRow("Angle:", m_gradientAngleSpinBox);

    m_backgroundStack->addWidget(gradientPage);

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
    m_gradientStartColor = theme.gradientStartColor();
    m_gradientEndColor = theme.gradientEndColor();

    updateColorButton(m_backgroundColorButton, m_backgroundColor);
    updateColorButton(m_textColorButton, m_textColor);
    updateColorButton(m_accentColorButton, m_accentColor);
    updateColorButton(m_gradientStartColorButton, m_gradientStartColor);
    updateColorButton(m_gradientEndColorButton, m_gradientEndColor);

    // Typography
    m_fontFamilyCombo->setCurrentText(theme.fontFamily());
    m_titleFontSizeSpinBox->setValue(theme.titleFontSize());
    m_bodyFontSizeSpinBox->setValue(theme.bodyFontSize());

    // Background type
    int typeIndex = (theme.backgroundType() == Slide::Gradient) ? 1 : 0;
    m_backgroundTypeCombo->setCurrentIndex(typeIndex);
    m_backgroundStack->setCurrentIndex(typeIndex);

    m_gradientAngleSpinBox->setValue(theme.gradientAngle());

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
        theme.setGradientStartColor(m_gradientStartColor);
        theme.setGradientEndColor(m_gradientEndColor);
        theme.setGradientAngle(m_gradientAngleSpinBox->value());
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

void ThemeEditorDialog::onChooseGradientStartColor()
{
    QColor color = QColorDialog::getColor(m_gradientStartColor, this, "Choose Gradient Start Color");
    if (color.isValid()) {
        m_gradientStartColor = color;
        updateColorButton(m_gradientStartColorButton, color);
        updatePreview();
    }
}

void ThemeEditorDialog::onChooseGradientEndColor()
{
    QColor color = QColorDialog::getColor(m_gradientEndColor, this, "Choose Gradient End Color");
    if (color.isValid()) {
        m_gradientEndColor = color;
        updateColorButton(m_gradientEndColorButton, color);
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
    if (typeIndex == 1) {  // Gradient
        // Convert angle to QLinearGradient coordinates
        int angle = m_gradientAngleSpinBox->value();
        double radians = angle * M_PI / 180.0;

        // Calculate gradient start and end points
        double centerX = width / 2.0;
        double centerY = height / 2.0;
        double diagonal = qSqrt(width * width + height * height) / 2.0;

        // Start and end points calculated to match QML's clockwise rotation behavior
        QPointF start(
            centerX + diagonal * qSin(radians),
            centerY - diagonal * qCos(radians)
        );
        QPointF end(
            centerX - diagonal * qSin(radians),
            centerY + diagonal * qCos(radians)
        );

        QLinearGradient gradient(start, end);
        gradient.setColorAt(0, m_gradientStartColor);
        gradient.setColorAt(1, m_gradientEndColor);

        painter.fillRect(pixmap.rect(), gradient);
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
