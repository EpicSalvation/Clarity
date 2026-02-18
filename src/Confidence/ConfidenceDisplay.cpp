#include "ConfidenceDisplay.h"
#include <QDebug>
#include <QCoreApplication>

namespace Clarity {

ConfidenceDisplay::ConfidenceDisplay(QObject* parent)
    : QObject(parent)
    , m_ipcClient(new IpcClient("confidence", this))
    , m_hasNextSlide(false)
    , m_isCleared(true)
    , m_currentSlideIndex(0)
    , m_totalSlides(0)
    , m_updateTimer(new QTimer(this))
    , m_pausedElapsedMs(0)
    , m_timerRunning(false)
    , m_autoAdvanceActive(false)
    , m_autoAdvancePaused(false)
    , m_autoAdvanceRemaining(0)
    , m_autoAdvanceTotal(0)
{
    connect(m_ipcClient, &IpcClient::connected, this, &ConfidenceDisplay::onConnected);
    connect(m_ipcClient, &IpcClient::disconnected, this, &ConfidenceDisplay::onDisconnected);
    connect(m_ipcClient, &IpcClient::messageReceived, this, &ConfidenceDisplay::onMessageReceived);

    // Setup update timer for clock and elapsed time display
    // Updates every second to keep the display current
    connect(m_updateTimer, &QTimer::timeout, this, &ConfidenceDisplay::onTimerTick);
    m_updateTimer->start(1000); // Update every second

    // Connect to server
    m_ipcClient->connectToServer();
}

QString ConfidenceDisplay::currentBackgroundType() const
{
    switch (m_currentSlide.backgroundType()) {
        case Slide::SolidColor:
            return "solidColor";
        case Slide::Image:
            return "image";
        case Slide::Gradient:
            return "gradient";
        default:
            return "solidColor";
    }
}

QString ConfidenceDisplay::currentBackgroundImageDataBase64() const
{
    return QString(m_currentSlide.backgroundImageData().toBase64());
}

QString ConfidenceDisplay::nextBackgroundType() const
{
    switch (m_nextSlide.backgroundType()) {
        case Slide::SolidColor:
            return "solidColor";
        case Slide::Image:
            return "image";
        case Slide::Gradient:
            return "gradient";
        default:
            return "solidColor";
    }
}

QString ConfidenceDisplay::nextBackgroundImageDataBase64() const
{
    return QString(m_nextSlide.backgroundImageData().toBase64());
}

void ConfidenceDisplay::onConnected()
{
    qDebug() << "ConfidenceDisplay: Connected to control server";
}

void ConfidenceDisplay::onDisconnected()
{
    qDebug() << "ConfidenceDisplay: Disconnected from control server";
    // Keep displaying current content when disconnected
}

void ConfidenceDisplay::onMessageReceived(const QJsonObject& message)
{
    QString type = message["type"].toString();

    if (type == "confidenceData") {
        // Enhanced message specifically for confidence monitor
        // Contains current slide, next slide, and presentation metadata
        int currentIndex = message["currentIndex"].toInt();
        int total = message["totalSlides"].toInt();

        QJsonObject currentSlideJson = message["currentSlide"].toObject();
        m_currentSlide = Slide::fromJson(currentSlideJson);
        m_currentSlideIndex = currentIndex;
        m_totalSlides = total;

        // Check if there's a next slide
        if (message.contains("nextSlide")) {
            QJsonObject nextSlideJson = message["nextSlide"].toObject();
            m_nextSlide = Slide::fromJson(nextSlideJson);
            m_hasNextSlide = true;
        } else {
            m_hasNextSlide = false;
            m_nextSlide = Slide(); // Reset to default
        }

        m_isCleared = false;

        emit currentSlideChanged();
        emit nextSlideChanged();
        emit currentSlideIndexChanged();
        emit totalSlidesChanged();
        emit isClearedChanged();

        qDebug() << "ConfidenceDisplay: Updated (current:" << currentIndex << "total:" << total << "hasNext:" << m_hasNextSlide << ")";

    } else if (type == "slideData") {
        // Fallback for regular slide data (backward compatibility)
        int index = message["index"].toInt();
        QJsonObject slideJson = message["slide"].toObject();

        m_currentSlide = Slide::fromJson(slideJson);
        m_currentSlideIndex = index;
        m_isCleared = false;

        emit currentSlideChanged();
        emit currentSlideIndexChanged();
        emit isClearedChanged();

        qDebug() << "ConfidenceDisplay: Updated current slide (index:" << index << ")";

    } else if (type == "clearOutput") {
        clearDisplay();
    } else if (type == "timerStart") {
        startTimer();
    } else if (type == "timerPause") {
        pauseTimer();
    } else if (type == "timerReset") {
        resetTimer();
    } else if (type == "settingsChanged") {
        // Settings were changed in Control app, notify QML to re-read them
        emit settingsChanged();
        qDebug() << "ConfidenceDisplay: Settings refreshed";
    } else if (type == "autoAdvanceState") {
        m_autoAdvanceActive = message["active"].toBool(false);
        m_autoAdvancePaused = message["paused"].toBool(false);
        m_autoAdvanceRemaining = message["remainingSeconds"].toInt(0);
        m_autoAdvanceTotal = message["totalDuration"].toInt(0);
        emit autoAdvanceChanged();
    } else if (type == "toggleVisibility") {
        emit toggleVisibility();
    } else if (type == "quit") {
        qDebug() << "ConfidenceDisplay: Received quit command, shutting down";
        QCoreApplication::quit();
    } else {
        qDebug() << "ConfidenceDisplay: Unknown message type:" << type;
    }
}

void ConfidenceDisplay::clearDisplay()
{
    m_currentSlide = Slide();
    m_nextSlide = Slide();
    m_hasNextSlide = false;
    m_isCleared = true;
    m_currentSlideIndex = 0;
    m_totalSlides = 0;

    emit currentSlideChanged();
    emit nextSlideChanged();
    emit currentSlideIndexChanged();
    emit totalSlidesChanged();
    emit isClearedChanged();

    qDebug() << "ConfidenceDisplay: Display cleared";
}

QString ConfidenceDisplay::elapsedTime() const
{
    // Calculate total elapsed milliseconds
    qint64 totalMs = m_pausedElapsedMs;
    if (m_timerRunning && m_elapsedTimer.isValid()) {
        totalMs += m_elapsedTimer.elapsed();
    }

    // Convert to hours, minutes, seconds
    int totalSeconds = static_cast<int>(totalMs / 1000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // Format as HH:MM:SS
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString ConfidenceDisplay::currentTime() const
{
    // Return current time in HH:MM:SS format (12-hour with AM/PM)
    return QDateTime::currentDateTime().toString("h:mm:ss AP");
}

void ConfidenceDisplay::startTimer()
{
    if (!m_timerRunning) {
        m_elapsedTimer.start();
        m_timerRunning = true;
        emit timerRunningChanged();
        qDebug() << "ConfidenceDisplay: Timer started";
    }
}

void ConfidenceDisplay::pauseTimer()
{
    if (m_timerRunning) {
        // Save the current elapsed time before pausing
        m_pausedElapsedMs += m_elapsedTimer.elapsed();
        m_timerRunning = false;
        emit timerRunningChanged();
        qDebug() << "ConfidenceDisplay: Timer paused at" << elapsedTime();
    }
}

void ConfidenceDisplay::resetTimer()
{
    m_pausedElapsedMs = 0;
    m_timerRunning = false;
    m_elapsedTimer.invalidate();
    emit timerRunningChanged();
    emit elapsedTimeChanged();
    qDebug() << "ConfidenceDisplay: Timer reset";
}

void ConfidenceDisplay::onTimerTick()
{
    // Emit signals to update the clock display every second
    emit currentTimeChanged();

    // Also update elapsed time display if timer is running
    if (m_timerRunning) {
        emit elapsedTimeChanged();
    }
}

QString ConfidenceDisplay::settingsFontFamily() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    return settings.value("ConfidenceMonitor/FontFamily", "Arial").toString();
}

int ConfidenceDisplay::settingsFontSize() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    return settings.value("ConfidenceMonitor/FontSize", 32).toInt();
}

QColor ConfidenceDisplay::settingsTextColor() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    return QColor(settings.value("ConfidenceMonitor/TextColor", "#ffffff").toString());
}

QColor ConfidenceDisplay::settingsBackgroundColor() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    return QColor(settings.value("ConfidenceMonitor/BackgroundColor", "#2a2a2a").toString());
}

QString ConfidenceDisplay::redLetterColor() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    return settings.value("Bible/RedLetterColor", "#cc0000").toString();
}

} // namespace Clarity
