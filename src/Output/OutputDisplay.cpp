#include "OutputDisplay.h"
#include <QDebug>

namespace Clarity {

OutputDisplay::OutputDisplay(QObject* parent)
    : QObject(parent)
    , m_ipcClient(new IpcClient("output", this))
    , m_backgroundColor("#000000")
    , m_textColor("#ffffff")
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_isCleared(true)
    , m_backgroundType("solidColor")
{
    connect(m_ipcClient, &IpcClient::connected, this, &OutputDisplay::onConnected);
    connect(m_ipcClient, &IpcClient::disconnected, this, &OutputDisplay::onDisconnected);
    connect(m_ipcClient, &IpcClient::messageReceived, this, &OutputDisplay::onMessageReceived);

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
        updateSlide(slide);
    } else if (type == "clearOutput") {
        clearDisplay();
    } else {
        qDebug() << "OutputDisplay: Unknown message type:" << type;
    }
}

void OutputDisplay::updateSlide(const Slide& slide)
{
    bool changed = false;

    if (m_slideText != slide.text()) {
        m_slideText = slide.text();
        emit slideTextChanged();
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

    if (m_isCleared) {
        m_isCleared = false;
        emit isClearedChanged();
    }

    if (changed) {
        qDebug() << "OutputDisplay: Updated slide:" << m_slideText.left(30);
    }
}

void OutputDisplay::clearDisplay()
{
    m_slideText.clear();
    m_backgroundColor = QColor("#000000");
    m_backgroundType = "solidColor";
    m_backgroundImageData.clear();
    m_isCleared = true;

    emit slideTextChanged();
    emit backgroundColorChanged();
    emit backgroundTypeChanged();
    emit backgroundImageDataChanged();
    emit isClearedChanged();

    qDebug() << "OutputDisplay: Display cleared";
}

} // namespace Clarity
