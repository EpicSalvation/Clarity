#pragma once

#include "PresentationItem.h"
#include "ApiBibleClient.h"

namespace Clarity {

/**
 * @brief A presentation item that displays scripture from the API.bible service
 *
 * Stores pre-fetched verse text from API.bible. Unlike EsvScriptureItem,
 * API.bible does not require strict cache purging, but the item type
 * is still distinct so items can be identified by source.
 */
class ApiBibleScriptureItem : public PresentationItem {
    Q_OBJECT

public:
    explicit ApiBibleScriptureItem(QObject* parent = nullptr);

    /**
     * @brief Construct with pre-fetched passage data
     * @param passage Passage data fetched from API.bible
     * @param parent Parent QObject
     */
    ApiBibleScriptureItem(const ApiBiblePassage& passage, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return ApiBibleScriptureItemType; }
    QString displayName() const override;
    QString displaySubtitle() const override;
    QList<Slide> generateSlides() const override;

    // Passage data
    /**
     * @brief Get the human-readable reference (e.g., "John 3:16-17")
     */
    QString reference() const { return m_reference; }
    void setReference(const QString& reference);

    /**
     * @brief Get the cached verse data
     */
    QList<ApiBibleVerse> verses() const { return m_verses; }
    void setVerses(const QList<ApiBibleVerse>& verses);

    /**
     * @brief Get the copyright notice (required by API.bible terms)
     */
    QString copyright() const { return m_copyright; }

    /**
     * @brief Get the Bible version abbreviation (e.g., "KJV")
     */
    QString bibleAbbreviation() const { return m_bibleAbbreviation; }
    void setBibleAbbreviation(const QString& abbr);

    /**
     * @brief Get the number of cached verses in this item
     */
    int verseCount() const { return m_verses.count(); }

    // Slide generation options
    bool oneVersePerSlide() const { return m_oneVersePerSlide; }
    void setOneVersePerSlide(bool onePerSlide);

    bool includeVerseNumbers() const { return m_includeVerseNumbers; }
    void setIncludeVerseNumbers(bool include);

    bool includeHeaderSlide() const { return m_includeHeaderSlide; }
    void setIncludeHeaderSlide(bool include);

    // JSON serialization
    QJsonObject toJson() const override;
    static ApiBibleScriptureItem* fromJson(const QJsonObject& json);

private:
    QString m_reference;
    QList<ApiBibleVerse> m_verses;
    QString m_copyright;
    QString m_bibleAbbreviation;

    // Generation options
    bool m_oneVersePerSlide;
    bool m_includeVerseNumbers;
    bool m_includeHeaderSlide;
};

} // namespace Clarity
