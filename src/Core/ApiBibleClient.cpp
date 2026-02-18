#include "ApiBibleClient.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

namespace Clarity {

const QString ApiBibleClient::API_BASE_URL = QStringLiteral("https://api.scripture.api.bible/v1");

ApiBibleClient::ApiBibleClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void ApiBibleClient::setApiKey(const QString& key)
{
    m_apiKey = key.trimmed();
}

QNetworkRequest ApiBibleClient::buildRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    // API.bible uses a custom 'api-key' header (not Authorization)
    request.setRawHeader("api-key", m_apiKey.toUtf8());
    return request;
}

void ApiBibleClient::fetchBibles(const QString& languageFilter)
{
    if (!hasApiKey()) {
        emit fetchError(tr("No API.bible key configured. Please set your API key in Settings > Bible."));
        return;
    }

    QUrl url(API_BASE_URL + "/bibles");

    if (!languageFilter.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem("language", languageFilter);
        url.setQuery(query);
    }

    QNetworkRequest request = buildRequest(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onBiblesReply(reply);
    });

    qDebug() << "ApiBibleClient: Fetching Bibles list" << (languageFilter.isEmpty() ? "" : "(language: " + languageFilter + ")");
}

void ApiBibleClient::fetchPassage(const QString& bibleId, const QString& passageId)
{
    if (!hasApiKey()) {
        emit fetchError(tr("No API.bible key configured. Please set your API key in Settings > Bible."));
        return;
    }

    if (bibleId.isEmpty() || passageId.isEmpty()) {
        emit fetchError(tr("Bible version and passage reference are required."));
        return;
    }

    // Use content-type=text for plain text, include verse numbers
    QUrl url(API_BASE_URL + "/bibles/" + bibleId + "/passages/" + passageId);
    QUrlQuery query;
    query.addQueryItem("content-type", "text");
    query.addQueryItem("include-notes", "false");
    query.addQueryItem("include-titles", "false");
    query.addQueryItem("include-chapter-numbers", "false");
    query.addQueryItem("include-verse-numbers", "true");
    query.addQueryItem("include-verse-spans", "false");
    url.setQuery(query);

    QNetworkRequest request = buildRequest(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onPassageReply(reply);
    });

    qDebug() << "ApiBibleClient: Fetching passage:" << passageId << "from Bible:" << bibleId;
}

void ApiBibleClient::searchPassage(const QString& bibleId, const QString& queryText)
{
    if (!hasApiKey()) {
        emit fetchError(tr("No API.bible key configured. Please set your API key in Settings > Bible."));
        return;
    }

    if (bibleId.isEmpty() || queryText.trimmed().isEmpty()) {
        emit fetchError(tr("Bible version and search query are required."));
        return;
    }

    QUrl url(API_BASE_URL + "/bibles/" + bibleId + "/search");
    QUrlQuery query;
    query.addQueryItem("query", queryText.trimmed());
    url.setQuery(query);

    QNetworkRequest request = buildRequest(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });

    qDebug() << "ApiBibleClient: Searching:" << queryText << "in Bible:" << bibleId;
}

void ApiBibleClient::onBiblesReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorMsg;

        if (statusCode == 401 || statusCode == 403) {
            errorMsg = tr("API.bible authentication failed. Please check your API key in Settings > Bible.");
        } else if (statusCode == 429) {
            errorMsg = tr("API.bible rate limit exceeded. Please wait before making more requests.");
        } else if (reply->error() == QNetworkReply::HostNotFoundError ||
                   reply->error() == QNetworkReply::ConnectionRefusedError) {
            errorMsg = tr("Could not connect to API.bible. Please check your internet connection.");
        } else {
            errorMsg = tr("API.bible error: %1 (HTTP %2)").arg(reply->errorString()).arg(statusCode);
        }

        qWarning() << "ApiBibleClient: Bibles request error:" << reply->errorString() << "HTTP" << statusCode;
        emit fetchError(errorMsg);
        return;
    }

    QByteArray data = reply->readAll();
    m_cachedBibles = parseBiblesResponse(data);

    qDebug() << "ApiBibleClient: Loaded" << m_cachedBibles.count() << "Bible versions";
    emit biblesLoaded(m_cachedBibles);
}

void ApiBibleClient::onPassageReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorMsg;

        if (statusCode == 401 || statusCode == 403) {
            errorMsg = tr("API.bible authentication failed. Please check your API key in Settings > Bible.");
        } else if (statusCode == 404) {
            errorMsg = tr("Passage not found. Please check the reference format (e.g., JHN.3.16 or JHN.3.16-JHN.3.18).");
        } else if (statusCode == 429) {
            errorMsg = tr("API.bible rate limit exceeded. Please wait before making more requests.");
        } else if (reply->error() == QNetworkReply::HostNotFoundError ||
                   reply->error() == QNetworkReply::ConnectionRefusedError) {
            errorMsg = tr("Could not connect to API.bible. Please check your internet connection.");
        } else {
            errorMsg = tr("API.bible error: %1 (HTTP %2)").arg(reply->errorString()).arg(statusCode);
        }

        qWarning() << "ApiBibleClient: Passage request error:" << reply->errorString() << "HTTP" << statusCode;
        emit fetchError(errorMsg);
        return;
    }

    QByteArray data = reply->readAll();
    ApiBiblePassage passage = parsePassageResponse(data);

    if (passage.isValid()) {
        qDebug() << "ApiBibleClient: Fetched" << passage.verseCount << "verses for" << passage.reference;
        emit passageFetched(passage);
    } else {
        emit fetchError(tr("No passage found for the given reference. Please check the reference format."));
    }
}

