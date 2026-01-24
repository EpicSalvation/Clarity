#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QByteArray>

namespace Clarity {

/**
 * @brief Represents a single slide in a presentation
 *
 * Phase 1: Simple text on solid color background
 * Phase 2: Image backgrounds, gradients
 */
class Slide {
public:
    /**
     * @brief Background type enumeration
     */
    enum BackgroundType {
        SolidColor,  ///< Solid color background (default)
        Image,       ///< Image background
        Gradient     ///< Gradient background (future)
    };

    Slide();
    Slide(const QString& text, const QColor& backgroundColor = QColor("#1e3a8a"), const QColor& textColor = QColor("#ffffff"));

    // Getters
    QString text() const { return m_text; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor textColor() const { return m_textColor; }
    QString fontFamily() const { return m_fontFamily; }
    int fontSize() const { return m_fontSize; }
    BackgroundType backgroundType() const { return m_backgroundType; }
    QString backgroundImagePath() const { return m_backgroundImagePath; }
    QByteArray backgroundImageData() const { return m_backgroundImageData; }

    // Setters
    void setText(const QString& text) { m_text = text; }
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
    void setTextColor(const QColor& color) { m_textColor = color; }
    void setFontFamily(const QString& family) { m_fontFamily = family; }
    void setFontSize(int size) { m_fontSize = size; }
    void setBackgroundType(BackgroundType type) { m_backgroundType = type; }
    void setBackgroundImagePath(const QString& path) { m_backgroundImagePath = path; }
    void setBackgroundImageData(const QByteArray& data) { m_backgroundImageData = data; }

    // JSON serialization
    QJsonObject toJson() const;
    static Slide fromJson(const QJsonObject& json);

private:
    QString m_text;
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;

    // Phase 2: Background type and image support
    BackgroundType m_backgroundType;
    QString m_backgroundImagePath;      ///< Original image file path (for reference)
    QByteArray m_backgroundImageData;   ///< Base64-encoded image data for IPC/storage
};

} // namespace Clarity
