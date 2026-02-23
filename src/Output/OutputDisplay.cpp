#include "OutputDisplay.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

namespace Clarity {

OutputDisplay::OutputDisplay(QObject* parent)
    : OutputDisplay("output", parent)
{
}

OutputDisplay::OutputDisplay(const QString& clientType, QObject* parent)
    : QObject(parent)
    , m_ipcClient(new IpcClient(clientType, this))
    , m_useRichText(false)
    , m_redLetterColor("#cc0000")
    , m_backgroundColor("#000000")
    , m_textColor("#ffffff")
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_isCleared(true)
    , m_backgroundType("solidColor")
    , m_gradientStopsJson("[]")
    , m_gradientType("linear")
    , m_gradientAngle(135)
    , m_radialCenterX(0.5)
    , m_radialCenterY(0.5)
    , m_radialRadius(0.5)
    , m_videoLoop(true)
    , m_dropShadowEnabled(true)
    , m_dropShadowColor("#000000")
    , m_dropShadowOffsetX(2)
    , m_dropShadowOffsetY(2)
    , m_dropShadowBlur(4)
    , m_overlayEnabled(false)
    , m_overlayColor("#80000000")
    , m_overlayBlur(0)
    , m_textContainerEnabled(false)
    , m_textContainerColor("#80000000")
    , m_textContainerPadding(20)
    , m_textContainerRadius(8)
    , m_textContainerBlur(0)
    , m_textBandEnabled(false)
    , m_textBandColor("#80000000")
    , m_textBandBlur(0)
    , m_transitionType("fade")
    , m_transitionDuration(500)
{
    connect(m_ipcClient, &IpcClient::connected, this, &OutputDisplay::onConnected);
    connect(m_ipcClient, &IpcClient::disconnected, this, &OutputDisplay::onDisconnected);
    connect(m_ipcClient, &IpcClient::messageReceived, this, &OutputDisplay::onMessageReceived);

    // Initialize blur performance log
    initBlurLog();

    // Connect to server
    m_ipcClient->connectToServer();
}

void OutputDisplay::onConnected()
{
    qDebug() << "OutputDisplay: Connected to control server";
}

void OutputDisplay::onDisconnected()
{
    qDebug() << "OutputDisplay: Disconnected from control server";
    // Keep displaying current slide when disconnected
}

void OutputDisplay::onMessageReceived(const QJsonObject& message)
{
    QString type = message["type"].toString();

    if (type == "slideData") {
        QJsonObject slideJson = message["slide"].toObject();
        Slide slide = Slide::fromJson(slideJson);

        // Update red letter color if provided
        if (message.contains("redLetterColor")) {
            QString newColor = message["redLetterColor"].toString();
            if (m_redLetterColor != newColor) {
                m_redLetterColor = newColor;
                emit redLetterColorChanged();
            }
        }

        // Update transition settings if provided
        if (message.contains("transitionType")) {
            QString newType = message["transitionType"].toString();
            if (m_transitionType != newType) {
                m_transitionType = newType;
                emit transitionTypeChanged();
            }
        }
        if (message.contains("transitionDuration")) {
            int newDuration = message["transitionDuration"].toInt();
            if (m_transitionDuration != newDuration) {
                m_transitionDuration = newDuration;
                emit transitionDurationChanged();
            }
        }

        // For "cut" transition or first slide, update immediately
        if (m_transitionType == "cut" || m_isCleared) {
            updateSlide(slide);
            emit cutTransition();  // Tell QML to update the current container
        } else {
            // Update the display controller properties with the NEW slide data
            // QML will copy this to the incoming container before animating
            updateSlide(slide);
            // Signal QML to start the transition animation
            emit startTransition();
        }
    } else if (type == "clearOutput") {
        clearDisplay();
    } else if (type == "clearText") {
        clearTextOnly();
    } else if (type == "clearBackground") {
        QString bgColor = message["backgroundColor"].toString("#000000");
        clearBackgroundOnly(QColor(bgColor));
    } else if (type == "toggleFullscreen") {
        emit toggleFullscreen();
    } else if (type == "toggleVisibility") {
        emit toggleVisibility();
    } else if (type == "quit") {
        qDebug() << "OutputDisplay: Received quit command, shutting down";
        QCoreApplication::quit();
    } else {
        qDebug() << "OutputDisplay: Unknown message type:" << type;
    }
}

void OutputDisplay::updateSlide(const Slide& slide)
{
    m_slideUpdateTimer.start();
    bool changed = false;

    if (m_slideText != slide.text()) {
        m_slideText = slide.text();
        emit slideTextChanged();
        changed = true;
    }

    // Handle rich text with red letter markup
    if (m_slideRichText != slide.richText()) {
        m_slideRichText = slide.richText();
        emit slideRichTextChanged();
        changed = true;
    }

    bool shouldUseRichText = slide.hasRichText();
    if (m_useRichText != shouldUseRichText) {
        m_useRichText = shouldUseRichText;
        emit useRichTextChanged();
        changed = true;
    }

    if (m_backgroundColor != slide.backgroundColor()) {
        m_backgroundColor = slide.backgroundColor();
        emit backgroundColorChanged();
        changed = true;
    }

    if (m_textColor != slide.textColor()) {
        m_textColor = slide.textColor();
        emit textColorChanged();
        changed = true;
    }

    if (m_fontFamily != slide.fontFamily()) {
        m_fontFamily = slide.fontFamily();
        emit fontFamilyChanged();
        changed = true;
    }

    if (m_fontSize != slide.fontSize()) {
        m_fontSize = slide.fontSize();
        emit fontSizeChanged();
        changed = true;
    }

    // Handle background type and image data
    QString bgType = "solidColor";
    if (slide.backgroundType() == Slide::Image) {
        bgType = "image";
    } else if (slide.backgroundType() == Slide::Gradient) {
        bgType = "gradient";
    } else if (slide.backgroundType() == Slide::Video) {
        bgType = "video";
    }

    if (m_backgroundType != bgType) {
        m_backgroundType = bgType;
        emit backgroundTypeChanged();
        changed = true;
    }

    if (m_backgroundImageData != slide.backgroundImageData()) {
        m_backgroundImageData = slide.backgroundImageData();
        emit backgroundImageDataChanged();
        changed = true;
    }

    // Handle gradient properties (multi-stop + radial)
    QString newStopsJson = slide.gradientStopsJson();
    if (m_gradientStopsJson != newStopsJson) {
        m_gradientStopsJson = newStopsJson;
        emit gradientStopsJsonChanged();
        changed = true;
    }

    QString newGradientType = (slide.gradientType() == RadialGradient) ? "radial" : "linear";
    if (m_gradientType != newGradientType) {
        m_gradientType = newGradientType;
        emit gradientTypeChanged();
        changed = true;
    }

    if (m_gradientAngle != slide.gradientAngle()) {
        m_gradientAngle = slide.gradientAngle();
        emit gradientAngleChanged();
        changed = true;
    }

    if (!qFuzzyCompare(m_radialCenterX, slide.radialCenterX())) {
        m_radialCenterX = slide.radialCenterX();
        emit radialCenterXChanged();
        changed = true;
    }

    if (!qFuzzyCompare(m_radialCenterY, slide.radialCenterY())) {
        m_radialCenterY = slide.radialCenterY();
        emit radialCenterYChanged();
        changed = true;
    }

    if (!qFuzzyCompare(m_radialRadius, slide.radialRadius())) {
        m_radialRadius = slide.radialRadius();
        emit radialRadiusChanged();
        changed = true;
    }

    // Handle video background
    QString videoSource;
    if (slide.backgroundType() == Slide::Video && !slide.backgroundVideoPath().isEmpty()) {
        // Check if video file exists
        QFileInfo fileInfo(slide.backgroundVideoPath());
        if (fileInfo.exists()) {
            // Convert file path to URL format for QML MediaPlayer
            videoSource = QUrl::fromLocalFile(slide.backgroundVideoPath()).toString();
        } else {
            qWarning() << "Video file not found:" << slide.backgroundVideoPath();
        }
    }

    if (m_backgroundVideoSource != videoSource) {
        m_backgroundVideoSource = videoSource;
        emit backgroundVideoSourceChanged();
        changed = true;
    }

    if (m_videoLoop != slide.videoLoop()) {
        m_videoLoop = slide.videoLoop();
        emit videoLoopChanged();
        changed = true;
    }

    // Handle text legibility: Drop shadow
    if (m_dropShadowEnabled != slide.dropShadowEnabled()) {
        m_dropShadowEnabled = slide.dropShadowEnabled();
        emit dropShadowEnabledChanged();
        changed = true;
    }
    if (m_dropShadowColor != slide.dropShadowColor()) {
        m_dropShadowColor = slide.dropShadowColor();
        emit dropShadowColorChanged();
        changed = true;
    }
    if (m_dropShadowOffsetX != slide.dropShadowOffsetX()) {
        m_dropShadowOffsetX = slide.dropShadowOffsetX();
        emit dropShadowOffsetXChanged();
        changed = true;
    }
    if (m_dropShadowOffsetY != slide.dropShadowOffsetY()) {
        m_dropShadowOffsetY = slide.dropShadowOffsetY();
        emit dropShadowOffsetYChanged();
        changed = true;
    }
    if (m_dropShadowBlur != slide.dropShadowBlur()) {
        m_dropShadowBlur = slide.dropShadowBlur();
        emit dropShadowBlurChanged();
        changed = true;
    }

    // Handle text legibility: Overlay
    if (m_overlayEnabled != slide.overlayEnabled()) {
        m_overlayEnabled = slide.overlayEnabled();
        emit overlayEnabledChanged();
        changed = true;
    }
    if (m_overlayColor != slide.overlayColor()) {
        m_overlayColor = slide.overlayColor();
        emit overlayColorChanged();
        changed = true;
    }
    if (m_overlayBlur != slide.overlayBlur()) {
        m_overlayBlur = slide.overlayBlur();
        emit overlayBlurChanged();
        changed = true;
    }

    // Handle text legibility: Text container
    if (m_textContainerEnabled != slide.textContainerEnabled()) {
        m_textContainerEnabled = slide.textContainerEnabled();
        emit textContainerEnabledChanged();
        changed = true;
    }
    if (m_textContainerColor != slide.textContainerColor()) {
        m_textContainerColor = slide.textContainerColor();
        emit textContainerColorChanged();
        changed = true;
    }
    if (m_textContainerPadding != slide.textContainerPadding()) {
        m_textContainerPadding = slide.textContainerPadding();
        emit textContainerPaddingChanged();
        changed = true;
    }
    if (m_textContainerRadius != slide.textContainerRadius()) {
        m_textContainerRadius = slide.textContainerRadius();
        emit textContainerRadiusChanged();
        changed = true;
    }
    if (m_textContainerBlur != slide.textContainerBlur()) {
        m_textContainerBlur = slide.textContainerBlur();
        emit textContainerBlurChanged();
        changed = true;
    }

    // Handle text legibility: Text band
    if (m_textBandEnabled != slide.textBandEnabled()) {
        m_textBandEnabled = slide.textBandEnabled();
        emit textBandEnabledChanged();
        changed = true;
    }
    if (m_textBandColor != slide.textBandColor()) {
        m_textBandColor = slide.textBandColor();
        emit textBandColorChanged();
        changed = true;
    }
    if (m_textBandBlur != slide.textBandBlur()) {
        m_textBandBlur = slide.textBandBlur();
        emit textBandBlurChanged();
        changed = true;
    }

    if (m_isCleared) {
        m_isCleared = false;
        emit isClearedChanged();
    }

    if (changed) {
        qint64 elapsed = m_slideUpdateTimer.elapsed();
        qDebug() << "OutputDisplay: Updated slide:" << m_slideText.left(30);

        // Log blur configuration if any blur is active
        logBlurConfig(slide);

        if (m_blurLogFile && m_blurLogFile->isOpen()) {
            QTextStream stream(m_blurLogFile);
            stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                   << " | SLIDE_UPDATE | duration=" << elapsed << "ms"
                   << " | text=" << m_slideText.left(30)
                   << "\n";
            stream.flush();
        }
    }
}

void OutputDisplay::clearDisplay()
{
    m_slideText.clear();
    m_slideRichText.clear();
    m_useRichText = false;
    m_backgroundColor = QColor("#000000");
    m_backgroundType = "solidColor";
    m_backgroundImageData.clear();
    m_gradientStopsJson = "[]";
    m_gradientType = "linear";
    m_gradientAngle = 135;
    m_radialCenterX = 0.5;
    m_radialCenterY = 0.5;
    m_radialRadius = 0.5;
    m_backgroundVideoSource.clear();
    m_videoLoop = true;

    // Reset text legibility to defaults
    m_dropShadowEnabled = true;
    m_dropShadowColor = QColor("#000000");
    m_dropShadowOffsetX = 2;
    m_dropShadowOffsetY = 2;
    m_dropShadowBlur = 4;
    m_overlayEnabled = false;
    m_overlayColor = QColor("#80000000");
    m_overlayBlur = 0;
    m_textContainerEnabled = false;
    m_textContainerColor = QColor("#80000000");
    m_textContainerPadding = 20;
    m_textContainerRadius = 8;
    m_textContainerBlur = 0;
    m_textBandEnabled = false;
    m_textBandColor = QColor("#80000000");
    m_textBandBlur = 0;

    m_isCleared = true;

    emit slideTextChanged();
    emit slideRichTextChanged();
    emit useRichTextChanged();
    emit backgroundColorChanged();
    emit backgroundTypeChanged();
    emit backgroundImageDataChanged();
    emit gradientStopsJsonChanged();
    emit gradientTypeChanged();
    emit gradientAngleChanged();
    emit radialCenterXChanged();
    emit radialCenterYChanged();
    emit radialRadiusChanged();
    emit backgroundVideoSourceChanged();
    emit videoLoopChanged();

    // Emit text legibility signals
    emit dropShadowEnabledChanged();
    emit dropShadowColorChanged();
    emit dropShadowOffsetXChanged();
    emit dropShadowOffsetYChanged();
    emit dropShadowBlurChanged();
    emit overlayEnabledChanged();
    emit overlayColorChanged();
    emit overlayBlurChanged();
    emit textContainerEnabledChanged();
    emit textContainerColorChanged();
    emit textContainerPaddingChanged();
    emit textContainerRadiusChanged();
    emit textContainerBlurChanged();
    emit textBandEnabledChanged();
    emit textBandColorChanged();
    emit textBandBlurChanged();

    emit isClearedChanged();

    qDebug() << "OutputDisplay: Display cleared";
}

void OutputDisplay::clearTextOnly()
{
    m_slideText.clear();
    m_slideRichText.clear();
    m_useRichText = false;

    emit slideTextChanged();
    emit slideRichTextChanged();
    emit useRichTextChanged();

    // Trigger immediate update in QML
    emit cutTransition();

    qDebug() << "OutputDisplay: Text cleared (background preserved)";
}

void OutputDisplay::clearBackgroundOnly(const QColor& bgColor)
{
    m_backgroundColor = bgColor;
    m_backgroundType = "solidColor";
    m_backgroundImageData.clear();
    m_gradientStopsJson = "[]";
    m_backgroundVideoSource.clear();

    // Disable legibility effects that depend on the original background
    m_overlayEnabled = false;
    m_textContainerEnabled = false;
    m_textBandEnabled = false;

    emit backgroundColorChanged();
    emit backgroundTypeChanged();
    emit backgroundImageDataChanged();
    emit gradientStopsJsonChanged();
    emit backgroundVideoSourceChanged();
    emit overlayEnabledChanged();
    emit textContainerEnabledChanged();
    emit textBandEnabledChanged();

    // Trigger immediate update in QML
    emit cutTransition();

    qDebug() << "OutputDisplay: Background cleared to" << bgColor.name() << "(text preserved)";
}

void OutputDisplay::transitionComplete()
{
    // Called by QML when transition animation finishes
    // Slide data was already applied before transition started
    qDebug() << "OutputDisplay: Transition complete";
}

void OutputDisplay::initBlurLog()
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    QString logPath = logDir + "/clarity_blur_perf.log";

