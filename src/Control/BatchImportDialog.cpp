// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "BatchImportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

namespace Clarity {

BatchImportDialog::BatchImportDialog(SongLibrary* library, QWidget* parent)
    : QDialog(parent)
    , m_library(library)
    , m_importedCount(0)
{
    setupUI();
    setWindowTitle(tr("Batch Import Songs"));
    resize(600, 500);
}

BatchImportDialog::BatchImportDialog(SongLibrary* library, const QStringList& filePaths, QWidget* parent)
    : QDialog(parent)
    , m_library(library)
    , m_importedCount(0)
{
    setupUI();
    setWindowTitle(tr("Batch Import Songs"));
    resize(600, 500);

    // Load the provided files
    loadFiles(filePaths);
}

void BatchImportDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File selection section
    QGroupBox* fileGroup = new QGroupBox("Files to Import", this);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);

    // Preview list
    m_previewList = new QListWidget(this);
    m_previewList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_previewList, &QListWidget::itemSelectionChanged,
            this, &BatchImportDialog::onItemSelectionChanged);
    fileLayout->addWidget(m_previewList);

    // File buttons
    QHBoxLayout* fileButtonLayout = new QHBoxLayout();

    m_addFilesButton = new QPushButton("Add Files...", this);
    connect(m_addFilesButton, &QPushButton::clicked, this, &BatchImportDialog::onAddFiles);
    fileButtonLayout->addWidget(m_addFilesButton);

    m_removeSelectedButton = new QPushButton("Remove Selected", this);
    m_removeSelectedButton->setEnabled(false);
    connect(m_removeSelectedButton, &QPushButton::clicked, this, &BatchImportDialog::onRemoveSelected);
    fileButtonLayout->addWidget(m_removeSelectedButton);

    m_clearAllButton = new QPushButton("Clear All", this);
    m_clearAllButton->setEnabled(false);
    connect(m_clearAllButton, &QPushButton::clicked, this, &BatchImportDialog::onClearAll);
    fileButtonLayout->addWidget(m_clearAllButton);

    fileButtonLayout->addStretch();
    fileLayout->addLayout(fileButtonLayout);

    mainLayout->addWidget(fileGroup);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsGroup);

    optionsLayout->addWidget(new QLabel("When duplicate CCLI# found:"));
    m_duplicateActionCombo = new QComboBox(this);
    m_duplicateActionCombo->addItem("Skip duplicate", "skip");
    m_duplicateActionCombo->addItem("Update existing", "update");
    m_duplicateActionCombo->addItem("Import as new", "import");
    m_duplicateActionCombo->setCurrentIndex(0);
    connect(m_duplicateActionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BatchImportDialog::onDuplicateActionChanged);
    optionsLayout->addWidget(m_duplicateActionCombo);

    optionsLayout->addStretch();

    m_selectAllCheck = new QCheckBox("Select all", this);
    m_selectAllCheck->setChecked(true);
    connect(m_selectAllCheck, &QCheckBox::toggled, this, [this](bool checked) {
        for (int i = 0; i < m_previewList->count(); ++i) {
            m_previewList->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    });
    optionsLayout->addWidget(m_selectAllCheck);

    mainLayout->addWidget(optionsGroup);

    // Status
    m_statusLabel = new QLabel("No files selected", this);
    mainLayout->addWidget(m_statusLabel);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_importButton = new QPushButton("Import", this);
    m_importButton->setEnabled(false);
    connect(m_importButton, &QPushButton::clicked, this, &BatchImportDialog::onImport);
    buttonLayout->addWidget(m_importButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void BatchImportDialog::onAddFiles()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select Songs to Import",
        QString(),
        "Song Files (*.xml *.txt *.usr);;OpenLyrics (*.xml);;Text Files (*.txt);;SongSelect USR (*.usr);;All Files (*)"
    );

    if (!filePaths.isEmpty()) {
        loadFiles(filePaths);
    }
}

void BatchImportDialog::loadFiles(const QStringList& filePaths)
{
    for (const QString& path : filePaths) {
        // Check if already in list
        bool alreadyLoaded = false;
        for (const ImportPreviewItem& item : m_previewItems) {
            if (item.filePath == path) {
                alreadyLoaded = true;
                break;
            }
        }
        if (alreadyLoaded) {
            continue;
        }

        // Import the song
        Song song = m_library->importFromFile(path);

        // Skip if import failed completely
        if (song.title().isEmpty() && song.sectionCount() == 0) {
            qWarning() << "Failed to import:" << path;
            continue;
        }

        ImportPreviewItem item;
        item.filePath = path;
        item.song = song;
        item.selected = true;

        m_previewItems.append(item);
    }

    checkForDuplicates();
    updatePreviewList();
}

