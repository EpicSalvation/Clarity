// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "MediaLibraryDialog.h"
#include "Core/VideoThumbnailGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QFileInfo>
#include <QDebug>

namespace Clarity {

// Thumbnail size for library grid
static const QSize THUMBNAIL_SIZE(120, 90);

/**
 * @brief Create a centered thumbnail within a fixed-size pixmap
 * Scales the source to fit within the target size while maintaining aspect ratio,
 * then centers it on a dark background for consistent appearance.
 */
static QPixmap createCenteredThumbnail(const QPixmap& source, const QSize& targetSize = THUMBNAIL_SIZE)
{
    if (source.isNull()) {
        return source;
    }

    // Scale source to fit within target while maintaining aspect ratio
    QPixmap scaled = source.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Create target pixmap with dark background (matches video placeholder)
    QPixmap result(targetSize);
    result.fill(QColor(30, 30, 30));

    // Draw scaled pixmap centered
    QPainter painter(&result);
    int x = (targetSize.width() - scaled.width()) / 2;
    int y = (targetSize.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.end();

    return result;
}

MediaLibraryDialog::MediaLibraryDialog(MediaLibrary* library,
                                       MediaLibrary::MediaType type,
                                       VideoThumbnailGenerator* thumbnailGen,
                                       QWidget* parent)
    : QDialog(parent)
    , m_library(library)
    , m_mediaType(type)
    , m_thumbnailGen(thumbnailGen)
{
    setupUI();
    populateList();

    // Refresh list when video thumbnails become available
    if (m_thumbnailGen && m_mediaType == MediaLibrary::Video) {
        connect(m_thumbnailGen, &VideoThumbnailGenerator::thumbnailReady,
                this, &MediaLibraryDialog::onThumbnailReady);
    }

    QString typeStr = (type == MediaLibrary::Video) ? "Video" : "Image";
    setWindowTitle(typeStr + " Library");
    resize(700, 500);
}

MediaLibraryDialog::~MediaLibraryDialog()
{
    // Explicitly disconnect from thumbnail generator to prevent callbacks to destroyed object
    if (m_thumbnailGen) {
        disconnect(m_thumbnailGen, &VideoThumbnailGenerator::thumbnailReady,
                   this, &MediaLibraryDialog::onThumbnailReady);
    }
}

void MediaLibraryDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Content area: list on left, preview on right
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Media list - uniform grid of thumbnails
    m_listWidget = new QListWidget(this);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(120, 90));
    m_listWidget->setGridSize(QSize(130, 100));  // Uniform grid cells
    m_listWidget->setSpacing(5);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMinimumWidth(400);
    m_listWidget->setUniformItemSizes(true);  // Optimize for uniform items
    m_listWidget->setWordWrap(false);  // No text wrapping needed

    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &MediaLibraryDialog::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &MediaLibraryDialog::onItemDoubleClicked);

    contentLayout->addWidget(m_listWidget, 1);

    // Preview panel
    QVBoxLayout* previewLayout = new QVBoxLayout();

    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(200, 150);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f0f0f0; }");
    m_previewLabel->setText("No selection");
    previewLayout->addWidget(m_previewLabel);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setAlignment(Qt::AlignTop);
    m_infoLabel->setMinimumHeight(80);
    previewLayout->addWidget(m_infoLabel);

    previewLayout->addStretch();

    contentLayout->addLayout(previewLayout);
    mainLayout->addLayout(contentLayout, 1);

    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_importButton = new QPushButton("Import New...", this);
    connect(m_importButton, &QPushButton::clicked, this, &MediaLibraryDialog::onImportClicked);
    buttonLayout->addWidget(m_importButton);

    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &MediaLibraryDialog::onDeleteClicked);
    buttonLayout->addWidget(m_deleteButton);

    buttonLayout->addStretch();

    m_selectButton = new QPushButton("Select", this);
    m_selectButton->setEnabled(false);
    m_selectButton->setDefault(true);
    connect(m_selectButton, &QPushButton::clicked, this, &MediaLibraryDialog::onSelectClicked);
    buttonLayout->addWidget(m_selectButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void MediaLibraryDialog::populateList()
{
    m_listWidget->clear();

    if (!m_library) {
        qWarning() << "MediaLibraryDialog: library is null";
        return;
    }

    QList<MediaLibrary::MediaItem> items;
    if (m_mediaType == MediaLibrary::Video) {
        items = m_library->getVideos();
    } else {
        items = m_library->getImages();
    }

    for (const MediaLibrary::MediaItem& item : items) {
        // Verify file exists before adding to list
        if (!QFile::exists(item.libraryPath)) {
            qWarning() << "MediaLibraryDialog: file not found:" << item.libraryPath;
            continue;
        }

        QListWidgetItem* listItem = new QListWidgetItem();
        // No text - just thumbnails in a clean grid. Filename shown on hover via tooltip.
        listItem->setData(Qt::UserRole, item.libraryPath);
        listItem->setToolTip(item.filename);

        if (m_mediaType == MediaLibrary::Image) {
            // Load thumbnail for images, centered in fixed-size cell
            QPixmap pixmap(item.libraryPath);
            if (!pixmap.isNull()) {
                listItem->setIcon(QIcon(createCenteredThumbnail(pixmap)));
            }
        } else {
            // Generate video thumbnail, centered in fixed-size cell
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

    // Update empty state message
    if (items.isEmpty() || m_listWidget->count() == 0) {
        QString typeStr = (m_mediaType == MediaLibrary::Video) ? "videos" : "images";
        m_previewLabel->setText("No " + typeStr + " in library.\nClick 'Import New...' to add.");
    }
}

void MediaLibraryDialog::onItemSelectionChanged()
{
    if (!m_listWidget || !m_selectButton || !m_deleteButton || !m_previewLabel || !m_infoLabel) {
        return;
    }

    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    bool hasSelection = !selected.isEmpty();

    m_selectButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);

    if (hasSelection) {
        updatePreview();
    } else {
        m_previewLabel->setText("No selection");
        m_infoLabel->clear();
    }
}

void MediaLibraryDialog::updatePreview()
{
    if (!m_listWidget || !m_previewLabel || !m_infoLabel || !m_library) {
        return;
    }

    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (selected.isEmpty() || !selected.first()) {
        return;
    }

    QString path = selected.first()->data(Qt::UserRole).toString();
    if (path.isEmpty() || !QFile::exists(path)) {
        m_previewLabel->setText("File not found");
        m_infoLabel->clear();
        return;
    }

    MediaLibrary::MediaItem item = m_library->getMediaByPath(path);

    if (m_mediaType == MediaLibrary::Image) {
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            m_previewLabel->setPixmap(pixmap.scaled(m_previewLabel->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        // Show video thumbnail in preview
        if (m_thumbnailGen) {
            QPixmap thumbnail = m_thumbnailGen->getThumbnail(path, m_previewLabel->size());
            m_previewLabel->setPixmap(thumbnail);
        } else {
            m_previewLabel->setPixmap(VideoThumbnailGenerator::placeholderThumbnail(m_previewLabel->size()));
        }
    }

    // Show file info
    QFileInfo fileInfo(path);
    QString sizeStr;
    qint64 size = fileInfo.size();
    if (size < 1024) {
        sizeStr = QString::number(size) + " B";
    } else if (size < 1024 * 1024) {
        sizeStr = QString::number(size / 1024.0, 'f', 1) + " KB";
    } else {
        sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
    }

    QString info = QString("<b>%1</b><br>Size: %2<br>Added: %3")
        .arg(item.filename.isEmpty() ? fileInfo.fileName() : item.filename)
        .arg(sizeStr)
        .arg(item.dateAdded.isValid() ? item.dateAdded.toString("yyyy-MM-dd hh:mm") : "Unknown");

    m_infoLabel->setText(info);
}

void MediaLibraryDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    }
}

void MediaLibraryDialog::onImportClicked()
{
    if (!m_library) {
        qWarning() << "MediaLibraryDialog: library is null in onImportClicked";
        return;
    }

    QString filter;
    QString title;

    if (m_mediaType == MediaLibrary::Video) {
        filter = "Video Files (*.mp4 *.webm *.avi *.mov *.mkv);;All Files (*)";
        title = "Import Video";
    } else {
        filter = "Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)";
        title = "Import Image";
    }

    QString filePath = QFileDialog::getOpenFileName(this, title, QString(), filter);
    if (filePath.isEmpty()) {
        return;
    }

    QString libraryPath;
    if (m_mediaType == MediaLibrary::Video) {
        libraryPath = m_library->addVideo(filePath);
    } else {
        libraryPath = m_library->addImage(filePath);
    }

    if (libraryPath.isEmpty()) {
        QMessageBox::warning(this, "Import Failed",
            "Failed to import the file to the media library.");
        return;
    }

    // Refresh list and select the new item
    populateList();

    // Find and select the newly added item
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == libraryPath) {
            m_listWidget->setCurrentItem(item);
            break;
        }
    }
}

