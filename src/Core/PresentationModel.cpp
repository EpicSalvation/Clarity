// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "PresentationModel.h"
#include "ScriptureItem.h"
#include "SlideGroupItem.h"
#include "SongItem.h"
#include <QIODevice>

namespace Clarity {

PresentationModel::PresentationModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_presentation(nullptr)
    , m_ownsPresentation(false)
    , m_resetting(false)
{
    // Create a default empty presentation
    m_presentation = new Presentation(this);
    m_ownsPresentation = true;

    connect(m_presentation, &Presentation::slidesChanged,
            this, &PresentationModel::onSlidesChanged);
    connect(m_presentation, &Presentation::currentSlideChanged,
            this, &PresentationModel::onCurrentSlideChanged);
    connect(m_presentation, &Presentation::presentationModified,
            this, &PresentationModel::onPresentationModified);
    connect(m_presentation, &Presentation::itemsChanged,
            this, &PresentationModel::itemsChanged);
}

PresentationModel::~PresentationModel()
{
    // Presentation is a QObject child, will be deleted automatically
}

int PresentationModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_presentation) {
        return 0;
    }
    return m_presentation->totalSlideCount();
}

QVariant PresentationModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_presentation ||
        index.row() >= m_presentation->totalSlideCount()) {
        return QVariant();
    }

    int flatIndex = index.row();
    Slide slide = m_presentation->resolvedSlideAt(flatIndex);
    SlidePosition pos = m_presentation->positionForFlatIndex(flatIndex);

    switch (role) {
    case TextRole:
        return slide.text();
    case BackgroundColorRole:
        return slide.backgroundColor();
    case TextColorRole:
        return slide.textColor();
    case FontFamilyRole:
        return slide.fontFamily();
    case FontSizeRole:
        return slide.fontSize();
    case SlideObjectRole:
        // Return the full Slide object for custom delegates
        return QVariant::fromValue(slide);
    case ItemIndexRole:
        return pos.itemIndex;
    case SlideInItemRole:
        return pos.slideInItem;
    case ItemNameRole:
        if (pos.isValid()) {
            PresentationItem* item = m_presentation->itemAt(pos.itemIndex);
            if (item) {
                return item->displayName();
            }
        }
        return QString();
    case ItemTypeRole:
        if (pos.isValid()) {
            PresentationItem* item = m_presentation->itemAt(pos.itemIndex);
            if (item) {
                return static_cast<int>(item->type());
            }
        }
        return -1;
    case FlatIndexRole:
        // Return the source model row index (for proxy model support)
        return flatIndex;
    case GroupLabelRole:
        return slide.groupLabel();
    case GroupIndexRole:
        return slide.groupIndex();
    case Qt::DisplayRole:
        // For default list view display, show truncated text
        return slide.text().left(50) + (slide.text().length() > 50 ? "..." : "");
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PresentationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TextRole] = "text";
    roles[BackgroundColorRole] = "backgroundColor";
    roles[TextColorRole] = "textColor";
    roles[FontFamilyRole] = "fontFamily";
    roles[FontSizeRole] = "fontSize";
    roles[ItemIndexRole] = "itemIndex";
    roles[SlideInItemRole] = "slideInItem";
    roles[ItemNameRole] = "itemName";
    roles[ItemTypeRole] = "itemType";
    roles[FlatIndexRole] = "flatIndex";
    roles[GroupLabelRole] = "groupLabel";
    roles[GroupIndexRole] = "groupIndex";
    return roles;
}

Qt::ItemFlags PresentationModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (!index.isValid() || !m_presentation) {
        return defaultFlags | Qt::ItemIsDropEnabled;
    }

    // Allow dragging slides that belong to a SlideGroupItem, SongItem, or ScriptureItem
    SlidePosition pos = m_presentation->positionForFlatIndex(index.row());
    if (pos.isValid()) {
        PresentationItem* item = m_presentation->itemAt(pos.itemIndex);
        if (qobject_cast<SlideGroupItem*>(item) || qobject_cast<SongItem*>(item)
            || qobject_cast<ScriptureItem*>(item)) {
            return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
    }
    return defaultFlags;
}

Qt::DropActions PresentationModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList PresentationModel::mimeTypes() const
{
    return { "application/x-clarity-slide-index" };
}

QMimeData* PresentationModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex& index : indexes) {
        if (index.isValid()) {
            stream << index.row();  // flat slide index
        }
    }

    mimeData->setData("application/x-clarity-slide-index", encodedData);
    return mimeData;
}

