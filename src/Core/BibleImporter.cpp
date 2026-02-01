#include "BibleImporter.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

namespace Clarity {

// =============================================================================
// Book Name Mapping Tables
// =============================================================================

// OSIS book IDs to canonical names
static const QMap<QString, QString> OSIS_BOOK_IDS = {
    // Old Testament
    {"gen", "Genesis"}, {"exod", "Exodus"}, {"lev", "Leviticus"},
    {"num", "Numbers"}, {"deut", "Deuteronomy"}, {"josh", "Joshua"},
    {"judg", "Judges"}, {"ruth", "Ruth"}, {"1sam", "1 Samuel"},
    {"2sam", "2 Samuel"}, {"1kgs", "1 Kings"}, {"2kgs", "2 Kings"},
    {"1chr", "1 Chronicles"}, {"2chr", "2 Chronicles"}, {"ezra", "Ezra"},
    {"neh", "Nehemiah"}, {"esth", "Esther"}, {"job", "Job"},
    {"ps", "Psalms"}, {"prov", "Proverbs"}, {"eccl", "Ecclesiastes"},
    {"song", "Song of Solomon"}, {"isa", "Isaiah"}, {"jer", "Jeremiah"},
    {"lam", "Lamentations"}, {"ezek", "Ezekiel"}, {"dan", "Daniel"},
    {"hos", "Hosea"}, {"joel", "Joel"}, {"amos", "Amos"},
    {"obad", "Obadiah"}, {"jonah", "Jonah"}, {"mic", "Micah"},
    {"nah", "Nahum"}, {"hab", "Habakkuk"}, {"zeph", "Zephaniah"},
    {"hag", "Haggai"}, {"zech", "Zechariah"}, {"mal", "Malachi"},
    // New Testament
    {"matt", "Matthew"}, {"mark", "Mark"}, {"luke", "Luke"},
    {"john", "John"}, {"acts", "Acts"}, {"rom", "Romans"},
    {"1cor", "1 Corinthians"}, {"2cor", "2 Corinthians"}, {"gal", "Galatians"},
    {"eph", "Ephesians"}, {"phil", "Philippians"}, {"col", "Colossians"},
    {"1thess", "1 Thessalonians"}, {"2thess", "2 Thessalonians"},
    {"1tim", "1 Timothy"}, {"2tim", "2 Timothy"}, {"titus", "Titus"},
    {"phlm", "Philemon"}, {"heb", "Hebrews"}, {"jas", "James"},
    {"1pet", "1 Peter"}, {"2pet", "2 Peter"}, {"1john", "1 John"},
    {"2john", "2 John"}, {"3john", "3 John"}, {"jude", "Jude"},
    {"rev", "Revelation"}
};

// USFM book IDs (3-letter codes) to canonical names
static const QMap<QString, QString> USFM_BOOK_IDS = {
    // Old Testament
    {"gen", "Genesis"}, {"exo", "Exodus"}, {"lev", "Leviticus"},
    {"num", "Numbers"}, {"deu", "Deuteronomy"}, {"jos", "Joshua"},
    {"jdg", "Judges"}, {"rut", "Ruth"}, {"1sa", "1 Samuel"},
    {"2sa", "2 Samuel"}, {"1ki", "1 Kings"}, {"2ki", "2 Kings"},
    {"1ch", "1 Chronicles"}, {"2ch", "2 Chronicles"}, {"ezr", "Ezra"},
    {"neh", "Nehemiah"}, {"est", "Esther"}, {"job", "Job"},
    {"psa", "Psalms"}, {"pro", "Proverbs"}, {"ecc", "Ecclesiastes"},
    {"sng", "Song of Solomon"}, {"isa", "Isaiah"}, {"jer", "Jeremiah"},
    {"lam", "Lamentations"}, {"ezk", "Ezekiel"}, {"dan", "Daniel"},
    {"hos", "Hosea"}, {"jol", "Joel"}, {"amo", "Amos"},
    {"oba", "Obadiah"}, {"jon", "Jonah"}, {"mic", "Micah"},
    {"nam", "Nahum"}, {"hab", "Habakkuk"}, {"zep", "Zephaniah"},
    {"hag", "Haggai"}, {"zec", "Zechariah"}, {"mal", "Malachi"},
    // New Testament
    {"mat", "Matthew"}, {"mrk", "Mark"}, {"luk", "Luke"},
    {"jhn", "John"}, {"act", "Acts"}, {"rom", "Romans"},
    {"1co", "1 Corinthians"}, {"2co", "2 Corinthians"}, {"gal", "Galatians"},
    {"eph", "Ephesians"}, {"php", "Philippians"}, {"col", "Colossians"},
    {"1th", "1 Thessalonians"}, {"2th", "2 Thessalonians"},
    {"1ti", "1 Timothy"}, {"2ti", "2 Timothy"}, {"tit", "Titus"},
    {"phm", "Philemon"}, {"heb", "Hebrews"}, {"jas", "James"},
    {"1pe", "1 Peter"}, {"2pe", "2 Peter"}, {"1jn", "1 John"},
    {"2jn", "2 John"}, {"3jn", "3 John"}, {"jud", "Jude"},
    {"rev", "Revelation"}
};

// Zefania numeric book IDs (1-66) to canonical names
static const QStringList ZEFANIA_BOOK_ORDER = {
    "Genesis", "Exodus", "Leviticus", "Numbers", "Deuteronomy",
    "Joshua", "Judges", "Ruth", "1 Samuel", "2 Samuel",
    "1 Kings", "2 Kings", "1 Chronicles", "2 Chronicles",
    "Ezra", "Nehemiah", "Esther", "Job", "Psalms", "Proverbs",
    "Ecclesiastes", "Song of Solomon", "Isaiah", "Jeremiah", "Lamentations",
    "Ezekiel", "Daniel", "Hosea", "Joel", "Amos",
    "Obadiah", "Jonah", "Micah", "Nahum", "Habakkuk",
    "Zephaniah", "Haggai", "Zechariah", "Malachi",
    "Matthew", "Mark", "Luke", "John", "Acts",
    "Romans", "1 Corinthians", "2 Corinthians", "Galatians", "Ephesians",
    "Philippians", "Colossians", "1 Thessalonians", "2 Thessalonians",
    "1 Timothy", "2 Timothy", "Titus", "Philemon", "Hebrews",
    "James", "1 Peter", "2 Peter", "1 John", "2 John", "3 John",
    "Jude", "Revelation"
};

// =============================================================================
// BibleImporter Base Class
// =============================================================================

BibleImporter::BibleImporter(QObject* parent)
    : QObject(parent)
{
}

BibleImporter::~BibleImporter()
{
}

QString BibleImporter::normalizeBookName(const QString& name)
{
    QString lower = name.trimmed().toLower();

    // Remove periods (common in OSIS IDs like "Gen.")
    lower.remove('.');

    // Try OSIS IDs first
    if (OSIS_BOOK_IDS.contains(lower)) {
        return OSIS_BOOK_IDS[lower];
    }

    // Try USFM IDs
    if (USFM_BOOK_IDS.contains(lower)) {
        return USFM_BOOK_IDS[lower];
    }

    // Try direct match with canonical names (case-insensitive)
    for (const QString& canonical : ZEFANIA_BOOK_ORDER) {
        if (canonical.toLower() == lower) {
            return canonical;
        }
    }

    // Handle some common variations
    static const QMap<QString, QString> COMMON_VARIATIONS = {
        {"genesis", "Genesis"}, {"psalms", "Psalms"}, {"psalm", "Psalms"},
        {"song of songs", "Song of Solomon"}, {"canticles", "Song of Solomon"},
        {"revelation", "Revelation"}, {"revelations", "Revelation"},
        {"1 samuel", "1 Samuel"}, {"2 samuel", "2 Samuel"},
        {"1 kings", "1 Kings"}, {"2 kings", "2 Kings"},
        {"1 chronicles", "1 Chronicles"}, {"2 chronicles", "2 Chronicles"},
        {"1 corinthians", "1 Corinthians"}, {"2 corinthians", "2 Corinthians"},
        {"1 thessalonians", "1 Thessalonians"}, {"2 thessalonians", "2 Thessalonians"},
        {"1 timothy", "1 Timothy"}, {"2 timothy", "2 Timothy"},
        {"1 peter", "1 Peter"}, {"2 peter", "2 Peter"},
        {"1 john", "1 John"}, {"2 john", "2 John"}, {"3 john", "3 John"}
    };

    if (COMMON_VARIATIONS.contains(lower)) {
        return COMMON_VARIATIONS[lower];
    }

    qWarning() << "BibleImporter: Unknown book name/ID:" << name;
    return QString();
}

QString BibleImporter::stripFormatting(const QString& text)
{
    QString result = text;

    // Remove XML/HTML tags
    static QRegularExpression tagPattern("<[^>]+>");
    result.remove(tagPattern);

    // Decode common HTML entities
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&amp;", "&");
    result.replace("&quot;", "\"");
    result.replace("&apos;", "'");
    result.replace("&#39;", "'");

    return result;
}

QString BibleImporter::normalizeWhitespace(const QString& text)
{
    QString result = text;

    // Replace all whitespace sequences with single space
    static QRegularExpression whitespacePattern("\\s+");
    result.replace(whitespacePattern, " ");

    return result.trimmed();
}

// =============================================================================
// OSIS Importer
// =============================================================================

OsisImporter::OsisImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool OsisImporter::canHandle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    // Read first 2KB to check for OSIS signature
    QByteArray header = file.read(2048);
    file.close();

