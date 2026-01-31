#include "BibleDatabase.h"
#include "BibleImporter.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>

namespace Clarity {

// Static data for book name normalization
static const QMap<QString, QString> BOOK_ABBREVIATIONS = {
    // Old Testament
    {"gen", "Genesis"}, {"ge", "Genesis"}, {"gn", "Genesis"},
    {"ex", "Exodus"}, {"exod", "Exodus"}, {"exo", "Exodus"},
    {"lev", "Leviticus"}, {"le", "Leviticus"}, {"lv", "Leviticus"},
    {"num", "Numbers"}, {"nu", "Numbers"}, {"nm", "Numbers"}, {"nb", "Numbers"},
    {"deut", "Deuteronomy"}, {"de", "Deuteronomy"}, {"dt", "Deuteronomy"},
    {"josh", "Joshua"}, {"jos", "Joshua"}, {"jsh", "Joshua"},
    {"judg", "Judges"}, {"jdg", "Judges"}, {"jg", "Judges"}, {"jdgs", "Judges"},
    {"ruth", "Ruth"}, {"rth", "Ruth"}, {"ru", "Ruth"},
    {"1 sam", "1 Samuel"}, {"1sam", "1 Samuel"}, {"1 sa", "1 Samuel"}, {"1sa", "1 Samuel"},
    {"2 sam", "2 Samuel"}, {"2sam", "2 Samuel"}, {"2 sa", "2 Samuel"}, {"2sa", "2 Samuel"},
    {"1 kings", "1 Kings"}, {"1kings", "1 Kings"}, {"1 ki", "1 Kings"}, {"1ki", "1 Kings"}, {"1 kgs", "1 Kings"},
    {"2 kings", "2 Kings"}, {"2kings", "2 Kings"}, {"2 ki", "2 Kings"}, {"2ki", "2 Kings"}, {"2 kgs", "2 Kings"},
    {"1 chron", "1 Chronicles"}, {"1chron", "1 Chronicles"}, {"1 ch", "1 Chronicles"}, {"1ch", "1 Chronicles"}, {"1 chr", "1 Chronicles"},
    {"2 chron", "2 Chronicles"}, {"2chron", "2 Chronicles"}, {"2 ch", "2 Chronicles"}, {"2ch", "2 Chronicles"}, {"2 chr", "2 Chronicles"},
    {"ezra", "Ezra"}, {"ezr", "Ezra"},
    {"neh", "Nehemiah"}, {"ne", "Nehemiah"},
    {"esth", "Esther"}, {"est", "Esther"}, {"es", "Esther"},
    {"job", "Job"}, {"jb", "Job"},
    {"ps", "Psalms"}, {"psalm", "Psalms"}, {"psa", "Psalms"}, {"psm", "Psalms"}, {"pss", "Psalms"},
    {"prov", "Proverbs"}, {"pro", "Proverbs"}, {"prv", "Proverbs"}, {"pr", "Proverbs"},
    {"eccl", "Ecclesiastes"}, {"ecc", "Ecclesiastes"}, {"ec", "Ecclesiastes"}, {"qoh", "Ecclesiastes"},
    {"song", "Song of Solomon"}, {"sos", "Song of Solomon"}, {"so", "Song of Solomon"}, {"canticles", "Song of Solomon"}, {"cant", "Song of Solomon"},
    {"isa", "Isaiah"}, {"is", "Isaiah"},
    {"jer", "Jeremiah"}, {"je", "Jeremiah"}, {"jr", "Jeremiah"},
    {"lam", "Lamentations"}, {"la", "Lamentations"},
    {"ezek", "Ezekiel"}, {"eze", "Ezekiel"}, {"ezk", "Ezekiel"},
    {"dan", "Daniel"}, {"da", "Daniel"}, {"dn", "Daniel"},
    {"hos", "Hosea"}, {"ho", "Hosea"},
    {"joel", "Joel"}, {"jl", "Joel"},
    {"amos", "Amos"}, {"am", "Amos"},
    {"obad", "Obadiah"}, {"ob", "Obadiah"},
    {"jonah", "Jonah"}, {"jnh", "Jonah"}, {"jon", "Jonah"},
    {"mic", "Micah"}, {"mi", "Micah"},
    {"nah", "Nahum"}, {"na", "Nahum"},
    {"hab", "Habakkuk"}, {"hb", "Habakkuk"},
    {"zeph", "Zephaniah"}, {"zep", "Zephaniah"}, {"zp", "Zephaniah"},
    {"hag", "Haggai"}, {"hg", "Haggai"},
    {"zech", "Zechariah"}, {"zec", "Zechariah"}, {"zc", "Zechariah"},
    {"mal", "Malachi"}, {"ml", "Malachi"},
    // New Testament
    {"matt", "Matthew"}, {"mt", "Matthew"}, {"mat", "Matthew"},
    {"mark", "Mark"}, {"mrk", "Mark"}, {"mk", "Mark"}, {"mr", "Mark"},
    {"luke", "Luke"}, {"luk", "Luke"}, {"lk", "Luke"},
    {"john", "John"}, {"joh", "John"}, {"jhn", "John"}, {"jn", "John"},
    {"acts", "Acts"}, {"act", "Acts"}, {"ac", "Acts"},
    {"rom", "Romans"}, {"ro", "Romans"}, {"rm", "Romans"},
    {"1 cor", "1 Corinthians"}, {"1cor", "1 Corinthians"}, {"1 co", "1 Corinthians"}, {"1co", "1 Corinthians"},
    {"2 cor", "2 Corinthians"}, {"2cor", "2 Corinthians"}, {"2 co", "2 Corinthians"}, {"2co", "2 Corinthians"},
    {"gal", "Galatians"}, {"ga", "Galatians"},
    {"eph", "Ephesians"}, {"ep", "Ephesians"},
    {"phil", "Philippians"}, {"php", "Philippians"}, {"pp", "Philippians"},
    {"col", "Colossians"}, {"co", "Colossians"},
    {"1 thess", "1 Thessalonians"}, {"1thess", "1 Thessalonians"}, {"1 th", "1 Thessalonians"}, {"1th", "1 Thessalonians"},
    {"2 thess", "2 Thessalonians"}, {"2thess", "2 Thessalonians"}, {"2 th", "2 Thessalonians"}, {"2th", "2 Thessalonians"},
    {"1 tim", "1 Timothy"}, {"1tim", "1 Timothy"}, {"1 ti", "1 Timothy"}, {"1ti", "1 Timothy"},
    {"2 tim", "2 Timothy"}, {"2tim", "2 Timothy"}, {"2 ti", "2 Timothy"}, {"2ti", "2 Timothy"},
    {"titus", "Titus"}, {"tit", "Titus"}, {"ti", "Titus"},
    {"phlm", "Philemon"}, {"phm", "Philemon"}, {"pm", "Philemon"},
    {"heb", "Hebrews"}, {"he", "Hebrews"},
    {"james", "James"}, {"jas", "James"}, {"jm", "James"},
    {"1 pet", "1 Peter"}, {"1pet", "1 Peter"}, {"1 pe", "1 Peter"}, {"1pe", "1 Peter"}, {"1 pt", "1 Peter"}, {"1pt", "1 Peter"},
    {"2 pet", "2 Peter"}, {"2pet", "2 Peter"}, {"2 pe", "2 Peter"}, {"2pe", "2 Peter"}, {"2 pt", "2 Peter"}, {"2pt", "2 Peter"},
    {"1 john", "1 John"}, {"1john", "1 John"}, {"1 jn", "1 John"}, {"1jn", "1 John"}, {"1 jo", "1 John"}, {"1jo", "1 John"},
    {"2 john", "2 John"}, {"2john", "2 John"}, {"2 jn", "2 John"}, {"2jn", "2 John"}, {"2 jo", "2 John"}, {"2jo", "2 John"},
    {"3 john", "3 John"}, {"3john", "3 John"}, {"3 jn", "3 John"}, {"3jn", "3 John"}, {"3 jo", "3 John"}, {"3jo", "3 John"},
    {"jude", "Jude"}, {"jud", "Jude"}, {"jd", "Jude"},
    {"rev", "Revelation"}, {"re", "Revelation"}, {"rv", "Revelation"}, {"revelation", "Revelation"}, {"revelations", "Revelation"}
};

// Full book names for normalization
static const QStringList BOOK_NAMES = {
    // Old Testament
    "Genesis", "Exodus", "Leviticus", "Numbers", "Deuteronomy",
    "Joshua", "Judges", "Ruth", "1 Samuel", "2 Samuel",
    "1 Kings", "2 Kings", "1 Chronicles", "2 Chronicles",
    "Ezra", "Nehemiah", "Esther", "Job", "Psalms", "Proverbs",
    "Ecclesiastes", "Song of Solomon", "Isaiah", "Jeremiah", "Lamentations",
    "Ezekiel", "Daniel", "Hosea", "Joel", "Amos",
    "Obadiah", "Jonah", "Micah", "Nahum", "Habakkuk",
    "Zephaniah", "Haggai", "Zechariah", "Malachi",
    // New Testament
    "Matthew", "Mark", "Luke", "John", "Acts",
    "Romans", "1 Corinthians", "2 Corinthians", "Galatians", "Ephesians",
    "Philippians", "Colossians", "1 Thessalonians", "2 Thessalonians",
    "1 Timothy", "2 Timothy", "Titus", "Philemon", "Hebrews",
    "James", "1 Peter", "2 Peter", "1 John", "2 John", "3 John",
    "Jude", "Revelation"
};

QString BibleVerse::reference() const
{
    return QString("%1 %2:%3").arg(book).arg(chapter).arg(verse);
}

QString BibleVerse::fullReference() const
{
    return QString("%1 %2:%3 (%4)").arg(book).arg(chapter).arg(verse).arg(translation);
}

BibleDatabase::BibleDatabase(QObject* parent)
    : QObject(parent)
    , m_defaultTranslation("KJV")
    , m_isValid(false)
{
    buildBookNameIndex();
}

BibleDatabase::~BibleDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool BibleDatabase::initialize(const QString& dbPath)
{
    QFileInfo fileInfo(dbPath);
    if (!fileInfo.exists()) {
        qWarning() << "Bible database file not found:" << dbPath;
        return false;
    }

    // Use unique connection name to avoid conflicts
    QString connectionName = QString("BibleDB_%1").arg(reinterpret_cast<quintptr>(this));

    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qWarning() << "Failed to open Bible database:" << m_database.lastError().text();
        return false;
    }

