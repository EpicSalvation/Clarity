#include "BibleImportDialog.h"
#include "Core/BibleDatabase.h"
#include "Core/BibleImporter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QDebug>

namespace Clarity {

BibleImportDialog::BibleImportDialog(BibleDatabase* database, QWidget* parent)
    : QDialog(parent)
    , m_pathEdit(nullptr)
    , m_browseFileButton(nullptr)
    , m_browseFolderButton(nullptr)
    , m_formatLabel(nullptr)
    , m_codeEdit(nullptr)
    , m_nameEdit(nullptr)
    , m_duplicateComboBox(nullptr)
    , m_previewText(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_importButton(nullptr)
    , m_cancelButton(nullptr)
    , m_database(database)
    , m_isDirectoryImport(false)
    , m_importing(false)
{
    setupUI();
}

BibleImportDialog::~BibleImportDialog()
{
}

void BibleImportDialog::setupUI()
{
    setWindowTitle(tr("Import Bible Translation"));
    resize(650, 550);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Source selection group
    QGroupBox* sourceGroup = new QGroupBox(tr("Source"), this);
    QVBoxLayout* sourceLayout = new QVBoxLayout(sourceGroup);

    // Path row
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit(sourceGroup);
    m_pathEdit->setPlaceholderText(tr("Select a file or folder..."));
    connect(m_pathEdit, &QLineEdit::textChanged,
            this, &BibleImportDialog::onPathChanged);
    pathLayout->addWidget(m_pathEdit);

    m_browseFileButton = new QPushButton(tr("File..."), sourceGroup);
    m_browseFileButton->setToolTip(tr("Select a single Bible file (OSIS XML, Zefania XML, etc.)"));
    connect(m_browseFileButton, &QPushButton::clicked,
            this, &BibleImportDialog::onBrowseFileClicked);
    pathLayout->addWidget(m_browseFileButton);

    m_browseFolderButton = new QPushButton(tr("Folder..."), sourceGroup);
    m_browseFolderButton->setToolTip(tr("Select a folder containing USFM files (one book per file)"));
    connect(m_browseFolderButton, &QPushButton::clicked,
            this, &BibleImportDialog::onBrowseFolderClicked);
    pathLayout->addWidget(m_browseFolderButton);

    sourceLayout->addLayout(pathLayout);

    // Format detection label
    m_formatLabel = new QLabel(sourceGroup);
    m_formatLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    m_formatLabel->setWordWrap(true);
    sourceLayout->addWidget(m_formatLabel);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("Use <b>File</b> for single-file Bibles (OSIS, Zefania XML).\n"
           "Use <b>Folder</b> for USFM Bibles with one book per file."),
        sourceGroup);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    helpLabel->setWordWrap(true);
    sourceLayout->addWidget(helpLabel);

    mainLayout->addWidget(sourceGroup);

    // Translation info group
    QGroupBox* infoGroup = new QGroupBox(tr("Translation Info"), this);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);

    m_codeEdit = new QLineEdit(infoGroup);
    m_codeEdit->setMaxLength(10);
    m_codeEdit->setPlaceholderText(tr("e.g., KJV, NIV, ESV"));
    infoLayout->addRow(tr("Translation Code:"), m_codeEdit);

    m_nameEdit = new QLineEdit(infoGroup);
    m_nameEdit->setPlaceholderText(tr("e.g., King James Version"));
    infoLayout->addRow(tr("Full Name:"), m_nameEdit);

    // Duplicate handling
    m_duplicateComboBox = new QComboBox(infoGroup);
    m_duplicateComboBox->addItem(tr("Overwrite existing"), "overwrite");
    m_duplicateComboBox->addItem(tr("Skip if exists"), "skip");
    infoLayout->addRow(tr("If already exists:"), m_duplicateComboBox);

    mainLayout->addWidget(infoGroup);

