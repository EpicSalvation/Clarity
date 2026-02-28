// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QSqlDatabase>

namespace Clarity {

// Forward declarations for import types (defined in BibleImporter.h)
struct ImportedVerse;
struct TranslationInfo;

/**
 * @brief Represents a single Bible verse
 */
struct BibleVerse {
    QString book;           ///< Book name (e.g., "John", "Genesis")
    int chapter;            ///< Chapter number (1-indexed)
    int verse;              ///< Verse number (1-indexed)
    QString text;           ///< Verse text content (plain text for searching)
    QString richText;       ///< HTML-formatted text with red letter markup (may be empty)
    QString translation;    ///< Translation identifier (e.g., "KJV", "WEB")

    /**
     * @brief Returns short reference string (e.g., "John 3:16")
     */
    QString reference() const;

    /**
     * @brief Returns full reference with translation (e.g., "John 3:16 (KJV)")
     */
    QString fullReference() const;
};

/**
 * @brief Parsed Bible reference for lookups
 */
struct BibleReference {
    QString book;           ///< Book name (normalized)
    int startChapter;       ///< Starting chapter
    int startVerse;         ///< Starting verse (0 = entire chapter)
    int endChapter;         ///< Ending chapter (same as start for single chapter)
    int endVerse;           ///< Ending verse (0 = entire chapter)
    bool isValid;           ///< Whether parsing succeeded

    BibleReference()
        : startChapter(0), startVerse(0), endChapter(0), endVerse(0), isValid(false) {}
};

/**
 * @brief SQLite interface for Bible verse lookup
 *
 * Provides access to embedded Bible translations for searching
 * and retrieving verses. Supports:
 * - Reference lookup (John 3:16, John 3:16-17)
 * - Keyword/phrase search
 * - Multiple translations
 */
class BibleDatabase : public QObject {
    Q_OBJECT

public:
    explicit BibleDatabase(QObject* parent = nullptr);
    ~BibleDatabase();

    /**
     * @brief Initialize the database
     * @param dbPath Path to SQLite database file
     * @return true if initialization succeeded
     */
    bool initialize(const QString& dbPath);

    /**
     * @brief Check if database is ready for queries
     */
    bool isValid() const;

    /**
     * @brief Look up verses by reference string
     * @param reference Reference string (e.g., "John 3:16", "Genesis 1:1-5")
     * @return List of matching verses (empty if not found)
     */
    QList<BibleVerse> lookupReference(const QString& reference);

    /**
     * @brief Search verses by keyword or phrase
     * @param keyword Search term
     * @param maxResults Maximum number of results to return
     * @return List of matching verses
     */
    QList<BibleVerse> searchKeyword(const QString& keyword, int maxResults = 50);

    /**
     * @brief Get list of available translations
     */
    QStringList availableTranslations() const;

    /**
     * @brief Set the default translation for lookups
     */
    void setDefaultTranslation(const QString& translation);

    /**
     * @brief Get the current default translation
     */
    QString defaultTranslation() const { return m_defaultTranslation; }

    /**
     * @brief Parse a reference string into components
     * @param text Reference text to parse
     * @return Parsed reference (check isValid for success)
     *
     * Supports formats:
     * - "John 3:16" - Single verse
     * - "John 3:16-17" - Verse range in same chapter
     * - "John 3:16-4:3" - Cross-chapter range
     * - "Jn 3:16" - Abbreviations
     * - "1 John 3:16" - Books with numbers
     * - "Psalm 23" - Entire chapter
     */
    static BibleReference parseReference(const QString& text);

    /**
     * @brief Get list of all Bible book names
     */
    QStringList bookNames() const;

    /**
     * @brief Normalize a book name (handle abbreviations)
     * @param name Book name or abbreviation
     * @return Full canonical book name, or empty if not found
     */
    QString normalizeBookName(const QString& name) const;

    /**
     * @brief Get the number of chapters in a book
     */
    int chapterCount(const QString& book) const;

    /**
     * @brief Get the number of verses in a chapter
     */
    int verseCount(const QString& book, int chapter) const;

    // =========================================================================
    // Translation Import Methods
    // =========================================================================

    /**
     * @brief Check if a translation already exists in the database
     * @param code Translation code (e.g., "KJV")
     * @return true if translation exists
     */
    bool translationExists(const QString& code) const;

    /**
     * @brief Delete a translation and all its verses
     * @param code Translation code to delete
     * @return true on success
     */
    bool deleteTranslation(const QString& code);

    /**
     * @brief Import a translation into the database
     * @param translation Translation metadata
     * @param verses List of verses to import
     * @return true on success
     *
     * Uses a single transaction for performance. Emits importProgress signal.
     * If the translation already exists, call deleteTranslation() first.
     */
    bool importTranslation(const TranslationInfo& translation, const QList<ImportedVerse>& verses);

    /**
     * @brief Get list of translation info (code and name)
     * @return List of pairs: (code, name)
     */
    QList<QPair<QString, QString>> translationInfo() const;

signals:
    /**
     * @brief Emitted during import to report progress
     * @param current Current verse number
     * @param total Total number of verses
     */
    void importProgress(int current, int total);

private:
    /**
     * @brief Execute lookup with parsed reference
     */
    QList<BibleVerse> lookupParsedReference(const BibleReference& ref);

    /**
     * @brief Build book name lookup table
     */
    void buildBookNameIndex();

    QSqlDatabase m_database;
    QString m_defaultTranslation;
    QMap<QString, QString> m_bookAbbreviations;  ///< Abbreviation -> Full name
    bool m_isValid;
};

} // namespace Clarity