    return header.contains("<osis") || header.contains("<OSIS");
}

ImportResult OsisImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    qint64 fileSize = file.size();
    QXmlStreamReader xml(&file);

    QString currentBook;
    int currentChapter = 0;
    int verseCount = 0;
    int estimatedTotal = 31102; // Typical verse count

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString().toLower();

            // Extract translation metadata from osisText element
            if (elementName == "osistext") {
                QString osisIDWork = xml.attributes().value("osisIDWork").toString();
                if (!osisIDWork.isEmpty()) {
                    result.translation.code = osisIDWork.toUpper();
                }
            }

            // Extract work info
            if (elementName == "work") {
                // Will be populated from child elements
            } else if (elementName == "title" && result.translation.name.isEmpty()) {
                QString title = xml.readElementText();
                if (!title.isEmpty() && result.translation.name.isEmpty()) {
                    result.translation.name = title;
                }
            } else if (elementName == "language") {
                QString lang = xml.readElementText();
                if (!lang.isEmpty()) {
                    result.translation.language = lang;
                }
            } else if (elementName == "rights") {
                QString rights = xml.readElementText();
                if (!rights.isEmpty()) {
                    result.translation.copyright = rights;
                }
            }

            // Parse verse elements
            if (elementName == "verse") {
                QString osisID = xml.attributes().value("osisID").toString();
                QString sID = xml.attributes().value("sID").toString();
                QString eID = xml.attributes().value("eID").toString();

                // Skip end markers
                if (!eID.isEmpty()) {
                    continue;
                }

                // Parse osisID: "Gen.1.1" or "Gen.1.1-Gen.1.2" for ranges
                QString verseId = !osisID.isEmpty() ? osisID : sID;
                if (verseId.isEmpty()) {
                    continue;
                }

                // Handle verse ranges - take first verse
                if (verseId.contains('-')) {
                    verseId = verseId.split('-').first();
                }

                QStringList parts = verseId.split('.');
                if (parts.size() >= 3) {
                    QString bookId = parts[0];
                    int chapter = parts[1].toInt();
                    int verse = parts[2].toInt();

                    QString canonicalBook = normalizeBookName(bookId);
                    if (!canonicalBook.isEmpty() && chapter > 0 && verse > 0) {
                        currentBook = canonicalBook;
                        currentChapter = chapter;

                        // Read verse content, preserving red letter markup
                        // OSIS uses <q who="Jesus"> for words of Jesus
                        QString verseText;
                        QString richText;
                        bool inRedLetter = false;
                        bool hasRedLetterMarkup = false;
                        int depth = 1;

                        while (depth > 0 && !xml.atEnd()) {
                            token = xml.readNext();
                            if (token == QXmlStreamReader::StartElement) {
                                depth++;
                                QString childElement = xml.name().toString().toLower();
                                // Check for <q who="Jesus"> - words of Jesus marker
                                if (childElement == "q") {
                                    QString who = xml.attributes().value("who").toString().toLower();
                                    if (who == "jesus") {
                                        inRedLetter = true;
                                        hasRedLetterMarkup = true;
                                        richText += "<span class=\"jesus\">";
                                    }
                                }
                            } else if (token == QXmlStreamReader::EndElement) {
                                QString endElement = xml.name().toString().toLower();
                                if (endElement == "q" && inRedLetter) {
                                    inRedLetter = false;
                                    richText += "</span>";
                                }
                                depth--;
                            } else if (token == QXmlStreamReader::Characters) {
                                QString text = xml.text().toString();
                                verseText += text;
                                // Escape HTML entities in rich text
                                QString escapedText = text.toHtmlEscaped();
                                richText += escapedText;
                            }
                        }

                        verseText = stripFormatting(verseText);
                        verseText = normalizeWhitespace(verseText);
                        richText = normalizeWhitespace(richText);

                        if (!verseText.isEmpty()) {
                            ImportedVerse iv;
                            iv.book = canonicalBook;
                            iv.chapter = chapter;
                            iv.verse = verse;
                            iv.text = verseText;
                            // Only store richText if it contains red letter markup
                            if (hasRedLetterMarkup) {
                                iv.richText = richText;
                            }
                            result.verses.append(iv);
                            verseCount++;

                            // Emit progress every 500 verses
                            if (verseCount % 500 == 0) {
                                emit progressChanged(verseCount, estimatedTotal,
                                    tr("Parsing %1...").arg(currentBook));
                            }
                        }
                    }
                }
            }
        }
    }

    if (xml.hasError()) {
        result.error = tr("XML parsing error: %1 at line %2")
            .arg(xml.errorString())
            .arg(xml.lineNumber());
        return result;
    }

    // Set default translation info if not found
    if (result.translation.code.isEmpty()) {
        QFileInfo fi(filePath);
        result.translation.code = fi.baseName().toUpper().left(10);
    }
    if (result.translation.name.isEmpty()) {
        result.translation.name = result.translation.code;
    }

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in file");
        return result;
    }

    result.success = true;
    qDebug() << "OsisImporter: Imported" << result.verses.size() << "verses";
    return result;
}