    // Verify required tables exist
    QStringList tables = m_database.tables();
    if (!tables.contains("books") || !tables.contains("verses")) {
        qWarning() << "Bible database missing required tables (books, verses)";
        m_database.close();
        return false;
    }

    m_isValid = true;
    qDebug() << "Bible database initialized successfully from" << dbPath;
    return true;
}

bool BibleDatabase::isValid() const
{
    return m_isValid && m_database.isOpen();
}

void BibleDatabase::buildBookNameIndex()
{
    m_bookAbbreviations = BOOK_ABBREVIATIONS;

    // Also add full names as keys (lowercase)
    for (const QString& name : BOOK_NAMES) {
        m_bookAbbreviations[name.toLower()] = name;
    }
}

QString BibleDatabase::normalizeBookName(const QString& name) const
{
    QString normalized = name.trimmed().toLower();

    // Try direct lookup
    if (m_bookAbbreviations.contains(normalized)) {
        return m_bookAbbreviations[normalized];
    }

    // Try with common prefix patterns (e.g., "1st john" -> "1 john")
    normalized.replace(QRegularExpression("^1st\\s+"), "1 ");
    normalized.replace(QRegularExpression("^2nd\\s+"), "2 ");
    normalized.replace(QRegularExpression("^3rd\\s+"), "3 ");
    normalized.replace(QRegularExpression("^first\\s+"), "1 ");
    normalized.replace(QRegularExpression("^second\\s+"), "2 ");
    normalized.replace(QRegularExpression("^third\\s+"), "3 ");

    if (m_bookAbbreviations.contains(normalized)) {
        return m_bookAbbreviations[normalized];
    }

    // No match found
    return QString();
}

