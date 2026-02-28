// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "GradientEditorWidget.h"
#include "Core/SettingsManager.h"
#include "Core/WheelEventFilter.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QColorDialog>

namespace Clarity {

GradientEditorWidget::GradientEditorWidget(SettingsManager* settings, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Gradient type selector
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:", this));
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Linear", static_cast<int>(LinearGradient));
    m_typeCombo->addItem("Radial", static_cast<int>(RadialGradient));
    installWheelFilter(m_typeCombo);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GradientEditorWidget::onTypeChanged);
    typeLayout->addWidget(m_typeCombo);
    typeLayout->addStretch();
    mainLayout->addLayout(typeLayout);

    // Stop rows container
    m_stopsLayout = new QVBoxLayout();
    mainLayout->addLayout(m_stopsLayout);

    // Add stop button
    m_addStopButton = new QPushButton("+ Add Stop", this);
    connect(m_addStopButton, &QPushButton::clicked, this, &GradientEditorWidget::onAddStop);
    mainLayout->addWidget(m_addStopButton);

    // Linear controls (angle)
    m_linearControls = new QWidget(this);
    QFormLayout* linearLayout = new QFormLayout(m_linearControls);
    linearLayout->setContentsMargins(0, 4, 0, 0);
    m_angleSpinBox = new QSpinBox(this);
    m_angleSpinBox->setRange(0, 359);
    m_angleSpinBox->setValue(135);
    m_angleSpinBox->setSuffix(QString::fromUtf8("\u00B0"));
    m_angleSpinBox->setToolTip("0\u00B0 = top to bottom, 90\u00B0 = left to right");
    installWheelFilter(m_angleSpinBox);
    connect(m_angleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit gradientChanged(); });
    linearLayout->addRow("Angle:", m_angleSpinBox);
    mainLayout->addWidget(m_linearControls);

    // Radial controls (center X, Y, radius)
    m_radialControls = new QWidget(this);
    QFormLayout* radialLayout = new QFormLayout(m_radialControls);
    radialLayout->setContentsMargins(0, 4, 0, 0);

    m_centerXSpinBox = new QSpinBox(this);
    m_centerXSpinBox->setRange(0, 100);
    m_centerXSpinBox->setValue(50);
    m_centerXSpinBox->setSuffix("%");
    installWheelFilter(m_centerXSpinBox);
    connect(m_centerXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit gradientChanged(); });
    radialLayout->addRow("Center X:", m_centerXSpinBox);

    m_centerYSpinBox = new QSpinBox(this);
    m_centerYSpinBox->setRange(0, 100);
    m_centerYSpinBox->setValue(50);
    m_centerYSpinBox->setSuffix("%");
    installWheelFilter(m_centerYSpinBox);
    connect(m_centerYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit gradientChanged(); });
    radialLayout->addRow("Center Y:", m_centerYSpinBox);

    m_radiusSpinBox = new QSpinBox(this);
    m_radiusSpinBox->setRange(1, 100);
    m_radiusSpinBox->setValue(50);
    m_radiusSpinBox->setSuffix("%");
    installWheelFilter(m_radiusSpinBox);
    connect(m_radiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit gradientChanged(); });
    radialLayout->addRow("Radius:", m_radiusSpinBox);

    mainLayout->addWidget(m_radialControls);

    // Initialize with default 2-stop gradient
    m_stops = {GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))};
    rebuildStopRows();
    updateTypeControls();
}

QList<GradientStop> GradientEditorWidget::gradientStops() const
{
    return m_stops;
}

void GradientEditorWidget::setGradientStops(const QList<GradientStop>& stops)
{
    m_stops = stops;
    if (m_stops.size() < 2) {
        m_stops = {GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))};
    }
    rebuildStopRows();
}

GradientType GradientEditorWidget::gradientType() const
{
    return static_cast<GradientType>(m_typeCombo->currentData().toInt());
}

void GradientEditorWidget::setGradientType(GradientType type)
{
    int index = m_typeCombo->findData(static_cast<int>(type));
    if (index >= 0) m_typeCombo->setCurrentIndex(index);
    updateTypeControls();
}

int GradientEditorWidget::gradientAngle() const
{
    return m_angleSpinBox->value();
}

void GradientEditorWidget::setGradientAngle(int angle)
{
    m_angleSpinBox->setValue(angle);
}

double GradientEditorWidget::radialCenterX() const
{
    return m_centerXSpinBox->value() / 100.0;
}

void GradientEditorWidget::setRadialCenterX(double x)
{
    m_centerXSpinBox->setValue(qRound(x * 100));
}

double GradientEditorWidget::radialCenterY() const
{
    return m_centerYSpinBox->value() / 100.0;
}

void GradientEditorWidget::setRadialCenterY(double y)
{
    m_centerYSpinBox->setValue(qRound(y * 100));
}

double GradientEditorWidget::radialRadius() const
{
    return m_radiusSpinBox->value() / 100.0;
}

void GradientEditorWidget::setRadialRadius(double r)
{
    m_radiusSpinBox->setValue(qRound(r * 100));
}