// =============================================================================
// USFM Importer
// =============================================================================

UsfmImporter::UsfmImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool UsfmImporter::canHandle(const QString& filePath) const
{
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    // Check extension
    if (ext == "usfm" || ext == "sfm") {
        return true;
    }

    // Check content for USFM markers
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray header = file.read(4096);
    file.close();

    // Look for USFM markers
    return header.contains("\\id ") || header.contains("\\c ") || header.contains("\\v ");
}

QString UsfmImporter::stripUsfmMarkers(const QString& text)
{
    QString result = text;

    // Remove footnotes \f ... \f* (do this first before removing other markers)
    static QRegularExpression footnotePattern("\\\\f\\s+.*?\\\\f\\*",
        QRegularExpression::DotMatchesEverythingOption);
    result.remove(footnotePattern);

    // Remove cross-references \x ... \x*
    static QRegularExpression xrefPattern("\\\\x\\s+.*?\\\\x\\*",
        QRegularExpression::DotMatchesEverythingOption);
    result.remove(xrefPattern);

    // Remove Strong's numbers: |strong="H1234" or |strong="G5678"
    static QRegularExpression strongPattern("\\|strong=\"[HG]?\\d+\"");
    result.remove(strongPattern);

    // Remove lemma annotations: |lemma="..."
    static QRegularExpression lemmaPattern("\\|lemma=\"[^\"]*\"");
    result.remove(lemmaPattern);

    // Remove other word-level attributes: |x-morph="..." etc.
    static QRegularExpression attrPattern("\\|[a-z-]+=\"[^\"]*\"");
    result.remove(attrPattern);

    // Remove USFM character markers like \add, \wj, \nd, \+add, etc.
    // Matches: \marker, \+marker, \marker*, \+marker*
    static QRegularExpression markerPattern("\\\\\\+?[a-z]+\\d?\\*?");
    result.remove(markerPattern);

    return result;
}

