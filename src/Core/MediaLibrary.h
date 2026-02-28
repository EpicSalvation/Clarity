// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace Clarity {

/**
 * @brief Manages a local media library for images and videos
 *
 * Media files are copied to a local library directory and indexed
 * for quick access. The library is stored in the user's app data folder.
 */
class MediaLibrary : public QObject {
    Q_OBJECT

public:
    enum MediaType {
        Image,
        Video
    };

    struct MediaItem {
        QString id;                      // Unique identifier (UUID)
        QString filename;                // Original filename
        QString libraryPath;             // Full path in library
        MediaType type = Image;          // Default to Image
        QDateTime dateAdded;             // When added to library
        qint64 fileSize = 0;             // File size in bytes

        // Check if this item is valid/initialized
        bool isValid() const { return !id.isEmpty() && !libraryPath.isEmpty(); }
    };

    explicit MediaLibrary(QObject* parent = nullptr);
    ~MediaLibrary() = default;

    /**
     * @brief Add an image to the library
     * @param sourcePath Path to the source image file
     * @return The library path of the copied file, or empty string on failure
     */
    QString addImage(const QString& sourcePath);

    /**
     * @brief Add a video to the library
     * @param sourcePath Path to the source video file
     * @return The library path of the copied file, or empty string on failure
     */
    QString addVideo(const QString& sourcePath);

    /**
     * @brief Get all images in the library
     * @return List of image media items
     */
    QList<MediaItem> getImages() const;

    /**
     * @brief Get all videos in the library
     * @return List of video media items
     */
    QList<MediaItem> getVideos() const;

    /**
     * @brief Get a specific media item by its library path
     * @param libraryPath The path in the library
     * @return The media item, or invalid item if not found
     */
    MediaItem getMediaByPath(const QString& libraryPath) const;

    /**
     * @brief Check if a file is already in the library
     * @param sourcePath Original source path to check
     * @return Library path if exists, empty string if not
     */
    QString findExistingMedia(const QString& sourcePath) const;

    /**
     * @brief Get the library directory path
     */
    QString libraryPath() const { return m_libraryPath; }

    /**
     * @brief Get the images subdirectory path
     */
    QString imagesPath() const;

    /**
     * @brief Get the videos subdirectory path
     */
    QString videosPath() const;

    /**
     * @brief Remove a media item from the library
     * @param libraryPath Path of the item to remove
     * @return true if removed successfully
     */
    bool removeMedia(const QString& libraryPath);

signals:
    void mediaAdded(const MediaItem& item);
    void mediaRemoved(const QString& libraryPath);

private:
    void ensureDirectoriesExist();
    void loadIndex();
    void saveIndex();
    QString generateUniqueFilename(const QString& originalName, MediaType type);
    QString addMedia(const QString& sourcePath, MediaType type);

    QString m_libraryPath;
    QList<MediaItem> m_items;
};

} // namespace Clarity
