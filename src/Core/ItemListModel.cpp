// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "ItemListModel.h"
#include <QIODevice>

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

Qt::ItemFlags ItemListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsDragEnabled;
    }
    return defaultFlags | Qt::ItemIsDropEnabled;
}

Qt::DropActions ItemListModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList ItemListModel::mimeTypes() const
{
    return { "application/x-clarity-item-index" };
}

QMimeData* ItemListModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex& index : indexes) {
        if (index.isValid()) {
            stream << index.row();
        }
    }

    mimeData->setData("application/x-clarity-item-index", encodedData);
    return mimeData;
}

bool ItemListModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                  int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if (action == Qt::IgnoreAction) return true;
    if (!data->hasFormat("application/x-clarity-item-index")) return false;
    if (!m_presentation) return false;

    emit aboutToMutate(tr("Reorder Items"));

    // Decode the source row
    QByteArray encodedData = data->data("application/x-clarity-item-index");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int sourceRow = -1;
    if (!stream.atEnd()) {
        stream >> sourceRow;
    }
    if (sourceRow < 0) return false;

    // Determine the target row
    int targetRow;
    if (row >= 0) {
        targetRow = row;
    } else if (parent.isValid()) {
        targetRow = parent.row();
    } else {
        targetRow = m_presentation->itemCount();
    }

    // Adjust target if dragging downward (source removal shifts indices)
    if (sourceRow < targetRow) {
        targetRow--;
    }

    if (sourceRow == targetRow) return false;

    m_presentation->moveItem(sourceRow, targetRow);
    return true;
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