/**
 * @brief Convert USFM text to HTML preserving red letter markup
 *
 * Converts \wj ...\wj* markers to <span class="jesus">...</span>
 * while stripping other USFM markers.
 */
static QString usfmToRichText(const QString& text)
{
    QString result = text;

    // Remove footnotes and cross-references first
    static QRegularExpression footnotePattern("\\\\f\\s+.*?\\\\f\\*",
        QRegularExpression::DotMatchesEverythingOption);
    result.remove(footnotePattern);

    static QRegularExpression xrefPattern("\\\\x\\s+.*?\\\\x\\*",
        QRegularExpression::DotMatchesEverythingOption);
    result.remove(xrefPattern);

    // Remove Strong's numbers: |strong="H1234" or |strong="G5678"
    static QRegularExpression strongPattern("\\|strong=\"[HG]?\\d+\"");
    result.remove(strongPattern);

    // Remove lemma annotations: |lemma="..."
    static QRegularExpression lemmaPattern("\\|lemma=\"[^\"]*\"");
    result.remove(lemmaPattern);

    // Remove other word-level attributes: |x-morph="..." etc.
    static QRegularExpression attrPattern("\\|[a-z-]+=\"[^\"]*\"");
    result.remove(attrPattern);

    // Convert \wj ...\wj* to HTML spans
    // Note: \wj is "words of Jesus" marker in USFM
    static QRegularExpression wjPattern("\\\\wj\\s+(.*?)\\\\wj\\*",
        QRegularExpression::DotMatchesEverythingOption);
    result.replace(wjPattern, "<span class=\"jesus\">\\1</span>");

    // Remove remaining USFM markers (but preserve the converted spans)
    static QRegularExpression otherMarkerPattern("\\\\\\+?[a-z]+\\d?\\*?");
    result.remove(otherMarkerPattern);

    return result;
}

