// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "PresentationItem.h"
#include "Slide.h"

namespace Clarity {

/**
 * @brief A presentation item containing a single custom slide
 *
 * CustomSlideItem is used for standalone slides that aren't part of a
 * song or scripture passage. Common uses:
 * - Welcome/announcement slides
 * - Image-only slides
 * - Custom text slides
 *
 * The slide is stored directly in the item and can be edited in place.
 */
class CustomSlideItem : public PresentationItem {
    Q_OBJECT

public:
    explicit CustomSlideItem(QObject* parent = nullptr);
    explicit CustomSlideItem(const Slide& slide, QObject* parent = nullptr);

    // PresentationItem interface
    ItemType type() const override { return CustomSlideItemType; }
    QString displayName() const override;
    QList<Slide> generateSlides() const override;

    // Slide access
    /**
     * @brief Get the contained slide
     */
    Slide slide() const { return m_slide; }

    /**
     * @brief Set/update the contained slide
     */
    void setSlide(const Slide& slide);

    // JSON serialization
    QJsonObject toJson() const override;

    /**
     * @brief Create a CustomSlideItem from JSON
     * @param json JSON object with type="customSlide"
     * @return New CustomSlideItem or nullptr on error
     */
    static CustomSlideItem* fromJson(const QJsonObject& json);

private:
    Slide m_slide;
};

} // namespace Clarity