    // Preview group
    QGroupBox* previewGroup = new QGroupBox(tr("Preview"), this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_previewText = new QTextEdit(previewGroup);
    m_previewText->setReadOnly(true);
    m_previewText->setMaximumHeight(120);
    m_previewText->setPlaceholderText(tr("Sample verses will appear here after selecting a source..."));
    previewLayout->addWidget(m_previewText);

    mainLayout->addWidget(previewGroup);

    // Progress section
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: gray; }");
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_importButton = new QPushButton(tr("Import"), this);
    m_importButton->setEnabled(false);
    m_importButton->setDefault(true);
    connect(m_importButton, &QPushButton::clicked,
            this, &BibleImportDialog::onImportClicked);
    buttonLayout->addWidget(m_importButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void BibleImportDialog::onBrowseFileClicked()
{
    QString filter = BibleImporterFactory::fileFilter();
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select Bible File"),
        QString(),
        filter
    );

    if (!filePath.isEmpty()) {
        m_pathEdit->setText(filePath);
    }
}

void BibleImportDialog::onBrowseFolderClicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("Select Folder with USFM Files"),
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (!dirPath.isEmpty()) {
        m_pathEdit->setText(dirPath);
    }
}

void BibleImportDialog::onPathChanged(const QString& path)
{
    m_formatLabel->clear();
    m_previewText->clear();
    m_parseResult.reset();
    m_filesToImport.clear();
    m_isDirectoryImport = false;
    setImportEnabled(false);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    if (fi.isDir()) {
        parseDirectory(path);
    } else if (fi.isFile()) {
        parseFile(path);
    } else {
        m_formatLabel->setText(tr("Path does not exist"));
        m_formatLabel->setStyleSheet("QLabel { color: red; }");
    }
}