ImportResult UsfmImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString content = in.readAll();
    file.close();

    QString currentBook;
    int currentChapter = 0;
    int currentVerseNum = 0;
    QString currentVerseText;
    int verseCount = 0;
    int estimatedTotal = 31102;

    // Helper lambda to save the current verse
    auto saveCurrentVerse = [&]() {
        if (currentVerseNum > 0 && !currentBook.isEmpty() && currentChapter > 0) {
            QString text = stripUsfmMarkers(currentVerseText);
            text = normalizeWhitespace(text);
            if (!text.isEmpty()) {
                ImportedVerse iv;
                iv.book = currentBook;
                iv.chapter = currentChapter;
                iv.verse = currentVerseNum;
                iv.text = text;

                // Check for red letter markup (\wj markers)
                if (currentVerseText.contains("\\wj")) {
                    QString richText = usfmToRichText(currentVerseText);
                    richText = normalizeWhitespace(richText);
                    if (richText.contains("class=\"jesus\"")) {
                        iv.richText = richText;
                    }
                }

                result.verses.append(iv);
                verseCount++;

                if (verseCount % 500 == 0) {
                    emit progressChanged(verseCount, estimatedTotal,
                        tr("Parsing %1 %2...").arg(currentBook).arg(currentChapter));
                }
            }
        }
        currentVerseNum = 0;
        currentVerseText.clear();
    };

    // Process line by line for robustness
    QStringList lines = content.split('\n');

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        // Check for \id marker (book identifier)
        if (trimmed.startsWith("\\id ")) {
            saveCurrentVerse();
            // Extract book code (first word after \id)
            QString rest = trimmed.mid(4).trimmed();
            QString bookCode = rest.split(QRegularExpression("\\s+")).first();
            currentBook = normalizeBookName(bookCode);
            currentChapter = 0;
            qDebug() << "USFM: Found book" << bookCode << "->" << currentBook;
            continue;
        }

        // Check for \c marker (chapter)
        static QRegularExpression chapterPattern("^\\\\c\\s+(\\d+)");
        QRegularExpressionMatch chapterMatch = chapterPattern.match(trimmed);
        if (chapterMatch.hasMatch()) {
            saveCurrentVerse();
            currentChapter = chapterMatch.captured(1).toInt();
            continue;
        }

        // Check for \v marker (verse) - can appear at start or within line
        static QRegularExpression versePattern("\\\\v\\s+(\\d+[-\\d]*)\\s*");
        QRegularExpressionMatchIterator verseMatches = versePattern.globalMatch(trimmed);

        if (verseMatches.hasNext()) {
            // Line contains verse marker(s)
            int lastPos = 0;

            while (verseMatches.hasNext()) {
                QRegularExpressionMatch vm = verseMatches.next();

                // Text before this verse marker belongs to previous verse
                if (lastPos < vm.capturedStart()) {
                    QString textBefore = trimmed.mid(lastPos, vm.capturedStart() - lastPos);
                    if (currentVerseNum > 0) {
                        currentVerseText += " " + textBefore;
                    }
                }

                // Save previous verse and start new one
                saveCurrentVerse();

                // Parse verse number (handle ranges like "1-2" by taking first number)
                QString verseNumStr = vm.captured(1);
                if (verseNumStr.contains('-')) {
                    verseNumStr = verseNumStr.split('-').first();
                }
                currentVerseNum = verseNumStr.toInt();
                lastPos = vm.capturedEnd();
            }

            // Text after the last verse marker
            if (lastPos < trimmed.length()) {
                currentVerseText = trimmed.mid(lastPos);
            }
        } else if (currentVerseNum > 0) {
            // No verse marker - continuation of current verse
            // Skip paragraph/formatting markers at line start
            QString textToAdd = trimmed;
            static QRegularExpression lineStartMarker("^\\\\[a-z]+\\d?\\s*");
            textToAdd.remove(lineStartMarker);
            if (!textToAdd.isEmpty()) {
                currentVerseText += " " + textToAdd;
            }
        }
    }

    // Save final verse
    saveCurrentVerse();

    // Set default translation info if not found in file
    if (result.translation.code.isEmpty()) {
        QFileInfo fi(filePath);
        result.translation.code = fi.baseName().toUpper().left(10);
    }
    if (result.translation.name.isEmpty()) {
        result.translation.name = result.translation.code;
    }

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in file. Ensure the file contains \\id, \\c, and \\v markers.");
        return result;
    }

    result.success = true;
    result.warnings.append(tr("USFM files typically contain one book. Import all book files for complete Bible."));
    qDebug() << "UsfmImporter: Imported" << result.verses.size() << "verses from" << currentBook;
    return result;
}

// =============================================================================
// USX Importer
// =============================================================================

UsxImporter::UsxImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool UsxImporter::canHandle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray header = file.read(2048);
    file.close();

    return header.contains("<usx") || header.contains("<USX");
}

