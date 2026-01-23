#include "Slide.h"

namespace Clarity {

Slide::Slide()
    : m_backgroundColor("#1e3a8a")
    , m_textColor("#ffffff")
    , m_fontFamily("Arial")
    , m_fontSize(48)
{
}

Slide::Slide(const QString& text, const QColor& backgroundColor, const QColor& textColor)
    : m_text(text)
    , m_backgroundColor(backgroundColor)
    , m_textColor(textColor)
    , m_fontFamily("Arial")
    , m_fontSize(48)
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
    return slide;
}

} // namespace Clarity