BibleReference BibleDatabase::parseReference(const QString& text)
{
    BibleReference ref;
    QString cleaned = text.trimmed();

    // Pattern for book name (may start with number): captures book with optional leading number
    // Examples: "John", "1 John", "1John", "Song of Solomon"
    // Pattern for reference: book chapter:verse[-[chapter:]verse]

    // Regex to capture:
    // Group 1: Book name (including optional leading number and multi-word books)
    // Group 2: Start chapter
    // Group 3: Start verse (optional - if missing, means whole chapter)
    // Group 4: End chapter (optional - for cross-chapter ranges)
    // Group 5: End verse (optional)

    // Single verse or verse range: "John 3:16" or "John 3:16-17"
    static QRegularExpression singleChapterPattern(
        R"(^(\d?\s*[\w\s]+?)\s+(\d+):(\d+)(?:-(\d+))?$)",
        QRegularExpression::CaseInsensitiveOption
    );

    // Cross-chapter range: "John 3:16-4:3"
    static QRegularExpression crossChapterPattern(
        R"(^(\d?\s*[\w\s]+?)\s+(\d+):(\d+)-(\d+):(\d+)$)",
        QRegularExpression::CaseInsensitiveOption
    );

    // Whole chapter: "Psalm 23" or "John 3"
    static QRegularExpression wholeChapterPattern(
        R"(^(\d?\s*[\w\s]+?)\s+(\d+)$)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match;

    // Try cross-chapter pattern first (most specific)
    match = crossChapterPattern.match(cleaned);
    if (match.hasMatch()) {
        ref.book = match.captured(1).trimmed();
        ref.startChapter = match.captured(2).toInt();
        ref.startVerse = match.captured(3).toInt();
        ref.endChapter = match.captured(4).toInt();
        ref.endVerse = match.captured(5).toInt();
        ref.isValid = true;
        return ref;
    }

    // Try single chapter pattern (verse or verse range)
    match = singleChapterPattern.match(cleaned);
    if (match.hasMatch()) {
        ref.book = match.captured(1).trimmed();
        ref.startChapter = match.captured(2).toInt();
        ref.startVerse = match.captured(3).toInt();
        ref.endChapter = ref.startChapter;

        if (match.captured(4).isEmpty()) {
            // Single verse
            ref.endVerse = ref.startVerse;
        } else {
            // Verse range in same chapter
            ref.endVerse = match.captured(4).toInt();
        }
        ref.isValid = true;
        return ref;
    }

    // Try whole chapter pattern
    match = wholeChapterPattern.match(cleaned);
    if (match.hasMatch()) {
        ref.book = match.captured(1).trimmed();
        ref.startChapter = match.captured(2).toInt();
        ref.startVerse = 0;  // 0 means entire chapter
        ref.endChapter = ref.startChapter;
        ref.endVerse = 0;
        ref.isValid = true;
        return ref;
    }

    // No pattern matched
    ref.isValid = false;
    return ref;
}

QList<BibleVerse> BibleDatabase::lookupReference(const QString& reference)
{
    BibleReference ref = parseReference(reference);
    if (!ref.isValid) {
        qWarning() << "Invalid Bible reference:" << reference;
        return QList<BibleVerse>();
    }

    // Normalize book name
    QString normalizedBook = normalizeBookName(ref.book);
    if (normalizedBook.isEmpty()) {
        qWarning() << "Unknown book name:" << ref.book;
        return QList<BibleVerse>();
    }
    ref.book = normalizedBook;

    return lookupParsedReference(ref);
}

QList<BibleVerse> BibleDatabase::lookupParsedReference(const BibleReference& ref)
{
    QList<BibleVerse> results;

    if (!isValid()) {
        qWarning() << "Bible database not initialized";
        return results;
    }

    QSqlQuery query(m_database);

    // Build query based on reference type
    QString sql = R"(
        SELECT b.name, v.chapter, v.verse, v.text, v.translation
        FROM verses v
        JOIN books b ON v.book_id = b.id
        WHERE b.name = :book
          AND v.translation = :translation
    )";

    if (ref.startVerse == 0) {
        // Entire chapter
        sql += " AND v.chapter = :startChapter";
    } else if (ref.startChapter == ref.endChapter) {
        // Single chapter, verse or verse range
        sql += " AND v.chapter = :startChapter AND v.verse >= :startVerse AND v.verse <= :endVerse";
    } else {
        // Cross-chapter range
        sql += R"(
            AND (
                (v.chapter = :startChapter AND v.verse >= :startVerse) OR
                (v.chapter > :startChapter AND v.chapter < :endChapter) OR
                (v.chapter = :endChapter AND v.verse <= :endVerse)
            )
        )";
    }

    sql += " ORDER BY v.chapter, v.verse";

    query.prepare(sql);
    query.bindValue(":book", ref.book);
    query.bindValue(":translation", m_defaultTranslation);
    query.bindValue(":startChapter", ref.startChapter);

    if (ref.startVerse != 0) {
        query.bindValue(":startVerse", ref.startVerse);
        query.bindValue(":endVerse", ref.endVerse);
    }
    if (ref.startChapter != ref.endChapter) {
        query.bindValue(":endChapter", ref.endChapter);
    }

    if (!query.exec()) {
        qWarning() << "Bible query failed:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        BibleVerse verse;
        verse.book = query.value(0).toString();
        verse.chapter = query.value(1).toInt();
        verse.verse = query.value(2).toInt();
        verse.text = query.value(3).toString();
        verse.translation = query.value(4).toString();
        results.append(verse);
    }

    return results;
}

