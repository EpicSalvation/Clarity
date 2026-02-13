#pragma once

#include "Presentation.h"
#include "PresentationItem.h"
#include <QAbstractListModel>
#include <QMimeData>
#include <QStringList>

namespace Clarity {

/**
 * @brief Qt Model adapter for displaying presentation items in a list view
 *
 * This model exposes items (songs, scriptures, slide groups) rather than
 * individual slides, allowing the left panel to show logical groupings.
 */
class ItemListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ItemRoles {
        DisplayNameRole = Qt::UserRole + 1,
        SubtitleRole,
        ItemTypeRole,
        SlideCountRole,
        ItemPointerRole
    };

    explicit ItemListModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Drag-and-drop support for reordering items
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                      int row, int column, const QModelIndex& parent) override;

    /**
     * @brief Set the presentation to display items from
     */
    void setPresentation(Presentation* presentation);

    /**
     * @brief Get the current presentation
     */
    Presentation* presentation() const { return m_presentation; }

    /**
     * @brief Get an item by model index
     */
    PresentationItem* itemAt(int index) const;

    /**
     * @brief Get the index of an item containing the given flat slide index
     */
    int itemIndexForSlide(int flatSlideIndex) const;

signals:
    void itemSelected(int itemIndex);
    void aboutToMutate(const QString& description);

private slots:
    void onItemsChanged();

private:
    Presentation* m_presentation;
};

} // namespace Clarity
