// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "AppStyle.h"
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QWidget>
#include <QDebug>

namespace Clarity {

AppStyle::ThemeMode AppStyle::s_currentMode = AppStyle::ThemeMode::System;

void AppStyle::apply(QApplication* app, ThemeMode mode)
{
    s_currentMode = mode;
    // Apply palette — System resolves to Dark or Light based on OS preference
    ThemeMode resolved = (mode == ThemeMode::System) ? effectiveMode() : mode;
    switch (resolved) {
    case ThemeMode::Dark:
        app->setPalette(darkPalette());
        break;
    case ThemeMode::Light:
    default:
        app->setPalette(lightPalette());
        break;
    }

    // Apply stylesheet
    app->setStyleSheet(loadStylesheet(mode));

    // Apply platform-appropriate font
    applyFont(app);
}

AppStyle::ThemeMode AppStyle::fromString(const QString& str)
{
    if (str == "dark")  return ThemeMode::Dark;
    if (str == "light") return ThemeMode::Light;
    return ThemeMode::System;
}

QString AppStyle::toString(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Dark:   return "dark";
    case ThemeMode::Light:  return "light";
    case ThemeMode::System: return "system";
    }
    return "system";
}

AppStyle::ThemeMode AppStyle::effectiveMode()
{
    if (s_currentMode != ThemeMode::System)
        return s_currentMode;

    // Ask the OS which color scheme it prefers
    Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    return (scheme == Qt::ColorScheme::Dark) ? ThemeMode::Dark : ThemeMode::Light;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

QPalette AppStyle::darkPalette()
{
    // VS Code / ProPresenter-inspired dark palette
    QPalette p;

    const QColor window      ("#1f1f1f");
    const QColor windowText  ("#d4d4d4");
    const QColor base        ("#181818");
    const QColor altBase     ("#252525");
    const QColor button      ("#313131");
    const QColor buttonText  ("#d4d4d4");
    const QColor highlight   ("#1a85c7");
    const QColor highlightTx ("#ffffff");
    const QColor link        ("#4fc1ff");
    const QColor mid         ("#464646");
    const QColor midlight    ("#3c3c3c");
    const QColor dark        ("#141414");
    const QColor shadow      ("#000000");
    const QColor disabledTx  ("#6e6e6e");
    const QColor placeholder ("#6e6e6e");

    p.setColor(QPalette::Window,          window);
    p.setColor(QPalette::WindowText,      windowText);
    p.setColor(QPalette::Base,            base);
    p.setColor(QPalette::AlternateBase,   altBase);
    p.setColor(QPalette::Text,            windowText);
    p.setColor(QPalette::Button,          button);
    p.setColor(QPalette::ButtonText,      buttonText);
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       highlight);
    p.setColor(QPalette::HighlightedText, highlightTx);
    p.setColor(QPalette::Link,            link);
    p.setColor(QPalette::LinkVisited,     QColor("#9d91d4"));
    p.setColor(QPalette::Mid,             mid);
    p.setColor(QPalette::Midlight,        midlight);
    p.setColor(QPalette::Dark,            dark);
    p.setColor(QPalette::Shadow,          shadow);
    p.setColor(QPalette::ToolTipBase,     QColor("#2a2a2a"));
    p.setColor(QPalette::ToolTipText,     QColor("#d4d4d4"));
    p.setColor(QPalette::PlaceholderText, placeholder);

    // Disabled state
    p.setColor(QPalette::Disabled, QPalette::WindowText,  disabledTx);
    p.setColor(QPalette::Disabled, QPalette::ButtonText,  disabledTx);
    p.setColor(QPalette::Disabled, QPalette::Text,        disabledTx);
    p.setColor(QPalette::Disabled, QPalette::Highlight,   QColor("#264f78"));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledTx);

    return p;
}

