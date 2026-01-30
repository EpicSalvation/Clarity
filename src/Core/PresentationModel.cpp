#include "PresentationModel.h"

namespace Clarity {

PresentationModel::PresentationModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_presentation(nullptr)
    , m_ownsPresentation(false)
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
    Slide slide = m_presentation->slideAt(flatIndex);
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
    return roles;
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

    beginRemoveRows(QModelIndex(), index, index);
    m_presentation->removeSlide(index);
    endRemoveRows();
}

void PresentationModel::moveSlide(int fromIndex, int toIndex)
{
    if (!m_presentation) return;
    if (fromIndex < 0 || fromIndex >= m_presentation->totalSlideCount() ||
        toIndex < 0 || toIndex >= m_presentation->totalSlideCount() ||
        fromIndex == toIndex) {
        return;
    }

    // Use beginMoveRows/endMoveRows for proper model notification
    // Note: Qt's move semantics require special handling of the destination index
    int destIndex = (toIndex > fromIndex) ? toIndex + 1 : toIndex;

    if (!beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), destIndex)) {
        return;
    }

    m_presentation->moveSlide(fromIndex, toIndex);
    endMoveRows();
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

    // Calculate the flat indices that will be added
    int startIndex = m_presentation->totalSlideCount();
    int slideCount = item->slideCount();

    if (slideCount > 0) {
        beginInsertRows(QModelIndex(), startIndex, startIndex + slideCount - 1);
    }

    m_presentation->addItem(item);

    if (slideCount > 0) {
        endInsertRows();
    }
}

void PresentationModel::insertItem(int index, PresentationItem* item)
{
    if (!m_presentation || !item) return;
    if (index < 0) index = 0;
    if (index > m_presentation->itemCount()) index = m_presentation->itemCount();

    // Calculate the flat index where slides will be inserted
    int flatIndex = m_presentation->flatIndexForPosition(index, 0);
    if (flatIndex < 0) {
        flatIndex = m_presentation->totalSlideCount();
    }

    int slideCount = item->slideCount();

    if (slideCount > 0) {
        beginInsertRows(QModelIndex(), flatIndex, flatIndex + slideCount - 1);
    }

    m_presentation->insertItem(index, item);

    if (slideCount > 0) {
        endInsertRows();
    }
}

void PresentationModel::removeItem(int index)
{
    if (!m_presentation) return;
    if (index < 0 || index >= m_presentation->itemCount()) return;

    // Calculate the flat indices that will be removed
    int flatIndex = m_presentation->flatIndexForPosition(index, 0);
    PresentationItem* item = m_presentation->itemAt(index);
    int slideCount = item ? item->slideCount() : 0;

    if (slideCount > 0 && flatIndex >= 0) {
        beginRemoveRows(QModelIndex(), flatIndex, flatIndex + slideCount - 1);
    }

    m_presentation->removeItem(index);

    if (slideCount > 0 && flatIndex >= 0) {
        endRemoveRows();
    }
}

int PresentationModel::itemCount() const
{
    return m_presentation ? m_presentation->itemCount() : 0;
}

PresentationItem* PresentationModel::itemAt(int index) const
{
    return m_presentation ? m_presentation->itemAt(index) : nullptr;
}

// Private slots

void PresentationModel::onSlidesChanged()
{
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
