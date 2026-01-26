#include "Theme.h"
#include <QJsonArray>

namespace Clarity {

Theme::Theme()
    : m_name("Untitled Theme")
    , m_description("")
    , m_isBuiltIn(false)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_accentColor(QColor("#fbbf24"))
    , m_fontFamily("Arial")
    , m_titleFontSize(72)
    , m_bodyFontSize(48)
    , m_backgroundType(Slide::SolidColor)
    , m_gradientStartColor(QColor("#1e3a8a"))
    , m_gradientEndColor(QColor("#60a5fa"))
    , m_gradientAngle(135)
{
}

Theme::Theme(const QString& name, const QString& description)
    : m_name(name)
    , m_description(description)
    , m_isBuiltIn(false)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_accentColor(QColor("#fbbf24"))
    , m_fontFamily("Arial")
    , m_titleFontSize(72)
    , m_bodyFontSize(48)
    , m_backgroundType(Slide::SolidColor)
    , m_gradientStartColor(QColor("#1e3a8a"))
    , m_gradientEndColor(QColor("#60a5fa"))
    , m_gradientAngle(135)
{
}

void Theme::applyToSlide(Slide& slide) const
{
    // Apply text styling
    slide.setTextColor(m_textColor);
    slide.setFontFamily(m_fontFamily);
    slide.setFontSize(m_bodyFontSize);

    // Apply background based on type
    slide.setBackgroundType(m_backgroundType);

    switch (m_backgroundType) {
    case Slide::SolidColor:
        slide.setBackgroundColor(m_backgroundColor);
        break;

    case Slide::Gradient:
        slide.setGradientStartColor(m_gradientStartColor);
        slide.setGradientEndColor(m_gradientEndColor);
        slide.setGradientAngle(m_gradientAngle);
        break;

    case Slide::Image:
        slide.setBackgroundImageData(m_backgroundImageData);
        break;
    case Slide::Video:
        slide.setBackgroundVideoPath(m_backgroundVideoPath);
        break;
    }
}

Slide Theme::createSlide(const QString& text) const
{
    Slide slide;
    slide.setText(text);
    applyToSlide(slide);
    return slide;
}

QJsonObject Theme::toJson() const
{
    QJsonObject json;

    // Basic info
    json["name"] = m_name;
    json["description"] = m_description;
    json["isBuiltIn"] = m_isBuiltIn;

    // Colors
    json["backgroundColor"] = m_backgroundColor.name();
    json["textColor"] = m_textColor.name();
    json["accentColor"] = m_accentColor.name();

    // Typography
    json["fontFamily"] = m_fontFamily;
    json["titleFontSize"] = m_titleFontSize;
    json["bodyFontSize"] = m_bodyFontSize;

    // Background type
    QString typeStr;
    switch (m_backgroundType) {
    case Slide::SolidColor:
        typeStr = "solidColor";
        break;
    case Slide::Gradient:
        typeStr = "gradient";
        break;
    case Slide::Image:
        typeStr = "image";
        break;
    case Slide::Video:
        typeStr = "video";
        break;
    }
    json["backgroundType"] = typeStr;

    // Gradient settings (always include for completeness)
    json["gradientStartColor"] = m_gradientStartColor.name();
    json["gradientEndColor"] = m_gradientEndColor.name();
    json["gradientAngle"] = m_gradientAngle;

    // Image data (only if present)
    if (!m_backgroundImageData.isEmpty()) {
        json["backgroundImageData"] = QString::fromLatin1(m_backgroundImageData.toBase64());
    }

    if (!m_backgroundVideoPath.isEmpty()) {
        json["backgroundVideoPath"] = m_backgroundVideoPath;
    }

    return json;
}

Theme Theme::fromJson(const QJsonObject& json)
{
    Theme theme;

    // Basic info
    theme.m_name = json["name"].toString("Untitled Theme");
    theme.m_description = json["description"].toString();
    theme.m_isBuiltIn = json["isBuiltIn"].toBool(false);

    // Colors
    theme.m_backgroundColor = QColor(json["backgroundColor"].toString("#1e3a8a"));
    theme.m_textColor = QColor(json["textColor"].toString("#ffffff"));
    theme.m_accentColor = QColor(json["accentColor"].toString("#fbbf24"));

    // Typography
    theme.m_fontFamily = json["fontFamily"].toString("Arial");
    theme.m_titleFontSize = json["titleFontSize"].toInt(72);
    theme.m_bodyFontSize = json["bodyFontSize"].toInt(48);

    // Background type
    QString typeStr = json["backgroundType"].toString("solidColor");
    if (typeStr == "gradient") {
        theme.m_backgroundType = Slide::Gradient;
    } else if (typeStr == "image") {
        theme.m_backgroundType = Slide::Image;
    } else if (typeStr == "video") {
        theme.m_backgroundType = Slide::Video;
    } else {
        theme.m_backgroundType = Slide::SolidColor;
    }

    // Gradient settings
    theme.m_gradientStartColor = QColor(json["gradientStartColor"].toString("#1e3a8a"));
    theme.m_gradientEndColor = QColor(json["gradientEndColor"].toString("#60a5fa"));
    theme.m_gradientAngle = json["gradientAngle"].toInt(135);

    // Image data
    if (json.contains("backgroundImageData")) {
        theme.m_backgroundImageData = QByteArray::fromBase64(
            json["backgroundImageData"].toString().toLatin1()
        );
    }

    if (json.contains("backgroundVideoPath")) {
        theme.m_backgroundVideoPath = json["backgroundVideoPath"].toString();
    }

    return theme;
}

} // namespace Clarity
