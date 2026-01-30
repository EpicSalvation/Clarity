#include "ItemListModel.h"

namespace Clarity {

ItemListModel::ItemListModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_presentation(nullptr)
{
}

int ItemListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_presentation) {
        return 0;
    }
    return m_presentation->itemCount();
}

QVariant ItemListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_presentation ||
        index.row() >= m_presentation->itemCount()) {
        return QVariant();
    }

    PresentationItem* item = m_presentation->itemAt(index.row());
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return item->displayName();

    case SubtitleRole:
        return item->displaySubtitle();

    case ItemTypeRole:
        return static_cast<int>(item->type());

    case SlideCountRole:
        return item->slideCount();

    case ItemPointerRole:
        return QVariant::fromValue(item);

    case Qt::ToolTipRole: {
        QString tooltip = item->displayName();
        QString subtitle = item->displaySubtitle();
        if (!subtitle.isEmpty()) {
            tooltip += "\n" + subtitle;
        }
        tooltip += QString("\n%1 slide(s)").arg(item->slideCount());
        return tooltip;
    }

    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ItemListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DisplayNameRole] = "displayName";
    roles[SubtitleRole] = "subtitle";
    roles[ItemTypeRole] = "itemType";
    roles[SlideCountRole] = "slideCount";
    roles[ItemPointerRole] = "item";
    return roles;
}

void ItemListModel::setPresentation(Presentation* presentation)
{
    if (m_presentation == presentation) {
        return;
    }

    beginResetModel();

    // Disconnect from old presentation
    if (m_presentation) {
        disconnect(m_presentation, nullptr, this, nullptr);
    }

    m_presentation = presentation;

    // Connect to new presentation
    if (m_presentation) {
        connect(m_presentation, &Presentation::itemsChanged,
                this, &ItemListModel::onItemsChanged);
        connect(m_presentation, &Presentation::itemAdded,
                this, &ItemListModel::onItemsChanged);
        connect(m_presentation, &Presentation::itemRemoved,
                this, &ItemListModel::onItemsChanged);
        connect(m_presentation, &Presentation::itemMoved,
                this, &ItemListModel::onItemsChanged);
    }

    endResetModel();
}

PresentationItem* ItemListModel::itemAt(int index) const
{
    if (!m_presentation || index < 0 || index >= m_presentation->itemCount()) {
        return nullptr;
    }
    return m_presentation->itemAt(index);
}

int ItemListModel::itemIndexForSlide(int flatSlideIndex) const
{
    if (!m_presentation) {
        return -1;
    }
    SlidePosition pos = m_presentation->positionForFlatIndex(flatSlideIndex);
    return pos.isValid() ? pos.itemIndex : -1;
}

void ItemListModel::onItemsChanged()
{
    beginResetModel();
    endResetModel();
}

} // namespace Clarity
