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
        Gradient,    ///< Gradient background
        Video        ///< Video background (Phase 3)
    };

    Slide();
    Slide(const QString& text, const QColor& backgroundColor = QColor("#1e3a8a"), const QColor& textColor = QColor("#ffffff"));

    // Getters
    QString text() const { return m_text; }
    QString richText() const { return m_richText; }
    bool hasRichText() const { return !m_richText.isEmpty(); }
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
    QString backgroundVideoPath() const { return m_backgroundVideoPath; }
    bool videoLoop() const { return m_videoLoop; }

    // Text legibility: Drop shadow
    bool dropShadowEnabled() const { return m_dropShadowEnabled; }
    QColor dropShadowColor() const { return m_dropShadowColor; }
    int dropShadowOffsetX() const { return m_dropShadowOffsetX; }
    int dropShadowOffsetY() const { return m_dropShadowOffsetY; }
    int dropShadowBlur() const { return m_dropShadowBlur; }

    // Text legibility: Background overlay (darkening layer over entire background)
    bool overlayEnabled() const { return m_overlayEnabled; }
    QColor overlayColor() const { return m_overlayColor; }
    int overlayBlur() const { return m_overlayBlur; }

    // Text legibility: Text container (box behind text)
    bool textContainerEnabled() const { return m_textContainerEnabled; }
    QColor textContainerColor() const { return m_textContainerColor; }
    int textContainerPadding() const { return m_textContainerPadding; }
    int textContainerRadius() const { return m_textContainerRadius; }
    int textContainerBlur() const { return m_textContainerBlur; }

    // Text legibility: Text band (horizontal strip across screen)
    bool textBandEnabled() const { return m_textBandEnabled; }
    QColor textBandColor() const { return m_textBandColor; }
    int textBandBlur() const { return m_textBandBlur; }

    // Transition overrides (empty/negative means use default)
    QString transitionType() const { return m_transitionType; }
    int transitionDuration() const { return m_transitionDuration; }
    bool hasTransitionOverride() const { return !m_transitionType.isEmpty() || m_transitionDuration >= 0; }

    // Presenter notes (shown on confidence monitor, not on output)
    QString notes() const { return m_notes; }
    void setNotes(const QString& notes) { m_notes = notes; }

    // Section grouping metadata (set during slide generation from songs)
    QString groupLabel() const { return m_groupLabel; }
    void setGroupLabel(const QString& label) { m_groupLabel = label; }
    int groupIndex() const { return m_groupIndex; }
    void setGroupIndex(int index) { m_groupIndex = index; }

    // Setters
    void setText(const QString& text) { m_text = text; }
    void setRichText(const QString& richText) { m_richText = richText; }
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
    void setBackgroundVideoPath(const QString& path) { m_backgroundVideoPath = path; }
    void setVideoLoop(bool loop) { m_videoLoop = loop; }

    // Text legibility: Drop shadow setters
    void setDropShadowEnabled(bool enabled) { m_dropShadowEnabled = enabled; }
    void setDropShadowColor(const QColor& color) { m_dropShadowColor = color; }
    void setDropShadowOffsetX(int offset) { m_dropShadowOffsetX = offset; }
    void setDropShadowOffsetY(int offset) { m_dropShadowOffsetY = offset; }
    void setDropShadowBlur(int blur) { m_dropShadowBlur = blur; }

    // Text legibility: Background overlay setters
    void setOverlayEnabled(bool enabled) { m_overlayEnabled = enabled; }
    void setOverlayColor(const QColor& color) { m_overlayColor = color; }
    void setOverlayBlur(int blur) { m_overlayBlur = blur; }

    // Text legibility: Text container setters
    void setTextContainerEnabled(bool enabled) { m_textContainerEnabled = enabled; }
    void setTextContainerColor(const QColor& color) { m_textContainerColor = color; }
    void setTextContainerPadding(int padding) { m_textContainerPadding = padding; }
    void setTextContainerRadius(int radius) { m_textContainerRadius = radius; }
    void setTextContainerBlur(int blur) { m_textContainerBlur = blur; }

    // Text legibility: Text band setters
    void setTextBandEnabled(bool enabled) { m_textBandEnabled = enabled; }
    void setTextBandColor(const QColor& color) { m_textBandColor = color; }
    void setTextBandBlur(int blur) { m_textBandBlur = blur; }

    // Transition overrides
    void setTransitionType(const QString& type) { m_transitionType = type; }
    void setTransitionDuration(int duration) { m_transitionDuration = duration; }
    void clearTransitionOverride() { m_transitionType.clear(); m_transitionDuration = -1; }

    // JSON serialization
    QJsonObject toJson() const;
    static Slide fromJson(const QJsonObject& json);

private:
    QString m_text;
    QString m_richText;  ///< HTML with red letter markup (may be empty)
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

    // Phase 3: Video background support
    QString m_backgroundVideoPath;      ///< Path to video file (not embedded due to file size)
    bool m_videoLoop;                   ///< Whether video loops (default true)

    // Phase 3: Text legibility - Drop shadow
    bool m_dropShadowEnabled;           ///< Enable drop shadow on text
    QColor m_dropShadowColor;           ///< Shadow color (typically black with alpha)
    int m_dropShadowOffsetX;            ///< Shadow horizontal offset in pixels
    int m_dropShadowOffsetY;            ///< Shadow vertical offset in pixels
    int m_dropShadowBlur;               ///< Shadow blur radius in pixels

    // Phase 3: Text legibility - Background overlay
    bool m_overlayEnabled;              ///< Enable darkening overlay over background
    QColor m_overlayColor;              ///< Overlay color (typically black with alpha)
    int m_overlayBlur;                  ///< Blur radius for background under overlay

    // Phase 3: Text legibility - Text container (box behind text)
    bool m_textContainerEnabled;        ///< Enable text container box
    QColor m_textContainerColor;        ///< Container color (typically semi-transparent)
    int m_textContainerPadding;         ///< Padding around text in pixels
    int m_textContainerRadius;          ///< Corner radius in pixels
    int m_textContainerBlur;            ///< Blur radius for background under container

    // Phase 3: Text legibility - Text band (horizontal strip)
    bool m_textBandEnabled;             ///< Enable horizontal band across screen
    QColor m_textBandColor;             ///< Band color (typically semi-transparent)
    int m_textBandBlur;                 ///< Blur radius for background under band

    // Phase 3: Per-slide transition override
    QString m_transitionType;           ///< Override transition type (empty = use default)
    int m_transitionDuration;           ///< Override transition duration in ms (-1 = use default)

    // Phase 3: Presenter notes
    QString m_notes;                    ///< Presenter notes (shown only on confidence monitor)

    // Section grouping metadata
    QString m_groupLabel;               ///< Section label (e.g., "Verse 1", "Chorus")
    int m_groupIndex;                   ///< Section order position (-1 = title slide/no group)
};

} // namespace Clarity

// Register Slide with Qt's meta-type system for use in QVariant
Q_DECLARE_METATYPE(Clarity::Slide)
