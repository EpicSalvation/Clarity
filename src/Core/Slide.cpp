#include "Slide.h"
#include <QJsonDocument>
#include <algorithm>

namespace Clarity {

Slide::Slide()
    : m_backgroundColor("#1e3a8a")
    , m_textColor("#ffffff")
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_backgroundType(SolidColor)
    , m_gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
    , m_gradientType(LinearGradient)
    , m_gradientAngle(135)
    , m_radialCenterX(0.5)
    , m_radialCenterY(0.5)
    , m_radialRadius(0.5)
    , m_videoLoop(true)
    , m_dropShadowEnabled(true)
    , m_dropShadowColor("#000000")
    , m_dropShadowOffsetX(2)
    , m_dropShadowOffsetY(2)
    , m_dropShadowBlur(0)
    , m_overlayEnabled(false)
    , m_overlayColor("#80000000")  // 50% black
    , m_overlayBlur(0)
    , m_textContainerEnabled(false)
    , m_textContainerColor("#80000000")  // 50% black
    , m_textContainerPadding(20)
    , m_textContainerRadius(8)
    , m_textContainerBlur(0)
    , m_textBandEnabled(false)
    , m_textBandColor("#80000000")  // 50% black
    , m_textBandBlur(0)
    , m_transitionDuration(-1)
    , m_autoAdvanceDuration(0)
    , m_groupIndex(-1)
{
}

Slide::Slide(const QString& text, const QColor& backgroundColor, const QColor& textColor)
    : m_text(text)
    , m_backgroundColor(backgroundColor)
    , m_textColor(textColor)
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_backgroundType(SolidColor)
    , m_gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
    , m_gradientType(LinearGradient)
    , m_gradientAngle(135)
    , m_radialCenterX(0.5)
    , m_radialCenterY(0.5)
    , m_radialRadius(0.5)
    , m_videoLoop(true)
    , m_dropShadowEnabled(true)
    , m_dropShadowColor("#000000")
    , m_dropShadowOffsetX(2)
    , m_dropShadowOffsetY(2)
    , m_dropShadowBlur(0)
    , m_overlayEnabled(false)
    , m_overlayColor("#80000000")  // 50% black
    , m_overlayBlur(0)
    , m_textContainerEnabled(false)
    , m_textContainerColor("#80000000")  // 50% black
    , m_textContainerPadding(20)
    , m_textContainerRadius(8)
    , m_textContainerBlur(0)
    , m_textBandEnabled(false)
    , m_textBandColor("#80000000")  // 50% black
    , m_textBandBlur(0)
    , m_transitionDuration(-1)
    , m_autoAdvanceDuration(0)
    , m_groupIndex(-1)
{
}

QJsonObject Slide::toJson() const
{
    QJsonObject json;
    json["text"] = m_text;
    if (!m_richText.isEmpty()) {
        json["richText"] = m_richText;
    }
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
        case Video:
            bgTypeString = "video";
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
        // Multi-stop gradient array
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

        // Radial-specific fields
        if (m_gradientType == RadialGradient) {
            json["radialCenterX"] = m_radialCenterX;
            json["radialCenterY"] = m_radialCenterY;
            json["radialRadius"] = m_radialRadius;
        }

        // Legacy fields for backward compat with older readers
        if (!m_gradientStops.isEmpty()) {
            json["gradientStartColor"] = m_gradientStops.first().color.name();
            json["gradientEndColor"] = m_gradientStops.last().color.name();
        }
    }

    // Include video data if background type is Video
    if (m_backgroundType == Video) {
        json["backgroundVideoPath"] = m_backgroundVideoPath;
        json["videoLoop"] = m_videoLoop;
    }

    // Phase 3: Per-slide transition override (only include if set)
    if (!m_transitionType.isEmpty()) {
        json["transitionType"] = m_transitionType;
    }
    if (m_transitionDuration >= 0) {
        json["transitionDuration"] = m_transitionDuration;
    }