void BibleImportDialog::parseFile(const QString& filePath)
{
    m_isDirectoryImport = false;

    // Detect format
    QString format = BibleImporterFactory::detectFormat(filePath);
    if (format.isEmpty()) {
        m_formatLabel->setText(tr("Unrecognized file format"));
        m_formatLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    m_formatLabel->setText(tr("Detected Format: %1").arg(format));
    m_formatLabel->setStyleSheet("QLabel { color: green; }");

    // Create importer and parse file
    auto importer = BibleImporterFactory::createForFile(filePath, this);
    if (!importer) {
        return;
    }

    // Connect progress signal for parsing
    connect(importer.get(), &BibleImporter::progressChanged,
            this, &BibleImportDialog::onParseProgress);

    m_statusLabel->setText(tr("Parsing file..."));
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(true);
    QApplication::processEvents();

    // Parse the file
    m_parseResult = std::make_unique<ImportResult>(importer->import(filePath));

    m_progressBar->setVisible(false);

    if (!m_parseResult->success) {
        m_statusLabel->setText(tr("Error: %1").arg(m_parseResult->error));
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    // Populate translation info from parsed data
    if (!m_parseResult->translation.code.isEmpty() && m_codeEdit->text().isEmpty()) {
        m_codeEdit->setText(m_parseResult->translation.code);
    }
    if (!m_parseResult->translation.name.isEmpty() && m_nameEdit->text().isEmpty()) {
        m_nameEdit->setText(m_parseResult->translation.name);
    }

    // Show status
    QString statusText = tr("Parsed %1 verses").arg(m_parseResult->verses.size());
    if (!m_parseResult->warnings.isEmpty()) {
        statusText += "\n" + m_parseResult->warnings.join("\n");
    }
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet("QLabel { color: gray; }");

    updatePreview();
    setImportEnabled(true);
}

void BibleImportDialog::parseDirectory(const QString& dirPath)
{
    m_isDirectoryImport = true;

    QDir dir(dirPath);

    // Find USFM files
    QStringList filters;
    filters << "*.usfm" << "*.USFM" << "*.sfm" << "*.SFM";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (files.isEmpty()) {
        m_formatLabel->setText(tr("No USFM files found in folder"));
        m_formatLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    m_formatLabel->setText(tr("Found %1 USFM files").arg(files.size()));
    m_formatLabel->setStyleSheet("QLabel { color: green; }");

    m_filesToImport.clear();
    for (const QFileInfo& fi : files) {
        m_filesToImport.append(fi.absoluteFilePath());
    }

    // Parse all files
    m_parseResult = std::make_unique<ImportResult>();
    m_parseResult->success = true;

    m_progressBar->setRange(0, files.size());
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);

    int fileIndex = 0;
    int totalVerses = 0;
    QStringList booksFound;

    for (const QString& filePath : m_filesToImport) {
        QFileInfo fi(filePath);
        m_statusLabel->setText(tr("Parsing %1...").arg(fi.fileName()));
        QApplication::processEvents();

        auto importer = BibleImporterFactory::createForFile(filePath, this);
        if (!importer) {
            m_parseResult->warnings.append(tr("Skipped %1: unrecognized format").arg(fi.fileName()));
            continue;
        }

        ImportResult fileResult = importer->import(filePath);

        if (fileResult.success && !fileResult.verses.isEmpty()) {
            // Append verses from this file
            m_parseResult->verses.append(fileResult.verses);
            totalVerses += fileResult.verses.size();

            // Track which book this was
            if (!fileResult.verses.isEmpty()) {
                QString book = fileResult.verses.first().book;
                if (!booksFound.contains(book)) {
                    booksFound.append(book);
                }
            }

            // Grab translation info from first successful file
            if (m_parseResult->translation.code.isEmpty() && !fileResult.translation.code.isEmpty()) {
                m_parseResult->translation = fileResult.translation;
            }
        } else if (!fileResult.error.isEmpty()) {
            m_parseResult->warnings.append(tr("%1: %2").arg(fi.fileName()).arg(fileResult.error));
        }

        fileIndex++;
        m_progressBar->setValue(fileIndex);
        QApplication::processEvents();
    }

    m_progressBar->setVisible(false);

    if (m_parseResult->verses.isEmpty()) {
        m_parseResult->success = false;
        m_parseResult->error = tr("No verses found in any of the %1 files").arg(files.size());
        m_statusLabel->setText(tr("Error: %1").arg(m_parseResult->error));
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    // Populate translation info
    if (!m_parseResult->translation.code.isEmpty() && m_codeEdit->text().isEmpty()) {
        m_codeEdit->setText(m_parseResult->translation.code);
    }
    if (!m_parseResult->translation.name.isEmpty() && m_nameEdit->text().isEmpty()) {
        m_nameEdit->setText(m_parseResult->translation.name);
    }

    // Show status
    QString statusText = tr("Parsed %1 verses from %2 books (%3 files)")
        .arg(totalVerses)
        .arg(booksFound.size())
        .arg(files.size());
    if (!m_parseResult->warnings.isEmpty()) {
        statusText += "\n" + tr("Warnings: %1").arg(m_parseResult->warnings.size());
    }
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet("QLabel { color: gray; }");

    updatePreview();
    setImportEnabled(true);
}

void BibleImportDialog::updatePreview()
{
    if (!m_parseResult || m_parseResult->verses.isEmpty()) {
        return;
    }

    QString preview;

    // For directory imports, show verses from different books
    if (m_isDirectoryImport) {
        QSet<QString> booksShown;
        int count = 0;
        for (const ImportedVerse& v : m_parseResult->verses) {
            if (!booksShown.contains(v.book) && count < 5) {
                preview += QString("%1 %2:%3 - %4\n")
                    .arg(v.book)
                    .arg(v.chapter)
                    .arg(v.verse)
                    .arg(v.text.left(60) + (v.text.length() > 60 ? "..." : ""));
                booksShown.insert(v.book);
                count++;
            }
        }
        preview += tr("\n... from %1 books, %2 total verses")
            .arg(booksShown.size())
            .arg(m_parseResult->verses.size());
    } else {
        // Single file - show first verses
        int previewCount = qMin(5, m_parseResult->verses.size());

        for (int i = 0; i < previewCount; ++i) {
            const ImportedVerse& v = m_parseResult->verses[i];
            preview += QString("%1 %2:%3 - %4\n")
                .arg(v.book)
                .arg(v.chapter)
                .arg(v.verse)
                .arg(v.text.left(80) + (v.text.length() > 80 ? "..." : ""));
        }

        if (m_parseResult->verses.size() > previewCount) {
            preview += tr("\n... and %1 more verses")
                .arg(m_parseResult->verses.size() - previewCount);
        }
    }

    m_previewText->setPlainText(preview);
}

void BibleImportDialog::setImportEnabled(bool enabled)
{
    m_importButton->setEnabled(enabled && !m_importing);
}

void BibleImportDialog::onImportClicked()
{
    if (!m_parseResult || !m_parseResult->success || m_parseResult->verses.isEmpty()) {
        QMessageBox::warning(this, tr("Import Error"),
            tr("No verses to import. Please select a valid Bible file or folder."));
        return;
    }

    QString code = m_codeEdit->text().trimmed().toUpper();
    if (code.isEmpty()) {
        QMessageBox::warning(this, tr("Import Error"),
            tr("Please enter a translation code (e.g., KJV)."));
        m_codeEdit->setFocus();
        return;
    }

    // Check for existing translation
    bool exists = m_database->translationExists(code);
    if (exists) {
        QString duplicateAction = m_duplicateComboBox->currentData().toString();
        if (duplicateAction == "skip") {
            QMessageBox::information(this, tr("Translation Exists"),
                tr("Translation '%1' already exists. Import skipped.").arg(code));
            return;
        } else {
            // Overwrite - delete existing
            int ret = QMessageBox::question(this, tr("Confirm Overwrite"),
                tr("Translation '%1' already exists. Do you want to replace it?").arg(code),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }

            m_statusLabel->setText(tr("Deleting existing translation..."));
            QApplication::processEvents();

            if (!m_database->deleteTranslation(code)) {
                QMessageBox::critical(this, tr("Import Error"),
                    tr("Failed to delete existing translation."));
                return;
            }
        }
    }

    // Update translation info with user input
    TranslationInfo info;
    info.code = code;
    info.name = m_nameEdit->text().trimmed();
    if (info.name.isEmpty()) {
        info.name = code;
    }
    info.language = m_parseResult->translation.language;
    info.copyright = m_parseResult->translation.copyright;

    // Start import
    m_importing = true;
    setImportEnabled(false);
    m_browseFileButton->setEnabled(false);
    m_browseFolderButton->setEnabled(false);
    m_codeEdit->setEnabled(false);
    m_nameEdit->setEnabled(false);
    m_duplicateComboBox->setEnabled(false);

    m_progressBar->setRange(0, m_parseResult->verses.size());
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_statusLabel->setText(tr("Importing verses..."));

    // Connect database progress signal
    connect(m_database, &BibleDatabase::importProgress,
            this, &BibleImportDialog::onImportProgress);

    QApplication::processEvents();

    // Perform import
    bool success = m_database->importTranslation(info, m_parseResult->verses);

    disconnect(m_database, &BibleDatabase::importProgress,
               this, &BibleImportDialog::onImportProgress);

    m_importing = false;
    m_progressBar->setVisible(false);

    if (success) {
        QMessageBox::information(this, tr("Import Complete"),
            tr("Successfully imported %1 verses for translation '%2'.")
                .arg(m_parseResult->verses.size())
                .arg(code));
        accept();
    } else {
        QMessageBox::critical(this, tr("Import Failed"),
            tr("Failed to import translation. Check the log for details."));
        m_browseFileButton->setEnabled(true);
        m_browseFolderButton->setEnabled(true);
        m_codeEdit->setEnabled(true);
        m_nameEdit->setEnabled(true);
        m_duplicateComboBox->setEnabled(true);
        setImportEnabled(true);
    }
}

void BibleImportDialog::onImportProgress(int current, int total)
{
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
    m_statusLabel->setText(tr("Importing... (%1 / %2 verses)").arg(current).arg(total));
    QApplication::processEvents();
}

void BibleImportDialog::onParseProgress(int current, int total, const QString& message)
{
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
    m_statusLabel->setText(message);
    QApplication::processEvents();
}

} // namespace Clarity
