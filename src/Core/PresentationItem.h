#pragma once

#include "Slide.h"
#include "Song.h"  // For SlideStyle
#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QMap>
#include <QUuid>

namespace Clarity {

// Forward declarations
class SongLibrary;
class BibleDatabase;

/**
 * @brief Abstract base class for items in a presentation playlist
 *
 * A PresentationItem represents a logical unit in a presentation (a song,
 * scripture passage, or custom slides). Each item can generate one or more
 * slides on demand, enabling:
 * - Item-level theming (apply style to all slides in item)
 * - Source tracking (reference back to song ID, scripture reference)
 * - Logical grouping (navigate by item or by slide)
 *
 * Items cache their generated slides for performance and invalidate
 * the cache when source data or style changes.
 */
class PresentationItem : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Type of presentation item
     */
    enum ItemType {
        SongItemType,           ///< Song from the library
        ScriptureItemType,      ///< Bible scripture passage
        CustomSlideItemType,    ///< Single custom slide
        SlideGroupItemType,     ///< Group of slides (for v1.0 migration)
        EsvScriptureItemType,   ///< ESV API scripture passage (purgeable cache)
        ApiBibleScriptureItemType ///< API.bible scripture passage
    };
    Q_ENUM(ItemType)

    explicit PresentationItem(QObject* parent = nullptr);
    virtual ~PresentationItem() = default;

    /**
     * @brief Get the type of this item
     */
    virtual ItemType type() const = 0;

    /**
     * @brief Get the type name as a string (for JSON serialization)
     */
    QString typeName() const;

    /**
     * @brief Get the display name for this item (shown in item list)
     * @return Display name (e.g., "Amazing Grace", "John 3:16")
     */
    virtual QString displayName() const = 0;

    /**
     * @brief Get optional subtitle for this item
     * @return Subtitle (e.g., "by John Newton", "KJV") or empty string
     */
    virtual QString displaySubtitle() const { return QString(); }

    /**
     * @brief Generate slides for this item
     *
     * This is called internally to populate the cache. Subclasses implement
     * the actual slide generation logic here.
     *
     * @return List of slides for this item
     */
    virtual QList<Slide> generateSlides() const = 0;

    /**
     * @brief Get cached slides (regenerates if cache is invalid)
     * @return List of slides for this item
     */
    QList<Slide> cachedSlides() const;

    /**
     * @brief Get the number of slides this item generates
     */
    int slideCount() const;

    /**
     * @brief Invalidate the slide cache
     *
     * Call this when item properties change that affect slide generation.
     */
    void invalidateSlideCache();

    // Per-item theming
    /**
     * @brief Check if this item has a custom style override
     */
    bool hasCustomStyle() const { return m_hasCustomStyle; }

    /**
     * @brief Get the item's style (custom or default)
     */
    SlideStyle itemStyle() const { return m_itemStyle; }

    /**
     * @brief Set a custom style for this item
     */
    void setItemStyle(const SlideStyle& style);

    /**
     * @brief Clear custom style and use default
     */
    void clearCustomStyle();

    // Per-slide style overrides (for overriding individual slides within an item)
    /**
     * @brief Set a style override for a specific slide index within this item
     */
    void setSlideStyleOverride(int slideIndex, const SlideStyle& style);

    /**
     * @brief Clear the style override for a specific slide index
     */
    void clearSlideStyleOverride(int slideIndex);

    /**
     * @brief Check if a specific slide has a style override
     */
    bool hasSlideStyleOverride(int slideIndex) const { return m_perSlideStyles.contains(slideIndex); }

    /**
     * @brief Get all per-slide style overrides
     */
    QMap<int, SlideStyle> slideStyleOverrides() const { return m_perSlideStyles; }

    // Auto-advance timer (group-level default)
    /**
     * @brief Get the default auto-advance duration for slides in this item (seconds, 0 = disabled)
     */
    int defaultAutoAdvanceDuration() const { return m_defaultAutoAdvanceDuration; }

    /**
     * @brief Set the default auto-advance duration for slides in this item
     * @param seconds Duration in seconds (0 = disabled)
     */
    void setDefaultAutoAdvanceDuration(int seconds);

    /**
     * @brief Get the effective auto-advance duration for a slide
     *
     * Returns the per-slide override if set (> 0), otherwise the item-level default.
     * @param slideIndex Index of the slide within this item
     * @return Duration in seconds (0 = disabled)
     */
    int effectiveAutoAdvanceDuration(int slideIndex) const;

    // Unique identifier
    /**
     * @brief Get the unique identifier for this item
     */
    QString uuid() const { return m_uuid; }

    /**
     * @brief Set the unique identifier (used during deserialization)
     */
    void setUuid(const QString& uuid) { m_uuid = uuid; }

    // JSON serialization
    /**
     * @brief Serialize this item to JSON
     *
     * Subclasses should call baseToJson() first, then add their own fields.
     */
    virtual QJsonObject toJson() const;

    /**
     * @brief Deserialize an item from JSON
     * @param json JSON object
     * @param songLibrary Song library for SongItem (optional)
     * @param bibleDatabase Bible database for ScriptureItem (optional)
     * @return New PresentationItem (caller takes ownership), or nullptr on error
     */
    static PresentationItem* fromJson(const QJsonObject& json,
                                       SongLibrary* songLibrary = nullptr,
                                       BibleDatabase* bibleDatabase = nullptr);

signals:
    /**
     * @brief Emitted when the item changes (slides need regeneration)
     */
    void itemChanged();

    /**
     * @brief Emitted when the slide cache is invalidated
     */
    void slideCacheInvalidated();

protected:
    /**
     * @brief Serialize base class fields to JSON
     *
     * Called by subclasses in their toJson() implementation.
     */
    QJsonObject baseToJson() const;

    /**
     * @brief Apply base class fields from JSON
     *
     * Called by subclasses after construction during fromJson().
     */
    void applyBaseJson(const QJsonObject& json);

    /**
     * @brief Apply item style to a slide
     *
     * Used by subclasses when generating slides to apply the item's style.
     */
    void applyStyleToSlide(Slide& slide) const;

    QString m_uuid;
    SlideStyle m_itemStyle;
    bool m_hasCustomStyle;
    QMap<int, SlideStyle> m_perSlideStyles;  ///< Per-slide style overrides
    int m_defaultAutoAdvanceDuration;        ///< Group-level auto-advance (seconds, 0 = disabled)

    // Slide cache
    mutable QList<Slide> m_cachedSlides;
    mutable bool m_cacheValid;
};

} // namespace Clarity
