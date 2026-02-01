#pragma once

#include "Core/Slide.h"
#include <QStyledItemDelegate>
#include <QHash>
#include <QPixmap>
#include <QSize>

namespace Clarity {

/**
 * @brief Custom delegate for rendering slide thumbnails in a grid view
 *
 * Renders slide previews as thumbnails with:
 * - Proper background rendering (solid, gradient, image)
 * - Slide number overlay
 * - Selection highlight
 * - "Live" indicator for currently displayed slide
 */
class SlideGridDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SlideGridDelegate(QObject* parent = nullptr);

    /**
     * @brief Paint the slide thumbnail
     */
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    /**
     * @brief Return the size hint for thumbnails
     */
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    /**
     * @brief Set the thumbnail size (maintains 16:9 aspect ratio)
     */
    void setThumbnailSize(const QSize& size);

    /**
     * @brief Get the current thumbnail size
     */
    QSize thumbnailSize() const { return m_thumbnailSize; }

    /**
     * @brief Set the index of the currently displayed slide
     *
     * This slide will be drawn with a special "live" indicator
     */
    void setCurrentSlideIndex(int index);

    /**
     * @brief Get the current slide index
     */
    int currentSlideIndex() const { return m_currentSlideIndex; }

    /**
     * @brief Invalidate the thumbnail cache
     *
     * Call this when slides are modified to force re-rendering
     */
    void invalidateCache();

    /**
     * @brief Invalidate a specific cached thumbnail
     */
    void invalidateCacheFor(int index);

    /**
     * @brief Set the red letter color for scripture slides
     */
    void setRedLetterColor(const QString& color);

private:
    QSize m_thumbnailSize;          ///< Size of each thumbnail (default 160x90)
    int m_currentSlideIndex;        ///< Index of currently displayed slide
    int m_spacing;                  ///< Spacing around thumbnails
    QString m_redLetterColor;       ///< Color for red letter text

    // Mutable cache for thumbnails (can be modified in const paint method)
    mutable QHash<int, QPixmap> m_thumbnailCache;
};

} // namespace Clarity
