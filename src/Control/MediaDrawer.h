#pragma once

#include "Core/MediaLibrary.h"
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
 * @brief Collapsible media drawer that sits at the bottom of the control window
 *
 * Has a clickable toggle bar that's always visible at the bottom of the screen.
 * Clicking it expands/collapses the thumbnail grid content. The toggle bar shows
 * a chevron and "Media" label. When expanded, shows Images/Videos tabs and
 * a thumbnail grid with import capability.
 */
class MediaDrawer : public QWidget {
    Q_OBJECT

public:
    explicit MediaDrawer(MediaLibrary* library,
                         VideoThumbnailGenerator* thumbnailGen,
                         QWidget* parent = nullptr);
    ~MediaDrawer();

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);

signals:
    void expandedChanged(bool expanded);

private slots:
    void onToggleClicked();
    void onTabChanged();
    void onImportClicked();
    void onMediaAdded(const MediaLibrary::MediaItem& item);
    void onMediaRemoved(const QString& libraryPath);
    void onThumbnailReady(const QString& videoPath, const QImage& thumbnail);

private:
    void setupUI();
    void populateList();
    void updateToggleBar();

    MediaLibrary* m_library;
    VideoThumbnailGenerator* m_thumbnailGen;
    MediaLibrary::MediaType m_currentType = MediaLibrary::Image;
    bool m_expanded = false;

    // Toggle bar (always visible)
    QPushButton* m_toggleBar;

    // Content area (collapsible)
    QWidget* m_contentWidget;
    MediaListWidget* m_listWidget;
    QPushButton* m_imagesButton;
    QPushButton* m_videosButton;
    QPushButton* m_importButton;
};

} // namespace Clarity
