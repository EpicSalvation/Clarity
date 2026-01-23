#include "OutputMain.h"
#include "OutputDisplay.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace Clarity {

int OutputMain::run(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Clarity Output");
    app.setApplicationVersion("1.0.0");

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

    return app.exec();
}

} // namespace Clarity