ImportResult UsxImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    QXmlStreamReader xml(&file);

    QString currentBook;
    int currentChapter = 0;
    int verseCount = 0;
    int estimatedTotal = 31102;
    QString currentVerseText;
    QString currentVerseRichText;
    int currentVerseNum = 0;
    bool inRedLetter = false;
    bool hasRedLetterMarkup = false;

    // Helper lambda to save current verse
    auto saveVerse = [&]() {
        if (currentVerseNum > 0 && !currentVerseText.isEmpty() && !currentBook.isEmpty()) {
            ImportedVerse iv;
            iv.book = currentBook;
            iv.chapter = currentChapter;
            iv.verse = currentVerseNum;
            iv.text = normalizeWhitespace(currentVerseText);
            if (hasRedLetterMarkup) {
                iv.richText = normalizeWhitespace(currentVerseRichText);
            }
            result.verses.append(iv);
            verseCount++;
        }
    };

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString().toLower();

            // Book element
            if (elementName == "book") {
                QString bookCode = xml.attributes().value("code").toString();
                currentBook = normalizeBookName(bookCode);
            }
            // Chapter element
            else if (elementName == "chapter") {
                QString num = xml.attributes().value("number").toString();
                if (!num.isEmpty()) {
                    // Save previous verse if any
                    saveVerse();

                    currentChapter = num.toInt();
                    currentVerseNum = 0;
                    currentVerseText.clear();
                    currentVerseRichText.clear();
                    hasRedLetterMarkup = false;
                }
            }
            // Verse element
            else if (elementName == "verse") {
                QString num = xml.attributes().value("number").toString();
                QString eid = xml.attributes().value("eid").toString();

                if (!eid.isEmpty()) {
                    // End of verse - save it
                    saveVerse();

                    if (verseCount % 500 == 0 && verseCount > 0) {
                        emit progressChanged(verseCount, estimatedTotal,
                            tr("Parsing %1...").arg(currentBook));
                    }

                    currentVerseNum = 0;
                    currentVerseText.clear();
                    currentVerseRichText.clear();
                    hasRedLetterMarkup = false;
                } else if (!num.isEmpty()) {
                    // Save previous verse
                    saveVerse();

                    // Handle verse ranges like "1-2"
                    if (num.contains('-')) {
                        num = num.split('-').first();
                    }
                    currentVerseNum = num.toInt();
                    currentVerseText.clear();
                    currentVerseRichText.clear();
                    hasRedLetterMarkup = false;
                }
            }
            // Character style element - check for "wj" (words of Jesus)
            else if (elementName == "char") {
                QString style = xml.attributes().value("style").toString().toLower();
                if (style == "wj") {
                    inRedLetter = true;
                    hasRedLetterMarkup = true;
                    currentVerseRichText += "<span class=\"jesus\">";
                }
            }
        } else if (token == QXmlStreamReader::EndElement) {
            QString elementName = xml.name().toString().toLower();
            if (elementName == "char" && inRedLetter) {
                inRedLetter = false;
                currentVerseRichText += "</span>";
            }
        } else if (token == QXmlStreamReader::Characters) {
            if (currentVerseNum > 0) {
                QString text = xml.text().toString();
                currentVerseText += text;
                currentVerseRichText += text.toHtmlEscaped();
            }
        }
    }

    // Save last verse
    saveVerse();

    if (xml.hasError()) {
        result.error = tr("XML parsing error: %1").arg(xml.errorString());
        return result;
    }

    if (result.translation.code.isEmpty()) {
        QFileInfo fi(filePath);
        result.translation.code = fi.baseName().toUpper().left(10);
    }
    if (result.translation.name.isEmpty()) {
        result.translation.name = result.translation.code;
    }

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in file");
        return result;
    }

    result.success = true;
    qDebug() << "UsxImporter: Imported" << result.verses.size() << "verses";
    return result;
}

// =============================================================================
// USFX Importer
// =============================================================================

UsfxImporter::UsfxImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool UsfxImporter::canHandle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray header = file.read(2048);
    file.close();

    return header.contains("<usfx") || header.contains("<USFX");
}

ImportResult UsfxImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    QXmlStreamReader xml(&file);

    QString currentBook;
    int currentChapter = 0;
    int verseCount = 0;
    int estimatedTotal = 31102;
    QString currentVerseText;
    QString currentVerseRichText;
    int currentVerseNum = 0;
    bool inVerse = false;
    bool inRedLetter = false;
    bool hasRedLetterMarkup = false;

    // Helper lambda to save current verse
    auto saveVerse = [&]() {
        if (inVerse && currentVerseNum > 0 && !currentVerseText.isEmpty() && !currentBook.isEmpty()) {
            ImportedVerse iv;
            iv.book = currentBook;
            iv.chapter = currentChapter;
            iv.verse = currentVerseNum;
            iv.text = normalizeWhitespace(stripFormatting(currentVerseText));
            if (hasRedLetterMarkup) {
                iv.richText = normalizeWhitespace(currentVerseRichText);
            }
            result.verses.append(iv);
            verseCount++;
        }
    };

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString().toLower();

            // Book element
            if (elementName == "book") {
                QString id = xml.attributes().value("id").toString();
                if (!id.isEmpty()) {
                    currentBook = normalizeBookName(id);
                }
            }
            // Chapter element
            else if (elementName == "c") {
                QString id = xml.attributes().value("id").toString();
                if (!id.isEmpty()) {
                    currentChapter = id.toInt();
                }
            }
            // Verse element
            else if (elementName == "v") {
                // Save previous verse
                saveVerse();

                if (verseCount % 500 == 0 && verseCount > 0) {
                    emit progressChanged(verseCount, estimatedTotal,
                        tr("Parsing %1...").arg(currentBook));
                }

                QString id = xml.attributes().value("id").toString();
                if (!id.isEmpty()) {
                    currentVerseNum = id.toInt();
                    currentVerseText.clear();
                    currentVerseRichText.clear();
                    hasRedLetterMarkup = false;
                    inVerse = true;
                }
            }
            // Verse end marker
            else if (elementName == "ve") {
                saveVerse();
                inVerse = false;
                currentVerseNum = 0;
                currentVerseText.clear();
                currentVerseRichText.clear();
                hasRedLetterMarkup = false;
            }
            // Words of Jesus element
            else if (elementName == "wj") {
                inRedLetter = true;
                hasRedLetterMarkup = true;
                currentVerseRichText += "<span class=\"jesus\">";
            }
        } else if (token == QXmlStreamReader::EndElement) {
            QString elementName = xml.name().toString().toLower();
            if (elementName == "wj" && inRedLetter) {
                inRedLetter = false;
                currentVerseRichText += "</span>";
            }
        } else if (token == QXmlStreamReader::Characters) {
            if (inVerse) {
                QString text = xml.text().toString();
                currentVerseText += text;
                currentVerseRichText += text.toHtmlEscaped();
            }
        }
    }

    // Save last verse
    saveVerse();

    if (xml.hasError()) {
        result.error = tr("XML parsing error: %1").arg(xml.errorString());
        return result;
    }

    if (result.translation.code.isEmpty()) {
        QFileInfo fi(filePath);
        result.translation.code = fi.baseName().toUpper().left(10);
    }
    if (result.translation.name.isEmpty()) {
        result.translation.name = result.translation.code;
    }

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in file");
        return result;
    }

    result.success = true;
    qDebug() << "UsfxImporter: Imported" << result.verses.size() << "verses";
    return result;
}