QList<BibleVerse> BibleDatabase::searchKeyword(const QString& keyword, int maxResults)
{
    QList<BibleVerse> results;

    if (!isValid()) {
        qWarning() << "Bible database not initialized";
        return results;
    }

    if (keyword.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(m_database);

    // Check if FTS table exists for faster search
    QStringList tables = m_database.tables();

    QString sql;
    if (tables.contains("verses_fts")) {
        // Use full-text search
        sql = R"(
            SELECT b.name, v.chapter, v.verse, v.text, v.translation
            FROM verses v
            JOIN books b ON v.book_id = b.id
            JOIN verses_fts fts ON v.id = fts.rowid
            WHERE verses_fts MATCH :keyword
              AND v.translation = :translation
            ORDER BY rank
            LIMIT :limit
        )";
    } else {
        // Fallback to LIKE search
        sql = R"(
            SELECT b.name, v.chapter, v.verse, v.text, v.translation
            FROM verses v
            JOIN books b ON v.book_id = b.id
            WHERE v.text LIKE :keyword
              AND v.translation = :translation
            LIMIT :limit
        )";
    }

    query.prepare(sql);

    if (tables.contains("verses_fts")) {
        query.bindValue(":keyword", keyword);
    } else {
        query.bindValue(":keyword", QString("%%1%").arg(keyword));
    }
    query.bindValue(":translation", m_defaultTranslation);
    query.bindValue(":limit", maxResults);

    if (!query.exec()) {
        qWarning() << "Bible keyword search failed:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        BibleVerse verse;
        verse.book = query.value(0).toString();
        verse.chapter = query.value(1).toInt();
        verse.verse = query.value(2).toInt();
        verse.text = query.value(3).toString();
        verse.translation = query.value(4).toString();
        results.append(verse);
    }

    return results;
}