void ApiBibleClient::onSearchReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorMsg;

        if (statusCode == 401 || statusCode == 403) {
            errorMsg = tr("API.bible authentication failed. Please check your API key.");
        } else if (statusCode == 429) {
            errorMsg = tr("API.bible rate limit exceeded. Please wait before making more requests.");
        } else {
            errorMsg = tr("API.bible search error: %1 (HTTP %2)").arg(reply->errorString()).arg(statusCode);
        }

        qWarning() << "ApiBibleClient: Search error:" << reply->errorString() << "HTTP" << statusCode;
        emit fetchError(errorMsg);
        return;
    }

    QByteArray data = reply->readAll();
    ApiBiblePassage passage = parseSearchResponse(data);

    if (passage.isValid()) {
        qDebug() << "ApiBibleClient: Search found" << passage.verseCount << "verses for" << passage.reference;
        emit passageFetched(passage);
    } else {
        emit fetchError(tr("No results found for the search query. Try a scripture reference (e.g., John 3:16)."));
    }
}

QList<ApiBibleVersion> ApiBibleClient::parseBiblesResponse(const QByteArray& data) const
{
    QList<ApiBibleVersion> bibles;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ApiBibleClient: JSON parse error (bibles):" << parseError.errorString();
        return bibles;
    }

    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();

    for (const QJsonValue& val : dataArray) {
        QJsonObject bibleObj = val.toObject();

        // Only include text Bibles (skip audio-only)
        if (bibleObj["type"].toString() != "text") {
            continue;
        }

        ApiBibleVersion version;
        version.id = bibleObj["id"].toString();
        version.abbreviation = bibleObj["abbreviation"].toString();
        version.name = bibleObj["name"].toString();
        version.description = bibleObj["descriptionLocal"].toString();
        version.copyright = bibleObj["copyright"].toString();

        QJsonObject langObj = bibleObj["language"].toObject();
        version.language = langObj["name"].toString();
        version.languageId = langObj["id"].toString();

        if (!version.id.isEmpty() && !version.name.isEmpty()) {
            bibles.append(version);
        }
    }

    return bibles;
}

ApiBiblePassage ApiBibleClient::parsePassageResponse(const QByteArray& data) const
{
    ApiBiblePassage passage;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ApiBibleClient: JSON parse error (passage):" << parseError.errorString();
        return passage;
    }

    QJsonObject root = doc.object();
    QJsonObject dataObj = root["data"].toObject();

    passage.reference = dataObj["reference"].toString();
    passage.copyright = dataObj["copyright"].toString();

    // The content field contains the passage text (format depends on content-type param)
    QString content = dataObj["content"].toString();
    passage.fullText = stripHtml(content).trimmed();

    // Get verse count from API
    passage.verseCount = dataObj["verseCount"].toInt(0);

    // Parse individual verses from the text
    passage.verses = parseVerses(passage.fullText);
    if (passage.verses.count() > 0) {
        passage.verseCount = passage.verses.count();
    }

    // If we still have no verse count but have text, count it as one
    if (passage.verseCount == 0 && !passage.fullText.isEmpty()) {
        passage.verseCount = 1;
    }

    return passage;
}

