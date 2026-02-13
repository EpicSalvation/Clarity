#pragma once

#include "Core/Slide.h"
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QList>

namespace Clarity {

class SettingsManager;

/**
 * @brief Reusable gradient editor widget for multi-stop and radial gradients
 *
 * Provides UI for:
 * - Gradient type selection (linear/radial)
 * - 2-8 color stops with position and color
 * - Angle control for linear gradients
 * - Center X/Y and radius controls for radial gradients
 */
class GradientEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit GradientEditorWidget(SettingsManager* settings, QWidget* parent = nullptr);

    // Get/set gradient data
    QList<GradientStop> gradientStops() const;
    void setGradientStops(const QList<GradientStop>& stops);

    GradientType gradientType() const;
    void setGradientType(GradientType type);

    int gradientAngle() const;
    void setGradientAngle(int angle);

    double radialCenterX() const;
    void setRadialCenterX(double x);

    double radialCenterY() const;
    void setRadialCenterY(double y);

    double radialRadius() const;
    void setRadialRadius(double r);

signals:
    void gradientChanged();

private slots:
    void onTypeChanged(int index);
    void onAddStop();
    void onRemoveStop(int index);
    void onStopColorClicked(int index);
    void onStopPositionChanged(int index, int value);

private:
    void rebuildStopRows();
    void updateAddRemoveState();
    void updateTypeControls();
    void installWheelFilter(QWidget* widget);
    void updateColorButton(QPushButton* button, const QColor& color);

    SettingsManager* m_settings;

    QComboBox* m_typeCombo;
    QVBoxLayout* m_stopsLayout;
    QPushButton* m_addStopButton;

    // Linear-specific
    QWidget* m_linearControls;
    QSpinBox* m_angleSpinBox;

    // Radial-specific
    QWidget* m_radialControls;
    QSpinBox* m_centerXSpinBox;
    QSpinBox* m_centerYSpinBox;
    QSpinBox* m_radiusSpinBox;

    // Current stop data
    QList<GradientStop> m_stops;

    // Per-stop UI elements
    struct StopRow {
        QWidget* widget;
        QSpinBox* positionSpin;
        QPushButton* colorButton;
        QPushButton* removeButton;
    };
    QList<StopRow> m_stopRows;
};

} // namespace Clarity