bool PresentationModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                         int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(column);

    if (!data->hasFormat("application/x-clarity-slide-index") || !m_presentation) {
        return false;
    }

    // Decode source row
    QByteArray encodedData = data->data("application/x-clarity-slide-index");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int sourceRow = -1;
    if (!stream.atEnd()) {
        stream >> sourceRow;
    }
    if (sourceRow < 0) return false;

    SlidePosition sourcePos = m_presentation->positionForFlatIndex(sourceRow);
    if (!sourcePos.isValid()) return false;

    // Only allow drop within the same item (SlideGroupItem, SongItem, or ScriptureItem)
    int targetRow = (row >= 0) ? row : (parent.isValid() ? parent.row() : -1);
    if (targetRow < 0) return false;

    // Resolve which item the target belongs to.
    // targetRow may point to the first slide of the NEXT item (or past the end)
    // when the user wants to insert after the last slide of the current group.
    int total = m_presentation->totalSlideCount();
    int lookupRow = qMin(targetRow, total - 1);
    SlidePosition targetPos = m_presentation->positionForFlatIndex(lookupRow);
    if (!targetPos.isValid()) return false;

    // If target lands in a different item, check if the previous row is in the
    // source item — that means "insert at end of group"
    if (sourcePos.itemIndex != targetPos.itemIndex && targetRow > 0) {
        SlidePosition prevPos = m_presentation->positionForFlatIndex(targetRow - 1);
        if (!prevPos.isValid() || prevPos.itemIndex != sourcePos.itemIndex) {
            return false;
        }
    } else if (sourcePos.itemIndex != targetPos.itemIndex) {
        return false;
    }

    PresentationItem* sourceItem = m_presentation->itemAt(sourcePos.itemIndex);
    return qobject_cast<SlideGroupItem*>(sourceItem) || qobject_cast<SongItem*>(sourceItem)
        || qobject_cast<ScriptureItem*>(sourceItem);
}

bool PresentationModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                      int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if (action == Qt::IgnoreAction) return true;
    if (!data->hasFormat("application/x-clarity-slide-index") || !m_presentation) return false;

    emit aboutToMutate(tr("Reorder Slides"));

    // Decode source row
    QByteArray encodedData = data->data("application/x-clarity-slide-index");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int sourceRow = -1;
    if (!stream.atEnd()) {
        stream >> sourceRow;
    }
    if (sourceRow < 0) return false;

    // Determine target row
    int targetRow;
    if (row >= 0) {
        targetRow = row;
    } else if (parent.isValid()) {
        targetRow = parent.row();
    } else {
        targetRow = m_presentation->totalSlideCount();
    }

    // Verify both are in the same SlideGroupItem
    SlidePosition sourcePos = m_presentation->positionForFlatIndex(sourceRow);
    if (!sourcePos.isValid()) return false;

    // Clamp targetRow to valid range
    int total = m_presentation->totalSlideCount();
    if (targetRow > total) {
        targetRow = total;
    }

    // Resolve which item the target belongs to.
    // targetRow may point to the first slide of the NEXT item (or past the end)
    // when the user wants to insert after the last slide of the current group.
    int lookupRow = qMin(targetRow, total - 1);
    SlidePosition targetPos = m_presentation->positionForFlatIndex(lookupRow);
    if (!targetPos.isValid()) return false;

    if (sourcePos.itemIndex != targetPos.itemIndex && targetRow > 0) {
        SlidePosition prevPos = m_presentation->positionForFlatIndex(targetRow - 1);
        if (!prevPos.isValid() || prevPos.itemIndex != sourcePos.itemIndex) return false;
        targetPos = prevPos;  // Use the previous position for item lookup below
    } else if (sourcePos.itemIndex != targetPos.itemIndex) {
        return false;
    }

    PresentationItem* item = m_presentation->itemAt(sourcePos.itemIndex);
    int itemIndex = sourcePos.itemIndex;

    // Adjust for removal shift when dragging downward
    if (sourceRow < targetRow) {
        targetRow--;
    }
    if (sourceRow == targetRow) return false;

    // SongItem: section-level drag-drop (move entire sections)
    if (SongItem* songItem = qobject_cast<SongItem*>(item)) {
        int fromSection = songItem->sectionOrderIndexForSlide(sourcePos.slideInItem);
        if (fromSection < 0) return false;  // Can't drag title slide

        // Determine target section
        SlidePosition adjustedTarget = m_presentation->positionForFlatIndex(targetRow);
        if (!adjustedTarget.isValid() || adjustedTarget.itemIndex != itemIndex) return false;
        int toSection = songItem->sectionOrderIndexForSlide(adjustedTarget.slideInItem);
        if (toSection < 0) toSection = 0;  // Dragging to title slide position = first section

        if (fromSection == toSection) return false;

        m_resetting = true;
        beginResetModel();
        songItem->moveSongSection(fromSection, toSection);
        endResetModel();
        m_resetting = false;

        emit presentationModified();
        return true;
    }

    // ScriptureItem: convert to SlideGroupItem and move (existing behavior)
    if (qobject_cast<ScriptureItem*>(item)) {
        QList<Slide> slides = item->cachedSlides();
        QString name = item->displayName();

        SlidePosition adjustedTarget = m_presentation->positionForFlatIndex(targetRow);
        if (!adjustedTarget.isValid() || adjustedTarget.itemIndex != itemIndex) return false;

        slides.move(sourcePos.slideInItem, adjustedTarget.slideInItem);

        m_resetting = true;
        beginResetModel();
        m_presentation->removeItem(itemIndex);
        auto* groupItem = new SlideGroupItem(name, slides);
        m_presentation->insertItem(itemIndex, groupItem);
        endResetModel();
        m_resetting = false;

        setCurrentSlideIndex(targetRow);
        emit presentationModified();
        return true;
    }

    if (!qobject_cast<SlideGroupItem*>(item)) return false;

    // SlideGroupItem — use moveSlide which handles within-group moves properly
    moveSlide(sourceRow, targetRow);
    setCurrentSlideIndex(targetRow);

    return true;
}

