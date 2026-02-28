// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Slide.h"
#include "PresentationItem.h"
#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>

namespace Clarity {

// Forward declarations
class SongLibrary;
class BibleDatabase;
class SettingsManager;
class ThemeManager;

/**
 * @brief Position information for a slide within the presentation
 *
 * Maps a flat slide index to its item and position within that item.
 */
struct SlidePosition {
    int itemIndex;      ///< Index of the item containing this slide
    int slideInItem;    ///< Zero-based position within the item
    int flatIndex;      ///< Absolute flat index in presentation

    SlidePosition() : itemIndex(-1), slideInItem(-1), flatIndex(-1) {}
    SlidePosition(int item, int slide, int flat)
        : itemIndex(item), slideInItem(slide), flatIndex(flat) {}

    bool isValid() const { return itemIndex >= 0 && slideInItem >= 0 && flatIndex >= 0; }
};

/**
 * @brief Represents a complete presentation as a playlist of items
 *
 * A Presentation is a collection of PresentationItems (songs, scripture,
 * custom slides, slide groups). Each item generates one or more slides,
 * and navigation operates on the flat list of all slides.
 *
 * This design allows:
 * - Item-level operations (apply theme to all slides in a song)
 * - Source tracking (which song/scripture generated a slide)
 * - Logical grouping in the UI
 * - Flat navigation for seamless presentation flow
 *
 * Version 2.0 file format stores items; version 1.0 files are migrated
 * by wrapping all slides in a single SlideGroupItem.
 */
class Presentation : public QObject {
    Q_OBJECT

public:
    explicit Presentation(QObject* parent = nullptr);
    explicit Presentation(const QString& title, QObject* parent = nullptr);
    ~Presentation();

    // Basic properties
    QString title() const { return m_title; }
    void setTitle(const QString& title);

    // === Item management ===

    /**
     * @brief Get all items in the presentation
     */
    QList<PresentationItem*> items() const { return m_items; }

    /**
     * @brief Get the number of items
     */
    int itemCount() const { return m_items.count(); }

    /**
     * @brief Get an item by index
     */
    PresentationItem* itemAt(int index) const;

    /**
     * @brief Add an item to the end of the presentation
     *
     * The Presentation takes ownership of the item.
     */
    void addItem(PresentationItem* item);

    /**
     * @brief Insert an item at a specific position
     *
     * The Presentation takes ownership of the item.
     */
    void insertItem(int index, PresentationItem* item);

    /**
     * @brief Remove and delete an item at a specific position
     */
    void removeItem(int index);

    /**
     * @brief Move an item from one position to another
     */
    void moveItem(int from, int to);

    /**
     * @brief Remove all items
     */
    void clearItems();

    // === Flat slide access (for navigation and IPC) ===

    /**
     * @brief Get the total number of slides across all items
     */
    int totalSlideCount() const;

    /**
     * @brief Get a slide by its flat index
     */
    Slide slideAt(int flatIndex) const;

    /**
     * @brief Get a slide with cascading background resolution applied
     *
     * If cascading backgrounds is enabled and the slide does not have an
     * explicit background, walks backward to find the nearest explicit
     * background and copies it. Also applies scripture theme overrides.
     */
    Slide resolvedSlideAt(int flatIndex) const;

    /**
     * @brief Check if a virtual copyright slide should be appended
     *
     * Returns true when the "Show copyright slide" setting is enabled AND
     * at least one item in the presentation has copyright data.
     */
    bool hasCopyrightSlide() const;

    /**
     * @brief Generate the virtual copyright slide
     *
     * Collects copyright/attribution data from all items and produces
     * a single summary slide.
     */
    Slide generateCopyrightSlide() const;

    /**
     * @brief Set the settings manager for cascading background settings
     */
    void setSettingsManager(SettingsManager* mgr) { m_settingsManager = mgr; }

    /**
     * @brief Set the theme manager for scripture theme overrides
     */
    void setThemeManager(ThemeManager* mgr) { m_themeManager = mgr; }

    /**
     * @brief Get position information for a flat index
     */
    SlidePosition positionForFlatIndex(int flatIndex) const;

    /**
     * @brief Get flat index for an item/slide position
     */
    int flatIndexForPosition(int itemIndex, int slideInItem) const;

    // === Navigation (flat index based) ===

    /**
     * @brief Get the current flat slide index
     */
    int currentSlideIndex() const { return m_currentSlideIndex; }

    /**
     * @brief Set the current flat slide index
     */
    void setCurrentSlideIndex(int index);

    /**
     * @brief Move to the next slide
     * @return true if moved, false if already at end
     */
    bool nextSlide();

    /**
     * @brief Move to the previous slide
     * @return true if moved, false if already at beginning
     */
    bool prevSlide();

    /**
     * @brief Go to a specific slide
     * @return true if index was valid
     */
    bool gotoSlide(int index);

