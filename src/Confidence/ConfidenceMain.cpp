#include "ConfidenceMain.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QDebug>

namespace Clarity {

int ConfidenceMain::run(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Clarity Confidence");
    app.setApplicationVersion("1.0.0");

    QQmlApplicationEngine engine;

    // Load QML
    engine.load(QUrl("qrc:/qml/ConfidenceMonitor.qml"));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load ConfidenceMonitor.qml";
        return -1;
    }

    qDebug() << "Confidence monitor started (stub mode - Phase 1)";

    return app.exec();
}

} // namespace Clarity
