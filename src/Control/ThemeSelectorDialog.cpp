#include "ThemeSelectorDialog.h"
#include "ThemeEditorDialog.h"
#include "Core/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QScrollArea>
#include <QtMath>

namespace Clarity {

ThemeSelectorDialog::ThemeSelectorDialog(ThemeManager* themeManager, SettingsManager* settings, QWidget* parent)
    : QDialog(parent)
    , m_themeManager(themeManager)
    , m_settings(settings)
    , m_themeList(nullptr)
    , m_previewLabel(nullptr)
    , m_descriptionLabel(nullptr)
    , m_newButton(nullptr)
    , m_editButton(nullptr)
    , m_deleteButton(nullptr)
    , m_duplicateButton(nullptr)
    , m_applyToAllCheck(nullptr)
    , m_applyButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    populateThemeList();
    updateButtons();
}

void ThemeSelectorDialog::setupUI()
{
    setWindowTitle(tr("Select Theme"));
    setMinimumSize(600, 500);

    // Main vertical layout for the dialog
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Scroll area for the content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // Container widget for scroll area
    QWidget* scrollContent = new QWidget();
    QHBoxLayout* contentLayout = new QHBoxLayout(scrollContent);

    // Left side: theme list and management buttons
    QVBoxLayout* leftLayout = new QVBoxLayout();

    // Theme list
    QGroupBox* listGroup = new QGroupBox("Available Themes", this);
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    m_themeList = new QListWidget(this);
    m_themeList->setMinimumWidth(200);
    m_themeList->setMinimumHeight(200);
    connect(m_themeList, &QListWidget::currentItemChanged, this, &ThemeSelectorDialog::onThemeSelected);
    connect(m_themeList, &QListWidget::itemDoubleClicked, this, &ThemeSelectorDialog::onThemeDoubleClicked);
    listLayout->addWidget(m_themeList);

    leftLayout->addWidget(listGroup, 1);  // Give list stretch priority

    // Theme management buttons
    QHBoxLayout* managementLayout = new QHBoxLayout();

    m_newButton = new QPushButton("New", this);
    connect(m_newButton, &QPushButton::clicked, this, &ThemeSelectorDialog::onNewTheme);
    managementLayout->addWidget(m_newButton);

    m_editButton = new QPushButton("Edit", this);
    connect(m_editButton, &QPushButton::clicked, this, &ThemeSelectorDialog::onEditTheme);
    managementLayout->addWidget(m_editButton);

    m_duplicateButton = new QPushButton("Duplicate", this);
    connect(m_duplicateButton, &QPushButton::clicked, this, &ThemeSelectorDialog::onDuplicateTheme);
    managementLayout->addWidget(m_duplicateButton);

    m_deleteButton = new QPushButton("Delete", this);
    connect(m_deleteButton, &QPushButton::clicked, this, &ThemeSelectorDialog::onDeleteTheme);
    managementLayout->addWidget(m_deleteButton);

    leftLayout->addLayout(managementLayout);

    contentLayout->addLayout(leftLayout, 1);

    // Right side: preview and description
    QVBoxLayout* rightLayout = new QVBoxLayout();

    // Preview
    QGroupBox* previewGroup = new QGroupBox("Preview", this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumSize(300, 200);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    previewLayout->addWidget(m_previewLabel);

    rightLayout->addWidget(previewGroup);

    // Description
    QGroupBox* descriptionGroup = new QGroupBox("Description", this);
    QVBoxLayout* descriptionLayout = new QVBoxLayout(descriptionGroup);

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_descriptionLabel->setMinimumHeight(60);
    descriptionLayout->addWidget(m_descriptionLabel);

    rightLayout->addWidget(descriptionGroup);

    // Apply options
    m_applyToAllCheck = new QCheckBox("Apply to all slides in presentation", this);
    rightLayout->addWidget(m_applyToAllCheck);

    rightLayout->addStretch();

    contentLayout->addLayout(rightLayout, 1);

    // Set scroll area widget
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Dialog buttons at the bottom (outside scroll area)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_applyButton = new QPushButton("Apply", this);
    m_applyButton->setDefault(true);
    connect(m_applyButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_applyButton);

    mainLayout->addLayout(buttonLayout);
}

void ThemeSelectorDialog::populateThemeList()
{
    m_themeList->clear();

    // Add built-in themes section
    QListWidgetItem* builtInHeader = new QListWidgetItem("--- Built-in Themes ---");
    builtInHeader->setFlags(Qt::NoItemFlags);
    builtInHeader->setForeground(Qt::gray);
    m_themeList->addItem(builtInHeader);

    for (const Theme& theme : m_themeManager->builtInThemes()) {
        QListWidgetItem* item = new QListWidgetItem(theme.name());
        item->setData(Qt::UserRole, theme.name());
        item->setData(Qt::UserRole + 1, true);  // isBuiltIn
        m_themeList->addItem(item);
    }

    // Add custom themes section if any exist
    QList<Theme> customThemes = m_themeManager->customThemes();
    if (!customThemes.isEmpty()) {
        QListWidgetItem* customHeader = new QListWidgetItem("--- Custom Themes ---");
        customHeader->setFlags(Qt::NoItemFlags);
        customHeader->setForeground(Qt::gray);
        m_themeList->addItem(customHeader);

        for (const Theme& theme : customThemes) {
            QListWidgetItem* item = new QListWidgetItem(theme.name());
            item->setData(Qt::UserRole, theme.name());
            item->setData(Qt::UserRole + 1, false);  // isBuiltIn
            m_themeList->addItem(item);
        }
    }

    // Select the first selectable item
    for (int i = 0; i < m_themeList->count(); i++) {
        QListWidgetItem* item = m_themeList->item(i);
        if (item->flags() & Qt::ItemIsSelectable) {
            m_themeList->setCurrentItem(item);
            break;
        }
    }
}

void ThemeSelectorDialog::onThemeSelected(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        m_previewLabel->clear();
        m_descriptionLabel->clear();
        updateButtons();
        return;
    }

    QString themeName = current->data(Qt::UserRole).toString();
    Theme theme = m_themeManager->getTheme(themeName);

    updatePreview(theme);
    m_descriptionLabel->setText(theme.description().isEmpty() ? "(No description)" : theme.description());
    updateButtons();
}