void BatchImportDialog::checkForDuplicates()
{
    for (int i = 0; i < m_previewItems.count(); ++i) {
        ImportPreviewItem& item = m_previewItems[i];
        item.hasDuplicate = false;
        item.duplicateTitle.clear();
        item.duplicateId = 0;

        if (!item.song.ccliNumber().isEmpty()) {
            QList<Song> existing = m_library->findByCcliNumber(item.song.ccliNumber());
            if (!existing.isEmpty()) {
                item.hasDuplicate = true;
                item.duplicateTitle = existing.first().title();
                item.duplicateId = existing.first().id();
            }
        }
    }
}

void BatchImportDialog::updatePreviewList()
{
    m_previewList->clear();

    int totalCount = m_previewItems.count();
    int duplicateCount = 0;
    int selectedCount = 0;

    for (int i = 0; i < m_previewItems.count(); ++i) {
        const ImportPreviewItem& item = m_previewItems[i];

        QString displayText = item.song.title();
        if (!item.song.author().isEmpty()) {
            displayText += " - " + item.song.author();
        }
        if (!item.song.ccliNumber().isEmpty()) {
            displayText += " [CCLI# " + item.song.ccliNumber() + "]";
        }

        QListWidgetItem* listItem = new QListWidgetItem(displayText);
        listItem->setData(Qt::UserRole, i);  // Store index
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(item.selected ? Qt::Checked : Qt::Unchecked);

        if (item.hasDuplicate) {
            duplicateCount++;
            listItem->setForeground(QColor("#cc7000"));  // Orange for duplicates
            listItem->setToolTip(QString("Duplicate of existing song: %1").arg(item.duplicateTitle));
        }

        if (item.selected) {
            selectedCount++;
        }

        m_previewList->addItem(listItem);
    }

    // Update status
    QString status;
    if (totalCount == 0) {
        status = "No files loaded";
    } else {
        status = QString("%1 song(s)").arg(totalCount);
        if (duplicateCount > 0) {
            status += QString(", %1 duplicate(s)").arg(duplicateCount);
        }
        status += QString(" - %1 selected for import").arg(selectedCount);
    }
    m_statusLabel->setText(status);

    // Update button states
    m_clearAllButton->setEnabled(totalCount > 0);
    m_importButton->setEnabled(selectedCount > 0);
}

void BatchImportDialog::onRemoveSelected()
{
    QList<int> indicesToRemove;

    for (int i = m_previewList->count() - 1; i >= 0; --i) {
        if (m_previewList->item(i)->isSelected()) {
            indicesToRemove.append(m_previewList->item(i)->data(Qt::UserRole).toInt());
        }
    }

    // Sort indices in descending order to remove from end first
    std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<int>());

    for (int index : indicesToRemove) {
        m_previewItems.removeAt(index);
    }

    updatePreviewList();
}

void BatchImportDialog::onClearAll()
{
    m_previewItems.clear();
    updatePreviewList();
}

void BatchImportDialog::onItemSelectionChanged()
{
    m_removeSelectedButton->setEnabled(m_previewList->selectedItems().count() > 0);
}

void BatchImportDialog::onDuplicateActionChanged(int index)
{
    Q_UNUSED(index)
    // Action is applied during import, no immediate update needed
}

void BatchImportDialog::onImport()
{
    performImport();
    accept();
}

void BatchImportDialog::performImport()
{
    QString duplicateAction = m_duplicateActionCombo->currentData().toString();
    m_importedCount = 0;

    // Update selected state from checkboxes
    for (int i = 0; i < m_previewList->count(); ++i) {
        int index = m_previewList->item(i)->data(Qt::UserRole).toInt();
        m_previewItems[index].selected = (m_previewList->item(i)->checkState() == Qt::Checked);
    }

    for (const ImportPreviewItem& item : m_previewItems) {
        if (!item.selected) {
            continue;
        }

        if (item.hasDuplicate) {
            if (duplicateAction == "skip") {
                qDebug() << "Skipping duplicate:" << item.song.title();
                continue;
            } else if (duplicateAction == "update") {
                qDebug() << "Updating existing song:" << item.duplicateTitle;
                m_library->updateSong(item.duplicateId, item.song);
                m_importedCount++;
                continue;
            }
            // "import" falls through to add as new
        }

        // Add as new song
        m_library->addSong(item.song);
        m_importedCount++;
    }

    m_library->saveLibrary();
    qDebug() << "Batch import complete:" << m_importedCount << "songs imported";
}

} // namespace Clarity
