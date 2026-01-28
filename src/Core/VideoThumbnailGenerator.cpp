#include "VideoThumbnailGenerator.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QPainter>
#include <QDebug>
#include <QProcess>
#include <QDateTime>
#include <QMutexLocker>

namespace Clarity {

// ============================================================================
// ThumbnailWorker implementation
// ============================================================================

ThumbnailWorker::ThumbnailWorker(QObject* parent)
    : QObject(parent)
{
}

void ThumbnailWorker::generateThumbnail(const QString& videoPath)
{
    qDebug() << "Worker: generating thumbnail for" << videoPath;

    // Verify file exists and is readable before attempting extraction
    QFile file(videoPath);
    if (!file.exists()) {
        qWarning() << "Worker: video file does not exist:" << videoPath;
        emit thumbnailGenerated(videoPath, QImage());
        return;
    }

    // Try to open file to ensure it's accessible (not locked)
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Worker: cannot open video file (may be locked):" << videoPath;
        emit thumbnailGenerated(videoPath, QImage());
        return;
    }
    file.close();

    QImage thumbnail = extractFrameWithFFmpeg(videoPath);
    emit thumbnailGenerated(videoPath, thumbnail);
}

QString ThumbnailWorker::findFFmpeg()
{
    // Cache the result after first lookup
    static QString cachedPath;
    static bool searched = false;

    if (searched) {
        return cachedPath;
    }
    searched = true;

    // Check common locations for ffmpeg
    QStringList candidates;

#ifdef Q_OS_WIN
    // Windows: check common install locations and PATH
    candidates << "C:/ffmpeg/bin/ffmpeg.exe"
               << "C:/Program Files/ffmpeg/bin/ffmpeg.exe"
               << "C:/Program Files (x86)/ffmpeg/bin/ffmpeg.exe"
               << QDir::homePath() + "/ffmpeg/bin/ffmpeg.exe"
               << QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
#else
    // Linux/Mac: check common locations
    candidates << "/usr/bin/ffmpeg"
               << "/usr/local/bin/ffmpeg"
               << "/opt/homebrew/bin/ffmpeg";
#endif

    // First check if ffmpeg is in PATH
    QProcess testProcess;
#ifdef Q_OS_WIN
    testProcess.start("where", QStringList() << "ffmpeg");
#else
    testProcess.start("which", QStringList() << "ffmpeg");
#endif
    if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
        QString path = QString::fromUtf8(testProcess.readAllStandardOutput()).trimmed();
        if (!path.isEmpty()) {
            // On Windows, 'where' might return multiple lines, take the first
            path = path.split('\n').first().trimmed();
            qDebug() << "Worker: found ffmpeg in PATH at" << path;
            cachedPath = path;
            return cachedPath;
        }
    }

    // Check specific locations
    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            qDebug() << "Worker: found ffmpeg at" << path;
            cachedPath = path;
            return cachedPath;
        }
    }

    qWarning() << "===========================================";
    qWarning() << "Worker: FFmpeg not found!";
    qWarning() << "Video thumbnails will not be generated.";
    qWarning() << "Please install FFmpeg from https://ffmpeg.org";
    qWarning() << "Or place ffmpeg.exe next to the Clarity executable.";
    qWarning() << "Searched locations:" << candidates;
    qWarning() << "===========================================";

    return QString();
}

QImage ThumbnailWorker::extractFrameWithFFmpeg(const QString& videoPath)
{
    QString ffmpegPath = findFFmpeg();
    if (ffmpegPath.isEmpty()) {
        qWarning() << "Worker: cannot generate thumbnail - ffmpeg not installed";
        return QImage();
    }

    // Create a temporary file for the output
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = tempDir + "/clarity_thumb_" +
                       QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg";

    // Build ffmpeg command:
    // -ss 1 : seek to 1 second (fast seek before input)
    // -i input : input file
    // -vframes 1 : extract only 1 frame
    // -vf scale=320:180:force_original_aspect_ratio=decrease : scale to fit 320x180
    // -y : overwrite output without asking
    QStringList args;
    args << "-ss" << "1"           // Seek to 1 second
         << "-i" << videoPath      // Input file
         << "-vframes" << "1"      // Extract 1 frame
         << "-vf" << "scale=320:180:force_original_aspect_ratio=decrease"
         << "-y"                   // Overwrite
         << tempFile;              // Output file

    qDebug() << "Worker: running ffmpeg with args:" << args;

    QProcess process;
    process.start(ffmpegPath, args);

    if (!process.waitForFinished(10000)) {  // 10 second timeout
        qWarning() << "Worker: ffmpeg timed out for" << videoPath;
        process.kill();
        QFile::remove(tempFile);
        return QImage();
    }

    if (process.exitCode() != 0) {
        qWarning() << "Worker: ffmpeg failed for" << videoPath
                   << "exit code:" << process.exitCode()
                   << "stderr:" << process.readAllStandardError();
        QFile::remove(tempFile);
        return QImage();
    }

    // Load the generated thumbnail
    QImage result(tempFile);
    if (result.isNull()) {
        qWarning() << "Worker: failed to load generated thumbnail from" << tempFile;
    } else {
        qDebug() << "Worker: successfully generated thumbnail for" << videoPath
                 << "size:" << result.size();
    }

    // Clean up temp file
    QFile::remove(tempFile);

    return result;
}