// =============================================================================
// Zefania Importer
// =============================================================================

ZefaniaImporter::ZefaniaImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool ZefaniaImporter::canHandle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray header = file.read(2048);
    file.close();

    // Zefania uses <XMLBIBLE> or <xmlbible> root element
    return header.contains("<XMLBIBLE") || header.contains("<xmlbible") ||
           header.contains("<BIBLEBOOK") || header.contains("<biblebook");
}

QString ZefaniaImporter::bookFromNumber(int number)
{
    if (number >= 1 && number <= ZEFANIA_BOOK_ORDER.size()) {
        return ZEFANIA_BOOK_ORDER[number - 1];
    }
    return QString();
}

ImportResult ZefaniaImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    QXmlStreamReader xml(&file);

    QString currentBook;
    int currentChapter = 0;
    int verseCount = 0;
    int estimatedTotal = 31102;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString().toLower();

            // Translation info from INFORMATION element
            if (elementName == "title" && result.translation.name.isEmpty()) {
                result.translation.name = xml.readElementText();
            } else if (elementName == "identifier" && result.translation.code.isEmpty()) {
                result.translation.code = xml.readElementText().toUpper();
            } else if (elementName == "language") {
                result.translation.language = xml.readElementText();
            } else if (elementName == "rights" || elementName == "copyright") {
                result.translation.copyright = xml.readElementText();
            }

            // Book element: <BIBLEBOOK bnumber="1"> or <BIBLEBOOK bname="Genesis">
            if (elementName == "biblebook") {
                QString bnum = xml.attributes().value("bnumber").toString();
                QString bname = xml.attributes().value("bname").toString();

                if (!bnum.isEmpty()) {
                    currentBook = bookFromNumber(bnum.toInt());
                } else if (!bname.isEmpty()) {
                    currentBook = normalizeBookName(bname);
                }
            }

            // Chapter element: <CHAPTER cnumber="1">
            if (elementName == "chapter") {
                QString cnum = xml.attributes().value("cnumber").toString();
                if (!cnum.isEmpty()) {
                    currentChapter = cnum.toInt();
                }
            }

            // Verse element: <VERS vnumber="1">verse text</VERS>
            if (elementName == "vers") {
                QString vnum = xml.attributes().value("vnumber").toString();
                if (!vnum.isEmpty() && !currentBook.isEmpty() && currentChapter > 0) {
                    int verseNum = vnum.toInt();
                    QString verseText = xml.readElementText();

                    verseText = stripFormatting(verseText);
                    verseText = normalizeWhitespace(verseText);

                    if (!verseText.isEmpty() && verseNum > 0) {
                        ImportedVerse iv;
                        iv.book = currentBook;
                        iv.chapter = currentChapter;
                        iv.verse = verseNum;
                        iv.text = verseText;
                        result.verses.append(iv);
                        verseCount++;

                        if (verseCount % 500 == 0) {
                            emit progressChanged(verseCount, estimatedTotal,
                                tr("Parsing %1...").arg(currentBook));
                        }
                    }
                }
            }
        }
    }

    if (xml.hasError()) {
        result.error = tr("XML parsing error: %1").arg(xml.errorString());
        return result;
    }

    if (result.translation.code.isEmpty()) {
        QFileInfo fi(filePath);
        result.translation.code = fi.baseName().toUpper().left(10);
    }
    if (result.translation.name.isEmpty()) {
        result.translation.name = result.translation.code;
    }

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in file");
        return result;
    }

    result.success = true;
    qDebug() << "ZefaniaImporter: Imported" << result.verses.size() << "verses";
    return result;
}

// =============================================================================
// TSV Importer
// =============================================================================

TsvImporter::TsvImporter(QObject* parent)
    : BibleImporter(parent)
{
}

bool TsvImporter::canHandle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    // Read first line to check for TSV header
    QByteArray firstLine = file.readLine();
    file.close();

    // Check for expected TSV header columns
    return firstLine.contains("orig_book_index") &&
           firstLine.contains("orig_chapter") &&
           firstLine.contains("text");
}

