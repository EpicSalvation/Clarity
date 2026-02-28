// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "MediaDrawer.h"
#include "AppStyle.h"
#include "Core/VideoThumbnailGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QInputDialog>
#include <QMenu>
#include <QDebug>

namespace Clarity {

// --- MediaListWidget implementation ---

MediaListWidget::MediaListWidget(QWidget* parent)
    : QListWidget(parent)
{
}

void MediaListWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_dragStartPos = event->pos();
        m_dragButton = event->button();
    }
    // Let base class handle left-click selection
    if (event->button() == Qt::LeftButton) {
        QListWidget::mousePressEvent(event);
    }
}

void MediaListWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragButton == Qt::NoButton) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    // Check if mouse has moved far enough to start a drag
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    bool applyToGroup = (m_dragButton == Qt::RightButton);
    startMediaDrag(applyToGroup);
    m_dragButton = Qt::NoButton;
}

void MediaListWidget::startMediaDrag(bool applyToGroup)
{
    QListWidgetItem* item = itemAt(m_dragStartPos);
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    QString mediaType = (m_mediaType == MediaLibrary::Video) ? "video" : "image";

    QJsonObject json;
    json["path"] = path;
    json["mediaType"] = mediaType;
    json["applyToGroup"] = applyToGroup;

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QMimeData* mimeData = new QMimeData();
    mimeData->setData("application/x-clarity-media-path", data);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Use the item's icon as drag pixmap
    QIcon icon = item->icon();
    if (!icon.isNull()) {
        drag->setPixmap(icon.pixmap(64, 48));
    }

    drag->exec(Qt::CopyAction);
}

static const QSize THUMBNAIL_SIZE(120, 90);

/**
 * @brief Create a centered thumbnail within a fixed-size pixmap
 * Scales the source to fit within the target size while maintaining aspect ratio,
 * then centers it on a dark background.
 */
