#pragma once

#include "Slide.h"
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QColor>
#include <QDateTime>

namespace Clarity {

/**
 * @brief Represents a section of a song (verse, chorus, bridge, etc.)
 */
struct SongSection {
    QString type;       ///< Section type: "verse", "chorus", "bridge", "pre-chorus", "tag", "intro", "outro"
    QString label;      ///< Display label: "Verse 1", "Chorus", "Bridge", etc.
    QString text;       ///< Section lyrics text

    SongSection() = default;
    SongSection(const QString& type, const QString& label, const QString& text)
        : type(type), label(label), text(text) {}

    QJsonObject toJson() const;
    static SongSection fromJson(const QJsonObject& json);
};

/**
 * @brief Represents a single usage record for CCLI reporting
 */
struct SongUsage {
    QDateTime dateTime;  ///< When the song was used
    QString eventName;   ///< Event name (e.g., "Sunday Service", "Wednesday Night")

    SongUsage() = default;
    SongUsage(const QDateTime& dt, const QString& event = QString())
        : dateTime(dt), eventName(event) {}

    QJsonObject toJson() const;
    static SongUsage fromJson(const QJsonObject& json);
};

/**
 * @brief Style settings for generating slides from songs and other items
 *
 * Supports solid color and gradient backgrounds for item-level theming.
 */
struct SlideStyle {
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int fontSize;

    // Gradient support (multi-stop + radial)
    Slide::BackgroundType backgroundType;
    QList<GradientStop> gradientStops;
    GradientType gradientType;
    int gradientAngle;
    double radialCenterX;
    double radialCenterY;
    double radialRadius;

    // Backward-compat convenience accessors
    QColor gradientStartColor() const { return gradientStops.isEmpty() ? QColor("#1e3a8a") : gradientStops.first().color; }
    QColor gradientEndColor() const { return gradientStops.isEmpty() ? QColor("#60a5fa") : gradientStops.last().color; }

    // Image/video background support
    QString backgroundImagePath;
    QByteArray backgroundImageData;
    QString backgroundVideoPath;
    bool videoLoop = true;

    SlideStyle()
        : backgroundColor("#1e3a8a")
        , textColor("#ffffff")
        , fontFamily("Arial")
        , fontSize(48)
        , backgroundType(Slide::SolidColor)
        , gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
        , gradientType(LinearGradient)
        , gradientAngle(135)
        , radialCenterX(0.5)
        , radialCenterY(0.5)
        , radialRadius(0.5) {}

    SlideStyle(const QColor& bg, const QColor& text, const QString& font, int size)
        : backgroundColor(bg), textColor(text), fontFamily(font), fontSize(size)
        , backgroundType(Slide::SolidColor)
        , gradientStops({GradientStop(0.0, QColor("#1e3a8a")), GradientStop(1.0, QColor("#60a5fa"))})
        , gradientType(LinearGradient)
        , gradientAngle(135)
        , radialCenterX(0.5)
        , radialCenterY(0.5)
        , radialRadius(0.5) {}

    /**
     * @brief Apply this style to a slide (sets all style properties)
     */
    void applyTo(Slide& slide) const {
        slide.setBackgroundType(backgroundType);
        slide.setBackgroundColor(backgroundColor);
        slide.setTextColor(textColor);
        slide.setFontFamily(fontFamily);
        slide.setFontSize(fontSize);
        if (backgroundType == Slide::Gradient) {
            slide.setGradientStops(gradientStops);
            slide.setGradientType(gradientType);
            slide.setGradientAngle(gradientAngle);
            slide.setRadialCenterX(radialCenterX);
            slide.setRadialCenterY(radialCenterY);
            slide.setRadialRadius(radialRadius);
        }
        if (backgroundType == Slide::Image) {
            slide.setBackgroundImagePath(backgroundImagePath);
            slide.setBackgroundImageData(backgroundImageData);
        }
        if (backgroundType == Slide::Video) {
            slide.setBackgroundVideoPath(backgroundVideoPath);
            slide.setVideoLoop(videoLoop);
        }
    }
};

/**
 * @brief Represents a song with metadata and lyrics sections
 *
 * Songs can be imported from various formats:
 * - OpenLyrics XML
 * - ChordPro (future)
 * - Plain text with section markers
 *
 * Songs are stored in a JSON library file and can be
 * converted to presentation slides.
 */
class Song {
public:
    Song();

    // Metadata
    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }

    QString author() const { return m_author; }
    void setAuthor(const QString& author) { m_author = author; }