QStringList BibleDatabase::availableTranslations() const
{
    QStringList translations;

    if (!isValid()) {
        return translations;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT translation FROM verses ORDER BY translation");

    if (query.exec()) {
        while (query.next()) {
            translations.append(query.value(0).toString());
        }
    }

    return translations;
}

void BibleDatabase::setDefaultTranslation(const QString& translation)
{
    m_defaultTranslation = translation;
}

QStringList BibleDatabase::bookNames() const
{
    return BOOK_NAMES;
}

int BibleDatabase::chapterCount(const QString& book) const
{
    if (!isValid()) {
        return 0;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT MAX(v.chapter)
        FROM verses v
        JOIN books b ON v.book_id = b.id
        WHERE b.name = :book AND v.translation = :translation
    )");
    query.bindValue(":book", book);
    query.bindValue(":translation", m_defaultTranslation);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

int BibleDatabase::verseCount(const QString& book, int chapter) const
{
    if (!isValid()) {
        return 0;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT MAX(v.verse)
        FROM verses v
        JOIN books b ON v.book_id = b.id
        WHERE b.name = :book AND v.chapter = :chapter AND v.translation = :translation
    )");
    query.bindValue(":book", book);
    query.bindValue(":chapter", chapter);
    query.bindValue(":translation", m_defaultTranslation);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

// =============================================================================
// Translation Import Methods
// =============================================================================

bool BibleDatabase::translationExists(const QString& code) const
{
    if (!isValid()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM verses WHERE translation = :code LIMIT 1");
    query.bindValue(":code", code);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool BibleDatabase::deleteTranslation(const QString& code)
{
    if (!isValid()) {
        qWarning() << "BibleDatabase: Cannot delete translation - database not initialized";
        return false;
    }

    QSqlQuery query(m_database);

    // Start transaction for consistency
    m_database.transaction();

    // Delete all verses for this translation
    query.prepare("DELETE FROM verses WHERE translation = :code");
    query.bindValue(":code", code);

    if (!query.exec()) {
        qWarning() << "BibleDatabase: Failed to delete translation" << code << ":" << query.lastError().text();
        m_database.rollback();
        return false;
    }

    int rowsDeleted = query.numRowsAffected();
    m_database.commit();

    qDebug() << "BibleDatabase: Deleted" << rowsDeleted << "verses for translation" << code;
    return true;
}

bool BibleDatabase::importTranslation(const TranslationInfo& translation, const QList<ImportedVerse>& verses)
{
    if (!isValid()) {
        qWarning() << "BibleDatabase: Cannot import - database not initialized";
        return false;
    }

    if (verses.isEmpty()) {
        qWarning() << "BibleDatabase: No verses to import";
        return false;
    }

    qDebug() << "BibleDatabase: Starting import of" << verses.size() << "verses for" << translation.code;

    // Build book name to ID map from the books table
    QMap<QString, int> bookIdMap;
    QSqlQuery bookQuery(m_database);
    bookQuery.prepare("SELECT id, name FROM books");
    if (bookQuery.exec()) {
        while (bookQuery.next()) {
            bookIdMap[bookQuery.value(1).toString()] = bookQuery.value(0).toInt();
        }
    }

    // Start transaction for bulk insert performance
    if (!m_database.transaction()) {
        qWarning() << "BibleDatabase: Failed to start transaction:" << m_database.lastError().text();
        return false;
    }

    // Prepare insert statement (reused for all verses)
    QSqlQuery insertQuery(m_database);
    insertQuery.prepare(R"(
        INSERT INTO verses (book_id, chapter, verse, text, translation)
        VALUES (:book_id, :chapter, :verse, :text, :translation)
    )");

    int total = verses.size();
    int imported = 0;
    int skipped = 0;

    for (int i = 0; i < total; ++i) {
        const ImportedVerse& v = verses[i];

        // Look up book ID
        int bookId = bookIdMap.value(v.book, -1);
        if (bookId < 0) {
            // Try to find book by case-insensitive match
            for (auto it = bookIdMap.constBegin(); it != bookIdMap.constEnd(); ++it) {
                if (it.key().compare(v.book, Qt::CaseInsensitive) == 0) {
                    bookId = it.value();
                    break;
                }
            }
        }

        if (bookId < 0) {
            qWarning() << "BibleDatabase: Unknown book" << v.book << "- skipping verse";
            skipped++;
            continue;
        }

        insertQuery.bindValue(":book_id", bookId);
        insertQuery.bindValue(":chapter", v.chapter);
        insertQuery.bindValue(":verse", v.verse);
        insertQuery.bindValue(":text", v.text);
        insertQuery.bindValue(":translation", translation.code);

        if (!insertQuery.exec()) {
            qWarning() << "BibleDatabase: Failed to insert verse" << v.book << v.chapter << ":" << v.verse
                       << "-" << insertQuery.lastError().text();
            skipped++;
            continue;
        }

        imported++;

        // Emit progress every 1000 verses
        if (imported % 1000 == 0) {
            emit importProgress(imported, total);
        }
    }

    // Commit transaction
    if (!m_database.commit()) {
        qWarning() << "BibleDatabase: Failed to commit transaction:" << m_database.lastError().text();
        m_database.rollback();
        return false;
    }

    // Rebuild FTS index if it exists
    QStringList tables = m_database.tables();
    if (tables.contains("verses_fts")) {
        QSqlQuery ftsQuery(m_database);
        if (ftsQuery.exec("INSERT INTO verses_fts(verses_fts) VALUES('rebuild')")) {
            qDebug() << "BibleDatabase: Rebuilt FTS index";
        } else {
            qWarning() << "BibleDatabase: Failed to rebuild FTS index:" << ftsQuery.lastError().text();
        }
    }

    qDebug() << "BibleDatabase: Import complete -" << imported << "verses imported," << skipped << "skipped";
    emit importProgress(total, total);

    return imported > 0;
}

QList<QPair<QString, QString>> BibleDatabase::translationInfo() const
{
    QList<QPair<QString, QString>> result;

    if (!isValid()) {
        return result;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT translation FROM verses ORDER BY translation");

    if (query.exec()) {
        while (query.next()) {
            QString code = query.value(0).toString();
            // For now, use code as name since we don't store full names
            // A future enhancement could add a translations table
            result.append(qMakePair(code, code));
        }
    }

    return result;
}

} // namespace Clarity