static QPixmap createCenteredThumbnail(const QPixmap& source, const QSize& targetSize = THUMBNAIL_SIZE)
{
    if (source.isNull()) {
        return source;
    }

    QPixmap scaled = source.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap result(targetSize);
    result.fill(QColor(30, 30, 30));

    QPainter painter(&result);
    int x = (targetSize.width() - scaled.width()) / 2;
    int y = (targetSize.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.end();

    return result;
}

MediaDrawer::MediaDrawer(MediaLibrary* library,
                         VideoThumbnailGenerator* thumbnailGen,
                         SlideGroupLibrary* slideGroupLibrary,
                         QWidget* parent)
    : QWidget(parent)
    , m_library(library)
    , m_thumbnailGen(thumbnailGen)
    , m_slideGroupLibrary(slideGroupLibrary)
{
    setupUI();
    populateList();

    // Auto-refresh when library changes (e.g., from MediaLibraryDialog)
    connect(m_library, &MediaLibrary::mediaAdded, this, &MediaDrawer::onMediaAdded);
    connect(m_library, &MediaLibrary::mediaRemoved, this, &MediaDrawer::onMediaRemoved);

    // Update video thumbnails when async generation completes
    if (m_thumbnailGen) {
        connect(m_thumbnailGen, &VideoThumbnailGenerator::thumbnailReady,
                this, &MediaDrawer::onThumbnailReady);
    }

    // Slide group library signals
    if (m_slideGroupLibrary) {
        connect(m_slideGroupLibrary, &SlideGroupLibrary::groupAdded,
                this, &MediaDrawer::onGroupAdded);
        connect(m_slideGroupLibrary, &SlideGroupLibrary::groupRemoved,
                this, &MediaDrawer::onGroupRemoved);
        connect(m_slideGroupLibrary, &SlideGroupLibrary::groupUpdated,
                this, &MediaDrawer::onGroupUpdated);
    }
}

MediaDrawer::~MediaDrawer()
{
    if (m_thumbnailGen) {
        disconnect(m_thumbnailGen, &VideoThumbnailGenerator::thumbnailReady,
                   this, &MediaDrawer::onThumbnailReady);
    }
}

void MediaDrawer::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toggle bar — always visible, clickable to expand/collapse
    m_toggleBar = new QPushButton(this);
    m_toggleBar->setFixedHeight(22);
    m_toggleBar->setCursor(Qt::PointingHandCursor);
    m_toggleBar->setStyleSheet(
        "QPushButton {"
        "  border: none;"
        "  border-top: 1px solid palette(mid);"
        "  background: palette(button);"
        "  padding: 0 8px;"
        "  text-align: left;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background: palette(midlight);"
        "}"
    );
    connect(m_toggleBar, &QPushButton::clicked, this, &MediaDrawer::onToggleClicked);
    updateToggleBar();
    layout->addWidget(m_toggleBar);

    // Content area — hidden when collapsed
    m_contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(4);

    // Header bar with tabs and action buttons
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    // Tab buttons (checkable toggle group)
    m_imagesButton = new QPushButton(tr("Images"), this);
    m_imagesButton->setCheckable(true);
    m_imagesButton->setChecked(true);
    m_imagesButton->setFixedHeight(24);
    connect(m_imagesButton, &QPushButton::clicked, this, &MediaDrawer::onTabChanged);
    headerLayout->addWidget(m_imagesButton);

    m_videosButton = new QPushButton(tr("Videos"), this);
    m_videosButton->setCheckable(true);
    m_videosButton->setFixedHeight(24);
    connect(m_videosButton, &QPushButton::clicked, this, &MediaDrawer::onTabChanged);
    headerLayout->addWidget(m_videosButton);

    m_slideGroupsButton = new QPushButton(tr("Slide Groups"), this);
    m_slideGroupsButton->setCheckable(true);
    m_slideGroupsButton->setFixedHeight(24);
    connect(m_slideGroupsButton, &QPushButton::clicked, this, &MediaDrawer::onTabChanged);
    headerLayout->addWidget(m_slideGroupsButton);

    headerLayout->addStretch();

    // Import button (visible for Images/Videos tabs)
    m_importButton = new QPushButton(tr("Import..."), this);
    m_importButton->setFixedHeight(24);
    connect(m_importButton, &QPushButton::clicked, this, &MediaDrawer::onImportClicked);
    headerLayout->addWidget(m_importButton);

    // Save Group button (visible for Slide Groups tab)
    m_saveGroupButton = new QPushButton(tr("Save to Library"), this);
    m_saveGroupButton->setFixedHeight(24);
    m_saveGroupButton->setVisible(false);
    connect(m_saveGroupButton, &QPushButton::clicked, this, &MediaDrawer::onSaveGroupClicked);
    headerLayout->addWidget(m_saveGroupButton);

    contentLayout->addLayout(headerLayout);

    // Thumbnail grid (custom subclass for drag-and-drop with media mime data)
    m_listWidget = new MediaListWidget(this);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(120, 90));
    m_listWidget->setGridSize(QSize(130, 100));
    m_listWidget->setFlow(QListView::LeftToRight);
    m_listWidget->setWrapping(true);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setWordWrap(false);
    m_listWidget->setDragEnabled(true);

    contentLayout->addWidget(m_listWidget);

    // Slide group list (name-based list, hidden by default)
    m_slideGroupListWidget = new QListWidget(this);
    m_slideGroupListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_slideGroupListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_slideGroupListWidget->setVisible(false);
    connect(m_slideGroupListWidget, &QListWidget::itemDoubleClicked,
            this, &MediaDrawer::onGroupDoubleClicked);
    connect(m_slideGroupListWidget, &QWidget::customContextMenuRequested,
            this, &MediaDrawer::onGroupContextMenu);

    contentLayout->addWidget(m_slideGroupListWidget);

    m_contentWidget->setVisible(false);
    layout->addWidget(m_contentWidget);

    // Start collapsed — only show the toggle bar
    setFixedHeight(m_toggleBar->height());
}