    QString copyright() const { return m_copyright; }
    void setCopyright(const QString& copyright) { m_copyright = copyright; }

    QString ccliNumber() const { return m_ccliNumber; }
    void setCcliNumber(const QString& ccli) { m_ccliNumber = ccli; }

    QDateTime addedDate() const { return m_addedDate; }
    void setAddedDate(const QDateTime& date) { m_addedDate = date; }

    QDateTime lastUsed() const { return m_lastUsed; }
    void setLastUsed(const QDateTime& date) { m_lastUsed = date; }

    // Usage history for CCLI reporting
    QList<SongUsage> usageHistory() const { return m_usageHistory; }
    void setUsageHistory(const QList<SongUsage>& history) { m_usageHistory = history; }
    void addUsage(const SongUsage& usage);
    void clearUsageHistory() { m_usageHistory.clear(); }
    int usageCount() const { return m_usageHistory.count(); }

    /**
     * @brief Get usage count within a date range
     * @param from Start date (inclusive)
     * @param to End date (inclusive)
     * @return Number of uses within the date range
     */
    int usageCountInRange(const QDate& from, const QDate& to) const;

    /**
     * @brief Get usage records within a date range
     * @param from Start date (inclusive)
     * @param to End date (inclusive)
     * @return List of usage records within the date range
     */
    QList<SongUsage> usageInRange(const QDate& from, const QDate& to) const;

    // Sections
    QList<SongSection> sections() const { return m_sections; }
    void setSections(const QList<SongSection>& sections) { m_sections = sections; }
    void addSection(const SongSection& section) { m_sections.append(section); }
    void clearSections() { m_sections.clear(); }
    int sectionCount() const { return m_sections.count(); }

    /**
     * @brief Get all lyrics as plain text (for search indexing)
     */
    QString allLyrics() const;

    /**
     * @brief Convert song to presentation slides
     * @param style Visual style for the slides
     * @param includeTitleSlide If true, add a title slide at the beginning with the song title
     * @param includeSectionLabels If true, prepend section labels to slides
     * @param maxLinesPerSlide If > 0, split sections with more lines into multiple slides
     * @return List of Slide objects
     */
    QList<Slide> toSlides(const SlideStyle& style, bool includeTitleSlide = true, bool includeSectionLabels = false, int maxLinesPerSlide = 0) const;

    /**
     * @brief Generate slides for a single section
     * @param sectionIndex Index into m_sections
     * @param style Visual style for the slides
     * @param includeSectionLabel If true, prepend section label to slide text
     * @param maxLinesPerSlide If > 0, split section into multiple slides
     * @return List of Slide objects for this section
     */
    QList<Slide> sectionToSlides(int sectionIndex, const SlideStyle& style, bool includeSectionLabel = false, int maxLinesPerSlide = 0) const;

    // JSON serialization
    QJsonObject toJson() const;
    static Song fromJson(const QJsonObject& json);

    // Import from file formats
    /**
     * @brief Import from OpenLyrics XML format
     * @param xml XML content string
     * @return Song object (empty title if parsing failed)
     */
    static Song fromOpenLyrics(const QString& xml);

    /**
     * @brief Import from plain text with section markers
     * @param text Plain text with [Verse 1], [Chorus], etc. markers
     * @param title Song title
     * @return Song object
     *
     * Section markers recognized:
     * - [Verse 1], [Verse 2], etc.
     * - [Chorus], [Chorus 1], etc.
     * - [Bridge], [Pre-Chorus], [Tag], [Intro], [Outro]
     */
    static Song fromPlainText(const QString& text, const QString& title);

    /**
     * @brief Import from SongSelect USR file format
     * @param content USR file content string
     * @return Song object (empty title if parsing failed)
     *
     * USR is an INI-style format used by SongSelect:
     * - [File] section with Type and Version
     * - [S <CCLI#>] section with song data
     * - Title, Author (pipe-delimited), Copyright (pipe-delimited)
     * - Themes (slash-delimited)
     * - Fields (tab-delimited section names like "Vers 1", "Chorus 1")
     * - Words (tab-delimited sections, /n for newlines within sections)
     */
    static Song fromUsrFile(const QString& content);

private:
    int m_id;
    QString m_title;
    QString m_author;
    QString m_copyright;
    QString m_ccliNumber;
    QList<SongSection> m_sections;
    QDateTime m_addedDate;
    QDateTime m_lastUsed;
    QList<SongUsage> m_usageHistory;
};

} // namespace Clarity
