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

} // namespace Clarity