void MediaLibraryDialog::onDeleteClicked()
{
    if (!m_library || !m_listWidget) {
        qWarning() << "MediaLibraryDialog: null pointer in onDeleteClicked";
        return;
    }

    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (selected.isEmpty() || !selected.first()) {
        return;
    }

    QString path = selected.first()->data(Qt::UserRole).toString();
    QString filename = selected.first()->toolTip();  // Filename stored in tooltip

    if (path.isEmpty()) {
        qWarning() << "MediaLibraryDialog: empty path in onDeleteClicked";
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Media",
        QString("Are you sure you want to delete '%1' from the library?\n\n"
                "This will not affect slides that are already using this file.")
            .arg(filename),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_library->removeMedia(path)) {
            populateList();
        } else {
            QMessageBox::warning(this, "Delete Failed",
                "Failed to delete the file from the media library.");
        }
    }
}

void MediaLibraryDialog::onSelectClicked()
{
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (!selected.isEmpty()) {
        m_selectedPath = selected.first()->data(Qt::UserRole).toString();
        accept();
    }
}

void MediaLibraryDialog::onThumbnailReady(const QString& videoPath, const QImage& thumbnail)
{
    // Ignore if thumbnail is null
    if (thumbnail.isNull()) {
        qDebug() << "MediaLibraryDialog: received null thumbnail for" << videoPath;
        return;
    }

    // Ignore if list widget is not valid
    if (!m_listWidget) {
        return;
    }

    // Find the list item matching this video path and update its icon
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

    // Also update preview if this video is currently selected
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (!selected.isEmpty() && selected.first()->data(Qt::UserRole).toString() == videoPath) {
        updatePreview();
    }
}

} // namespace Clarity
