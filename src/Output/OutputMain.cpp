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
    // Use same org/app name as Control app so QSettings reads the same file
    app.setOrganizationName("Clarity");
    app.setApplicationName("Clarity");
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

    // Load QML (but don't show window yet)
    engine.load(QUrl("qrc:/qml/OutputDisplay.qml"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    // Get the window object
    QWindow* window = qobject_cast<QWindow*>(engine.rootObjects().first());
    if (!window) {
        qWarning() << "OutputMain: Failed to get window object";
        return -1;
    }

    // Set the window to the specified screen BEFORE showing it
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screenIndex >= 0 && screenIndex < screens.size()) {
        QScreen* targetScreen = screens[screenIndex];

        // Move window to target screen first
        window->setScreen(targetScreen);

        // Position window on the target screen
        window->setGeometry(targetScreen->geometry());

        qDebug() << "OutputMain: Set window to screen" << screenIndex
                 << "(" << targetScreen->name() << ")"
                 << "at geometry" << targetScreen->geometry();
    } else {
        qWarning() << "OutputMain: Invalid screen index" << screenIndex
                   << "- using default screen (available screens:" << screens.size() << ")";
    }

    // Now show the window in fullscreen mode
    window->showFullScreen();
    qDebug() << "OutputMain: Window shown in fullscreen on screen" << window->screen()->name();

    return app.exec();
}

} // namespace Clarity
