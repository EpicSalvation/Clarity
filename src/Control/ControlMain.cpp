// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "ControlMain.h"
#include "ControlWindow.h"
#include "AppStyle.h"
#include "Core/SettingsManager.h"
#include "Core/Version.h"
#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>

namespace Clarity {

int ControlMain::run(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Clarity");
    app.setApplicationVersion(CLARITY_VERSION);
    app.setOrganizationName("Clarity");
    app.setWindowIcon(QIcon(":/icons/clarity.ico"));

    // Use Qt Fusion style as the base — it renders consistently across all
    // platforms and is designed to work with custom QPalettes.
    app.setStyle(QStyleFactory::create("Fusion"));

    // Load translations
    QTranslator qtTranslator;
    QTranslator appTranslator;

    // Get language setting (need a temporary settings manager to read the setting)
    SettingsManager tempSettings;
    QString languageCode = tempSettings.language();

    // Determine which locale to use
    QLocale locale;
    if (languageCode == "system") {
        locale = QLocale::system();
        qDebug() << "Using system language:" << locale.name();
    } else {
        locale = QLocale(languageCode);
        qDebug() << "Using configured language:" << languageCode;
    }

    // Load Qt's built-in translations (for standard dialogs, buttons, etc.)
    if (qtTranslator.load(locale, "qtbase", "_",
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
        qDebug() << "Loaded Qt translations for" << locale.name();
    }

    // Load Clarity's translations
    // Try multiple locations: embedded resources, then filesystem paths
    QString langCode = locale.name().left(2);  // "es", "de", "fr", etc.
    QString translationFile = QString("clarity_%1").arg(langCode);

    bool loaded = false;

    // Qt's qt_add_translations embeds at :/i18n/ prefix
    if (appTranslator.load(translationFile, ":/i18n")) {
        app.installTranslator(&appTranslator);
        qDebug() << "Loaded Clarity translations from embedded resources:" << translationFile;
        loaded = true;
    }
    // Try translations folder next to executable
    else if (appTranslator.load(translationFile, QApplication::applicationDirPath() + "/translations")) {
        app.installTranslator(&appTranslator);
        qDebug() << "Loaded Clarity translations from app directory:" << translationFile;
        loaded = true;
    }
    // Try relative to current working directory
    else if (appTranslator.load(translationFile, "translations")) {
        app.installTranslator(&appTranslator);
        qDebug() << "Loaded Clarity translations from CWD:" << translationFile;
        loaded = true;
    }
    // Try loading .qm file directly with full path
    else if (appTranslator.load(QApplication::applicationDirPath() + "/translations/" + translationFile + ".qm")) {
        app.installTranslator(&appTranslator);
        qDebug() << "Loaded Clarity translations (direct path):" << translationFile;
        loaded = true;
    }

    if (!loaded) {
        qDebug() << "No translations found for" << translationFile << ", using English";
        qDebug() << "Searched: :/i18n," << QApplication::applicationDirPath() + "/translations";
    }

    // Prevent spinboxes and combo boxes from consuming scroll-wheel events when
    // they don't have keyboard focus (which would stop the parent scroll area).
    AppStyle::installScrollFix(&app);

    // Apply the saved visual theme (dark / light / system) before any windows open.
    // We use a temporary SettingsManager here; the real one is created inside
    // ControlWindow but shares the same QSettings storage.
    {
        SettingsManager themeSettings;
        AppStyle::ThemeMode mode = AppStyle::fromString(themeSettings.themeMode());
        AppStyle::apply(&app, mode);
    }

    // When "System Default" is active, re-apply the theme if the OS color scheme
    // changes while the app is running (e.g. user toggles Windows dark mode).
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     &app, [&app]() {
                         if (AppStyle::currentMode() == AppStyle::ThemeMode::System)
                             AppStyle::apply(&app, AppStyle::ThemeMode::System);
                     });

    ControlWindow window;
    window.show();

    return app.exec();
}

} // namespace Clarity
