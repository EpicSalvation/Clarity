// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SlideFilterProxyModel.h"
#include "PresentationModel.h"
#include <QMimeData>

namespace Clarity {

SlideFilterProxyModel::SlideFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_filterItemIndex(-1)
    , m_showAllSlides(false)
{
}

void SlideFilterProxyModel::setFilterItemIndex(int itemIndex)
{
    if (m_filterItemIndex != itemIndex) {
        beginFilterChange();
        m_filterItemIndex = itemIndex;
        endFilterChange();
    }
}

void SlideFilterProxyModel::setShowAllSlides(bool showAll)
{
    if (m_showAllSlides != showAll) {
        beginFilterChange();
        m_showAllSlides = showAll;
        endFilterChange();
    }
}

bool SlideFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent)

    // If showing all slides, accept everything
    if (m_showAllSlides || m_filterItemIndex < 0) {
        return true;
    }

    // Get the item index for this row
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    int itemIndex = index.data(PresentationModel::ItemIndexRole).toInt();

    return itemIndex == m_filterItemIndex;
}

int SlideFilterProxyModel::mapRowToSource(int proxyRow) const
{
    int count = rowCount();
    if (proxyRow < count) {
        return mapToSource(index(proxyRow, 0)).row();
    }
    // Past the end: source row after the last mapped item
    if (count > 0) {
        return mapToSource(index(count - 1, 0)).row() + 1;
    }
    return 0;
}

bool SlideFilterProxyModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                             int row, int column, const QModelIndex& parent) const
{
    int sourceRow = (row >= 0) ? mapRowToSource(row) : row;
    QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->canDropMimeData(data, action, sourceRow, column, sourceParent);
}

bool SlideFilterProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                          int row, int column, const QModelIndex& parent)
{
    int sourceRow = (row >= 0) ? mapRowToSource(row) : row;
    QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->dropMimeData(data, action, sourceRow, column, sourceParent);
}

} // namespace Clarity
