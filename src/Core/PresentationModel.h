#pragma once

#include "Presentation.h"
#include <QAbstractListModel>
#include <QMimeData>
#include <QObject>
#include <QStringList>

namespace Clarity {

/**
 * @brief Qt Model adapter for Presentation to use with QListView
 *
 * Provides the data interface for displaying slides in the control window.
 * Works with the flat slide list from Presentation for backward compatibility.
 *
 * Note: This model exposes the flat slide list. For item-based views,
 * a separate ItemModel should be created.
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
        SlideObjectRole,     ///< Returns the full Slide object as QVariant
        ItemIndexRole,       ///< Item index containing this slide
        SlideInItemRole,     ///< Position within the item (0-based)
        ItemNameRole,        ///< Display name of containing item
        ItemTypeRole,        ///< Type of containing item
        FlatIndexRole,       ///< The row index in the source model (for proxy model support)
        GroupLabelRole,      ///< Section label (e.g., "Verse 1") for song slides
        GroupIndexRole       ///< Section order position (-1 = title/no group)
    };

    explicit PresentationModel(QObject* parent = nullptr);
    ~PresentationModel();

    // QAbstractItemModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Drag-and-drop support for reordering slides within a SlideGroupItem
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
                         int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                      int row, int column, const QModelIndex& parent) override;

    // Presentation access
    /**
     * @brief Set the presentation (takes ownership)
     *
     * The model takes ownership of the presentation and will delete it
     * when a new one is set or the model is destroyed.
     */
    void setPresentation(Presentation* presentation);

    /**
     * @brief Get the presentation
     */
    Presentation* presentation() const { return m_presentation; }

    /**
     * @brief Create a copy of the presentation data for serialization
     *
     * This is used when saving - it returns a JSON object representing
     * the current state.
     */
    QJsonObject presentationToJson() const;

    // Navigation helpers
    int currentSlideIndex() const;
    void setCurrentSlideIndex(int index);

    // Slide manipulation (flat index based)
    void addSlide(const Slide& slide);
    void insertSlide(int index, const Slide& slide);
    void updateSlide(int index, const Slide& slide);
    void removeSlide(int index);
    void moveSlide(int fromIndex, int toIndex);
    Slide getSlide(int index) const;

    // Item management (for new item-based UI)
    /**
     * @brief Add an item to the presentation
     */
    void addItem(PresentationItem* item);

    /**
     * @brief Insert an item at a specific position
     */
    void insertItem(int index, PresentationItem* item);

    /**
     * @brief Remove an item
     */
    void removeItem(int index);

    /**
     * @brief Get item count
     */
    int itemCount() const;

    /**
     * @brief Get an item by index
     */
    PresentationItem* itemAt(int index) const;

    /**
     * @brief Notify that a group item's slides changed (added/removed/reordered)
     *
     * Call this after directly modifying a SlideGroupItem's slides to trigger
     * a model reset so views reflect the new slide count.
     */
    void notifyGroupItemChanged();

signals:
    void currentSlideChanged(int index);
    void presentationModified();
    void itemsChanged();
    void aboutToMutate(const QString& description);

private slots:
    void onSlidesChanged();
    void onCurrentSlideChanged(int index);
    void onPresentationModified();

private:
    Presentation* m_presentation;
    bool m_ownsPresentation;
};

} // namespace Clarity
