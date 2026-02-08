#include "SlideGridView.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QMimeData>

namespace Clarity {

SlideGridView::SlideGridView(QWidget* parent)
    : QListView(parent)
{
}

void SlideGridView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-clarity-slide-index")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void SlideGridView::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-clarity-slide-index")) {
        event->ignore();
        return;
    }

    int insertionIdx = insertionIndexAt(event->position().toPoint());

    // Validate with model's canDropMimeData using the insertion index
    // Map the proxy insertion index to a source row for validation
    if (model() && model()->canDropMimeData(
            event->mimeData(), event->dropAction(),
            insertionIdx, 0, QModelIndex())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    if (m_dropIndicatorIndex != insertionIdx) {
        m_dropIndicatorIndex = insertionIdx;
        viewport()->update();
    }
}

void SlideGridView::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event);
    m_dropIndicatorIndex = -1;
    viewport()->update();
}

void SlideGridView::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-clarity-slide-index")) {
        event->ignore();
        return;
    }

    int insertionIdx = insertionIndexAt(event->position().toPoint());

    // Perform the drop at the calculated insertion index
    if (model() && model()->dropMimeData(
            event->mimeData(), event->dropAction(),
            insertionIdx, 0, QModelIndex())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    m_dropIndicatorIndex = -1;
    viewport()->update();
}

void SlideGridView::paintEvent(QPaintEvent* event)
{
    // Draw the base list view first
    QListView::paintEvent(event);

    // Draw insertion indicator line
    int count = model() ? model()->rowCount() : 0;
    if (m_dropIndicatorIndex < 0 || m_dropIndicatorIndex > count) {
        return;
    }

    QRect itemRect;
    bool drawAtRightEdge = false;

    if (m_dropIndicatorIndex < count) {
        // Draw at the left edge of the item at this index
        itemRect = visualRect(model()->index(m_dropIndicatorIndex, 0));
    } else {
        // Insert after last item — draw at the right edge of the last item
        if (count > 0) {
            itemRect = visualRect(model()->index(count - 1, 0));
            drawAtRightEdge = true;
        } else {
            return;  // No items, nothing to draw
        }
    }

    if (!itemRect.isValid()) {
        return;
    }

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPen pen(QColor("#3b82f6"), 3);
    painter.setPen(pen);

    int x = drawAtRightEdge ? itemRect.right() + 1 : itemRect.left() - 1;
    painter.drawLine(x, itemRect.top(), x, itemRect.bottom());
}

int SlideGridView::insertionIndexAt(const QPoint& pos) const
{
    if (!model()) {
        return 0;
    }

    int count = model()->rowCount();
    if (count == 0) {
        return 0;
    }

    // Find the closest insertion gap by comparing cursor X against item centers.
    // For a wrapped grid, we first identify which row the cursor is on, then
    // find the gap within that row.

    // Collect the visual rects of all items
    int bestIndex = count;  // Default: after the last item
    int bestDistance = INT_MAX;

    for (int i = 0; i < count; i++) {
        QRect rect = visualRect(model()->index(i, 0));
        if (!rect.isValid()) {
            continue;
        }

        // Check if cursor is in this item's row (vertically overlapping)
        if (pos.y() >= rect.top() && pos.y() <= rect.bottom()) {
            // Cursor is in this row — compare X position against item center
            int centerX = rect.center().x();

            // Distance to the left edge (inserting before this item)
            int distLeft = qAbs(pos.x() - rect.left());
            if (distLeft < bestDistance) {
                bestDistance = distLeft;
                bestIndex = i;
            }

            // Distance to the right edge (inserting after this item)
            int distRight = qAbs(pos.x() - rect.right());
            if (distRight < bestDistance) {
                bestDistance = distRight;
                bestIndex = i + 1;
            }
        }
    }

    // If cursor is below all items, insert at the end
    if (bestDistance == INT_MAX) {
        // Check if cursor is below the last row
        QRect lastRect = visualRect(model()->index(count - 1, 0));
        if (lastRect.isValid() && pos.y() > lastRect.bottom()) {
            return count;
        }
        // Above all items — insert at beginning
        return 0;
    }

    return bestIndex;
}

} // namespace Clarity