    m_blurLogFile = new QFile(logPath, this);
    if (m_blurLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(m_blurLogFile);
        stream << "\n=== Clarity Blur Performance Log ===\n"
               << "Session started: " << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "\n"
               << "Log file: " << logPath << "\n"
               << "---\n";
        stream.flush();
        qDebug() << "OutputDisplay: Blur performance log at:" << logPath;
    } else {
        qWarning() << "OutputDisplay: Failed to open blur log at:" << logPath;
    }
}

void OutputDisplay::logBlurConfig(const Slide& slide)
{
    if (!m_blurLogFile || !m_blurLogFile->isOpen())
        return;

    bool hasOverlayBlur = slide.overlayBlur() > 0;
    bool hasContainerBlur = slide.textContainerEnabled() && slide.textContainerBlur() > 0;
    bool hasBandBlur = slide.textBandEnabled() && slide.textBandBlur() > 0;
    bool hasShadowBlur = slide.dropShadowEnabled() && slide.dropShadowBlur() > 0;

    if (!hasOverlayBlur && !hasContainerBlur && !hasBandBlur && !hasShadowBlur)
        return;

    QTextStream stream(m_blurLogFile);
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
           << " | BLUR_CONFIG";

    if (hasOverlayBlur)
        stream << " | overlay_blur=" << slide.overlayBlur();
    if (hasContainerBlur)
        stream << " | container_blur=" << slide.textContainerBlur();
    if (hasBandBlur)
        stream << " | band_blur=" << slide.textBandBlur();
    if (hasShadowBlur)
        stream << " | shadow_blur=" << slide.dropShadowBlur();

    stream << "\n";
    stream.flush();
}

void OutputDisplay::logBlurStatus(const QString& details)
{
    if (!m_blurLogFile || !m_blurLogFile->isOpen())
        return;

    QTextStream stream(m_blurLogFile);
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
           << " | BLUR_STATUS | " << details << "\n";
    stream.flush();
}

} // namespace Clarity
