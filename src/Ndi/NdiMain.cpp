#include "NdiMain.h"
#include "NdiSender.h"
#include "Output/OutputDisplay.h"

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QQuickRenderTarget>
#include <QSGRendererInterface>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>

// Log handler that writes all Qt debug/warning/critical messages to a file
static QFile* s_logFile = nullptr;
static void ndiLogHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);
    if (!s_logFile) return;
    const char* prefix = "";
    switch (type) {
        case QtDebugMsg:    prefix = "DEBUG"; break;
        case QtInfoMsg:     prefix = "INFO"; break;
        case QtWarningMsg:  prefix = "WARN"; break;
        case QtCriticalMsg: prefix = "CRIT"; break;
        case QtFatalMsg:    prefix = "FATAL"; break;
    }
    QTextStream stream(s_logFile);
    stream << "[" << prefix << "] " << msg << "\n";
    stream.flush();
}

namespace Clarity {

int NdiMain::run(int argc, char* argv[])
{
    // Set up file logging so we can diagnose issues (WIN32 GUI app has no console)
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/clarity-ndi.log";
    QFile logFile(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        s_logFile = &logFile;
        qInstallMessageHandler(ndiLogHandler);
    }

    qDebug() << "NdiMain: Log file:" << logPath;

    // Force OpenGL as the scene graph backend — Qt 6 on Windows defaults to
    // D3D11 which won't work with our OpenGL FBO readback pipeline.
    // Must be called before QGuiApplication is constructed.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Clarity");
    app.setApplicationName("Clarity");
    app.setApplicationVersion("1.0.0");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Clarity NDI Output");
    parser.addHelpOption();
    parser.addVersionOption();

    // Already-processed mode flag
    QCommandLineOption ndiOption("ndi", "Run in NDI streaming mode");
    parser.addOption(ndiOption);

    QCommandLineOption widthOption("width", "Output width in pixels", "pixels", "1920");
    parser.addOption(widthOption);

    QCommandLineOption heightOption("height", "Output height in pixels", "pixels", "1080");
    parser.addOption(heightOption);

    QCommandLineOption fpsOption("fps", "Target frames per second", "fps", "30");
    parser.addOption(fpsOption);

    QCommandLineOption nameOption("ndi-name", "NDI source name", "name", "Clarity Output");
    parser.addOption(nameOption);

    parser.process(app);

    int renderWidth = parser.value(widthOption).toInt();
    int renderHeight = parser.value(heightOption).toInt();
    int targetFps = parser.value(fpsOption).toInt();
    QString ndiName = parser.value(nameOption);

    // Clamp to sane values
    renderWidth = qBound(320, renderWidth, 3840);
    renderHeight = qBound(240, renderHeight, 2160);
    targetFps = qBound(1, targetFps, 60);

    qDebug() << "NdiMain: Starting NDI output -"
             << renderWidth << "x" << renderHeight
             << "@" << targetFps << "fps"
             << "as" << ndiName;

    // --- Initialize NDI sender ---
    NdiSender ndiSender;
    if (!ndiSender.initialize(ndiName)) {
        qCritical() << "NdiMain: Failed to initialize NDI. Exiting.";
        return 1;
    }

    // --- Set up OpenGL offscreen rendering ---
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);

    QOpenGLContext glContext;
    glContext.setFormat(format);
    if (!glContext.create()) {
        qCritical() << "NdiMain: Failed to create OpenGL context";
        return 1;
    }
    qDebug() << "NdiMain: OpenGL context created -"
             << "version:" << glContext.format().majorVersion() << "." << glContext.format().minorVersion()
             << "profile:" << (glContext.format().profile() == QSurfaceFormat::CoreProfile ? "Core" : "Compat");

    QOffscreenSurface surface;
    surface.setFormat(glContext.format());
    surface.create();
    if (!surface.isValid()) {
        qCritical() << "NdiMain: Failed to create offscreen surface";
        return 1;
    }

    // --- Set up QQuickRenderControl pipeline ---
    QQuickRenderControl renderControl;

    // QQuickWindow that renders to the render control (no native window)
    QQuickWindow quickWindow(&renderControl);
    quickWindow.setGeometry(0, 0, renderWidth, renderHeight);
    quickWindow.setColor(Qt::black);

    // Create QML engine and expose the display controller
    // Register as "ndi" client type so the control app can distinguish it from output
    QQmlEngine engine;
    OutputDisplay displayController("ndi");
    engine.rootContext()->setContextProperty("displayController", &displayController);

    // Load the shared Item-rooted QML content
    QQmlComponent component(&engine, QUrl("qrc:/qml/OutputDisplayContent.qml"));
    if (component.isError()) {
        for (const auto& error : component.errors()) {
            qCritical() << "NdiMain: QML error:" << error.toString();
        }
        return 1;
    }

