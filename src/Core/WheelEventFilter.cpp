// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "WheelEventFilter.h"
#include "SettingsManager.h"
#include <QWidget>
#include <QWheelEvent>

namespace Clarity {

WheelEventFilter::WheelEventFilter(SettingsManager* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
{
}

bool WheelEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Wheel) {
        // If the setting allows scroll wheel to change inputs, don't filter
        if (m_settings && m_settings->scrollWheelChangesInputs()) {
            return false;  // Allow the event
        }

        // Block wheel events on unfocused widgets
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (widget && !widget->hasFocus()) {
            // Ignore the wheel event - don't change the value
            return true;  // Event handled (blocked)
        }
    }

    // Pass other events through
    return QObject::eventFilter(watched, event);
}

} // namespace Clarity