void PresentationModel::setPresentation(Presentation* presentation)
{
    if (m_presentation == presentation) {
        return;
    }

    beginResetModel();

    // Disconnect from old presentation
    if (m_presentation) {
        disconnect(m_presentation, nullptr, this, nullptr);
        if (m_ownsPresentation) {
            delete m_presentation;
        }
    }

    m_presentation = presentation;
    m_ownsPresentation = (presentation != nullptr && presentation->parent() == this);

    // Connect to new presentation
    if (m_presentation) {
        if (!m_presentation->parent()) {
            m_presentation->setParent(this);
            m_ownsPresentation = true;
        }

        connect(m_presentation, &Presentation::slidesChanged,
                this, &PresentationModel::onSlidesChanged);
        connect(m_presentation, &Presentation::currentSlideChanged,
                this, &PresentationModel::onCurrentSlideChanged);
        connect(m_presentation, &Presentation::presentationModified,
                this, &PresentationModel::onPresentationModified);
        connect(m_presentation, &Presentation::itemsChanged,
                this, &PresentationModel::itemsChanged);
    }

    endResetModel();
    emit presentationModified();
    emit itemsChanged();
}

QJsonObject PresentationModel::presentationToJson() const
{
    if (!m_presentation) {
        return QJsonObject();
    }
    return m_presentation->toJson();
}

int PresentationModel::currentSlideIndex() const
{
    return m_presentation ? m_presentation->currentSlideIndex() : 0;
}

void PresentationModel::setCurrentSlideIndex(int index)
{
    if (m_presentation) {
        m_presentation->setCurrentSlideIndex(index);
    }
}

void PresentationModel::addSlide(const Slide& slide)
{
    if (!m_presentation) return;

    int index = m_presentation->totalSlideCount();
    beginInsertRows(QModelIndex(), index, index);
    m_presentation->addSlide(slide);
    endInsertRows();
}

void PresentationModel::insertSlide(int index, const Slide& slide)
{
    if (!m_presentation) return;
    if (index < 0 || index > m_presentation->totalSlideCount()) {
        return;
    }

    beginInsertRows(QModelIndex(), index, index);
    m_presentation->insertSlide(index, slide);
    endInsertRows();
}

void PresentationModel::updateSlide(int index, const Slide& slide)
{
    if (!m_presentation) return;
    if (index < 0 || index >= m_presentation->totalSlideCount()) {
        return;
    }

    m_presentation->updateSlide(index, slide);
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
}