ApiBiblePassage ApiBibleClient::parseSearchResponse(const QByteArray& data) const
{
    ApiBiblePassage passage;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ApiBibleClient: JSON parse error (search):" << parseError.errorString();
        return passage;
    }

    QJsonObject root = doc.object();
    QJsonObject dataObj = root["data"].toObject();

    // Search results can contain "passages" (for reference searches) or "verses" (for keyword searches)
    QJsonArray passagesArray = dataObj["passages"].toArray();
    if (!passagesArray.isEmpty()) {
        // Reference-based search result - use first passage
        QJsonObject passageObj = passagesArray.first().toObject();
        passage.reference = passageObj["reference"].toString();
        passage.copyright = passageObj["copyright"].toString();

        QString content = passageObj["content"].toString();
        passage.fullText = stripHtml(content).trimmed();

        passage.verseCount = passageObj["verseCount"].toInt(0);
        passage.verses = parseVerses(passage.fullText);
        if (passage.verses.count() > 0) {
            passage.verseCount = passage.verses.count();
        }
        if (passage.verseCount == 0 && !passage.fullText.isEmpty()) {
            passage.verseCount = 1;
        }
        return passage;
    }

    // Keyword-based search result
    QJsonArray versesArray = dataObj["verses"].toArray();
    if (!versesArray.isEmpty()) {
        // Combine all verse results
        QStringList references;
        for (const QJsonValue& val : versesArray) {
            QJsonObject verseObj = val.toObject();
            ApiBibleVerse verse;
            // Extract verse number from the ID (e.g., "JHN.3.16" -> 16)
            QString verseId = verseObj["id"].toString();
            QStringList parts = verseId.split('.');
            verse.number = parts.isEmpty() ? 0 : parts.last().toInt();
            verse.text = stripHtml(verseObj["text"].toString()).trimmed();

            if (!verse.text.isEmpty()) {
                passage.verses.append(verse);
            }

            QString ref = verseObj["reference"].toString();
            if (!ref.isEmpty() && !references.contains(ref)) {
                references.append(ref);
            }
        }

        passage.reference = references.join("; ");
        passage.verseCount = passage.verses.count();

        // Build full text from verses
        QStringList textParts;
        for (const ApiBibleVerse& v : passage.verses) {
            textParts.append(v.text);
        }
        passage.fullText = textParts.join(" ");
    }

    return passage;
}

QString ApiBibleClient::stripHtml(const QString& html) const
{
    // Remove HTML tags while preserving paragraph/line breaks
    QString text = html;

    // Replace <p> and <br> tags with newlines before stripping all tags
    text.replace(QRegularExpression("<br\\s*/?>"), "\n");
    text.replace(QRegularExpression("<p[^>]*>"), "\n");
    text.replace(QRegularExpression("</p>"), "");

    // Strip remaining HTML tags
    text.replace(QRegularExpression("<[^>]+>"), "");

    // Decode common HTML entities
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&quot;", "\"");
    text.replace("&#39;", "'");
    text.replace("&nbsp;", " ");

    // Collapse multiple whitespace/newlines
    text.replace(QRegularExpression("[ \\t]+"), " ");
    text.replace(QRegularExpression("\\n\\s*\\n+"), "\n");

    return text.trimmed();
}

QList<ApiBibleVerse> ApiBibleClient::parseVerses(const QString& content) const
{
    QList<ApiBibleVerse> verses;

    // API.bible with include-verse-numbers=true and content-type=text
    // typically produces text with verse number markers like [16] or just numbers at line starts.
    // Try the [N] format first (common with text content-type).
    static const QRegularExpression bracketPattern(R"(\[(\d+)\]\s*)");

    QRegularExpressionMatchIterator it = bracketPattern.globalMatch(content);

    struct VerseMatch {
        int number;
        int textStart;
    };

    QList<VerseMatch> matches;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        VerseMatch vm;
        vm.number = match.captured(1).toInt();
        vm.textStart = match.capturedEnd();
        matches.append(vm);
    }

    // Extract text between consecutive markers
    for (int i = 0; i < matches.count(); ++i) {
        ApiBibleVerse verse;
        verse.number = matches[i].number;

        int start = matches[i].textStart;
        int end = content.length();

        if (i + 1 < matches.count()) {
            // Find the opening bracket of the next marker
            QString nextMarker = QStringLiteral("[%1]").arg(matches[i + 1].number);
            int markerPos = content.indexOf(nextMarker, start);
            if (markerPos >= 0) {
                end = markerPos;
            }
        }

        verse.text = content.mid(start, end - start).trimmed();
        if (!verse.text.isEmpty()) {
            verses.append(verse);
        }
    }

    // If no [N] markers found, try plain numeric markers at line/sentence starts
    if (verses.isEmpty()) {
        // Try matching "N " at the start of lines or after newlines
        static const QRegularExpression numPattern(R"((?:^|\n)\s*(\d+)\s+)");
        it = numPattern.globalMatch(content);

        matches.clear();
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            VerseMatch vm;
            vm.number = match.captured(1).toInt();
            vm.textStart = match.capturedEnd();
            matches.append(vm);
        }

        for (int i = 0; i < matches.count(); ++i) {
            ApiBibleVerse verse;
            verse.number = matches[i].number;

            int start = matches[i].textStart;
            int end = content.length();

            if (i + 1 < matches.count()) {
                // Look backward from next match start for start of its line
                int nextMatchStart = matches[i + 1].textStart - QString::number(matches[i + 1].number).length() - 1;
                if (nextMatchStart > start) {
                    end = nextMatchStart;
                }
            }

            verse.text = content.mid(start, end - start).trimmed();
            if (!verse.text.isEmpty()) {
                verses.append(verse);
            }
        }
    }

    // Last resort: treat entire text as a single verse
    if (verses.isEmpty() && !content.trimmed().isEmpty()) {
        ApiBibleVerse verse;
        verse.number = 1;
        verse.text = content.trimmed();
        verses.append(verse);
    }

    return verses;
}

} // namespace Clarity
