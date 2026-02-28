// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/MediaLibrary.h"
#include "Core/SlideGroupLibrary.h"
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>

namespace Clarity {

class VideoThumbnailGenerator;

/**
 * @brief QListWidget subclass that produces custom drag mime data for media items.
 *
 * Left-click drag sets applyToGroup=false (single slide).
 * Right-click drag sets applyToGroup=true (whole group).
 */
class MediaListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit MediaListWidget(QWidget* parent = nullptr);

    void setCurrentMediaType(MediaLibrary::MediaType type) { m_mediaType = type; }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void startMediaDrag(bool applyToGroup);

    QPoint m_dragStartPos;
    Qt::MouseButton m_dragButton = Qt::NoButton;
    MediaLibrary::MediaType m_mediaType = MediaLibrary::Image;
};

/**
 * @brief Collapsible library drawer that sits at the bottom of the control window
 *
 * Has a clickable toggle bar that's always visible at the bottom of the screen.
 * Clicking it expands/collapses the content. The toggle bar shows a chevron and
 * "Library" label. When expanded, shows Images/Videos/Slide Groups tabs.
 */
class MediaDrawer : public QWidget {
    Q_OBJECT

public:
    explicit MediaDrawer(MediaLibrary* library,
                         VideoThumbnailGenerator* thumbnailGen,
                         SlideGroupLibrary* slideGroupLibrary,
                         QWidget* parent = nullptr);
    ~MediaDrawer();

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);

    enum DrawerTab { ImagesTab, VideosTab, SlideGroupsTab };

signals:
    void expandedChanged(bool expanded);
    void slideGroupInsertRequested(int groupId);
    void saveGroupToLibraryRequested();

private slots:
    void onToggleClicked();
    void onTabChanged();
    void onImportClicked();
    void onSaveGroupClicked();
    void onMediaAdded(const MediaLibrary::MediaItem& item);
    void onMediaRemoved(const QString& libraryPath);
    void onThumbnailReady(const QString& videoPath, const QImage& thumbnail);
    void onGroupDoubleClicked(QListWidgetItem* item);
    void onGroupContextMenu(const QPoint& pos);
    void onGroupAdded(int id);
    void onGroupRemoved(int id);
    void onGroupUpdated(int id);

private:
    void setupUI();
    void populateList();
    void populateSlideGroupList();
    void updateToggleBar();
    void switchToTab(DrawerTab tab);

    MediaLibrary* m_library;
    VideoThumbnailGenerator* m_thumbnailGen;
    SlideGroupLibrary* m_slideGroupLibrary;
    MediaLibrary::MediaType m_currentType = MediaLibrary::Image;
    DrawerTab m_currentTab = ImagesTab;
    bool m_expanded = false;

    // Toggle bar (always visible)
    QPushButton* m_toggleBar;

    // Content area (collapsible)
    QWidget* m_contentWidget;
    MediaListWidget* m_listWidget;
    QPushButton* m_imagesButton;
    QPushButton* m_videosButton;
    QPushButton* m_slideGroupsButton;
    QPushButton* m_importButton;
    QPushButton* m_saveGroupButton;

    // Slide groups list
    QListWidget* m_slideGroupListWidget;
};

} // namespace Clarity
