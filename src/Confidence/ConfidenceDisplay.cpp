#include "ConfidenceDisplay.h"
#include <QDebug>

namespace Clarity {

ConfidenceDisplay::ConfidenceDisplay(QObject* parent)
    : QObject(parent)
    , m_ipcClient(new IpcClient("confidence", this))
    , m_hasNextSlide(false)
    , m_isCleared(true)
    , m_currentSlideIndex(0)
    , m_totalSlides(0)
{
    connect(m_ipcClient, &IpcClient::connected, this, &ConfidenceDisplay::onConnected);
    connect(m_ipcClient, &IpcClient::disconnected, this, &ConfidenceDisplay::onDisconnected);
    connect(m_ipcClient, &IpcClient::messageReceived, this, &ConfidenceDisplay::onMessageReceived);

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

} // namespace Clarity
