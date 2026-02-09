#include "SlideGridDelegate.h"
#include "Core/PresentationModel.h"
#include "Core/SlidePreviewRenderer.h"
#include <QPainter>

namespace Clarity {

// Color palette for section types, derived from the label prefix.
// Duplicated sections share the same label → same color.
static QColor sectionColor(const QString& label)
{
    QString lower = label.toLower();
    if (lower.startsWith("verse"))      return QColor("#3b82f6"); // blue
    if (lower.startsWith("chorus"))     return QColor("#22c55e"); // green
    if (lower.startsWith("bridge"))     return QColor("#f59e0b"); // amber
    if (lower.startsWith("pre-chorus")) return QColor("#a855f7"); // purple
    if (lower.startsWith("tag"))        return QColor("#ec4899"); // pink
    if (lower.startsWith("intro"))      return QColor("#14b8a6"); // teal
    if (lower.startsWith("outro"))      return QColor("#ef4444"); // red
    if (lower.startsWith("ending"))     return QColor("#ef4444"); // red
    if (lower.startsWith("interlude"))  return QColor("#8b5cf6"); // violet
    if (lower.startsWith("vamp"))       return QColor("#f97316"); // orange
    return QColor("#6b7280"); // gray fallback
}

SlideGridDelegate::SlideGridDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_thumbnailSize(160, 90)  // 16:9 aspect ratio
    , m_currentSlideIndex(-1)
    , m_spacing(10)
    , m_redLetterColor("#cc0000")
{
}

void SlideGridDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }

    painter->save();

    // Get the slide from the model
    QVariant slideVar = index.data(PresentationModel::SlideObjectRole);
    if (!slideVar.isValid() || !slideVar.canConvert<Slide>()) {
        painter->restore();
        return;
    }

    Slide slide = slideVar.value<Slide>();

    // Get the flat index from the source model (works correctly with proxy models)
    QVariant flatIndexVar = index.data(PresentationModel::FlatIndexRole);
    int slideIndex = flatIndexVar.isValid() ? flatIndexVar.toInt() : index.row();

    // Calculate the thumbnail rect centered in the item rect
    QRect itemRect = option.rect;
    QRect thumbnailRect(
        itemRect.left() + (itemRect.width() - m_thumbnailSize.width()) / 2,
        itemRect.top() + m_spacing / 2,
        m_thumbnailSize.width(),
        m_thumbnailSize.height()
    );

    // Check cache for existing thumbnail (keyed by flat index)
    QPixmap thumbnail;
    if (m_thumbnailCache.contains(slideIndex)) {
        thumbnail = m_thumbnailCache.value(slideIndex);
    } else {
        // Render and cache the thumbnail
        SlidePreviewRenderer::RenderOptions renderOptions;
        renderOptions.showSlideNumber = true;
        renderOptions.slideNumber = slideIndex + 1;  // 1-based display
        renderOptions.redLetterColor = m_redLetterColor;

        thumbnail = SlidePreviewRenderer::render(slide, m_thumbnailSize, renderOptions);
        m_thumbnailCache.insert(slideIndex, thumbnail);
    }

    // Draw the thumbnail
    painter->drawPixmap(thumbnailRect, thumbnail);

    // Draw section color-coded banner for song slides
    QString groupLabel = index.data(PresentationModel::GroupLabelRole).toString();
    int groupIndex = index.data(PresentationModel::GroupIndexRole).toInt();

    if (groupIndex >= 0 && !groupLabel.isEmpty()) {
        QColor color = sectionColor(groupLabel);

        // Check if this is the first slide in its section
        bool isFirstInSection = true;
        if (index.row() > 0) {
            QModelIndex prevIndex = index.sibling(index.row() - 1, 0);
            if (prevIndex.isValid()) {
                int prevGroupIndex = prevIndex.data(PresentationModel::GroupIndexRole).toInt();
                if (prevGroupIndex == groupIndex) {
                    isFirstInSection = false;
                }
            }
        }

        if (isFirstInSection) {
            // Full-width color-coded banner overlaying the top of the thumbnail
            int bannerHeight = qMax(14, m_thumbnailSize.height() / 6);
            QRect bannerRect(thumbnailRect.left(), thumbnailRect.top(),
                             thumbnailRect.width(), bannerHeight);

            // Semi-transparent colored background so slide content peeks through
            QColor bannerColor = color;
            bannerColor.setAlpha(210);
            painter->fillRect(bannerRect, bannerColor);

            // Section label text
            int fontSize = qMax(8, bannerHeight - 5);
            QFont bannerFont("Arial", fontSize);
            bannerFont.setBold(true);
            painter->setFont(bannerFont);
            painter->setPen(Qt::white);
            painter->drawText(bannerRect, Qt::AlignCenter, groupLabel);
        } else {
            // Continuation slides: thin color strip at top for visual continuity
            QRect stripRect(thumbnailRect.left(), thumbnailRect.top(),
                            thumbnailRect.width(), 3);
            painter->fillRect(stripRect, color);
        }
    }

    // Draw selection/current indicators on top
    bool isSelected = option.state & QStyle::State_Selected;
    bool isCurrentSlide = (slideIndex == m_currentSlideIndex);

    if (isCurrentSlide) {
        // Draw "live" indicator (green border)
        QPen pen(QColor("#22c55e"));
        pen.setWidth(4);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(thumbnailRect.adjusted(2, 2, -2, -2));
    } else if (isSelected) {
        // Draw selection highlight (blue border)
        QPen pen(QColor("#3b82f6"));
        pen.setWidth(3);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(thumbnailRect.adjusted(1, 1, -1, -1));
    }

    // Draw focus indicator if the item has focus
    if (option.state & QStyle::State_HasFocus) {
        QPen focusPen(Qt::DotLine);
        focusPen.setColor(QColor("#60a5fa"));
        painter->setPen(focusPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(thumbnailRect.adjusted(-2, -2, 2, 2));
    }

    painter->restore();
}

QSize SlideGridDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    // Return thumbnail size plus spacing
    return QSize(m_thumbnailSize.width() + m_spacing,
                 m_thumbnailSize.height() + m_spacing + 5);  // Extra for number label
}

void SlideGridDelegate::setThumbnailSize(const QSize& size)
{
    if (m_thumbnailSize != size) {
        m_thumbnailSize = size;
        invalidateCache();
    }
}

void SlideGridDelegate::setCurrentSlideIndex(int index)
{
    if (m_currentSlideIndex != index) {
        int oldIndex = m_currentSlideIndex;
        m_currentSlideIndex = index;

        // We don't need to invalidate cache for current slide changes
        // since the indicator is drawn on top of the cached thumbnail
        Q_UNUSED(oldIndex);
    }
}

void SlideGridDelegate::invalidateCache()
{
    m_thumbnailCache.clear();
}

void SlideGridDelegate::invalidateCacheFor(int index)
{
    m_thumbnailCache.remove(index);
}

void SlideGridDelegate::setRedLetterColor(const QString& color)
{
    if (m_redLetterColor != color) {
        m_redLetterColor = color;
        invalidateCache();  // Re-render thumbnails with new color
    }
}

} // namespace Clarity