void GradientEditorWidget::onTypeChanged(int index)
{
    Q_UNUSED(index)
    updateTypeControls();
    emit gradientChanged();
}

void GradientEditorWidget::onAddStop()
{
    if (m_stops.size() >= 8) return;

    // Insert a new stop at the midpoint of the widest gap
    double maxGap = 0;
    int gapIndex = 0;
    for (int i = 0; i < m_stops.size() - 1; ++i) {
        double gap = m_stops[i + 1].position - m_stops[i].position;
        if (gap > maxGap) {
            maxGap = gap;
            gapIndex = i;
        }
    }

    double newPos = (m_stops[gapIndex].position + m_stops[gapIndex + 1].position) / 2.0;
    // Blend the two neighboring colors
    QColor c1 = m_stops[gapIndex].color;
    QColor c2 = m_stops[gapIndex + 1].color;
    QColor blended(
        (c1.red() + c2.red()) / 2,
        (c1.green() + c2.green()) / 2,
        (c1.blue() + c2.blue()) / 2,
        (c1.alpha() + c2.alpha()) / 2
    );

    m_stops.insert(gapIndex + 1, GradientStop(newPos, blended));
    rebuildStopRows();
    emit gradientChanged();
}

void GradientEditorWidget::onRemoveStop(int index)
{
    if (m_stops.size() <= 2 || index < 0 || index >= m_stops.size()) return;
    m_stops.removeAt(index);
    rebuildStopRows();
    emit gradientChanged();
}

void GradientEditorWidget::onStopColorClicked(int index)
{
    if (index < 0 || index >= m_stops.size()) return;

    QColor color = QColorDialog::getColor(m_stops[index].color, this, "Choose Stop Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_stops[index].color = color;
        updateColorButton(m_stopRows[index].colorButton, color);
        emit gradientChanged();
    }
}

void GradientEditorWidget::onStopPositionChanged(int index, int value)
{
    if (index < 0 || index >= m_stops.size()) return;
    m_stops[index].position = value / 100.0;
    emit gradientChanged();
}

void GradientEditorWidget::rebuildStopRows()
{
    // Clear existing rows — must remove from layout and delete immediately
    // (deleteLater leaves stale widgets visible until next event loop)
    for (auto& row : m_stopRows) {
        m_stopsLayout->removeWidget(row.widget);
        delete row.widget;
    }
    m_stopRows.clear();

    // Create a row for each stop
    for (int i = 0; i < m_stops.size(); ++i) {
        StopRow row;
        row.widget = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(row.widget);
        rowLayout->setContentsMargins(0, 1, 0, 1);

        QLabel* label = new QLabel(QString("Stop %1:").arg(i + 1), row.widget);
        label->setFixedWidth(50);
        rowLayout->addWidget(label);

        row.positionSpin = new QSpinBox(row.widget);
        row.positionSpin->setRange(0, 100);
        row.positionSpin->setValue(qRound(m_stops[i].position * 100));
        row.positionSpin->setSuffix("%");
        row.positionSpin->setMinimumWidth(90);
        installWheelFilter(row.positionSpin);
        int idx = i;
        connect(row.positionSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this, idx](int val) { onStopPositionChanged(idx, val); });
        rowLayout->addWidget(row.positionSpin);

        row.colorButton = new QPushButton(row.widget);
        row.colorButton->setMinimumWidth(100);
        updateColorButton(row.colorButton, m_stops[i].color);
        connect(row.colorButton, &QPushButton::clicked,
                this, [this, idx]() { onStopColorClicked(idx); });
        rowLayout->addWidget(row.colorButton);

        row.removeButton = new QPushButton("Remove", row.widget);
        row.removeButton->setFixedWidth(60);
        connect(row.removeButton, &QPushButton::clicked,
                this, [this, idx]() { onRemoveStop(idx); });
        rowLayout->addWidget(row.removeButton);

        rowLayout->addStretch();

        m_stopsLayout->addWidget(row.widget);
        m_stopRows.append(row);
    }

    updateAddRemoveState();
}

void GradientEditorWidget::updateAddRemoveState()
{
    m_addStopButton->setEnabled(m_stops.size() < 8);
    for (int i = 0; i < m_stopRows.size(); ++i) {
        m_stopRows[i].removeButton->setEnabled(m_stops.size() > 2);
    }
}

void GradientEditorWidget::updateTypeControls()
{
    bool isRadial = (gradientType() == RadialGradient);
    m_linearControls->setVisible(!isRadial);
    m_radialControls->setVisible(isRadial);
}

void GradientEditorWidget::installWheelFilter(QWidget* widget)
{
    if (m_settings) {
        widget->installEventFilter(new WheelEventFilter(m_settings, widget));
        widget->setFocusPolicy(Qt::StrongFocus);
    }
}

void GradientEditorWidget::updateColorButton(QPushButton* button, const QColor& color)
{
    QString textColor = (color.lightness() > 128) ? "#000000" : "#ffffff";
    button->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border: 1px solid #999; padding: 3px; }")
            .arg(color.name(), textColor)
    );
    button->setText(color.name().toUpper());
}

} // namespace Clarity
