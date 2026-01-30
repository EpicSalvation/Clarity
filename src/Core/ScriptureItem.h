#pragma once

#include "PresentationItem.h"
#include "BibleDatabase.h"

namespace Clarity {

/**
 * @brief A presentation item that displays a Bible scripture passage
 *
 * ScriptureItem stores a scripture reference string (e.g., "John 3:16-17")
 * and translation, then generates slides from the Bible database on demand.
 *
 * Options for slide generation:
 * - One verse per slide vs. entire passage on one slide
 * - Include/exclude verse references on each slide
 * - Include/exclude passage header slide
 */
class ScriptureItem : public PresentationItem {
    Q_OBJECT

public:
    explicit ScriptureItem(QObject* parent = nullptr);
    ScriptureItem(const QString& reference, const QString& translation,
                  BibleDatabase* database, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return ScriptureItemType; }
    QString displayName() const override;
    QString displaySubtitle() const override;
    QList<Slide> generateSlides() const override;

    // Scripture reference
    /**
     * @brief Get the scripture reference string
     */
    QString reference() const { return m_reference; }

    /**
     * @brief Set the scripture reference string
     */
    void setReference(const QString& reference);

    /**
     * @brief Get the translation (e.g., "KJV", "WEB")
     */
    QString translation() const { return m_translation; }

    /**
     * @brief Set the translation
     */
    void setTranslation(const QString& translation);

    /**
     * @brief Get the Bible database (may be null)
     */
    BibleDatabase* bibleDatabase() const { return m_bibleDatabase; }

    /**
     * @brief Set the Bible database for lookups
     */
    void setBibleDatabase(BibleDatabase* database);

    // Slide generation options
    /**
     * @brief Whether to put each verse on its own slide
     */
    bool oneVersePerSlide() const { return m_oneVersePerSlide; }
    void setOneVersePerSlide(bool onePerSlide);

    /**
     * @brief Whether to include verse references on each slide
     */
    bool includeVerseReferences() const { return m_includeVerseReferences; }
    void setIncludeVerseReferences(bool include);

    /**
     * @brief Whether to include a header slide with the full reference
     */
    bool includeHeaderSlide() const { return m_includeHeaderSlide; }
    void setIncludeHeaderSlide(bool include);

    // JSON serialization
    QJsonObject toJson() const override;

    /**
     * @brief Create a ScriptureItem from JSON
     * @param json JSON object with type="scripture"
     * @param database Bible database for looking up verses
     * @return New ScriptureItem or nullptr on error
     */
    static ScriptureItem* fromJson(const QJsonObject& json, BibleDatabase* database);

private:
    QString m_reference;
    QString m_translation;
    BibleDatabase* m_bibleDatabase;

    // Generation options
    bool m_oneVersePerSlide;
    bool m_includeVerseReferences;
    bool m_includeHeaderSlide;
};

} // namespace Clarity
