#include "Slide.h"

namespace Clarity {

Slide::Slide()
    : m_backgroundColor("#1e3a8a")
    , m_textColor("#ffffff")
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_backgroundType(SolidColor)
    , m_gradientStartColor("#1e3a8a")
    , m_gradientEndColor("#60a5fa")
    , m_gradientAngle(135)
{
}

Slide::Slide(const QString& text, const QColor& backgroundColor, const QColor& textColor)
    : m_text(text)
    , m_backgroundColor(backgroundColor)
    , m_textColor(textColor)
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_backgroundType(SolidColor)
    , m_gradientStartColor("#1e3a8a")
    , m_gradientEndColor("#60a5fa")
    , m_gradientAngle(135)
{
}

QJsonObject Slide::toJson() const
{
    QJsonObject json;
    json["text"] = m_text;
    json["backgroundColor"] = m_backgroundColor.name();
    json["textColor"] = m_textColor.name();
    json["fontFamily"] = m_fontFamily;
    json["fontSize"] = m_fontSize;

    // Phase 2: Background type and image data
    QString bgTypeString;
    switch (m_backgroundType) {
        case SolidColor:
            bgTypeString = "solidColor";
            break;
        case Image:
            bgTypeString = "image";
            break;
        case Gradient:
            bgTypeString = "gradient";
            break;
    }
    json["backgroundType"] = bgTypeString;

    // Only include image data if background type is Image
    if (m_backgroundType == Image) {
        json["backgroundImagePath"] = m_backgroundImagePath;
        json["backgroundImageData"] = QString(m_backgroundImageData.toBase64());
    }

    // Include gradient data if background type is Gradient
    if (m_backgroundType == Gradient) {
        json["gradientStartColor"] = m_gradientStartColor.name();
        json["gradientEndColor"] = m_gradientEndColor.name();
        json["gradientAngle"] = m_gradientAngle;
    }

    return json;
}

Slide Slide::fromJson(const QJsonObject& json)
{
    Slide slide;
    slide.m_text = json["text"].toString();
    slide.m_backgroundColor = QColor(json["backgroundColor"].toString("#1e3a8a"));
    slide.m_textColor = QColor(json["textColor"].toString("#ffffff"));
    slide.m_fontFamily = json["fontFamily"].toString("Arial");
    slide.m_fontSize = json["fontSize"].toInt(48);

    // Phase 2: Background type and image data
    QString bgTypeString = json["backgroundType"].toString("solidColor");
    if (bgTypeString == "image") {
        slide.m_backgroundType = Image;
        slide.m_backgroundImagePath = json["backgroundImagePath"].toString();

        // Decode base64 image data
        QString base64Data = json["backgroundImageData"].toString();
        slide.m_backgroundImageData = QByteArray::fromBase64(base64Data.toUtf8());
    } else if (bgTypeString == "gradient") {
        slide.m_backgroundType = Gradient;
        slide.m_gradientStartColor = QColor(json["gradientStartColor"].toString("#1e3a8a"));
        slide.m_gradientEndColor = QColor(json["gradientEndColor"].toString("#60a5fa"));
        slide.m_gradientAngle = json["gradientAngle"].toInt(135);
    } else {
        slide.m_backgroundType = SolidColor;
    }

    return slide;
}

} // namespace Clarity