void ThemeSelectorDialog::onThemeDoubleClicked(QListWidgetItem* item)
{
    if (!item || !(item->flags() & Qt::ItemIsSelectable)) {
        return;
    }

    // Double-click applies the theme
    accept();
}

void ThemeSelectorDialog::updatePreview(const Theme& theme)
{
    int width = m_previewLabel->width() - 4;
    int height = m_previewLabel->height() - 4;
    if (width < 10) width = 300;
    if (height < 10) height = 200;

    QPixmap pixmap(width, height);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    if (theme.backgroundType() == Slide::Gradient) {
        QList<GradientStop> stops = theme.gradientStops();

        if (theme.gradientType() == RadialGradient) {
            double cx = theme.radialCenterX() * width;
            double cy = theme.radialCenterY() * height;
            double r = theme.radialRadius() * qMax(width, height);
            QRadialGradient gradient(QPointF(cx, cy), r);
            for (const auto& stop : stops)
                gradient.setColorAt(stop.position, stop.color);
            painter.fillRect(pixmap.rect(), gradient);
        } else {
            int angle = theme.gradientAngle();
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
    } else {
        painter.fillRect(pixmap.rect(), theme.backgroundColor());
    }

    // Draw sample text
    QFont font(theme.fontFamily());
    int previewFontSize = qMax(14, theme.bodyFontSize() / 3);
    font.setPixelSize(previewFontSize);

    painter.setFont(font);
    painter.setPen(theme.textColor());
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "Amazing Grace\nHow Sweet the Sound");

    // Draw theme name at bottom
    QFont smallFont(theme.fontFamily());
    smallFont.setPixelSize(10);
    painter.setFont(smallFont);
    painter.drawText(pixmap.rect().adjusted(5, 0, -5, -5), Qt::AlignBottom | Qt::AlignRight, theme.name());

    painter.end();
    m_previewLabel->setPixmap(pixmap);
}

void ThemeSelectorDialog::updateButtons()
{
    QListWidgetItem* current = m_themeList->currentItem();
    bool hasSelection = current && (current->flags() & Qt::ItemIsSelectable);
    bool isBuiltIn = hasSelection ? current->data(Qt::UserRole + 1).toBool() : false;

    m_applyButton->setEnabled(hasSelection);
    m_editButton->setEnabled(hasSelection && !isBuiltIn);
    m_deleteButton->setEnabled(hasSelection && !isBuiltIn);
    m_duplicateButton->setEnabled(hasSelection);
}

void ThemeSelectorDialog::onNewTheme()
{
    ThemeEditorDialog dialog(m_settings, this);

    if (dialog.exec() == QDialog::Accepted) {
        Theme newTheme = dialog.theme();

        // Check for duplicate name
        if (m_themeManager->hasTheme(newTheme.name())) {
            QMessageBox::warning(this, "Duplicate Name",
                "A theme with this name already exists. Please choose a different name.");
            return;
        }

        m_themeManager->addTheme(newTheme);
        m_themeManager->saveCustomThemes();
        populateThemeList();

        // Select the new theme
        for (int i = 0; i < m_themeList->count(); i++) {
            QListWidgetItem* item = m_themeList->item(i);
            if (item->data(Qt::UserRole).toString() == newTheme.name()) {
                m_themeList->setCurrentItem(item);
                break;
            }
        }
    }
}

