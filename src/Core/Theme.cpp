#include "Theme.h"
#include <QJsonArray>
#include <algorithm>

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
    , m_gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
    , m_gradientType(LinearGradient)
    , m_gradientAngle(135)
    , m_radialCenterX(0.5)
    , m_radialCenterY(0.5)
    , m_radialRadius(0.5)
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
    , m_gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
    , m_gradientType(LinearGradient)
    , m_gradientAngle(135)
    , m_radialCenterX(0.5)
    , m_radialCenterY(0.5)
    , m_radialRadius(0.5)
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
        slide.setGradientStops(m_gradientStops);
        slide.setGradientType(m_gradientType);
        slide.setGradientAngle(m_gradientAngle);
        slide.setRadialCenterX(m_radialCenterX);
        slide.setRadialCenterY(m_radialCenterY);
        slide.setRadialRadius(m_radialRadius);
        break;

    case Slide::Image:
        slide.setBackgroundImageData(m_backgroundImageData);
        break;
    }

    // Apply drop shadow
    slide.setDropShadowEnabled(m_dropShadowEnabled);
    slide.setDropShadowColor(m_dropShadowColor);
    slide.setDropShadowOffsetX(m_dropShadowOffsetX);
    slide.setDropShadowOffsetY(m_dropShadowOffsetY);
    slide.setDropShadowBlur(m_dropShadowBlur);
}

Slide Theme::createSlide(const QString& text) const
{
    Slide slide;
    slide.setText(text);
    applyToSlide(slide);
    return slide;
}

SlideStyle Theme::toSlideStyle() const
{
    SlideStyle style;
    style.backgroundColor = m_backgroundColor;
    style.textColor = m_textColor;
    style.fontFamily = m_fontFamily;
    style.fontSize = m_bodyFontSize;
    style.backgroundType = m_backgroundType;
    style.gradientStops = m_gradientStops;
    style.gradientType = m_gradientType;
    style.gradientAngle = m_gradientAngle;
    style.radialCenterX = m_radialCenterX;
    style.radialCenterY = m_radialCenterY;
    style.radialRadius = m_radialRadius;
    return style;
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
    }
    json["backgroundType"] = typeStr;

    // Gradient settings (always include for completeness)
    QJsonArray stopsArray;
    for (const auto& stop : m_gradientStops) {
        QJsonObject stopObj;
        stopObj["position"] = stop.position;
        stopObj["color"] = stop.color.name(QColor::HexArgb);
        stopsArray.append(stopObj);
    }
    json["gradientStops"] = stopsArray;
    json["gradientType"] = (m_gradientType == RadialGradient) ? "radial" : "linear";
    json["gradientAngle"] = m_gradientAngle;
    json["radialCenterX"] = m_radialCenterX;
    json["radialCenterY"] = m_radialCenterY;
    json["radialRadius"] = m_radialRadius;

    // Legacy fields for backward compat
    if (!m_gradientStops.isEmpty()) {
        json["gradientStartColor"] = m_gradientStops.first().color.name();
        json["gradientEndColor"] = m_gradientStops.last().color.name();
    }

    // Image data (only if present)
    if (!m_backgroundImageData.isEmpty()) {
        json["backgroundImageData"] = QString::fromLatin1(m_backgroundImageData.toBase64());
    }

    // Drop shadow
    json["dropShadowEnabled"] = m_dropShadowEnabled;
    json["dropShadowColor"] = m_dropShadowColor.name(QColor::HexArgb);
    json["dropShadowOffsetX"] = m_dropShadowOffsetX;
    json["dropShadowOffsetY"] = m_dropShadowOffsetY;
    json["dropShadowBlur"] = m_dropShadowBlur;

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
    } else {
        theme.m_backgroundType = Slide::SolidColor;
    }

    // Gradient settings (multi-stop with legacy fallback)
    if (json.contains("gradientStops")) {
        theme.m_gradientStops.clear();
        QJsonArray stopsArray = json["gradientStops"].toArray();
        for (const auto& val : stopsArray) {
            QJsonObject stopObj = val.toObject();
            double pos = stopObj["position"].toDouble(0.0);
            QColor col = QColor(stopObj["color"].toString("#000000"));
            theme.m_gradientStops.append(GradientStop(pos, col));
        }
        std::sort(theme.m_gradientStops.begin(), theme.m_gradientStops.end(),
                  [](const GradientStop& a, const GradientStop& b) { return a.position < b.position; });
        while (theme.m_gradientStops.size() < 2) {
            theme.m_gradientStops.append(GradientStop(1.0, QColor("#60a5fa")));
        }
    } else {
        QColor startCol = QColor(json["gradientStartColor"].toString("#1e3a8a"));
        QColor endCol = QColor(json["gradientEndColor"].toString("#60a5fa"));
        theme.m_gradientStops = {GradientStop(0.0, startCol), GradientStop(1.0, endCol)};
    }
    QString gtStr = json["gradientType"].toString("linear");
    theme.m_gradientType = (gtStr == "radial") ? RadialGradient : LinearGradient;
    theme.m_gradientAngle = json["gradientAngle"].toInt(135);
    theme.m_radialCenterX = json["radialCenterX"].toDouble(0.5);
    theme.m_radialCenterY = json["radialCenterY"].toDouble(0.5);
    theme.m_radialRadius = json["radialRadius"].toDouble(0.5);

    // Image data
    if (json.contains("backgroundImageData")) {
        theme.m_backgroundImageData = QByteArray::fromBase64(
            json["backgroundImageData"].toString().toLatin1()
        );
    }

    // Drop shadow
    theme.m_dropShadowEnabled = json["dropShadowEnabled"].toBool(true);
    theme.m_dropShadowColor = QColor(json["dropShadowColor"].toString("#000000"));
    theme.m_dropShadowOffsetX = json["dropShadowOffsetX"].toInt(2);
    theme.m_dropShadowOffsetY = json["dropShadowOffsetY"].toInt(2);
    theme.m_dropShadowBlur = json["dropShadowBlur"].toInt(4);

    return theme;
}

} // namespace Clarity