QString TsvImporter::bookFromIndex(const QString& index)
{
    // Format: "01O" = book 1 Old Testament, "40N" = book 40 (Matthew) New Testament
    // Extract the numeric part
    QString numPart = index;
    numPart.remove(QRegularExpression("[ON]$")); // Remove O or N suffix

    int bookNum = numPart.toInt();
    if (bookNum < 1 || bookNum > ZEFANIA_BOOK_ORDER.size()) {
        return QString();
    }

    return ZEFANIA_BOOK_ORDER[bookNum - 1];
}

ImportResult TsvImporter::import(const QString& filePath)
{
    ImportResult result;
    result.success = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Failed to open file: %1").arg(file.errorString());
        return result;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // Read and parse header line
    QString headerLine = in.readLine();
    QStringList headers = headerLine.split('\t');

    // Find column indices
    int bookCol = headers.indexOf("orig_book_index");
    int chapterCol = headers.indexOf("orig_chapter");
    int verseCol = headers.indexOf("orig_verse");
    int textCol = headers.indexOf("text");

    if (bookCol < 0 || chapterCol < 0 || verseCol < 0 || textCol < 0) {
        result.error = tr("TSV file missing required columns (orig_book_index, orig_chapter, orig_verse, text)");
        return result;
    }

    int verseCount = 0;
    int estimatedTotal = 31102;
    int lineNum = 1;
    QString lastBook;

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;

        QStringList fields = line.split('\t');
        if (fields.size() <= textCol) {
            continue; // Skip malformed lines
        }

        QString bookIndex = fields[bookCol].trimmed();
        int chapter = fields[chapterCol].toInt();
        int verse = fields[verseCol].toInt();
        QString text = fields[textCol].trimmed();

        // Convert book index to canonical name
        QString bookName = bookFromIndex(bookIndex);
        if (bookName.isEmpty()) {
            // Try treating it as a direct book number
            int bookNum = bookIndex.toInt();
            if (bookNum >= 1 && bookNum <= ZEFANIA_BOOK_ORDER.size()) {
                bookName = ZEFANIA_BOOK_ORDER[bookNum - 1];
            }
        }

        if (bookName.isEmpty() || chapter < 1 || verse < 1 || text.isEmpty()) {
            continue; // Skip invalid rows
        }

        ImportedVerse iv;
        iv.book = bookName;
        iv.chapter = chapter;
        iv.verse = verse;
        iv.text = text;
        result.verses.append(iv);
        verseCount++;

        // Emit progress every 500 verses
        if (verseCount % 500 == 0) {
            emit progressChanged(verseCount, estimatedTotal,
                tr("Parsing %1...").arg(bookName));
        }

        lastBook = bookName;
    }

    file.close();

    // Set translation info from filename
    QFileInfo fi(filePath);
    result.translation.code = fi.baseName().toUpper().left(10);
    result.translation.name = result.translation.code;

    if (result.verses.isEmpty()) {
        result.error = tr("No verses found in TSV file");
        return result;
    }

    result.success = true;
    qDebug() << "TsvImporter: Imported" << result.verses.size() << "verses";
    return result;
}

// =============================================================================
// Importer Factory
// =============================================================================

std::unique_ptr<BibleImporter> BibleImporterFactory::createForFile(const QString& filePath, QObject* parent)
{
    // Try each importer in order of specificity

    // TSV format (check first - simple header detection)
    auto tsv = std::make_unique<TsvImporter>(parent);
    if (tsv->canHandle(filePath)) {
        return tsv;
    }

    // XML formats
    auto osis = std::make_unique<OsisImporter>(parent);
    if (osis->canHandle(filePath)) {
        return osis;
    }

    auto zefania = std::make_unique<ZefaniaImporter>(parent);
    if (zefania->canHandle(filePath)) {
        return zefania;
    }

    auto usx = std::make_unique<UsxImporter>(parent);
    if (usx->canHandle(filePath)) {
        return usx;
    }

    auto usfx = std::make_unique<UsfxImporter>(parent);
    if (usfx->canHandle(filePath)) {
        return usfx;
    }

    // USFM (text-based, check last as it's more permissive)
    auto usfm = std::make_unique<UsfmImporter>(parent);
    if (usfm->canHandle(filePath)) {
        return usfm;
    }

    return nullptr;
}

QString BibleImporterFactory::detectFormat(const QString& filePath)
{
    auto importer = createForFile(filePath);
    if (importer) {
        return importer->formatName();
    }
    return QString();
}

QString BibleImporterFactory::fileFilter()
{
    return QObject::tr("Bible Files (*.xml *.osis *.usfm *.sfm *.tsv *.csv *.txt);;"
                       "OSIS XML (*.xml *.osis);;"
                       "USFM (*.usfm *.sfm);;"
                       "Zefania XML (*.xml);;"
                       "TSV/CSV (*.tsv *.csv *.txt);;"
                       "All Files (*)");
}

} // namespace Clarity
