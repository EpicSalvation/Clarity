#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Clarity {

/**
 * @brief Represents a single verse returned from the ESV API
 */
struct EsvVerse {
    int number;      ///< Verse number (1-indexed)
    QString text;    ///< Verse text content
};

/**
 * @brief Represents a complete passage fetched from the ESV API
 */
struct EsvPassage {
    QString canonical;       ///< Canonical reference (e.g., "John 3:16-17")
    QList<EsvVerse> verses;  ///< Individual parsed verses
    QString fullText;        ///< Full passage text as returned by API
    QString copyright;       ///< Copyright notice (required by ESV terms)
    int verseCount;          ///< Number of verses in the passage

    bool isValid() const { return !canonical.isEmpty() && verseCount > 0; }
};

/**
 * @brief HTTP client for the ESV Bible API (api.esv.org v3)
 *
 * Handles authentication, request construction, and response parsing
 * for the ESV passage text endpoint.
 *
 * ESV API usage restrictions (enforced by this client):
 * - Max 500 cached verses at any time
 * - Max 5,000 queries/day, 1,000/hour, 60/minute
 * - Cached content should be periodically flushed
 *
 * @see https://api.esv.org/docs/passage-text/
 */
class EsvApiClient : public QObject {
    Q_OBJECT

public:
    explicit EsvApiClient(QObject* parent = nullptr);

    /**
     * @brief Set the API key for authentication
     * @param key ESV API key (obtained from api.esv.org)
     */
    void setApiKey(const QString& key);

    /**
     * @brief Get the current API key
     */
    QString apiKey() const { return m_apiKey; }

    /**
     * @brief Check if a valid API key is configured
     */
    bool hasApiKey() const { return !m_apiKey.trimmed().isEmpty(); }

    /**
     * @brief Fetch a passage from the ESV API (asynchronous)
     * @param reference Bible reference (e.g., "John 3:16", "Romans 8:28-30")
     *
     * Results are delivered via passageFetched() or fetchError() signals.
     */
    void fetchPassage(const QString& reference);

    /**
     * @brief Get the number of ESV verses currently cached in the session
     *
     * Per ESV API terms, applications may not cache more than 500 verses.
     */
    int cachedVerseCount() const { return m_cachedVerseCount; }

    /**
     * @brief Add to the cached verse count (called when ESV slides are created)
     */
    void addCachedVerses(int count);

    /**
     * @brief Subtract from the cached verse count (called when ESV slides are removed)
     */
    void removeCachedVerses(int count);

    /**
     * @brief Reset the cached verse count to zero (called on purge/close)
     */
    void resetCachedVerseCount();

    /**
     * @brief Check if adding more verses would exceed the 500-verse cache limit
     */
    bool wouldExceedCacheLimit(int additionalVerses) const;

    /**
     * @brief Maximum number of verses allowed to be cached (ESV API terms)
     */
    static constexpr int MAX_CACHED_VERSES = 500;

signals:
    /**
     * @brief Emitted when a passage is successfully fetched and parsed
     */
    void passageFetched(const Clarity::EsvPassage& passage);

    /**
     * @brief Emitted when a fetch fails
     */
    void fetchError(const QString& errorMessage);

    /**
     * @brief Emitted when the cached verse count changes
     */
    void cachedVerseCountChanged(int count);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    /**
     * @brief Parse the API JSON response into an EsvPassage
     */
    EsvPassage parseResponse(const QByteArray& data) const;

    /**
     * @brief Parse verse text with [N] markers into individual verses
     */
    QList<EsvVerse> parseVerses(const QString& passageText) const;

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    int m_cachedVerseCount;

    static const QString API_BASE_URL;
};

} // namespace Clarity