    // Phase 4: Auto-advance timer (only include if enabled)
    if (m_autoAdvanceDuration > 0) {
        json["autoAdvanceDuration"] = m_autoAdvanceDuration;
    }

    // Phase 3: Presenter notes (only include if not empty)
    if (!m_notes.isEmpty()) {
        json["notes"] = m_notes;
    }

    // Section grouping metadata (only include when set)
    if (!m_groupLabel.isEmpty()) {
        json["groupLabel"] = m_groupLabel;
    }
    if (m_groupIndex >= 0) {
        json["groupIndex"] = m_groupIndex;
    }

    // Phase 3: Text legibility - Drop shadow
    json["dropShadowEnabled"] = m_dropShadowEnabled;
    if (m_dropShadowEnabled) {
        json["dropShadowColor"] = m_dropShadowColor.name(QColor::HexArgb);
        json["dropShadowOffsetX"] = m_dropShadowOffsetX;
        json["dropShadowOffsetY"] = m_dropShadowOffsetY;
        json["dropShadowBlur"] = m_dropShadowBlur;
    }

    // Phase 3: Text legibility - Background overlay and blur
    json["overlayEnabled"] = m_overlayEnabled;
    if (m_overlayEnabled) {
        json["overlayColor"] = m_overlayColor.name(QColor::HexArgb);
    }
    // Blur is independent of overlay enable — always serialize
    json["overlayBlur"] = m_overlayBlur;

    // Phase 3: Text legibility - Text container
    json["textContainerEnabled"] = m_textContainerEnabled;
    if (m_textContainerEnabled) {
        json["textContainerColor"] = m_textContainerColor.name(QColor::HexArgb);
        json["textContainerPadding"] = m_textContainerPadding;
        json["textContainerRadius"] = m_textContainerRadius;
        json["textContainerBlur"] = m_textContainerBlur;
    }

    // Phase 3: Text legibility - Text band
    json["textBandEnabled"] = m_textBandEnabled;
    if (m_textBandEnabled) {
        json["textBandColor"] = m_textBandColor.name(QColor::HexArgb);
        json["textBandBlur"] = m_textBandBlur;
    }

    return json;
}

