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
    QColor gradientStartColor() const { return m_gradientStartColor; }
    QColor gradientEndColor() const { return m_gradientEndColor; }
    int gradientAngle() const { return m_gradientAngle; }

    // Transition overrides (empty/negative means use default)
    QString transitionType() const { return m_transitionType; }
    int transitionDuration() const { return m_transitionDuration; }
    bool hasTransitionOverride() const { return !m_transitionType.isEmpty() || m_transitionDuration >= 0; }

    // Setters
    void setText(const QString& text) { m_text = text; }
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
    void setTextColor(const QColor& color) { m_textColor = color; }
    void setFontFamily(const QString& family) { m_fontFamily = family; }
    void setFontSize(int size) { m_fontSize = size; }
    void setBackgroundType(BackgroundType type) { m_backgroundType = type; }
    void setBackgroundImagePath(const QString& path) { m_backgroundImagePath = path; }
    void setBackgroundImageData(const QByteArray& data) { m_backgroundImageData = data; }
    void setGradientStartColor(const QColor& color) { m_gradientStartColor = color; }
    void setGradientEndColor(const QColor& color) { m_gradientEndColor = color; }
    void setGradientAngle(int angle) { m_gradientAngle = angle; }

    // Transition overrides
    void setTransitionType(const QString& type) { m_transitionType = type; }
    void setTransitionDuration(int duration) { m_transitionDuration = duration; }
    void clearTransitionOverride() { m_transitionType.clear(); m_transitionDuration = -1; }

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

    // Phase 2: Gradient support
    QColor m_gradientStartColor;        ///< Gradient start color
    QColor m_gradientEndColor;          ///< Gradient end color
    int m_gradientAngle;                ///< Gradient angle in degrees (0=top-to-bottom, 90=left-to-right)

    // Phase 3: Per-slide transition override
    QString m_transitionType;           ///< Override transition type (empty = use default)
    int m_transitionDuration;           ///< Override transition duration in ms (-1 = use default)
};

} // namespace Clarity
