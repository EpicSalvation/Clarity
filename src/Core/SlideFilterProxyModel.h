#pragma once

#include <QSortFilterProxyModel>

namespace Clarity {

/**
 * @brief Proxy model that filters slides to show only those from a specific item
 *
 * When an item index is set, only slides belonging to that item are shown.
 * When set to -1 (or showAllSlides is true), all slides are shown.
 */
class SlideFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit SlideFilterProxyModel(QObject* parent = nullptr);

    /**
     * @brief Set the item index to filter by
     * @param itemIndex Index of the item to show slides for, or -1 for all
     */
    void setFilterItemIndex(int itemIndex);

    /**
     * @brief Get the current filter item index
     */
    int filterItemIndex() const { return m_filterItemIndex; }

    /**
     * @brief Set whether to show all slides regardless of item
     */
    void setShowAllSlides(bool showAll);

    /**
     * @brief Get whether showing all slides
     */
    bool showAllSlides() const { return m_showAllSlides; }

    // Override drop methods to map proxy rows to source rows.
    // QAbstractProxyModel passes 'row' unmapped, which breaks when filtering.
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
                         int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                      int row, int column, const QModelIndex& parent) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int mapRowToSource(int proxyRow) const;

private:
    int m_filterItemIndex;
    bool m_showAllSlides;
};

} // namespace Clarity