// ============================================================================
// VideoThumbnailGenerator implementation
// ============================================================================

VideoThumbnailGenerator::VideoThumbnailGenerator(QObject* parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new ThumbnailWorker())
{
    qDebug() << "VideoThumbnailGenerator: initializing...";

    // Set up cache directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_cacheDir = appDataPath + "/ThumbnailCache";
    qDebug() << "VideoThumbnailGenerator: cache directory:" << m_cacheDir;

    QDir dir;
    if (!dir.exists(m_cacheDir)) {
        dir.mkpath(m_cacheDir);
    }

    // Move worker to background thread
    m_worker->moveToThread(m_workerThread);

    // Connect signals
    connect(m_worker, &ThumbnailWorker::thumbnailGenerated,
            this, &VideoThumbnailGenerator::onThumbnailGenerated,
            Qt::QueuedConnection);

    // Clean up worker when thread finishes
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // Start the worker thread
    m_workerThread->start();

    if (m_workerThread->isRunning()) {
        qDebug() << "VideoThumbnailGenerator: worker thread started successfully";
    } else {
        qWarning() << "VideoThumbnailGenerator: worker thread FAILED to start!";
    }

    // Check if FFmpeg is available at startup (quick check on main thread)
    checkFFmpegAvailability();
}

void VideoThumbnailGenerator::checkFFmpegAvailability()
{
    QProcess testProcess;
#ifdef Q_OS_WIN
    testProcess.start("where", QStringList() << "ffmpeg");
#else
    testProcess.start("which", QStringList() << "ffmpeg");
#endif
    bool inPath = testProcess.waitForFinished(3000) && testProcess.exitCode() == 0;

    if (inPath) {
        QString path = QString::fromUtf8(testProcess.readAllStandardOutput()).trimmed().split('\n').first();
        qDebug() << "VideoThumbnailGenerator: FFmpeg found in PATH at" << path;
        return;
    }

    // Check application directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString ffmpegInAppDir = appDir + "/ffmpeg.exe";
    if (QFile::exists(ffmpegInAppDir)) {
        qDebug() << "VideoThumbnailGenerator: FFmpeg found at" << ffmpegInAppDir;
        return;
    }

    qWarning() << "===========================================";
    qWarning() << "VideoThumbnailGenerator: FFmpeg NOT FOUND!";
    qWarning() << "Video thumbnails will show placeholders only.";
    qWarning() << "To enable video thumbnails:";
    qWarning() << "  1. Download FFmpeg from https://www.gyan.dev/ffmpeg/builds/";
    qWarning() << "  2. Place ffmpeg.exe in:" << appDir;
    qWarning() << "  OR add FFmpeg to your system PATH";
    qWarning() << "===========================================";
}

VideoThumbnailGenerator::~VideoThumbnailGenerator()
{
    m_workerThread->quit();
    m_workerThread->wait();
}

QString VideoThumbnailGenerator::cacheDirectory() const
{
    return m_cacheDir;
}

QString VideoThumbnailGenerator::thumbnailCachePath(const QString& videoPath) const
{
    QFileInfo info(videoPath);
    // Use lastModified if available, otherwise use 0
    qint64 modTime = 0;
    if (info.exists()) {
        QDateTime lastMod = info.lastModified();
        if (lastMod.isValid()) {
            modTime = lastMod.toSecsSinceEpoch();
        }
    }
    QString uniqueKey = videoPath + QString::number(modTime);
    QByteArray hash = QCryptographicHash::hash(uniqueKey.toUtf8(), QCryptographicHash::Md5);
    return m_cacheDir + "/" + hash.toHex() + ".jpg";
}

QImage VideoThumbnailGenerator::loadCachedThumbnail(const QString& videoPath)
{
    QMutexLocker locker(&m_mutex);

    // Check memory cache first
    if (m_memoryCache.contains(videoPath)) {
        return m_memoryCache[videoPath];
    }

    // Check disk cache
    QString cachePath = thumbnailCachePath(videoPath);
    if (QFile::exists(cachePath)) {
        QImage img(cachePath);
        if (!img.isNull()) {
            m_memoryCache[videoPath] = img;
            return img;
        }
    }

    return QImage();
}

void VideoThumbnailGenerator::saveThumbnailToCache(const QString& videoPath, const QImage& thumbnail)
{
    if (thumbnail.isNull()) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Save to memory cache
    m_memoryCache[videoPath] = thumbnail;

    // Save to disk cache
    QString cachePath = thumbnailCachePath(videoPath);
    thumbnail.save(cachePath, "JPEG", 85);
}

