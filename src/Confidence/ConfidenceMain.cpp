#include "ConfidenceMain.h"
#include "ConfidenceDisplay.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QScreen>
#include <QWindow>
#include <QUrl>
#include <QDebug>

namespace Clarity {

int ConfidenceMain::run(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Clarity Confidence");
    app.setApplicationVersion("1.0.0");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Clarity Confidence Monitor");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add --confidence option (already processed by Main.cpp, but parser needs to know about it)
    QCommandLineOption confidenceOption("confidence", "Run in confidence monitor mode");
    parser.addOption(confidenceOption);

    QCommandLineOption screenOption("screen",
                                     "Screen index to display on",
                                     "index",
                                     "0");
    parser.addOption(screenOption);

    parser.process(app);

    // Get screen index from command line
    int screenIndex = parser.value(screenOption).toInt();

    QQmlApplicationEngine engine;

    // Create confidence display controller
    ConfidenceDisplay confidenceDisplay;

    // Expose to QML
    engine.rootContext()->setContextProperty("confidenceDisplay", &confidenceDisplay);

    // Load QML (but don't show window yet)
    engine.load(QUrl("qrc:/qml/ConfidenceMonitor.qml"));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load ConfidenceMonitor.qml";
        return -1;
    }

    // Get the window object
    QWindow* window = qobject_cast<QWindow*>(engine.rootObjects().first());
    if (!window) {
        qWarning() << "ConfidenceMain: Failed to get window object";
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

        qDebug() << "ConfidenceMain: Set window to screen" << screenIndex
                 << "(" << targetScreen->name() << ")"
                 << "at geometry" << targetScreen->geometry();
    } else {
        qWarning() << "ConfidenceMain: Invalid screen index" << screenIndex
                   << "- using default screen (available screens:" << screens.size() << ")";
    }

    // Now show the window in fullscreen mode
    window->showFullScreen();
    qDebug() << "ConfidenceMain: Window shown in fullscreen on screen" << window->screen()->name();

    return app.exec();
}

} // namespace Clarity
