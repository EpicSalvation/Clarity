// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "MediaLibrary.h"
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QUuid>
#include <QDebug>
#include <QCryptographicHash>

namespace Clarity {

MediaLibrary::MediaLibrary(QObject* parent)
    : QObject(parent)
{
    // Set up library path in user's app data directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_libraryPath = appDataPath + "/MediaLibrary";

    ensureDirectoriesExist();
    loadIndex();
}

void MediaLibrary::ensureDirectoriesExist()
{
    QDir dir;

    // Create main library directory
    if (!dir.exists(m_libraryPath)) {
        if (!dir.mkpath(m_libraryPath)) {
            qWarning() << "Failed to create media library directory:" << m_libraryPath;
        }
    }

    // Create images subdirectory
    QString imgPath = imagesPath();
    if (!dir.exists(imgPath)) {
        if (!dir.mkpath(imgPath)) {
            qWarning() << "Failed to create images directory:" << imgPath;
        }
    }

    // Create videos subdirectory
    QString vidPath = videosPath();
    if (!dir.exists(vidPath)) {
        if (!dir.mkpath(vidPath)) {
            qWarning() << "Failed to create videos directory:" << vidPath;
        }
    }
}

QString MediaLibrary::imagesPath() const
{
    return m_libraryPath + "/images";
}

QString MediaLibrary::videosPath() const
{
    return m_libraryPath + "/videos";
}

void MediaLibrary::loadIndex()
{
    QString indexPath = m_libraryPath + "/index.json";
    QFile file(indexPath);

    if (!file.exists()) {
        return;  // No index yet, that's fine
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open media library index:" << indexPath;
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse media library index:" << error.errorString();
        return;
    }

    m_items.clear();
    QJsonArray items = doc.object()["items"].toArray();

    for (const QJsonValue& value : items) {
        QJsonObject obj = value.toObject();
        MediaItem item;
        item.id = obj["id"].toString();
        item.filename = obj["filename"].toString();
        item.libraryPath = obj["libraryPath"].toString();
        item.type = obj["type"].toString() == "video" ? Video : Image;
        item.dateAdded = QDateTime::fromString(obj["dateAdded"].toString(), Qt::ISODate);
        item.fileSize = obj["fileSize"].toVariant().toLongLong();

        // Verify file still exists
        if (QFile::exists(item.libraryPath)) {
            m_items.append(item);
        }
    }

    qDebug() << "Loaded media library with" << m_items.size() << "items";
}

void MediaLibrary::saveIndex()
{
    QString indexPath = m_libraryPath + "/index.json";
    QFile file(indexPath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save media library index:" << indexPath;
        return;
    }

    QJsonArray items;
    for (const MediaItem& item : m_items) {
        QJsonObject obj;
        obj["id"] = item.id;
        obj["filename"] = item.filename;
        obj["libraryPath"] = item.libraryPath;
        obj["type"] = item.type == Video ? "video" : "image";
        obj["dateAdded"] = item.dateAdded.toString(Qt::ISODate);
        obj["fileSize"] = item.fileSize;
        items.append(obj);
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["items"] = items;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QString MediaLibrary::generateUniqueFilename(const QString& originalName, MediaType type)
{
    QFileInfo info(originalName);
    QString baseName = info.completeBaseName();
    QString extension = info.suffix().toLower();

    // Sanitize base name (remove special characters)
    baseName.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
    if (baseName.isEmpty()) {
        baseName = "media";
    }

    QString targetDir = (type == Video) ? videosPath() : imagesPath();
    QString targetPath = targetDir + "/" + baseName + "." + extension;

    // If file doesn't exist, use as-is
    if (!QFile::exists(targetPath)) {
        return targetPath;
    }

    // Otherwise, add a number suffix
    int counter = 1;
    while (QFile::exists(targetPath)) {
        targetPath = targetDir + "/" + baseName + "_" + QString::number(counter) + "." + extension;
        counter++;
    }

    return targetPath;
}

QString MediaLibrary::findExistingMedia(const QString& sourcePath) const
{
    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        return QString();
    }

    // Calculate hash of source file (first 64KB for quick comparison)
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(sourceFile.read(65536));
    QByteArray sourceHash = hash.result();
    sourceFile.close();

    // Check against existing items
    for (const MediaItem& item : m_items) {
        QFile libFile(item.libraryPath);
        if (!libFile.open(QIODevice::ReadOnly)) {
            continue;
        }

        QCryptographicHash libHash(QCryptographicHash::Md5);
        libHash.addData(libFile.read(65536));
        libFile.close();

        if (libHash.result() == sourceHash) {
            // Also verify file sizes match for extra certainty
            QFileInfo sourceInfo(sourcePath);
            if (sourceInfo.size() == item.fileSize) {
                return item.libraryPath;
            }
        }
    }

    return QString();
}

QString MediaLibrary::addMedia(const QString& sourcePath, MediaType type)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        qWarning() << "Source file does not exist:" << sourcePath;
        return QString();
    }

