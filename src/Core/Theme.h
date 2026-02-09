#pragma once

#include "Slide.h"
#include "Song.h"  // For SlideStyle
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QByteArray>

namespace Clarity {

/**
 * @brief Represents a visual theme that can be applied to slides
 *
 * A theme defines the visual style of a slide including colors, fonts,
 * and background settings. Themes can be built-in or custom-created
 * by users.
 */
class Theme {
public:
    Theme();
    Theme(const QString& name, const QString& description = QString());

    // Getters
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    bool isBuiltIn() const { return m_isBuiltIn; }

    // Colors
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor textColor() const { return m_textColor; }
    QColor accentColor() const { return m_accentColor; }

    // Typography
    QString fontFamily() const { return m_fontFamily; }
    int titleFontSize() const { return m_titleFontSize; }
    int bodyFontSize() const { return m_bodyFontSize; }

    // Background
    Slide::BackgroundType backgroundType() const { return m_backgroundType; }
    QColor gradientStartColor() const { return m_gradientStartColor; }
    QColor gradientEndColor() const { return m_gradientEndColor; }
    int gradientAngle() const { return m_gradientAngle; }
    QByteArray backgroundImageData() const { return m_backgroundImageData; }

    // Drop shadow
    bool dropShadowEnabled() const { return m_dropShadowEnabled; }
    QColor dropShadowColor() const { return m_dropShadowColor; }
    int dropShadowOffsetX() const { return m_dropShadowOffsetX; }
    int dropShadowOffsetY() const { return m_dropShadowOffsetY; }
    int dropShadowBlur() const { return m_dropShadowBlur; }

    // Setters
    void setName(const QString& name) { m_name = name; }
    void setDescription(const QString& description) { m_description = description; }
    void setBuiltIn(bool builtIn) { m_isBuiltIn = builtIn; }

    // Colors
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
    void setTextColor(const QColor& color) { m_textColor = color; }
    void setAccentColor(const QColor& color) { m_accentColor = color; }

    // Typography
    void setFontFamily(const QString& family) { m_fontFamily = family; }
    void setTitleFontSize(int size) { m_titleFontSize = size; }
    void setBodyFontSize(int size) { m_bodyFontSize = size; }

    // Background
    void setBackgroundType(Slide::BackgroundType type) { m_backgroundType = type; }
    void setGradientStartColor(const QColor& color) { m_gradientStartColor = color; }
    void setGradientEndColor(const QColor& color) { m_gradientEndColor = color; }
    void setGradientAngle(int angle) { m_gradientAngle = angle; }
    void setBackgroundImageData(const QByteArray& data) { m_backgroundImageData = data; }

    // Drop shadow
    void setDropShadowEnabled(bool enabled) { m_dropShadowEnabled = enabled; }
    void setDropShadowColor(const QColor& color) { m_dropShadowColor = color; }
    void setDropShadowOffsetX(int offset) { m_dropShadowOffsetX = offset; }
    void setDropShadowOffsetY(int offset) { m_dropShadowOffsetY = offset; }
    void setDropShadowBlur(int blur) { m_dropShadowBlur = blur; }

    // Apply theme to a slide
    void applyToSlide(Slide& slide) const;

    // Create a new slide with this theme's styling
    Slide createSlide(const QString& text) const;

    // Convert to SlideStyle for item-level theming
    // Note: SlideStyle only supports solid color backgrounds
    SlideStyle toSlideStyle() const;

    // JSON serialization
    QJsonObject toJson() const;
    static Theme fromJson(const QJsonObject& json);

    // Equality operator for finding themes
    bool operator==(const Theme& other) const { return m_name == other.m_name; }

private:
    QString m_name;
    QString m_description;
    bool m_isBuiltIn;

    // Colors
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_accentColor;

    // Typography
    QString m_fontFamily;
    int m_titleFontSize;
    int m_bodyFontSize;

    // Background
    Slide::BackgroundType m_backgroundType;
    QColor m_gradientStartColor;
    QColor m_gradientEndColor;
    int m_gradientAngle;
    QByteArray m_backgroundImageData;

    // Drop shadow
    bool m_dropShadowEnabled = true;
    QColor m_dropShadowColor = QColor("#000000");
    int m_dropShadowOffsetX = 2;
    int m_dropShadowOffsetY = 2;
    int m_dropShadowBlur = 4;
};

} // namespace Clarity