void ThemeSelectorDialog::onEditTheme()
{
    QListWidgetItem* current = m_themeList->currentItem();
    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        return;
    }

    bool isBuiltIn = current->data(Qt::UserRole + 1).toBool();
    if (isBuiltIn) {
        QMessageBox::information(this, "Cannot Edit",
            "Built-in themes cannot be edited. Use 'Duplicate' to create an editable copy.");
        return;
    }

    QString themeName = current->data(Qt::UserRole).toString();
    Theme theme = m_themeManager->getTheme(themeName);

    ThemeEditorDialog dialog(m_settings, theme, this);

    if (dialog.exec() == QDialog::Accepted) {
        Theme updatedTheme = dialog.theme();

        // If name changed, check for duplicates
        if (updatedTheme.name() != themeName && m_themeManager->hasTheme(updatedTheme.name())) {
            QMessageBox::warning(this, "Duplicate Name",
                "A theme with this name already exists. Please choose a different name.");
            return;
        }

        // If name changed, remove old and add new
        if (updatedTheme.name() != themeName) {
            m_themeManager->removeTheme(themeName);
            m_themeManager->addTheme(updatedTheme);
        } else {
            m_themeManager->updateTheme(themeName, updatedTheme);
        }

        m_themeManager->saveCustomThemes();
        populateThemeList();

        // Select the updated theme
        for (int i = 0; i < m_themeList->count(); i++) {
            QListWidgetItem* item = m_themeList->item(i);
            if (item->data(Qt::UserRole).toString() == updatedTheme.name()) {
                m_themeList->setCurrentItem(item);
                break;
            }
        }
    }
}

void ThemeSelectorDialog::onDeleteTheme()
{
    QListWidgetItem* current = m_themeList->currentItem();
    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        return;
    }

    bool isBuiltIn = current->data(Qt::UserRole + 1).toBool();
    if (isBuiltIn) {
        QMessageBox::information(this, "Cannot Delete", "Built-in themes cannot be deleted.");
        return;
    }

    QString themeName = current->data(Qt::UserRole).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Theme",
        QString("Are you sure you want to delete the theme '%1'?").arg(themeName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_themeManager->removeTheme(themeName);
        m_themeManager->saveCustomThemes();
        populateThemeList();
    }
}

void ThemeSelectorDialog::onDuplicateTheme()
{
    QListWidgetItem* current = m_themeList->currentItem();
    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        return;
    }

    QString themeName = current->data(Qt::UserRole).toString();
    Theme originalTheme = m_themeManager->getTheme(themeName);

    // Create a copy with a new name
    Theme duplicateTheme = originalTheme;
    duplicateTheme.setBuiltIn(false);

    // Generate a unique name
    QString baseName = originalTheme.name() + " Copy";
    QString newName = baseName;
    int counter = 1;
    while (m_themeManager->hasTheme(newName)) {
        counter++;
        newName = QString("%1 %2").arg(baseName).arg(counter);
    }
    duplicateTheme.setName(newName);

    // Open editor for the duplicate
    ThemeEditorDialog dialog(m_settings, duplicateTheme, this);

    if (dialog.exec() == QDialog::Accepted) {
        Theme finalTheme = dialog.theme();

        // Check for duplicate name
        if (m_themeManager->hasTheme(finalTheme.name())) {
            QMessageBox::warning(this, "Duplicate Name",
                "A theme with this name already exists. Please choose a different name.");
            return;
        }

        m_themeManager->addTheme(finalTheme);
        m_themeManager->saveCustomThemes();
        populateThemeList();

        // Select the new theme
        for (int i = 0; i < m_themeList->count(); i++) {
            QListWidgetItem* item = m_themeList->item(i);
            if (item->data(Qt::UserRole).toString() == finalTheme.name()) {
                m_themeList->setCurrentItem(item);
                break;
            }
        }
    }
}

Theme ThemeSelectorDialog::selectedTheme() const
{
    QListWidgetItem* current = m_themeList->currentItem();
    if (!current || !(current->flags() & Qt::ItemIsSelectable)) {
        return Theme();
    }

    QString themeName = current->data(Qt::UserRole).toString();
    return m_themeManager->getTheme(themeName);
}

bool ThemeSelectorDialog::hasSelection() const
{
    QListWidgetItem* current = m_themeList->currentItem();
    return current && (current->flags() & Qt::ItemIsSelectable);
}

bool ThemeSelectorDialog::applyToAllSlides() const
{
    return m_applyToAllCheck->isChecked();
}

} // namespace Clarity
