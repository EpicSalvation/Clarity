#include "PresentationModel.h"

namespace Clarity {

PresentationModel::PresentationModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int PresentationModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_presentation.slideCount();
}

QVariant PresentationModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_presentation.slideCount()) {
        return QVariant();
    }

    const Slide& slide = m_presentation.slides().at(index.row());

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
    return roles;
}

void PresentationModel::setPresentation(const Presentation& presentation)
{
    beginResetModel();
    m_presentation = presentation;
    endResetModel();
    emit presentationModified();
}

void PresentationModel::setCurrentSlideIndex(int index)
{
    int oldIndex = m_presentation.currentSlideIndex();
    m_presentation.setCurrentSlideIndex(index);
    if (oldIndex != m_presentation.currentSlideIndex()) {
        emit currentSlideChanged(m_presentation.currentSlideIndex());
    }
}

void PresentationModel::addSlide(const Slide& slide)
{
    int index = m_presentation.slideCount();
    beginInsertRows(QModelIndex(), index, index);
    m_presentation.addSlide(slide);
    endInsertRows();
    emit presentationModified();
}

void PresentationModel::insertSlide(int index, const Slide& slide)
{
    if (index < 0 || index > m_presentation.slideCount()) {
        return;
    }
    beginInsertRows(QModelIndex(), index, index);
    m_presentation.insertSlide(index, slide);
    endInsertRows();
    emit presentationModified();
}

void PresentationModel::updateSlide(int index, const Slide& slide)
{
    if (index < 0 || index >= m_presentation.slideCount()) {
        return;
    }
    m_presentation.updateSlide(index, slide);
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit presentationModified();
}

void PresentationModel::removeSlide(int index)
{
    if (index < 0 || index >= m_presentation.slideCount()) {
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    m_presentation.removeSlide(index);
    endRemoveRows();
    emit presentationModified();
}

void PresentationModel::moveSlide(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_presentation.slideCount() ||
        toIndex < 0 || toIndex >= m_presentation.slideCount() ||
        fromIndex == toIndex) {
        return;
    }

    // Use beginMoveRows/endMoveRows for proper model notification
    // Note: Qt's move semantics require special handling of the destination index
    int destIndex = (toIndex > fromIndex) ? toIndex + 1 : toIndex;

    if (!beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), destIndex)) {
        return;
    }

    m_presentation.moveSlide(fromIndex, toIndex);
    endMoveRows();
    emit presentationModified();
}

Slide PresentationModel::getSlide(int index) const
{
    return m_presentation.getSlide(index);
}

} // namespace Clarity
