#include "OutputMain.h"
#include "OutputDisplay.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QUrl>

namespace Clarity {

int OutputMain::run(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Clarity Output");
    app.setApplicationVersion("1.0.0");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Clarity Output Display");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add --output option (already processed by Main.cpp, but parser needs to know about it)
    QCommandLineOption outputOption("output", "Run in output display mode");
    parser.addOption(outputOption);

    QCommandLineOption screenOption("screen",
                                     "Screen index to display on",
                                     "index",
                                     "0");
    parser.addOption(screenOption);

    parser.process(app);

    // Get screen index from command line
    int screenIndex = parser.value(screenOption).toInt();

    QQmlApplicationEngine engine;

    // Create display controller
    OutputDisplay displayController;

    // Expose to QML
    engine.rootContext()->setContextProperty("displayController", &displayController);

    // Load QML
    engine.load(QUrl("qrc:/qml/OutputDisplay.qml"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    // Set the window to the specified screen
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screenIndex >= 0 && screenIndex < screens.size()) {
        QWindow* window = qobject_cast<QWindow*>(engine.rootObjects().first());
        if (window) {
            QScreen* targetScreen = screens[screenIndex];
            window->setScreen(targetScreen);
            qDebug() << "OutputMain: Set window to screen" << screenIndex
                     << "(" << targetScreen->name() << ")";
        }
    } else {
        qWarning() << "OutputMain: Invalid screen index" << screenIndex
                   << "- using default screen";
    }

    return app.exec();
}

} // namespace Clarity
