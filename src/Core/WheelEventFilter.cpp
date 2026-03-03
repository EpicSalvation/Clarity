// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "WheelEventFilter.h"
#include "SettingsManager.h"
#include <QAbstractScrollArea>
#include <QApplication>
#include <QScrollBar>
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
    if (event->type() != QEvent::Wheel)
        return QObject::eventFilter(watched, event);

    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget)
        return false;

    // Allow value change only when the setting is enabled AND the widget
    // has keyboard focus (user explicitly chose to interact with it).
    if (m_settings && m_settings->scrollWheelChangesInputs() && widget->hasFocus())
        return false;

    // Block value change — forward to the nearest scroll area's scrollbar
    // so the page continues to scroll normally.
    auto* we = static_cast<QWheelEvent*>(event);
    bool vertical = qAbs(we->angleDelta().y()) >= qAbs(we->angleDelta().x());
    for (QWidget* p = widget->parentWidget(); p; p = p->parentWidget()) {
        if (auto* sa = qobject_cast<QAbstractScrollArea*>(p)) {
            QScrollBar* bar = vertical ? sa->verticalScrollBar()
                                       : sa->horizontalScrollBar();
            if (bar) QApplication::sendEvent(bar, event);
            return true;
        }
    }

    // No scroll area ancestor — just swallow the event.
    return true;
}

} // namespace Clarity
