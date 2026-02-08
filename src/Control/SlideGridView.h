#pragma once

#include <QListView>

namespace Clarity {

/**
 * @brief Custom QListView subclass that provides insert-between drag-and-drop.
 *
 * Instead of Qt's default IconMode drop behavior (dropping "on top of" items),
 * this view calculates the insertion gap between items during a drag and draws
 * a vertical blue line to indicate where the slide will be inserted.
 */
class SlideGridView : public QListView {
    Q_OBJECT

public:
    explicit SlideGridView(QWidget* parent = nullptr);

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

    int m_dropIndicatorIndex = -1;  ///< Proxy row where the insertion line appears (-1 = hidden)
};

} // namespace Clarity
