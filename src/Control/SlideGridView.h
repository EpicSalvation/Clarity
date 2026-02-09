#pragma once

#include <QListView>

namespace Clarity {

/**
 * @brief Custom QListView subclass that provides insert-between drag-and-drop
 * and media-onto-slide drag-and-drop.
 *
 * For slide reordering (application/x-clarity-slide-index): draws a vertical
 * blue insertion line between items.
 *
 * For media drops (application/x-clarity-media-path): highlights the target
 * slide with a colored border (green=single, blue=group).
 */
class SlideGridView : public QListView {
    Q_OBJECT

public:
    explicit SlideGridView(QWidget* parent = nullptr);

signals:
    /**
     * @brief Emitted when a media item is dropped onto a slide.
     * @param index The proxy model index of the target slide
     * @param path Filesystem path of the media file
     * @param mediaType "image" or "video"
     * @param applyToGroup If true, apply to all slides in the target's group/item
     */
    void mediaDropped(const QModelIndex& index, const QString& path,
                      const QString& mediaType, bool applyToGroup);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    /**
     * @brief Calculate the insertion index for a drop at the given position.
     *
     * Iterates visible items and finds the gap closest to the cursor.
     * Returns 0 to rowCount, where rowCount means "after the last item".
     */
    int insertionIndexAt(const QPoint& pos) const;

    int m_dropIndicatorIndex = -1;      ///< Proxy row where the insertion line appears (-1 = hidden)
    QModelIndex m_mediaHighlightIndex;  ///< Proxy index of slide being targeted by media drag
    bool m_mediaApplyToGroup = false;   ///< Whether the current media drag targets the whole group
};

} // namespace Clarity
