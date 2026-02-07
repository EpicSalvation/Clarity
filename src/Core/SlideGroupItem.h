#pragma once

#include "PresentationItem.h"
#include "Slide.h"
#include <QList>

namespace Clarity {

/**
 * @brief A presentation item containing a group of arbitrary slides
 *
 * SlideGroupItem is used for:
 * - Migrating v1.0 presentations (all slides become one SlideGroupItem)
 * - Grouping related slides that aren't songs or scripture
 * - Copy/paste operations that include multiple slides
 *
 * Unlike SongItem or ScriptureItem, slides are stored directly rather
 * than being generated from a source.
 */
class SlideGroupItem : public PresentationItem {
    Q_OBJECT

public:
    explicit SlideGroupItem(QObject* parent = nullptr);
    explicit SlideGroupItem(const QString& name, QObject* parent = nullptr);
    SlideGroupItem(const QString& name, const QList<Slide>& slides, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return SlideGroupItemType; }
    QString displayName() const override;
    QList<Slide> generateSlides() const override;

    // Group name
    /**
     * @brief Get the group name
     */
    QString name() const { return m_name; }

    /**
     * @brief Set the group name
     */
    void setName(const QString& name);

    // Slide management
    /**
     * @brief Get all slides in this group
     */
    QList<Slide> slides() const { return m_slides; }

    /**
     * @brief Set all slides in this group
     */
    void setSlides(const QList<Slide>& slides);

    /**
     * @brief Add a slide to the end of the group
     */
    void addSlide(const Slide& slide);

    /**
     * @brief Insert a slide at a specific position
     */
    void insertSlide(int index, const Slide& slide);

    /**
     * @brief Update a slide at a specific position
     */
    void updateSlide(int index, const Slide& slide);

    /**
     * @brief Permanently apply the group custom style to each individual slide
     *
     * After baking, m_hasCustomStyle is cleared so generateSlides() returns
     * m_slides as-is, allowing individual slide changes to stick.
     */
    void bakeCustomStyle();

    /**
     * @brief Remove a slide at a specific position
     */
    void removeSlide(int index);

    /**
     * @brief Move a slide from one position to another
     */
    void moveSlide(int from, int to);

    /**
     * @brief Get a slide at a specific position
     */
    Slide slideAt(int index) const;

    // JSON serialization
    QJsonObject toJson() const override;

    /**
     * @brief Create a SlideGroupItem from JSON
     * @param json JSON object with type="slideGroup"
     * @return New SlideGroupItem or nullptr on error
     */
    static SlideGroupItem* fromJson(const QJsonObject& json);

private:
    QString m_name;
    QList<Slide> m_slides;
};

} // namespace Clarity
