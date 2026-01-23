#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>

namespace Clarity {

/**
 * @brief Represents a single slide in a presentation
 *
 * Phase 1: Simple text on solid color background
 * Future phases will add background images, videos, etc.
 */
class Slide {
public:
    Slide();
    Slide(const QString& text, const QColor& backgroundColor = QColor("#1e3a8a"), const QColor& textColor = QColor("#ffffff"));

    // Getters
    QString text() const { return m_text; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor textColor() const { return m_textColor; }
    QString fontFamily() const { return m_fontFamily; }
    int fontSize() const { return m_fontSize; }

    // Setters
    void setText(const QString& text) { m_text = text; }
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
    void setTextColor(const QColor& color) { m_textColor = color; }
    void setFontFamily(const QString& family) { m_fontFamily = family; }
    void setFontSize(int size) { m_fontSize = size; }

    // JSON serialization
    QJsonObject toJson() const;
    static Slide fromJson(const QJsonObject& json);

private:
    QString m_text;
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;
};

} // namespace Clarity
