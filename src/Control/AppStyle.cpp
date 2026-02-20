#include "AppStyle.h"
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>

namespace Clarity {

void AppStyle::apply(QApplication* app, ThemeMode mode)
{
    // Apply palette
    switch (mode) {
    case ThemeMode::Dark:
        app->setPalette(darkPalette());
        break;
    case ThemeMode::Light:
        app->setPalette(lightPalette());
        break;
    case ThemeMode::System:
    default:
        // Reset to Fusion defaults
        app->setPalette(app->style()->standardPalette());
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
    const QColor highlight   ("#0e639c");
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
    QString path;
    switch (mode) {
    case ThemeMode::Dark:   path = ":/styles/dark.qss";  break;
    case ThemeMode::Light:  path = ":/styles/light.qss"; break;
    case ThemeMode::System: path = ":/styles/light.qss"; break;
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

} // namespace Clarity