QPalette AppStyle::lightPalette()
{
    // Clean light palette — Fusion defaults with slightly warmer midtones
    QPalette p;

    const QColor window      ("#f5f5f5");
    const QColor windowText  ("#1a1a1a");
    const QColor base        ("#ffffff");
    const QColor altBase     ("#f0f0f0");
    const QColor button      ("#e8e8e8");
    const QColor buttonText  ("#1a1a1a");
    const QColor highlight   ("#0e639c");
    const QColor highlightTx ("#ffffff");
    const QColor link        ("#0066cc");
    const QColor mid         ("#b8b8b8");
    const QColor midlight    ("#d8d8d8");
    const QColor dark        ("#9a9a9a");
    const QColor shadow      ("#787878");
    const QColor disabledTx  ("#9a9a9a");

    p.setColor(QPalette::Window,          window);
    p.setColor(QPalette::WindowText,      windowText);
    p.setColor(QPalette::Base,            base);
    p.setColor(QPalette::AlternateBase,   altBase);
    p.setColor(QPalette::Text,            windowText);
    p.setColor(QPalette::Button,          button);
    p.setColor(QPalette::ButtonText,      buttonText);
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       highlight);
    p.setColor(QPalette::HighlightedText, highlightTx);
    p.setColor(QPalette::Link,            link);
    p.setColor(QPalette::LinkVisited,     QColor("#5500aa"));
    p.setColor(QPalette::Mid,             mid);
    p.setColor(QPalette::Midlight,        midlight);
    p.setColor(QPalette::Dark,            dark);
    p.setColor(QPalette::Shadow,          shadow);
    p.setColor(QPalette::ToolTipBase,     QColor("#ffffcc"));
    p.setColor(QPalette::ToolTipText,     QColor("#1a1a1a"));
    p.setColor(QPalette::PlaceholderText, QColor("#9a9a9a"));

    p.setColor(QPalette::Disabled, QPalette::WindowText,  disabledTx);
    p.setColor(QPalette::Disabled, QPalette::ButtonText,  disabledTx);
    p.setColor(QPalette::Disabled, QPalette::Text,        disabledTx);
    p.setColor(QPalette::Disabled, QPalette::Highlight,   QColor("#99c2e0"));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor("#ffffff"));

    return p;
}

QString AppStyle::loadStylesheet(ThemeMode mode)
{
    // Resolve System to the actual OS preference before picking a stylesheet
    ThemeMode resolved = (mode == ThemeMode::System) ? effectiveMode() : mode;

    QString path;
    switch (resolved) {
    case ThemeMode::Dark:   path = ":/styles/dark.qss";  break;
    case ThemeMode::Light:
    default:                path = ":/styles/light.qss"; break;
    }

    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "AppStyle: Could not load stylesheet" << path;
        return {};
    }
    return QString::fromUtf8(f.readAll());
}

void AppStyle::applyFont(QApplication* app)
{
    QFont font;

#if defined(Q_OS_WIN)
    font = QFont("Segoe UI", 10);
#elif defined(Q_OS_MACOS)
    font = QFont("SF Pro Text", 13);
#else
    // Linux / other: use system default but ensure point size is set
    font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    if (font.pointSize() <= 0) {
        font.setPointSize(10);
    }
#endif

    app->setFont(font);
}

QIcon AppStyle::themedIcon(const QString& svgPath)
{
    // Determine icon color based on effective theme (System resolves to Dark/Light)
    QColor color = (effectiveMode() == ThemeMode::Dark)
                       ? QColor("#d4d4d4")   // light gray on dark backgrounds
                       : QColor("#333333");  // dark gray on light backgrounds

    QFile f(svgPath);
    if (!f.open(QFile::ReadOnly))
        return QIcon(svgPath);

    QByteArray data = f.readAll();
    data.replace("currentColor", color.name().toUtf8());

    QSvgRenderer renderer(data);
    if (!renderer.isValid())
        return QIcon(svgPath);

    // Render each logical size at 1× and 2× so icons are sharp at every DPI.
    // Without 2× variants Qt upscales the nearest 1× pixmap, which blurs icons
    // on HiDPI screens. Without 40/48 the toolbar's 40 px icons upscale from 32 px.
    QIcon icon;
    for (int s : {16, 24, 32, 40, 48}) {
        for (int dpr : {1, 2}) {
            QPixmap pm(s * dpr, s * dpr);
            pm.fill(Qt::transparent);
            QPainter painter(&pm);
            renderer.render(&painter);
            painter.end();
            pm.setDevicePixelRatio(dpr);
            icon.addPixmap(pm);
        }
    }
    return icon;
}

void AppStyle::installScrollFix(QApplication* /*app*/)
{
    // Wheel-event forwarding is handled by WheelEventFilter installed
    // per-widget inside each dialog. No global filter is needed.
}

} // namespace Clarity
