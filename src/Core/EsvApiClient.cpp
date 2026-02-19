#include "EsvApiClient.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

namespace Clarity {

const QString EsvApiClient::API_BASE_URL = QStringLiteral("https://api.esv.org/v3/passage/text/");

EsvApiClient::EsvApiClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_cachedVerseCount(0)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &EsvApiClient::onNetworkReply);
}

void EsvApiClient::setApiKey(const QString& key)
{
    m_apiKey = key.trimmed();
}

void EsvApiClient::fetchPassage(const QString& reference)
{
    if (!hasApiKey()) {
        emit fetchError(tr("No ESV API key configured. Please set your API key in Settings > Bible."));
        return;
    }

    if (reference.trimmed().isEmpty()) {
        emit fetchError(tr("No reference provided."));
        return;
    }

    // Build the request URL with query parameters
    QUrl url(API_BASE_URL);
    QUrlQuery query;
    query.addQueryItem("q", reference.trimmed());
    query.addQueryItem("include-passage-references", "false");
    query.addQueryItem("include-verse-numbers", "true");
    query.addQueryItem("include-footnotes", "false");
    query.addQueryItem("include-footnote-body", "false");
    query.addQueryItem("include-headings", "false");
    query.addQueryItem("include-short-copyright", "true");
    query.addQueryItem("indent-poetry", "false");
    query.addQueryItem("indent-paragraphs", "0");
    query.addQueryItem("indent-declares", "0");
    query.addQueryItem("indent-psalm-doxology", "0");
    url.setQuery(query);

    // Build the request with authorization header
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QStringLiteral("Token %1").arg(m_apiKey).toUtf8());

    qDebug() << "EsvApiClient: Fetching passage:" << reference;
    m_networkManager->get(request);
}

void EsvApiClient::addCachedVerses(int count)
{
    if (count > 0) {
        m_cachedVerseCount += count;
        emit cachedVerseCountChanged(m_cachedVerseCount);
        qDebug() << "EsvApiClient: Cached verse count now:" << m_cachedVerseCount;
    }
}

void EsvApiClient::removeCachedVerses(int count)
{
    if (count > 0) {
        m_cachedVerseCount = qMax(0, m_cachedVerseCount - count);
        emit cachedVerseCountChanged(m_cachedVerseCount);
        qDebug() << "EsvApiClient: Cached verse count now:" << m_cachedVerseCount;
    }
}

void EsvApiClient::resetCachedVerseCount()
{
    m_cachedVerseCount = 0;
    emit cachedVerseCountChanged(0);
    qDebug() << "EsvApiClient: Cached verse count reset to 0";
}

bool EsvApiClient::wouldExceedCacheLimit(int additionalVerses) const
{
    return (m_cachedVerseCount + additionalVerses) > MAX_CACHED_VERSES;
}

void EsvApiClient::onNetworkReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg;
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 401 || statusCode == 403) {
            errorMsg = tr("ESV API authentication failed. Please check your API key in Settings > Bible.");
        } else if (statusCode == 429) {
            errorMsg = tr("ESV API rate limit exceeded. Please wait before making more requests.");
        } else if (reply->error() == QNetworkReply::HostNotFoundError ||
                   reply->error() == QNetworkReply::ConnectionRefusedError) {
            errorMsg = tr("Could not connect to the ESV API. Please check your internet connection.");
        } else {
            errorMsg = tr("ESV API error: %1 (HTTP %2)").arg(reply->errorString()).arg(statusCode);
        }

        qWarning() << "EsvApiClient: Network error:" << reply->errorString() << "HTTP" << statusCode;
        emit fetchError(errorMsg);
        return;
    }

    QByteArray data = reply->readAll();
    EsvPassage passage = parseResponse(data);

    if (passage.isValid()) {
        qDebug() << "EsvApiClient: Fetched" << passage.verseCount << "verses for" << passage.canonical;
        emit passageFetched(passage);
    } else {
        emit fetchError(tr("No passage found for the given reference. Please check the reference format."));
    }
}

EsvPassage EsvApiClient::parseResponse(const QByteArray& data) const
{
    EsvPassage passage;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "EsvApiClient: JSON parse error:" << parseError.errorString();
        return passage;
    }

    QJsonObject root = doc.object();
    passage.canonical = root["canonical"].toString();

    // Extract the passage text from the "passages" array
    QJsonArray passages = root["passages"].toArray();
    if (passages.isEmpty()) {
        qWarning() << "EsvApiClient: No passages in response";
        return passage;
    }

    // Combine all passages (usually just one)
    QStringList passageTexts;
    for (const QJsonValue& val : passages) {
        passageTexts.append(val.toString());
    }
    passage.fullText = passageTexts.join("\n\n");

    // Trim whitespace from the full text
    passage.fullText = passage.fullText.trimmed();

    // Extract copyright from the end of the text (ESV includes "(ESV)" marker)
    passage.copyright = QStringLiteral("Scripture quotations are from the ESV\u00AE Bible "
        "(The Holy Bible, English Standard Version\u00AE), "
        "\u00A9 2001 by Crossway, a publishing ministry of Good News Publishers.");

    // Parse individual verses from the text
    passage.verses = parseVerses(passage.fullText);
    passage.verseCount = passage.verses.count();

    return passage;
}

QList<EsvVerse> EsvApiClient::parseVerses(const QString& passageText) const
{
    QList<EsvVerse> verses;

    // The ESV API with include-verse-numbers=true produces text like:
    // "[16] For God so loved the world... [17] For God did not send..."
    // We split on the [N] markers to extract individual verses.
    static const QRegularExpression versePattern(R"(\[(\d+)\]\s*)");

    QRegularExpressionMatchIterator it = versePattern.globalMatch(passageText);

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
        EsvVerse verse;
        verse.number = matches[i].number;

        int start = matches[i].textStart;
        int end = (i + 1 < matches.count())
            ? passageText.lastIndexOf(QRegularExpression(R"(\[)"), matches[i + 1].textStart)
            : passageText.length();

        // Find the position of the next [N] marker, not just any [
        if (i + 1 < matches.count()) {
            // Search backward from the next match to find the opening bracket
            QString nextMarker = QStringLiteral("[%1]").arg(matches[i + 1].number);
            int markerPos = passageText.indexOf(nextMarker, start);
            if (markerPos >= 0) {
                end = markerPos;
            }
        }

        verse.text = passageText.mid(start, end - start).trimmed();

        // Remove trailing "(ESV)" copyright marker if present on last verse
        if (verse.text.endsWith("(ESV)")) {
            verse.text.chop(5);
            verse.text = verse.text.trimmed();
        }

        if (!verse.text.isEmpty()) {
            verses.append(verse);
        }
    }

    // If no verse markers found, treat entire text as a single verse
    if (verses.isEmpty() && !passageText.trimmed().isEmpty()) {
        EsvVerse verse;
        verse.number = 1;
        verse.text = passageText.trimmed();
        if (verse.text.endsWith("(ESV)")) {
            verse.text.chop(5);
            verse.text = verse.text.trimmed();
        }
        verses.append(verse);
    }

    return verses;
}

} // namespace Clarity
