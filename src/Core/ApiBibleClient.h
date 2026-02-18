#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Clarity {

/**
 * @brief Represents a single verse returned from the API.bible service
 */
struct ApiBibleVerse {
    int number;      ///< Verse number (1-indexed)
    QString text;    ///< Verse text content
};

/**
 * @brief Represents a complete passage fetched from API.bible
 */
struct ApiBiblePassage {
    QString reference;           ///< Human-readable reference (e.g., "John 3:16")
    QList<ApiBibleVerse> verses; ///< Individual parsed verses
    QString fullText;            ///< Full passage text as returned by API
    QString copyright;           ///< Copyright notice for the Bible version
    QString bibleAbbreviation;   ///< Bible version abbreviation (e.g., "KJV")
    int verseCount = 0;          ///< Number of verses in the passage

    bool isValid() const { return !reference.isEmpty() && verseCount > 0; }
};

/**
 * @brief Represents a Bible version available through API.bible
 */
struct ApiBibleVersion {
    QString id;           ///< Bible ID used in API requests
    QString abbreviation; ///< Short code (e.g., "KJV", "ASV")
    QString name;         ///< Full display name
    QString language;     ///< Language name (e.g., "English")
    QString languageId;   ///< Language code (e.g., "eng")
    QString description;  ///< Description of the version
    QString copyright;    ///< Copyright statement
};

/**
 * @brief HTTP client for the API.bible service (api.scripture.api.bible v1)
 *
 * Handles authentication, request construction, and response parsing
 * for the API.bible passage and Bibles endpoints.
 *
 * API.bible hosts nearly 2500 Bible versions across 1600+ languages.
 * Authentication uses a custom 'api-key' header.
 *
 * @see https://docs.api.bible
 */
class ApiBibleClient : public QObject {
    Q_OBJECT

public:
    explicit ApiBibleClient(QObject* parent = nullptr);

    /**
     * @brief Set the API key for authentication
     * @param key API.bible key (obtained from scripture.api.bible)
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
     * @brief Fetch the list of available Bible versions (asynchronous)
     *
     * Results are delivered via biblesLoaded() or fetchError() signals.
     * Optionally filter by language.
     */
    void fetchBibles(const QString& languageFilter = "eng");

    /**
     * @brief Fetch a passage from API.bible (asynchronous)
     * @param bibleId The Bible version ID (e.g., "de4e12af7f28f599-02")
     * @param passageId Dot-delimited passage ID (e.g., "JHN.3.16-JHN.3.18")
     *
     * Results are delivered via passageFetched() or fetchError() signals.
     */
    void fetchPassage(const QString& bibleId, const QString& passageId);

    /**
     * @brief Search for passages in a Bible (asynchronous)
     * @param bibleId The Bible version ID
     * @param query Search query (can be a reference like "John 3:16" or keywords)
     *
     * Results are delivered via passageFetched() or fetchError() signals.
     */
    void searchPassage(const QString& bibleId, const QString& query);

    /**
     * @brief Get the cached list of available Bibles
     */
    QList<ApiBibleVersion> cachedBibles() const { return m_cachedBibles; }

    /**
     * @brief Check if the Bibles list has been loaded
     */
    bool hasCachedBibles() const { return !m_cachedBibles.isEmpty(); }

signals:
    /**
     * @brief Emitted when the list of Bible versions is loaded
     */
    void biblesLoaded(const QList<Clarity::ApiBibleVersion>& bibles);

    /**
     * @brief Emitted when a passage is successfully fetched and parsed
     */
    void passageFetched(const Clarity::ApiBiblePassage& passage);

    /**
     * @brief Emitted when a fetch fails
     */
    void fetchError(const QString& errorMessage);

private slots:
    void onBiblesReply(QNetworkReply* reply);
    void onPassageReply(QNetworkReply* reply);
    void onSearchReply(QNetworkReply* reply);

private:
    /**
     * @brief Build a network request with the API key header
     */
    QNetworkRequest buildRequest(const QUrl& url) const;

    /**
     * @brief Parse the Bibles list response
     */
    QList<ApiBibleVersion> parseBiblesResponse(const QByteArray& data) const;

    /**
     * @brief Parse a passage response into an ApiBiblePassage
     */
    ApiBiblePassage parsePassageResponse(const QByteArray& data) const;

    /**
     * @brief Parse a search response into an ApiBiblePassage
     */
    ApiBiblePassage parseSearchResponse(const QByteArray& data) const;

    /**
     * @brief Strip HTML tags from API.bible content
     */
    QString stripHtml(const QString& html) const;

    /**
     * @brief Parse verse text with verse number markers into individual verses
     */
    QList<ApiBibleVerse> parseVerses(const QString& content) const;

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QList<ApiBibleVersion> m_cachedBibles;

    static const QString API_BASE_URL;
};

} // namespace Clarity
