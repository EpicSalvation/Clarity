#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QStringList>
#include <memory>

namespace Clarity {

class BibleDatabase;
class BibleImporter;
struct ImportResult;

/**
 * @brief Dialog for importing Bible translations
 *
 * Provides UI for:
 * - Selecting a Bible file (OSIS, USFM, USX, USFX, Zefania)
 * - Selecting a directory of USFM files (one book per file)
 * - Auto-detecting file format
 * - Setting translation code and name
 * - Handling duplicates (overwrite or skip)
 * - Previewing sample verses
 * - Showing import progress
 */
class BibleImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BibleImportDialog(BibleDatabase* database, QWidget* parent = nullptr);
    ~BibleImportDialog();

private slots:
    void onBrowseFileClicked();
    void onBrowseFolderClicked();
    void onImportClicked();
    void onPathChanged(const QString& path);
    void onImportProgress(int current, int total);
    void onParseProgress(int current, int total, const QString& message);

private:
    void setupUI();
    void parseFile(const QString& filePath);
    void parseDirectory(const QString& dirPath);
    void updatePreview();
    void setImportEnabled(bool enabled);

    // UI components
    QLineEdit* m_pathEdit;
    QPushButton* m_browseFileButton;
    QPushButton* m_browseFolderButton;
    QLabel* m_formatLabel;
    QLineEdit* m_codeEdit;
    QLineEdit* m_nameEdit;
    QComboBox* m_duplicateComboBox;
    QTextEdit* m_previewText;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_importButton;
    QPushButton* m_cancelButton;

    // Data
    BibleDatabase* m_database;
    std::unique_ptr<ImportResult> m_parseResult;
    QStringList m_filesToImport;  // For directory imports
    bool m_isDirectoryImport;
    bool m_importing;
};

} // namespace Clarity