    QQuickItem* rootItem = qobject_cast<QQuickItem*>(component.create());
    if (!rootItem) {
        qCritical() << "NdiMain: Failed to create QML root item";
        if (component.isError()) {
            for (const auto& error : component.errors()) {
                qCritical() << "  " << error.toString();
            }
        }
        return 1;
    }

    // Parent the root item to the QQuickWindow's content item
    rootItem->setParentItem(quickWindow.contentItem());
    rootItem->setWidth(renderWidth);
    rootItem->setHeight(renderHeight);

    // Make the GL context current and initialize render control
    if (!glContext.makeCurrent(&surface)) {
        qCritical() << "NdiMain: Failed to make OpenGL context current";
        return 1;
    }

    // Initialize the render control (sets up the scene graph with the current GL context)
    if (!renderControl.initialize()) {
        qCritical() << "NdiMain: Failed to initialize QQuickRenderControl"
                     << "(scene graph could not be set up)";
        return 1;
    }
    qDebug() << "NdiMain: QQuickRenderControl initialized successfully";

    // Create framebuffer object for rendering
    QOpenGLFramebufferObject fbo(QSize(renderWidth, renderHeight),
                                  QOpenGLFramebufferObject::CombinedDepthStencil);
    if (!fbo.isValid()) {
        qCritical() << "NdiMain: Failed to create OpenGL FBO";
        return 1;
    }

    // Set the FBO texture as the render target for the Quick scene
    quickWindow.setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(
        fbo.texture(), QSize(renderWidth, renderHeight)));

    qDebug() << "NdiMain: Render pipeline ready - FBO texture:" << fbo.texture()
             << "size:" << renderWidth << "x" << renderHeight;

    // Double buffer for frame data: one for readback, one for sending
    QByteArray frameBufferA(renderWidth * renderHeight * 4, 0);
    QByteArray frameBufferB(renderWidth * renderHeight * 4, 0);
    bool useBufferA = true;

    QElapsedTimer perfTimer;
    int frameCount = 0;

    // --- Render loop timer ---
    QTimer renderTimer;
    renderTimer.setTimerType(Qt::PreciseTimer);
    // NDI clock_video=true handles actual rate limiting, so we poll frequently
    // to let the scene graph advance animations, then NDI blocks to maintain rate
    renderTimer.setInterval(1000 / targetFps);

    QObject::connect(&renderTimer, &QTimer::timeout, [&]() {
        if (!perfTimer.isValid()) {
            perfTimer.start();
        }

        if (!glContext.makeCurrent(&surface)) {
            return;
        }

        // Polish, sync, and render the scene
        renderControl.polishItems();
        renderControl.beginFrame();
        renderControl.sync();
        renderControl.render();
        renderControl.endFrame();

        // Readback the rendered frame from the FBO via QOpenGLFunctions
        QOpenGLFunctions* gl = glContext.functions();
        fbo.bind();
        QByteArray& currentBuffer = useBufferA ? frameBufferA : frameBufferB;
        gl->glReadPixels(0, 0, renderWidth, renderHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                         currentBuffer.data());
        fbo.release();

        // OpenGL renders bottom-up, but NDI expects top-down.
        // Flip the image vertically in-place.
        int stride = renderWidth * 4;
        QByteArray rowBuffer(stride, 0);
        uint8_t* pixels = reinterpret_cast<uint8_t*>(currentBuffer.data());
        for (int y = 0; y < renderHeight / 2; ++y) {
            int topOffset = y * stride;
            int bottomOffset = (renderHeight - 1 - y) * stride;
            memcpy(rowBuffer.data(), pixels + topOffset, stride);
            memcpy(pixels + topOffset, pixels + bottomOffset, stride);
            memcpy(pixels + bottomOffset, rowBuffer.data(), stride);
        }

        // Send frame to NDI (blocks if clock_video is true)
        ndiSender.sendFrame(reinterpret_cast<const uint8_t*>(currentBuffer.constData()),
                            renderWidth, renderHeight, targetFps);

        useBufferA = !useBufferA;
        frameCount++;

        // Log stats every 5 seconds
        if (perfTimer.elapsed() >= 5000) {
            double elapsed = perfTimer.elapsed() / 1000.0;
            double actualFps = frameCount / elapsed;
            qDebug() << "NdiMain:" << QString::number(actualFps, 'f', 1) << "fps,"
                     << ndiSender.connectionCount() << "receivers connected";
            perfTimer.restart();
            frameCount = 0;
        }
    });

    renderTimer.start();
    qDebug() << "NdiMain: Render loop started";

    return app.exec();
}

} // namespace Clarity
