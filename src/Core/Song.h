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
 * @brief Style settings for generating slides from songs
 */
struct SlideStyle {
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int fontSize;

    SlideStyle()
        : backgroundColor("#1e3a8a")
        , textColor("#ffffff")
        , fontFamily("Arial")
        , fontSize(48) {}

    SlideStyle(const QColor& bg, const QColor& text, const QString& font, int size)
        : backgroundColor(bg), textColor(text), fontFamily(font), fontSize(size) {}
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
     * @param includeSectionLabels If true, prepend section labels to slides
     * @param maxLinesPerSlide If > 0, split sections with more lines into multiple slides
     * @return List of Slide objects
     */
    QList<Slide> toSlides(const SlideStyle& style, bool includeSectionLabels = false, int maxLinesPerSlide = 0) const;

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

private:
    int m_id;
    QString m_title;
    QString m_author;
    QString m_copyright;
    QString m_ccliNumber;
    QList<SongSection> m_sections;
    QDateTime m_addedDate;
    QDateTime m_lastUsed;
};

} // namespace Clarity