void MediaDrawer::updateToggleBar()
{
    // SVG chevron icon: points down when expanded (click to close), up when collapsed (click to open)
    QIcon chevronIcon = m_expanded ? AppStyle::themedIcon(":/icons/chevron-down.svg") : AppStyle::themedIcon(":/icons/chevron-up.svg");
    m_toggleBar->setIcon(chevronIcon);
    m_toggleBar->setIconSize(QSize(12, 12));
    m_toggleBar->setText(tr("  Library"));
}

void MediaDrawer::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;
    m_contentWidget->setVisible(m_expanded);

    if (m_expanded) {
        // Allow the widget to grow when expanded
        setMaximumHeight(16777215);  // QWIDGETSIZE_MAX
    } else {
        // Collapse to just the toggle bar height
        setFixedHeight(m_toggleBar->height());
    }

    updateToggleBar();
    emit expandedChanged(m_expanded);
}

void MediaDrawer::onToggleClicked()
{
    setExpanded(!m_expanded);
}

void MediaDrawer::switchToTab(DrawerTab tab)
{
    m_currentTab = tab;

    // Update button states
    m_imagesButton->setChecked(tab == ImagesTab);
    m_videosButton->setChecked(tab == VideosTab);
    m_slideGroupsButton->setChecked(tab == SlideGroupsTab);

    // Show/hide appropriate widgets
    bool isMedia = (tab == ImagesTab || tab == VideosTab);
    m_listWidget->setVisible(isMedia);
    m_importButton->setVisible(isMedia);
    m_slideGroupListWidget->setVisible(tab == SlideGroupsTab);
    m_saveGroupButton->setVisible(tab == SlideGroupsTab);

    if (isMedia) {
        m_currentType = (tab == VideosTab) ? MediaLibrary::Video : MediaLibrary::Image;
        m_listWidget->setCurrentMediaType(m_currentType);
        populateList();
    } else {
        populateSlideGroupList();
    }
}

void MediaDrawer::populateList()
{
    m_listWidget->clear();

    if (!m_library) {
        return;
    }

    QList<MediaLibrary::MediaItem> items;
    if (m_currentType == MediaLibrary::Video) {
        items = m_library->getVideos();
    } else {
        items = m_library->getImages();
    }

    for (const MediaLibrary::MediaItem& item : items) {
        if (!QFile::exists(item.libraryPath)) {
            continue;
        }

        QListWidgetItem* listItem = new QListWidgetItem();
        listItem->setData(Qt::UserRole, item.libraryPath);
        listItem->setToolTip(item.filename);

        if (m_currentType == MediaLibrary::Image) {
            QPixmap pixmap(item.libraryPath);
            if (!pixmap.isNull()) {
                listItem->setIcon(QIcon(createCenteredThumbnail(pixmap)));
            }
        } else {
            if (m_thumbnailGen) {
                QPixmap thumbnail = m_thumbnailGen->getThumbnail(item.libraryPath, THUMBNAIL_SIZE);
                listItem->setIcon(QIcon(createCenteredThumbnail(thumbnail)));
            } else {
                listItem->setIcon(QIcon(createCenteredThumbnail(
                    VideoThumbnailGenerator::placeholderThumbnail(THUMBNAIL_SIZE))));
            }
        }

        m_listWidget->addItem(listItem);
    }
}

void MediaDrawer::populateSlideGroupList()
{
    m_slideGroupListWidget->clear();

    if (!m_slideGroupLibrary) {
        return;
    }

    QList<LibrarySlideGroup> groups = m_slideGroupLibrary->allGroups();
    for (const LibrarySlideGroup& group : groups) {
        QString label = QString("%1  (%2 slides)").arg(group.name).arg(group.slides.count());
        QListWidgetItem* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, group.id);
        m_slideGroupListWidget->addItem(item);
    }
}

void MediaDrawer::onTabChanged()
{
    QPushButton* clicked = qobject_cast<QPushButton*>(sender());
    if (clicked == m_imagesButton) {
        switchToTab(ImagesTab);
    } else if (clicked == m_videosButton) {
        switchToTab(VideosTab);
    } else if (clicked == m_slideGroupsButton) {
        switchToTab(SlideGroupsTab);
    }
}

