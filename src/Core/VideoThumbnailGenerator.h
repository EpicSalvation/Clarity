#pragma once

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QSet>
#include <QThread>
#include <QMutex>

namespace Clarity {

class ThumbnailWorker;

/**
 * @brief Generates thumbnail images from video files asynchronously
 *
 * Uses Qt Multimedia to extract a frame from videos in a background thread.
 * Caches thumbnails to disk for faster subsequent access.
 * Emits thumbnailReady signal when async generation completes.
 */
class VideoThumbnailGenerator : public QObject {
    Q_OBJECT

public:
    explicit VideoThumbnailGenerator(QObject* parent = nullptr);
    ~VideoThumbnailGenerator();

    /**
     * @brief Get a thumbnail for a video file
     * @param videoPath Path to the video file
     * @param size Desired thumbnail size (will maintain aspect ratio)
     * @return Cached thumbnail pixmap, or a placeholder if not yet generated
     *
     * If the thumbnail is not cached, this returns a placeholder immediately
     * and queues async generation. Listen to thumbnailReady() for updates.
     */
    QPixmap getThumbnail(const QString& videoPath, const QSize& size = QSize(160, 90));

    /**
     * @brief Get a thumbnail as QImage
     * @param videoPath Path to the video file
     * @param size Desired thumbnail size
     * @return Cached thumbnail image, or null image if not yet generated
     */
    QImage getThumbnailImage(const QString& videoPath, const QSize& size = QSize(160, 90));

    /**
     * @brief Request async thumbnail generation for a video
     * @param videoPath Path to the video file
     *
     * If already cached or already pending, this does nothing.
     * Otherwise queues generation and emits thumbnailReady when done.
     */
    void requestThumbnail(const QString& videoPath);

    /**
     * @brief Check if a thumbnail is cached (available immediately)
     */
    bool hasCachedThumbnail(const QString& videoPath) const;

    /**
     * @brief Clear the thumbnail cache
     */
    void clearCache();

    /**
     * @brief Get the cache directory path
     */
    QString cacheDirectory() const;

    /**
     * @brief Generate a placeholder image for videos without thumbnails
     */
    static QPixmap placeholderThumbnail(const QSize& size);

signals:
    /**
     * @brief Emitted when a thumbnail has been generated
     * @param videoPath The video file path
     * @param thumbnail The generated thumbnail image
     */
    void thumbnailReady(const QString& videoPath, const QImage& thumbnail);

private slots:
    void onThumbnailGenerated(const QString& videoPath, const QImage& thumbnail);

private:
    void checkFFmpegAvailability();
    QString thumbnailCachePath(const QString& videoPath) const;
    QImage loadCachedThumbnail(const QString& videoPath);
    void saveThumbnailToCache(const QString& videoPath, const QImage& thumbnail);

    QString m_cacheDir;
    QMap<QString, QImage> m_memoryCache;
    QSet<QString> m_pendingRequests;
    mutable QMutex m_mutex;

    QThread* m_workerThread;
    ThumbnailWorker* m_worker;
};

/**
 * @brief Worker object that runs in a background thread to generate thumbnails
 */
class ThumbnailWorker : public QObject {
    Q_OBJECT

public:
    explicit ThumbnailWorker(QObject* parent = nullptr);

public slots:
    void generateThumbnail(const QString& videoPath);

signals:
    void thumbnailGenerated(const QString& videoPath, const QImage& thumbnail);

private:
    QString findFFmpeg();
    QImage extractFrameWithFFmpeg(const QString& videoPath);
};

} // namespace Clarity