Slide Slide::fromJson(const QJsonObject& json)
{
    Slide slide;
    slide.m_text = json["text"].toString();
    slide.m_richText = json["richText"].toString();  // May be empty/missing
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

        // Try multi-stop array first; fall back to legacy start/end colors
        if (json.contains("gradientStops")) {
            slide.m_gradientStops.clear();
            QJsonArray stopsArray = json["gradientStops"].toArray();
            for (const auto& val : stopsArray) {
                QJsonObject stopObj = val.toObject();
                double pos = stopObj["position"].toDouble(0.0);
                QColor col = QColor(stopObj["color"].toString("#000000"));
                slide.m_gradientStops.append(GradientStop(pos, col));
            }
            // Sort by position and enforce minimum 2 stops
            std::sort(slide.m_gradientStops.begin(), slide.m_gradientStops.end(),
                      [](const GradientStop& a, const GradientStop& b) { return a.position < b.position; });
            while (slide.m_gradientStops.size() < 2) {
                slide.m_gradientStops.append(GradientStop(1.0, QColor("#60a5fa")));
            }
        } else {
            // Legacy: convert start/end colors to 2-stop list
            QColor startCol = QColor(json["gradientStartColor"].toString("#1e3a8a"));
            QColor endCol = QColor(json["gradientEndColor"].toString("#60a5fa"));
            slide.m_gradientStops = {GradientStop(0.0, startCol), GradientStop(1.0, endCol)};
        }

        // Gradient type
        QString gtStr = json["gradientType"].toString("linear");
        slide.m_gradientType = (gtStr == "radial") ? RadialGradient : LinearGradient;

        slide.m_gradientAngle = json["gradientAngle"].toInt(135);

        // Radial fields
        slide.m_radialCenterX = json["radialCenterX"].toDouble(0.5);
        slide.m_radialCenterY = json["radialCenterY"].toDouble(0.5);
        slide.m_radialRadius = json["radialRadius"].toDouble(0.5);
    } else if (bgTypeString == "video") {
        slide.m_backgroundType = Video;
        slide.m_backgroundVideoPath = json["backgroundVideoPath"].toString();
        slide.m_videoLoop = json["videoLoop"].toBool(true);
    } else {
        slide.m_backgroundType = SolidColor;
    }

    // Phase 3: Per-slide transition override
    if (json.contains("transitionType")) {
        slide.m_transitionType = json["transitionType"].toString();
    }
    if (json.contains("transitionDuration")) {
        slide.m_transitionDuration = json["transitionDuration"].toInt();
    }

    // Phase 4: Auto-advance timer
    slide.m_autoAdvanceDuration = json["autoAdvanceDuration"].toInt(0);

    // Phase 3: Presenter notes
    if (json.contains("notes")) {
        slide.m_notes = json["notes"].toString();
    }

    // Section grouping metadata
    slide.m_groupLabel = json["groupLabel"].toString();
    slide.m_groupIndex = json["groupIndex"].toInt(-1);

    // Phase 3: Text legibility - Drop shadow
    slide.m_dropShadowEnabled = json["dropShadowEnabled"].toBool(true);
    if (json.contains("dropShadowColor")) {
        slide.m_dropShadowColor = QColor(json["dropShadowColor"].toString("#000000"));
    }
    slide.m_dropShadowOffsetX = json["dropShadowOffsetX"].toInt(2);
    slide.m_dropShadowOffsetY = json["dropShadowOffsetY"].toInt(2);
    slide.m_dropShadowBlur = json["dropShadowBlur"].toInt(0);

    // Phase 3: Text legibility - Background overlay
    slide.m_overlayEnabled = json["overlayEnabled"].toBool(false);
    if (json.contains("overlayColor")) {
        slide.m_overlayColor = QColor(json["overlayColor"].toString("#80000000"));
    }
    slide.m_overlayBlur = json["overlayBlur"].toInt(0);

    // Phase 3: Text legibility - Text container
    slide.m_textContainerEnabled = json["textContainerEnabled"].toBool(false);
    if (json.contains("textContainerColor")) {
        slide.m_textContainerColor = QColor(json["textContainerColor"].toString("#80000000"));
    }
    slide.m_textContainerPadding = json["textContainerPadding"].toInt(20);
    slide.m_textContainerRadius = json["textContainerRadius"].toInt(8);
    slide.m_textContainerBlur = json["textContainerBlur"].toInt(0);

    // Phase 3: Text legibility - Text band
    slide.m_textBandEnabled = json["textBandEnabled"].toBool(false);
    if (json.contains("textBandColor")) {
        slide.m_textBandColor = QColor(json["textBandColor"].toString("#80000000"));
    }
    slide.m_textBandBlur = json["textBandBlur"].toInt(0);

    return slide;
}

QString Slide::gradientStopsJson() const
{
    // Produce CSS-compatible color strings for the QML Canvas API.
    // Qt's HexArgb is #AARRGGBB but CSS 8-digit hex is #RRGGBBAA — they're
    // incompatible.  Use rgba() for colors with alpha, plain #RRGGBB otherwise.
    QJsonArray arr;
    for (const auto& stop : m_gradientStops) {
        QJsonObject obj;
        obj["p"] = stop.position;
        if (stop.color.alpha() < 255) {
            obj["c"] = QString("rgba(%1,%2,%3,%4)")
                .arg(stop.color.red())
                .arg(stop.color.green())
                .arg(stop.color.blue())
                .arg(stop.color.alphaF(), 0, 'f', 3);
        } else {
            obj["c"] = stop.color.name();  // #RRGGBB
        }
        arr.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

} // namespace Clarity
