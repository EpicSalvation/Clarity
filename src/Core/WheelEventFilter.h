#pragma once

#include <QObject>
#include <QEvent>

namespace Clarity {

class SettingsManager;

/**
 * @brief Event filter that blocks wheel events on unfocused widgets
 *
 * When installed on a QComboBox or QSpinBox, this filter prevents the
 * mouse wheel from changing the value unless the widget has focus.
 * This behavior can be toggled via SettingsManager::scrollWheelChangesInputs().
 *
 * Usage:
 *   WheelEventFilter* filter = new WheelEventFilter(settingsManager, this);
 *   myComboBox->installEventFilter(filter);
 *   mySpinBox->installEventFilter(filter);
 */
class WheelEventFilter : public QObject {
    Q_OBJECT

public:
    explicit WheelEventFilter(SettingsManager* settings, QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    SettingsManager* m_settings;
};

} // namespace Clarity
