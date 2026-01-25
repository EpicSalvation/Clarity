#pragma once

#include "Song.h"
#include <QObject>
#include <QList>
#include <QString>

namespace Clarity {

/**
 * @brief Manages a persistent collection of songs
 *
 * The SongLibrary stores songs in a JSON file in the user's
 * config directory. It provides methods for:
 * - Adding, updating, and removing songs
 * - Searching by title, author, or lyrics
 * - Importing songs from files
 * - Tracking recently used songs
 */
class SongLibrary : public QObject {
    Q_OBJECT

public:
    explicit SongLibrary(QObject* parent = nullptr);
    ~SongLibrary();

    /**
     * @brief Load the library from disk
     * @return true if loaded successfully (or file doesn't exist yet)
     */
    bool loadLibrary();

    /**
     * @brief Save the library to disk
     * @return true if saved successfully
     */
    bool saveLibrary();

    /**
     * @brief Get all songs in the library
     */
    QList<Song> allSongs() const { return m_songs; }

    /**
     * @brief Get song count
     */
    int songCount() const { return m_songs.count(); }

    /**
     * @brief Get a song by ID
     * @param id Song ID
     * @return Song object (empty title if not found)
     */
    Song getSong(int id) const;

    /**
     * @brief Get song index by ID
     * @param id Song ID
     * @return Index in list, or -1 if not found
     */
    int indexOf(int id) const;

    /**
     * @brief Add a new song to the library
     * @param song Song to add (ID will be assigned automatically)
     * @return Assigned song ID
     */
    int addSong(const Song& song);

    /**
     * @brief Update an existing song
     * @param id Song ID
     * @param song Updated song data
     * @return true if song was found and updated
     */
    bool updateSong(int id, const Song& song);

    /**
     * @brief Remove a song from the library
     * @param id Song ID
     * @return true if song was found and removed
     */
    bool removeSong(int id);

    /**
     * @brief Search songs by query string
     * @param query Search text (matches title, author, and lyrics)
     * @return List of matching songs
     */
    QList<Song> search(const QString& query) const;

    /**
     * @brief Get recently used songs
     * @param count Maximum number of songs to return
     * @return List of songs sorted by last used date (most recent first)
     */
    QList<Song> recentSongs(int count = 10) const;

    /**
     * @brief Mark a song as recently used
     * @param id Song ID
     */
    void markAsUsed(int id);

    /**
     * @brief Import a song from a file
     * @param filePath Path to the file
     * @return Imported song (empty title if import failed)
     *
     * Supported formats:
     * - .xml (OpenLyrics)
     * - .txt (plain text with section markers)
     */
    Song importFromFile(const QString& filePath);

    /**
     * @brief Get the library file path
     */
    QString libraryPath() const { return m_libraryPath; }

signals:
    /**
     * @brief Emitted when a song is added
     */
    void songAdded(int id);

    /**
     * @brief Emitted when a song is updated
     */
    void songUpdated(int id);

    /**
     * @brief Emitted when a song is removed
     */
    void songRemoved(int id);

    /**
     * @brief Emitted when the library is loaded
     */
    void libraryLoaded();

private:
    /**
     * @brief Generate next unique song ID
     */
    int nextId();

    QList<Song> m_songs;
    QString m_libraryPath;
    int m_nextId;
};

} // namespace Clarity