    /**
     * @brief Get the current slide
     */
    Slide currentSlide() const;

    // === Current item info (for confidence monitor) ===

    /**
     * @brief Get the item containing the current slide
     */
    PresentationItem* currentItem() const;

    /**
     * @brief Get the current slide's position within its item (0-based)
     */
    int currentSlideInItem() const;

    /**
     * @brief Get the total slides in the current item
     */
    int slidesInCurrentItem() const;

    // === Legacy flat slide API (for backward compatibility) ===
    // These methods are kept for code that hasn't migrated to items yet

    /**
     * @brief Get all slides as a flat list
     * @deprecated Use items() and navigate via flat index instead
     */
    QList<Slide> slides() const;

    /**
     * @brief Get slide count (alias for totalSlideCount)
     * @deprecated Use totalSlideCount() instead
     */
    int slideCount() const { return totalSlideCount(); }

    /**
     * @brief Get a slide by index (alias for slideAt)
     * @deprecated Use slideAt() instead
     */
    Slide getSlide(int index) const { return slideAt(index); }

    /**
     * @brief Add a single slide (wraps in CustomSlideItem)
     * @deprecated Use addItem() with appropriate item type
     */
    void addSlide(const Slide& slide);

    /**
     * @brief Insert a single slide (wraps in CustomSlideItem)
     * @deprecated Use insertItem() with appropriate item type
     */
    void insertSlide(int flatIndex, const Slide& slide);

    /**
     * @brief Update a slide at flat index
     *
     * Note: Only works for CustomSlideItem and SlideGroupItem.
     * For SongItem/ScriptureItem, edit the source instead.
     */
    void updateSlide(int flatIndex, const Slide& slide);

    /**
     * @brief Remove a slide at flat index
     *
     * If the slide is the only one in its item, removes the item.
     * For multi-slide items, only removes from SlideGroupItem.
     */
    void removeSlide(int flatIndex);

    /**
     * @brief Move a slide (limited support)
     *
     * Only fully supported within SlideGroupItems. Cross-item moves
     * are converted to remove+insert.
     */
    void moveSlide(int fromIndex, int toIndex);

    /**
     * @brief Clear all slides (alias for clearItems)
     * @deprecated Use clearItems() instead
     */
    void clearSlides() { clearItems(); }

    // === JSON serialization ===

    /**
     * @brief Serialize to JSON (v2.0 format)
     */
    QJsonObject toJson() const;

    /**
     * @brief Deserialize from JSON (v1.0 or v2.0 format)
     * @param json JSON object
     * @param songLibrary Song library for SongItem deserialization (optional)
     * @param bibleDatabase Bible database for ScriptureItem deserialization (optional)
     * @param settingsManager Settings manager for red letter settings (optional)
     * @return New Presentation (caller takes ownership)
     */
    static Presentation* fromJson(const QJsonObject& json,
                                   SongLibrary* songLibrary = nullptr,
                                   BibleDatabase* bibleDatabase = nullptr,
                                   SettingsManager* settingsManager = nullptr,
                                   QObject* parent = nullptr);

    /**
     * @brief Invalidate the flat index cache
     *
     * Call after directly modifying a group item's slides with signals blocked.
     */
    void invalidateFlatIndex() { m_flatIndexValid = false; }

signals:
    /**
     * @brief Emitted when an item is added
     */
    void itemAdded(int index);

    /**
     * @brief Emitted when an item is removed
     */
    void itemRemoved(int index);

    /**
     * @brief Emitted when an item is moved
     */
    void itemMoved(int from, int to);

    /**
     * @brief Emitted when items change (add/remove/move/item content change)
     */
    void itemsChanged();

    /**
     * @brief Emitted when the flat slide list changes
     */
    void slidesChanged();

    /**
     * @brief Emitted when the current slide changes
     */
    void currentSlideChanged(int flatIndex);

    /**
     * @brief Emitted when any modification is made
     */
    void presentationModified();

private slots:
    /**
     * @brief Handle item changes that affect slide cache
     */
    void onItemChanged();

private:
    /**
     * @brief Rebuild the flat index cache
     *
     * Call this when items are added/removed/moved or when
     * an item's slide count changes.
     */
    void rebuildFlatIndex() const;

    /**
     * @brief Connect signals for a newly added item
     */
    void connectItemSignals(PresentationItem* item);

    QString m_title;
    QList<PresentationItem*> m_items;
    int m_currentSlideIndex;

    // Cascading backgrounds support
    SettingsManager* m_settingsManager = nullptr;
    ThemeManager* m_themeManager = nullptr;

    // Cached flat index: m_itemStartIndices[i] is the flat index of item i's first slide
    // This enables O(log n) lookup via binary search
    mutable QList<int> m_itemStartIndices;
    mutable int m_cachedTotalSlides;
    mutable bool m_flatIndexValid;
};

} // namespace Clarity
