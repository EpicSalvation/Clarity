#include "SlideFilterProxyModel.h"
#include "PresentationModel.h"

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
        m_filterItemIndex = itemIndex;
        invalidateFilter();
    }
}

void SlideFilterProxyModel::setShowAllSlides(bool showAll)
{
    if (m_showAllSlides != showAll) {
        m_showAllSlides = showAll;
        invalidateFilter();
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

} // namespace Clarity
