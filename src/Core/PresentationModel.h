#pragma once

#include "Presentation.h"
#include <QAbstractListModel>
#include <QObject>

namespace Clarity {

/**
 * @brief Qt Model adapter for Presentation to use with QListView
 *
 * Provides the data interface for displaying slides in the control window
 */
class PresentationModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum SlideRoles {
        TextRole = Qt::UserRole + 1,
        BackgroundColorRole,
        TextColorRole,
        FontFamilyRole,
        FontSizeRole,
        SlideObjectRole  ///< Returns the full Slide object as QVariant
    };

    explicit PresentationModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Presentation access
    void setPresentation(const Presentation& presentation);
    Presentation presentation() const { return m_presentation; }

    // Navigation helpers
    int currentSlideIndex() const { return m_presentation.currentSlideIndex(); }
    void setCurrentSlideIndex(int index);

    // Slide manipulation
    void addSlide(const Slide& slide);
    void insertSlide(int index, const Slide& slide);
    void updateSlide(int index, const Slide& slide);
    void removeSlide(int index);
    void moveSlide(int fromIndex, int toIndex);
    Slide getSlide(int index) const;

signals:
    void currentSlideChanged(int index);
    void presentationModified();

private:
    Presentation m_presentation;
};

} // namespace Clarity
