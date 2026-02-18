#pragma once

#include "PresentationItem.h"
#include "EsvApiClient.h"

namespace Clarity {

class SettingsManager;

/**
 * @brief A presentation item that displays scripture from the ESV API
 *
 * Unlike ScriptureItem (which uses the local BibleDatabase), EsvScriptureItem
 * stores pre-fetched verse text from the ESV API. This text is considered
 * "cached" per ESV API terms and should be purged when the application closes.
 *
 * Design for purgeability:
 * - All EsvScriptureItems have type EsvScriptureItemType, making them trivially
 *   identifiable for batch removal
 * - Each item tracks its verse count for cache limit enforcement
 * - The Presentation can enumerate and remove all ESV items via type check
 */
class EsvScriptureItem : public PresentationItem {
    Q_OBJECT

public:
    explicit EsvScriptureItem(QObject* parent = nullptr);

    /**
     * @brief Construct with pre-fetched passage data
     * @param passage Passage data fetched from the ESV API
     * @param parent Parent QObject
     */
    EsvScriptureItem(const EsvPassage& passage, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return EsvScriptureItemType; }
    QString displayName() const override;
    QString displaySubtitle() const override;
    QList<Slide> generateSlides() const override;

    // ESV passage data
    /**
     * @brief Get the canonical reference (e.g., "John 3:16-17")
     */
    QString reference() const { return m_reference; }

    /**
     * @brief Set the canonical reference
     */
    void setReference(const QString& reference);

    /**
     * @brief Get the cached verse data
     */
    QList<EsvVerse> verses() const { return m_verses; }

    /**
     * @brief Set the cached verse data (from API response)
     */
    void setVerses(const QList<EsvVerse>& verses);

    /**
     * @brief Get the copyright notice (required by ESV terms)
     */
    QString copyright() const { return m_copyright; }

    /**
     * @brief Get the number of cached verses in this item
     */
    int verseCount() const { return m_verses.count(); }

    /**
     * @brief Get the settings manager
     */
    SettingsManager* settingsManager() const { return m_settingsManager; }

    /**
     * @brief Set the settings manager
     */
    void setSettingsManager(SettingsManager* settings);

    // Slide generation options
    bool oneVersePerSlide() const { return m_oneVersePerSlide; }
    void setOneVersePerSlide(bool onePerSlide);

    bool includeVerseNumbers() const { return m_includeVerseNumbers; }
    void setIncludeVerseNumbers(bool include);

    bool includeHeaderSlide() const { return m_includeHeaderSlide; }
    void setIncludeHeaderSlide(bool include);

    /**
     * @brief Clear cached verse text (for ESV compliance purge)
     *
     * After calling this, the item will generate placeholder slides
     * indicating the content has been purged.
     */
    void purgeCache();

    /**
     * @brief Check if the cached content has been purged
     */
    bool isPurged() const { return m_isPurged; }

    // JSON serialization
    QJsonObject toJson() const override;
    static EsvScriptureItem* fromJson(const QJsonObject& json, SettingsManager* settings = nullptr);

private:
    QString m_reference;
    QList<EsvVerse> m_verses;
    QString m_copyright;
    SettingsManager* m_settingsManager;

    // Generation options
    bool m_oneVersePerSlide;
    bool m_includeVerseNumbers;
    bool m_includeHeaderSlide;
    bool m_isPurged;
};

} // namespace Clarity