    // Check if already in library
    QString existing = findExistingMedia(sourcePath);
    if (!existing.isEmpty()) {
        qDebug() << "Media already in library:" << existing;
        return existing;
    }

    // Generate unique filename and copy
    QString targetPath = generateUniqueFilename(sourceInfo.fileName(), type);

    if (!QFile::copy(sourcePath, targetPath)) {
        qWarning() << "Failed to copy media to library:" << sourcePath << "->" << targetPath;
        return QString();
    }

    // Create media item
    MediaItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.filename = sourceInfo.fileName();
    item.libraryPath = targetPath;
    item.type = type;
    item.dateAdded = QDateTime::currentDateTime();
    item.fileSize = sourceInfo.size();

    m_items.append(item);
    saveIndex();

    qDebug() << "Added media to library:" << item.filename << "->" << targetPath;
    emit mediaAdded(item);

    return targetPath;
}

QString MediaLibrary::addImage(const QString& sourcePath)
{
    return addMedia(sourcePath, Image);
}

QString MediaLibrary::addVideo(const QString& sourcePath)
{
    return addMedia(sourcePath, Video);
}

QList<MediaLibrary::MediaItem> MediaLibrary::getImages() const
{
    QList<MediaItem> images;
    for (const MediaItem& item : m_items) {
        if (item.type == Image) {
            images.append(item);
        }
    }
    // Sort by date added, newest first
    std::sort(images.begin(), images.end(), [](const MediaItem& a, const MediaItem& b) {
        return a.dateAdded > b.dateAdded;
    });
    return images;
}

QList<MediaLibrary::MediaItem> MediaLibrary::getVideos() const
{
    QList<MediaItem> videos;
    for (const MediaItem& item : m_items) {
        if (item.type == Video) {
            videos.append(item);
        }
    }
    // Sort by date added, newest first
    std::sort(videos.begin(), videos.end(), [](const MediaItem& a, const MediaItem& b) {
        return a.dateAdded > b.dateAdded;
    });
    return videos;
}

MediaLibrary::MediaItem MediaLibrary::getMediaByPath(const QString& libraryPath) const
{
    for (const MediaItem& item : m_items) {
        if (item.libraryPath == libraryPath) {
            return item;
        }
    }
    return MediaItem();  // Return empty item if not found
}

bool MediaLibrary::removeMedia(const QString& libraryPath)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].libraryPath == libraryPath) {
            // Remove the file
            if (QFile::remove(libraryPath)) {
                m_items.removeAt(i);
                saveIndex();
                emit mediaRemoved(libraryPath);
                qDebug() << "Removed media from library:" << libraryPath;
                return true;
            } else {
                qWarning() << "Failed to remove media file:" << libraryPath;
                return false;
            }
        }
    }
    return false;  // Not found
}

} // namespace Clarity
