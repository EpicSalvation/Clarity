#include "ControlMain.h"
#include "ControlWindow.h"
#include "Core/SettingsManager.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>

namespace Clarity {

int ControlMain::run(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Clarity");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Clarity");

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

    ControlWindow window;
    window.show();

    return app.exec();
}

} // namespace Clarity
