// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Song.h"
#include "Core/SongLibrary.h"
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>

namespace Clarity {

/**
 * @brief Result of importing a single file
 */
struct ImportPreviewItem {
    QString filePath;
    Song song;
    bool hasDuplicate;
    QString duplicateTitle;  // Title of existing song if duplicate
    int duplicateId;         // ID of existing song if duplicate
    bool selected;           // Whether to import this song

    ImportPreviewItem()
        : hasDuplicate(false)
        , duplicateId(0)
        , selected(true) {}
};

/**
 * @brief Dialog for batch importing multiple song files
 *
 * Features:
 * - File selection via file dialog or accepts pre-loaded file list
 * - Preview list showing all songs to be imported
 * - Duplicate detection by CCLI number
 * - Options: Skip duplicates / Update existing / Import as new
 */
class BatchImportDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct dialog for selecting files
     * @param library Song library for duplicate detection
     * @param parent Parent widget
     */
    explicit BatchImportDialog(SongLibrary* library, QWidget* parent = nullptr);

    /**
     * @brief Construct dialog with pre-loaded file list (e.g., from drag & drop)
     * @param library Song library for duplicate detection
     * @param filePaths List of file paths to import
     * @param parent Parent widget
     */
    BatchImportDialog(SongLibrary* library, const QStringList& filePaths, QWidget* parent = nullptr);

    /**
     * @brief Get count of songs actually imported
     */
    int importedCount() const { return m_importedCount; }

private slots:
    void onAddFiles();
    void onRemoveSelected();
    void onClearAll();
    void onImport();
    void onItemSelectionChanged();
    void onDuplicateActionChanged(int index);

private:
    void setupUI();
    void loadFiles(const QStringList& filePaths);
    void updatePreviewList();
    void checkForDuplicates();
    void performImport();

    SongLibrary* m_library;

    // Preview list
    QListWidget* m_previewList;
    QLabel* m_statusLabel;

    // File management buttons
    QPushButton* m_addFilesButton;
    QPushButton* m_removeSelectedButton;
    QPushButton* m_clearAllButton;

    // Duplicate handling
    QComboBox* m_duplicateActionCombo;
    QCheckBox* m_selectAllCheck;

    // Dialog buttons
    QPushButton* m_importButton;
    QPushButton* m_cancelButton;

    // Data
    QList<ImportPreviewItem> m_previewItems;
    int m_importedCount;
};

} // namespace Clarity
