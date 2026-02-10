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
    , m_videoLoop(true)
    , m_dropShadowEnabled(true)
    , m_dropShadowColor("#000000")
    , m_dropShadowOffsetX(2)
    , m_dropShadowOffsetY(2)
    , m_dropShadowBlur(4)
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
    , m_gradientStartColor("#1e3a8a")
    , m_gradientEndColor("#60a5fa")
    , m_gradientAngle(135)
    , m_videoLoop(true)
    , m_dropShadowEnabled(true)
    , m_dropShadowColor("#000000")
    , m_dropShadowOffsetX(2)
    , m_dropShadowOffsetY(2)
    , m_dropShadowBlur(4)
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
        json["gradientStartColor"] = m_gradientStartColor.name();
        json["gradientEndColor"] = m_gradientEndColor.name();
        json["gradientAngle"] = m_gradientAngle;
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

    // Phase 3: Text legibility - Background overlay
    json["overlayEnabled"] = m_overlayEnabled;
    if (m_overlayEnabled) {
        json["overlayColor"] = m_overlayColor.name(QColor::HexArgb);
        json["overlayBlur"] = m_overlayBlur;
    }

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
        slide.m_gradientStartColor = QColor(json["gradientStartColor"].toString("#1e3a8a"));
        slide.m_gradientEndColor = QColor(json["gradientEndColor"].toString("#60a5fa"));
        slide.m_gradientAngle = json["gradientAngle"].toInt(135);
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
    slide.m_dropShadowBlur = json["dropShadowBlur"].toInt(4);

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

} // namespace Clarity
