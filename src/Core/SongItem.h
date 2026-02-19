#pragma once

#include "PresentationItem.h"
#include "Song.h"
#include "SongLibrary.h"

namespace Clarity { class SettingsManager; }

namespace Clarity {

/**
 * @brief A presentation item that references a song from the library
 *
 * SongItem stores a reference to a song (by ID) and generates slides
 * from the song data on demand. This allows:
 * - Songs to be updated in the library and presentations reflect changes
 * - Different presentations to use the same song with different styles
 * - Per-item options like title slide inclusion, section labels
 *
 * If the referenced song is deleted from the library, the item displays
 * an error state but doesn't crash.
 */
class SongItem : public PresentationItem {
    Q_OBJECT

public:
    explicit SongItem(QObject* parent = nullptr);
    SongItem(int songId, SongLibrary* library, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return SongItemType; }
    QString displayName() const override;
    QString displaySubtitle() const override;
    QList<Slide> generateSlides() const override;

    // Song reference
    /**
     * @brief Get the referenced song ID
     */
    int songId() const { return m_songId; }

    /**
     * @brief Set the referenced song ID
     */
    void setSongId(int songId);

    /**
     * @brief Get the song library (may be null)
     */
    SongLibrary* songLibrary() const { return m_songLibrary; }

    /**
     * @brief Set the song library for lookups
     */
    void setSongLibrary(SongLibrary* library);

    /**
     * @brief Check if the referenced song exists in the library
     */
    bool songExists() const;

    /**
     * @brief Get the referenced song (empty song if not found)
     */
    Song song() const;

    // Slide generation options
    /**
     * @brief Whether to include a title slide at the beginning
     */
    bool includeTitleSlide() const { return m_includeTitleSlide; }
    void setIncludeTitleSlide(bool include);

    /**
     * @brief Whether to include section labels (Verse 1, Chorus, etc.)
     */
    bool includeSectionLabels() const { return m_includeSectionLabels; }
    void setIncludeSectionLabels(bool include);

    /**
     * @brief Maximum lines per slide (0 = no limit, use section as-is)
     */
    int maxLinesPerSlide() const { return m_maxLinesPerSlide; }
    void setMaxLinesPerSlide(int maxLines);

    // Section order management (per-presentation, doesn't modify library song)

    /**
     * @brief Get the section order (custom or natural)
     * @return List of section indices into Song::sections()
     */
    QList<int> sectionOrder() const;

    /**
     * @brief Get the number of sections in the current order
     */
    int sectionCount() const;

    /**
     * @brief Map a slide-in-item index to its section order position
     * @param slideInItem 0-based slide index within this item
     * @return Section order position, or -1 if title slide or invalid
     */
    int sectionOrderIndexForSlide(int slideInItem) const;

    /**
     * @brief Get the section label for a given order position
     */
    QString sectionLabelAt(int orderIndex) const;

    /**
     * @brief Move a section from one order position to another
     */
    void moveSongSection(int from, int to);

    /**
     * @brief Duplicate a section at the given order position
     */
    void duplicateSongSection(int orderIndex);

    /**
     * @brief Remove a section at the given order position
     * @return true if removed, false if only 1 section remains
     */
    bool removeSongSection(int orderIndex);

    // Settings manager (for CCLI display on title slides)
    SettingsManager* settingsManager() const { return m_settingsManager; }
    void setSettingsManager(SettingsManager* settings);

    // JSON serialization
    QJsonObject toJson() const override;

    /**
     * @brief Create a SongItem from JSON
     * @param json JSON object with type="song"
     * @param library Song library for looking up songs
     * @param settings Settings manager for CCLI display (optional)
     * @return New SongItem or nullptr on error
     */
    static SongItem* fromJson(const QJsonObject& json, SongLibrary* library, SettingsManager* settings = nullptr);

private slots:
    /**
     * @brief Handle song updates from the library
     */
    void onSongUpdated(int id);

    /**
     * @brief Handle song removal from the library
     */
    void onSongRemoved(int id);

private:
    int m_songId;
    SongLibrary* m_songLibrary;
    SettingsManager* m_settingsManager;

    // Generation options
    bool m_includeTitleSlide;
    bool m_includeSectionLabels;
    int m_maxLinesPerSlide;

    // Custom section order (per-presentation)
    QList<int> m_sectionOrder;
    bool m_hasCustomSectionOrder;
};

} // namespace Clarity
