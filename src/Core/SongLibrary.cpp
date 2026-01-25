#include "SongLibrary.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFileInfo>
#include <algorithm>
#include <QDebug>

namespace Clarity {

SongLibrary::SongLibrary(QObject* parent)
    : QObject(parent)
    , m_nextId(1)
{
    // Set library path in config directory
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir dir(configDir);

    // Create Clarity subdirectory if needed
    if (!dir.exists("Clarity")) {
        dir.mkdir("Clarity");
    }

    m_libraryPath = configDir + "/Clarity/songs.json";
    qDebug() << "Song library path:" << m_libraryPath;
}

SongLibrary::~SongLibrary()
{
    // Auto-save on destruction
    saveLibrary();
}

bool SongLibrary::loadLibrary()
{
    QFile file(m_libraryPath);

    if (!file.exists()) {
        qDebug() << "Song library file doesn't exist yet, starting with empty library";
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open song library:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse song library JSON:" << error.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QString version = root["version"].toString();

    if (version != "1.0") {
        qWarning() << "Unknown song library version:" << version;
        // Continue anyway, try to load what we can
    }

    m_nextId = root["nextId"].toInt(1);

    m_songs.clear();
    QJsonArray songsArray = root["songs"].toArray();
    for (const QJsonValue& value : songsArray) {
        Song song = Song::fromJson(value.toObject());
        m_songs.append(song);

        // Ensure nextId is higher than any existing ID
        if (song.id() >= m_nextId) {
            m_nextId = song.id() + 1;
        }
    }

    qDebug() << "Loaded" << m_songs.count() << "songs from library";
    emit libraryLoaded();
    return true;
}

bool SongLibrary::saveLibrary()
{
    QJsonObject root;
    root["version"] = "1.0";
    root["nextId"] = m_nextId;

    QJsonArray songsArray;
    for (const Song& song : m_songs) {
        songsArray.append(song.toJson());
    }
    root["songs"] = songsArray;

    QJsonDocument doc(root);

    QFile file(m_libraryPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save song library:" << file.errorString();
        return false;
    }

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written == -1) {
        qWarning() << "Failed to write song library data";
        return false;
    }

    qDebug() << "Saved" << m_songs.count() << "songs to library";
    return true;
}

Song SongLibrary::getSong(int id) const
{
    for (const Song& song : m_songs) {
        if (song.id() == id) {
            return song;
        }
    }
    return Song();  // Empty song
}

int SongLibrary::indexOf(int id) const
{
    for (int i = 0; i < m_songs.count(); ++i) {
        if (m_songs[i].id() == id) {
            return i;
        }
    }
    return -1;
}

int SongLibrary::addSong(const Song& song)
{
    Song newSong = song;
    int id = nextId();
    newSong.setId(id);

    if (!newSong.addedDate().isValid()) {
        newSong.setAddedDate(QDateTime::currentDateTime());
    }

    m_songs.append(newSong);
    emit songAdded(id);
    return id;
}

bool SongLibrary::updateSong(int id, const Song& song)
{
    int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    Song updated = song;
    updated.setId(id);  // Preserve ID
    m_songs[index] = updated;
    emit songUpdated(id);
    return true;
}

bool SongLibrary::removeSong(int id)
{
    int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    m_songs.removeAt(index);
    emit songRemoved(id);
    return true;
}

QList<Song> SongLibrary::search(const QString& query) const
{
    QList<Song> results;

    if (query.isEmpty()) {
        return m_songs;  // Return all if no query
    }

    QString lowerQuery = query.toLower();

    for (const Song& song : m_songs) {
        // Search in title
        if (song.title().toLower().contains(lowerQuery)) {
            results.append(song);
            continue;
        }

        // Search in author
        if (song.author().toLower().contains(lowerQuery)) {
            results.append(song);
            continue;
        }

        // Search in lyrics
        if (song.allLyrics().toLower().contains(lowerQuery)) {
            results.append(song);
            continue;
        }

        // Search in CCLI number
        if (song.ccliNumber().contains(query)) {
            results.append(song);
            continue;
        }
    }

    return results;
}

QList<Song> SongLibrary::recentSongs(int count) const
{
    // Filter songs that have been used
    QList<Song> used;
    for (const Song& song : m_songs) {
        if (song.lastUsed().isValid()) {
            used.append(song);
        }
    }

    // Sort by last used date (most recent first)
    std::sort(used.begin(), used.end(), [](const Song& a, const Song& b) {
        return a.lastUsed() > b.lastUsed();
    });

    // Return up to count songs
    if (used.count() <= count) {
        return used;
    }
    return used.mid(0, count);
}

void SongLibrary::markAsUsed(int id)
{
    int index = indexOf(id);
    if (index >= 0) {
        m_songs[index].setLastUsed(QDateTime::currentDateTime());
        emit songUpdated(id);
    }
}

Song SongLibrary::importFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for import:" << file.errorString();
        return Song();
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString baseName = fileInfo.baseName();

    Song song;

    if (suffix == "xml") {
        // Try OpenLyrics format
        song = Song::fromOpenLyrics(content);
    } else if (suffix == "txt") {
        // Plain text with markers
        song = Song::fromPlainText(content, baseName);
    } else {
        // Unknown format, try plain text
        qDebug() << "Unknown file format, treating as plain text:" << suffix;
        song = Song::fromPlainText(content, baseName);
    }

    return song;
}

int SongLibrary::nextId()
{
    return m_nextId++;
}

} // namespace Clarity