void MediaDrawer::onImportClicked()
{
    if (!m_library) {
        return;
    }

    QString filter;
    QString title;

    if (m_currentType == MediaLibrary::Video) {
        filter = tr("Video Files (*.mp4 *.webm *.avi *.mov *.mkv);;All Files (*)");
        title = tr("Import Video");
    } else {
        filter = tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)");
        title = tr("Import Image");
    }

    QString filePath = QFileDialog::getOpenFileName(this, title, QString(), filter);
    if (filePath.isEmpty()) {
        return;
    }

    QString libraryPath;
    if (m_currentType == MediaLibrary::Video) {
        libraryPath = m_library->addVideo(filePath);
    } else {
        libraryPath = m_library->addImage(filePath);
    }

    if (libraryPath.isEmpty()) {
        QMessageBox::warning(this, tr("Import Failed"),
            tr("Failed to import the file to the media library."));
    }
    // No need to manually repopulate — onMediaAdded signal will handle it
}

void MediaDrawer::onSaveGroupClicked()
{
    emit saveGroupToLibraryRequested();
}

void MediaDrawer::onMediaAdded(const MediaLibrary::MediaItem& item)
{
    Q_UNUSED(item);
    if (m_currentTab != SlideGroupsTab) {
        populateList();
    }
}

void MediaDrawer::onMediaRemoved(const QString& libraryPath)
{
    Q_UNUSED(libraryPath);
    if (m_currentTab != SlideGroupsTab) {
        populateList();
    }
}

void MediaDrawer::onThumbnailReady(const QString& videoPath, const QImage& thumbnail)
{
    if (m_currentType != MediaLibrary::Video || thumbnail.isNull()) {
        return;
    }

    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == videoPath) {
            QPixmap pixmap = QPixmap::fromImage(thumbnail);
            if (!pixmap.isNull()) {
                item->setIcon(QIcon(createCenteredThumbnail(pixmap)));
            }
            break;
        }
    }
}

void MediaDrawer::onGroupDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    int groupId = item->data(Qt::UserRole).toInt();
    emit slideGroupInsertRequested(groupId);
}

void MediaDrawer::onGroupContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_slideGroupListWidget->itemAt(pos);
    if (!item) return;

    int groupId = item->data(Qt::UserRole).toInt();

    QMenu menu(this);
    QAction* insertAction = menu.addAction(tr("Insert into Presentation"));
    QAction* renameAction = menu.addAction(tr("Rename..."));
    menu.addSeparator();
    QAction* deleteAction = menu.addAction(tr("Delete"));

    QAction* chosen = menu.exec(m_slideGroupListWidget->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == insertAction) {
        emit slideGroupInsertRequested(groupId);
    } else if (chosen == renameAction) {
        LibrarySlideGroup group = m_slideGroupLibrary->getGroup(groupId);
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename Slide Group"),
            tr("Name:"), QLineEdit::Normal, group.name, &ok);
        if (ok && !newName.isEmpty()) {
            m_slideGroupLibrary->renameGroup(groupId, newName);
            m_slideGroupLibrary->saveLibrary();
        }
    } else if (chosen == deleteAction) {
        LibrarySlideGroup group = m_slideGroupLibrary->getGroup(groupId);
        int result = QMessageBox::question(this, tr("Delete Slide Group"),
            tr("Delete \"%1\" from the library?").arg(group.name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_slideGroupLibrary->removeGroup(groupId);
            m_slideGroupLibrary->saveLibrary();
        }
    }
}

void MediaDrawer::onGroupAdded(int id)
{
    Q_UNUSED(id);
    if (m_currentTab == SlideGroupsTab) {
        populateSlideGroupList();
    }
}

void MediaDrawer::onGroupRemoved(int id)
{
    Q_UNUSED(id);
    if (m_currentTab == SlideGroupsTab) {
        populateSlideGroupList();
    }
}

void MediaDrawer::onGroupUpdated(int id)
{
    Q_UNUSED(id);
    if (m_currentTab == SlideGroupsTab) {
        populateSlideGroupList();
    }
}

} // namespace Clarity