void PresentationModel::removeSlide(int index)
{
    if (!m_presentation) return;
    if (index < 0 || index >= m_presentation->totalSlideCount()) {
        return;
    }

    // Use model reset instead of beginRemoveRows: removing a slide from a
    // SlideGroupItem emits slidesChanged which would nest inside beginRemoveRows
    m_resetting = true;
    beginResetModel();
    m_presentation->removeSlide(index);
    endResetModel();
    m_resetting = false;
    emit presentationModified();
}

void PresentationModel::moveSlide(int fromIndex, int toIndex)
{
    if (!m_presentation) return;
    if (fromIndex < 0 || fromIndex >= m_presentation->totalSlideCount() ||
        toIndex < 0 || toIndex >= m_presentation->totalSlideCount() ||
        fromIndex == toIndex) {
        return;
    }

    // Check if this is a within-group move (simple row move) or a cross-item
    // move (structural change that removes/inserts items)
    SlidePosition fromPos = m_presentation->positionForFlatIndex(fromIndex);
    SlidePosition toPos = m_presentation->positionForFlatIndex(toIndex);

    bool isSimpleMove = fromPos.isValid() && toPos.isValid()
        && fromPos.itemIndex == toPos.itemIndex
        && qobject_cast<SlideGroupItem*>(m_presentation->itemAt(fromPos.itemIndex));

    // Block presentation signals to avoid nested model resets —
    // Presentation::moveSlide emits slidesChanged(), which would trigger
    // onSlidesChanged() → beginResetModel() while we're already inside
    // beginMoveRows or beginResetModel.
    m_resetting = true;
    if (isSimpleMove) {
        // Within same SlideGroupItem — safe to use beginMoveRows/endMoveRows
        int destIndex = (toIndex > fromIndex) ? toIndex + 1 : toIndex;
        if (!beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), destIndex)) {
            m_resetting = false;
            return;
        }
        m_presentation->moveSlide(fromIndex, toIndex);
        endMoveRows();
    } else {
        // Cross-item move restructures the item list, use model reset
        beginResetModel();
        m_presentation->moveSlide(fromIndex, toIndex);
        endResetModel();
    }
    m_resetting = false;
}

Slide PresentationModel::getSlide(int index) const
{
    if (!m_presentation) return Slide();
    return m_presentation->slideAt(index);
}

// Item management

void PresentationModel::addItem(PresentationItem* item)
{
    if (!m_presentation || !item) return;

    // Guard flag prevents onSlidesChanged() from nesting a reset
    // while still allowing Presentation signals to reach other models
    // (e.g., ItemListModel needs itemsChanged to update the playlist)
    m_resetting = true;
    beginResetModel();
    m_presentation->addItem(item);
    endResetModel();
    m_resetting = false;
}

void PresentationModel::insertItem(int index, PresentationItem* item)
{
    if (!m_presentation || !item) return;

    // Guard flag prevents onSlidesChanged() from nesting a reset
    // while still allowing Presentation signals to reach other models
    m_resetting = true;
    beginResetModel();
    m_presentation->insertItem(index, item);
    endResetModel();
    m_resetting = false;
}

void PresentationModel::removeItem(int index)
{
    if (!m_presentation) return;
    if (index < 0 || index >= m_presentation->itemCount()) return;

    // Guard flag prevents onSlidesChanged() from nesting a reset
    // while still allowing Presentation signals to reach other models
    m_resetting = true;
    beginResetModel();
    m_presentation->removeItem(index);
    endResetModel();
    m_resetting = false;
}

int PresentationModel::itemCount() const
{
    return m_presentation ? m_presentation->itemCount() : 0;
}

PresentationItem* PresentationModel::itemAt(int index) const
{
    return m_presentation ? m_presentation->itemAt(index) : nullptr;
}

void PresentationModel::notifyGroupItemChanged()
{
    if (!m_presentation) return;

    m_presentation->invalidateFlatIndex();
    beginResetModel();
    endResetModel();
}

// Private slots

void PresentationModel::onSlidesChanged()
{
    // Skip if we're already inside a beginResetModel/endResetModel block
    // (e.g., from addItem/insertItem/removeItem which triggered this signal)
    if (m_resetting) return;

    // When slides change in a way we didn't directly cause,
    // reset the model to be safe
    beginResetModel();
    endResetModel();
}

void PresentationModel::onCurrentSlideChanged(int index)
{
    emit currentSlideChanged(index);
}

void PresentationModel::onPresentationModified()
{
    emit presentationModified();
}

} // namespace Clarity
