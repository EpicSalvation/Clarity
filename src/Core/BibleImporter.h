#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <memory>

namespace Clarity {

/**
 * @brief Data for a single imported verse
 */
struct ImportedVerse {
    QString book;      ///< Canonical book name (e.g., "Genesis")
    int chapter;       ///< Chapter number (1-indexed)
    int verse;         ///< Verse number (1-indexed)
    QString text;      ///< Verse text content (plain text for searching)
    QString richText;  ///< HTML with red letter markup: <span class="jesus">...</span>
};

/**
 * @brief Translation metadata
 */
struct TranslationInfo {
    QString code;      ///< Short code (e.g., "KJV", "WEB")
    QString name;      ///< Full name (e.g., "King James Version")
    QString language;  ///< Language (e.g., "English")
    QString copyright; ///< Copyright notice
};

/**
 * @brief Result of an import operation
 */
struct ImportResult {
    TranslationInfo translation;
    QList<ImportedVerse> verses;
    QStringList warnings;
    QString error;
    bool success = false;
};

/**
 * @brief Abstract base class for Bible format importers
 *
 * Subclasses implement format-specific parsing for OSIS, USFM, USX, USFX, and Zefania.
 */
class BibleImporter : public QObject {
    Q_OBJECT

public:
    explicit BibleImporter(QObject* parent = nullptr);
    virtual ~BibleImporter();

    /**
     * @brief Import a Bible translation from a file
     * @param filePath Path to the Bible file
     * @return Import result with verses and metadata
     */
    virtual ImportResult import(const QString& filePath) = 0;

    /**
     * @brief Check if this importer can handle the given file
     * @param filePath Path to the file
     * @return true if this importer supports the format
     */
    virtual bool canHandle(const QString& filePath) const = 0;

    /**
     * @brief Get a human-readable format name
     */
    virtual QString formatName() const = 0;

    /**
     * @brief Normalize a book name/ID to canonical form
     *
     * Handles OSIS IDs (Gen, Exod), USFM IDs (GEN, EXO), and various abbreviations.
     *
     * @param name Book name or ID from the source format
     * @return Canonical book name (e.g., "Genesis"), or empty if unknown
     */
    static QString normalizeBookName(const QString& name);

signals:
    /**
     * @brief Emitted during import to report progress
     * @param current Current verse/item number
     * @param total Total number of verses/items
     * @param message Status message (e.g., "Parsing Genesis...")
     */
    void progressChanged(int current, int total, const QString& message);

protected:
    /**
     * @brief Strip HTML/XML tags and formatting from text
     */
    static QString stripFormatting(const QString& text);

    /**
     * @brief Normalize whitespace (collapse multiple spaces, trim)
     */
    static QString normalizeWhitespace(const QString& text);
};

/**
 * @brief OSIS XML format importer
 *
 * Parses Open Scripture Information Standard XML files.
 * Verse elements have osisID attributes like "Gen.1.1"
 */
class OsisImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit OsisImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("OSIS XML"); }
};

/**
 * @brief USFM plain text format importer
 *
 * Parses Unified Standard Format Markers text files.
 * Uses markers like \id, \c, \v for structure.
 */
class UsfmImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit UsfmImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("USFM"); }

private:
    /**
     * @brief Strip USFM formatting markers from verse text
     */
    static QString stripUsfmMarkers(const QString& text);
};

/**
 * @brief USX XML format importer
 *
 * Parses USFM in XML format (Digital Bible Library format).
 */
class UsxImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit UsxImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("USX XML"); }
};

/**
 * @brief USFX XML format importer
 *
 * Parses USFM-derived XML format.
 */
class UsfxImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit UsfxImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("USFX XML"); }
};

/**
 * @brief Zefania XML format importer
 *
 * Parses Zefania XML format, popular in Europe.
 * Uses numeric book IDs.
 */
class ZefaniaImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit ZefaniaImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("Zefania XML"); }

private:
    /**
     * @brief Convert numeric book ID to canonical name
     */
    static QString bookFromNumber(int number);
};

/**
 * @brief TSV (Tab-Separated Values) format importer
 *
 * Parses simple TSV files with columns:
 * orig_book_index, orig_chapter, orig_verse, orig_subverse, order_by, text
 *
 * Book index format: "01O" = book 1 Old Testament, "40N" = book 40 (Matthew) New Testament
 */
class TsvImporter : public BibleImporter {
    Q_OBJECT

public:
    explicit TsvImporter(QObject* parent = nullptr);

    ImportResult import(const QString& filePath) override;
    bool canHandle(const QString& filePath) const override;
    QString formatName() const override { return tr("TSV (Tab-Separated)"); }

private:
    /**
     * @brief Convert book index like "01O" or "40N" to canonical name
     */
    static QString bookFromIndex(const QString& index);
};

/**
 * @brief Factory for creating appropriate importer based on file format
 */
class BibleImporterFactory {
public:
    /**
     * @brief Create an importer for the given file
     * @param filePath Path to the Bible file
     * @param parent Parent QObject for the importer
     * @return Importer instance, or nullptr if format not recognized
     *
     * Caller takes ownership of the returned pointer.
     */
    static std::unique_ptr<BibleImporter> createForFile(const QString& filePath, QObject* parent = nullptr);

    /**
     * @brief Detect the format of a Bible file
     * @param filePath Path to the file
     * @return Format name, or empty string if unknown
     */
    static QString detectFormat(const QString& filePath);

    /**
     * @brief Get file filter string for QFileDialog
     * @return Filter string for all supported formats
     */
    static QString fileFilter();
};

} // namespace Clarity