bool VideoThumbnailGenerator::hasCachedThumbnail(const QString& videoPath) const
{
    // Normalize path for consistent comparison
    QString normalizedPath = QDir::toNativeSeparators(videoPath);

    QMutexLocker locker(&m_mutex);

    if (m_memoryCache.contains(normalizedPath)) {
        return true;
    }

    QString cachePath = thumbnailCachePath(normalizedPath);
    return QFile::exists(cachePath);
}

void VideoThumbnailGenerator::requestThumbnail(const QString& videoPath)
{
    // Normalize path for consistent comparison (Windows uses backslashes)
    QString normalizedPath = QDir::toNativeSeparators(videoPath);

    qDebug() << "VideoThumbnailGenerator::requestThumbnail called for" << normalizedPath;

    if (!QFile::exists(normalizedPath)) {
        qDebug() << "  -> File does not exist, skipping";
        return;
    }

    // Check if already cached
    if (hasCachedThumbnail(normalizedPath)) {
        qDebug() << "  -> Already cached, skipping";
        return;
    }

    // Check if already pending
    {
        QMutexLocker locker(&m_mutex);
        if (m_pendingRequests.contains(normalizedPath)) {
            qDebug() << "  -> Already pending, skipping";
            return;
        }
        m_pendingRequests.insert(normalizedPath);
    }

    qDebug() << "  -> Queuing thumbnail generation on worker thread (running:" << m_workerThread->isRunning() << ")";

    // Queue generation in worker thread
    QMetaObject::invokeMethod(m_worker, "generateThumbnail",
                              Qt::QueuedConnection,
                              Q_ARG(QString, normalizedPath));
}

void VideoThumbnailGenerator::onThumbnailGenerated(const QString& videoPath, const QImage& thumbnail)
{
    {
        QMutexLocker locker(&m_mutex);
        m_pendingRequests.remove(videoPath);
    }

    qDebug() << "VideoThumbnailGenerator: received thumbnail for" << videoPath
             << "size:" << thumbnail.size() << "null:" << thumbnail.isNull();

    if (!thumbnail.isNull()) {
        saveThumbnailToCache(videoPath, thumbnail);
        emit thumbnailReady(videoPath, thumbnail);
        qDebug() << "VideoThumbnailGenerator: emitted thumbnailReady for" << videoPath;
    } else {
        qWarning() << "VideoThumbnailGenerator: received null thumbnail for" << videoPath;
    }
}

QPixmap VideoThumbnailGenerator::getThumbnail(const QString& videoPath, const QSize& size)
{
    // Normalize path for consistent comparison
    QString normalizedPath = QDir::toNativeSeparators(videoPath);

    QImage img = getThumbnailImage(normalizedPath, size);
    if (img.isNull()) {
        // Request async generation
        requestThumbnail(normalizedPath);
        return placeholderThumbnail(size);
    }
    return QPixmap::fromImage(img);
}

QImage VideoThumbnailGenerator::getThumbnailImage(const QString& videoPath, const QSize& size)
{
    // Normalize path for consistent comparison
    QString normalizedPath = QDir::toNativeSeparators(videoPath);

    if (!QFile::exists(normalizedPath)) {
        return QImage();
    }

    // Try to load from cache
    QImage thumbnail = loadCachedThumbnail(normalizedPath);

    // Scale to requested size if needed
    if (!thumbnail.isNull() && thumbnail.size() != size) {
        thumbnail = thumbnail.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return thumbnail;
}

void VideoThumbnailGenerator::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_memoryCache.clear();

    QDir cacheDir(m_cacheDir);
    QStringList files = cacheDir.entryList(QStringList() << "*.jpg", QDir::Files);
    for (const QString& file : files) {
        QFile::remove(m_cacheDir + "/" + file);
    }
}

QPixmap VideoThumbnailGenerator::placeholderThumbnail(const QSize& size)
{
    QPixmap placeholder(size);
    placeholder.fill(QColor(40, 40, 40));

    QPainter painter(&placeholder);
    painter.setPen(QColor(150, 150, 150));

    // Draw a simple "play" triangle icon
    int iconSize = qMin(size.width(), size.height()) / 3;
    int centerX = size.width() / 2;
    int centerY = size.height() / 2;

    QPolygon playIcon;
    playIcon << QPoint(centerX - iconSize/2, centerY - iconSize/2)
             << QPoint(centerX + iconSize/2, centerY)
             << QPoint(centerX - iconSize/2, centerY + iconSize/2);

    painter.setBrush(QColor(100, 100, 100));
    painter.drawPolygon(playIcon);

    // Draw "VIDEO" text
    painter.setFont(QFont("Arial", 10));
    painter.drawText(placeholder.rect(), Qt::AlignBottom | Qt::AlignHCenter, "VIDEO");

    return placeholder;
}

} // namespace Clarity
