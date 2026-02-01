#include "SongLibraryDialog.h"
#include "SongEditorDialog.h"
#include "BatchImportDialog.h"
#include "CCLIReportDialog.h"
#include "SongSelectSearchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QSizePolicy>
#include <QMimeData>
#include <QUrl>
#include <QDate>
#include <QDebug>

namespace Clarity {

SongLibraryDialog::SongLibraryDialog(SongLibrary* library, QWidget* parent)
    : QDialog(parent)
    , m_library(library)
    , m_selectedSongId(0)
    , m_backgroundColor(QColor("#1e3a8a"))
    , m_textColor(QColor("#ffffff"))
    , m_fontFamily("Arial")
    , m_fontSize(48)
    , m_dropOverlay(nullptr)
{
    setupUI();

    setWindowTitle(tr("Song Library"));
    resize(800, 600);

    // Enable drag & drop
    setAcceptDrops(true);

    // Prevent the dialog from auto-resizing based on content
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Load and display all songs
    refreshSongList();

    // Set focus to search field
    m_searchEdit->setFocus();
}

void SongLibraryDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search section
    QHBoxLayout* searchLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search by title, author, or lyrics...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SongLibraryDialog::onSearch);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SongLibraryDialog::onSearch);
    searchLayout->addWidget(m_searchEdit);

    m_searchButton = new QPushButton("Search", this);
    connect(m_searchButton, &QPushButton::clicked, this, &SongLibraryDialog::onSearch);
    searchLayout->addWidget(m_searchButton);

    searchLayout->addSpacing(10);

    searchLayout->addWidget(new QLabel("Filter:"));
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("All Songs", "all");
    m_filterCombo->addItem("Used This Month", "month");
    m_filterCombo->addItem("Used This Year", "year");
    m_filterCombo->addItem("Never Used", "never");
    m_filterCombo->addItem("Has CCLI#", "ccli");
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SongLibraryDialog::onFilterChanged);
    searchLayout->addWidget(m_filterCombo);

    searchLayout->addStretch();

    m_ccliReportButton = new QPushButton("CCLI Report...", this);
    m_ccliReportButton->setToolTip("Generate CCLI usage report for licensing");
    connect(m_ccliReportButton, &QPushButton::clicked, this, &SongLibraryDialog::onCCLIReport);
    searchLayout->addWidget(m_ccliReportButton);

    mainLayout->addLayout(searchLayout);

    // Main content splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: Song list
    QWidget* listWidget = new QWidget(this);
    QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);

    m_songCountLabel = new QLabel("Songs: 0", this);
    listLayout->addWidget(m_songCountLabel);

    m_songList = new QListWidget(this);
    connect(m_songList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem*, QListWidgetItem*) { onSongSelectionChanged(); });
    connect(m_songList, &QListWidget::itemDoubleClicked,
            this, &SongLibraryDialog::onSongDoubleClicked);
    listLayout->addWidget(m_songList);

    // Library management buttons
    QHBoxLayout* libButtonLayout = new QHBoxLayout();

    m_importButton = new QPushButton("Import...", this);
    m_importButton->setToolTip("Import song from file (OpenLyrics XML, SongSelect TXT/USR)");
    connect(m_importButton, &QPushButton::clicked, this, &SongLibraryDialog::onImport);
    libButtonLayout->addWidget(m_importButton);

    m_batchImportButton = new QPushButton("Batch Import...", this);
    m_batchImportButton->setToolTip("Import multiple songs at once with duplicate detection");
    connect(m_batchImportButton, &QPushButton::clicked, this, &SongLibraryDialog::onBatchImport);
    libButtonLayout->addWidget(m_batchImportButton);

    QPushButton* searchSongSelectButton = new QPushButton("Search SongSelect...", this);
    searchSongSelectButton->setToolTip("Search CCLI SongSelect to find and download songs");
    connect(searchSongSelectButton, &QPushButton::clicked, this, &SongLibraryDialog::onSearchSongSelect);
    libButtonLayout->addWidget(searchSongSelectButton);

    m_newButton = new QPushButton("New", this);
    m_newButton->setToolTip("Create a new song");
    connect(m_newButton, &QPushButton::clicked, this, &SongLibraryDialog::onNewSong);
    libButtonLayout->addWidget(m_newButton);

    m_editButton = new QPushButton("Edit", this);
    m_editButton->setToolTip("Edit selected song");
    m_editButton->setEnabled(false);
    connect(m_editButton, &QPushButton::clicked, this, &SongLibraryDialog::onEditSong);
    libButtonLayout->addWidget(m_editButton);

    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setToolTip("Delete selected song");
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &SongLibraryDialog::onDeleteSong);
    libButtonLayout->addWidget(m_deleteButton);

    listLayout->addLayout(libButtonLayout);
    splitter->addWidget(listWidget);

    // Right side: Song details
    QWidget* detailsWidget = new QWidget(this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);

    // Song metadata
    QGroupBox* metadataGroup = new QGroupBox("Song Details", this);
    QFormLayout* metadataLayout = new QFormLayout(metadataGroup);

    // Helper to configure detail labels: clip text, show tooltip for full content
    auto setupDetailLabel = [](QLabel* label) {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    };

    m_titleLabel = new QLabel("-", this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14pt;");
    setupDetailLabel(m_titleLabel);
    metadataLayout->addRow("Title:", m_titleLabel);

    m_authorLabel = new QLabel("-", this);
    setupDetailLabel(m_authorLabel);
    metadataLayout->addRow("Author:", m_authorLabel);

    m_copyrightLabel = new QLabel("-", this);
    setupDetailLabel(m_copyrightLabel);
    metadataLayout->addRow("Copyright:", m_copyrightLabel);

    m_ccliLabel = new QLabel("-", this);
    setupDetailLabel(m_ccliLabel);
    metadataLayout->addRow("CCLI #:", m_ccliLabel);

    m_usageLabel = new QLabel("-", this);
    setupDetailLabel(m_usageLabel);
    metadataLayout->addRow("Times Used:", m_usageLabel);

    m_lastUsedLabel = new QLabel("-", this);
    setupDetailLabel(m_lastUsedLabel);
    metadataLayout->addRow("Last Used:", m_lastUsedLabel);

    detailsLayout->addWidget(metadataGroup);

    // Lyrics preview
    QLabel* previewLabel = new QLabel("Lyrics:", this);
    detailsLayout->addWidget(previewLabel);

    m_lyricsPreview = new QTextEdit(this);
    m_lyricsPreview->setReadOnly(true);
    m_lyricsPreview->setStyleSheet(
        "QTextEdit { background-color: #f5f5f5; color: #1a1a1a; font-family: monospace; font-size: 11pt; }"
    );
    detailsLayout->addWidget(m_lyricsPreview);

    splitter->addWidget(detailsWidget);

    // Set initial sizes: narrower song list (250), wider details (550)
    splitter->setSizes({250, 550});
    // Prevent the splitter from collapsing either side
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    mainLayout->addWidget(splitter, 1);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox("Insert Options", this);
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsGroup);

    m_includeLabelCheck = new QCheckBox("Include section labels (Verse 1, Chorus, etc.)", this);
    m_includeLabelCheck->setChecked(false);
    connect(m_includeLabelCheck, &QCheckBox::checkStateChanged,
            this, &SongLibraryDialog::onIncludeLabelChanged);
    optionsLayout->addWidget(m_includeLabelCheck);

    optionsLayout->addSpacing(20);

    optionsLayout->addWidget(new QLabel("Font size:"));
    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(12, 144);
    m_fontSizeSpinBox->setValue(48);
    m_fontSizeSpinBox->setSuffix(" pt");
    optionsLayout->addWidget(m_fontSizeSpinBox);

    optionsLayout->addSpacing(20);

    optionsLayout->addWidget(new QLabel("Max lines/slide:"));
    m_maxLinesSpinBox = new QSpinBox(this);
    m_maxLinesSpinBox->setRange(0, 20);
    m_maxLinesSpinBox->setValue(0);
    m_maxLinesSpinBox->setSpecialValueText("No limit");
    m_maxLinesSpinBox->setToolTip("Split sections with more lines into multiple slides (0 = no limit)");
    optionsLayout->addWidget(m_maxLinesSpinBox);

    optionsLayout->addStretch();

    mainLayout->addWidget(optionsGroup);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_insertButton = new QPushButton("Insert", this);
    m_insertButton->setEnabled(false);
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_insertButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void SongLibraryDialog::refreshSongList()
{
    QString query = m_searchEdit->text().trimmed();
    QString filter = m_filterCombo->currentData().toString();

    QList<Song> songs;

    if (query.isEmpty()) {
        songs = m_library->allSongs();
    } else {
        songs = m_library->search(query);
    }

    // Apply filter
    if (filter != "all") {
        QList<Song> filtered;
        QDate today = QDate::currentDate();

        for (const Song& song : songs) {
            if (filter == "month") {
                // Used this month
                QDate monthStart(today.year(), today.month(), 1);
                if (song.usageCountInRange(monthStart, today) > 0) {
                    filtered.append(song);
                }
            } else if (filter == "year") {
                // Used this year
                QDate yearStart(today.year(), 1, 1);
                if (song.usageCountInRange(yearStart, today) > 0) {
                    filtered.append(song);
                }
            } else if (filter == "never") {
                // Never used
                if (song.usageCount() == 0) {
                    filtered.append(song);
                }
            } else if (filter == "ccli") {
                // Has CCLI number
                if (!song.ccliNumber().isEmpty()) {
                    filtered.append(song);
                }
            }
        }
        songs = filtered;
    }

    populateSongList(songs);
}

void SongLibraryDialog::onFilterChanged(int index)
{
    Q_UNUSED(index)
    refreshSongList();
}

void SongLibraryDialog::onCCLIReport()
{
    CCLIReportDialog dialog(m_library, this);
    dialog.exec();
}

void SongLibraryDialog::onSearchSongSelect()
{
    SongSelectSearchDialog dialog(this);
    dialog.exec();
}

void SongLibraryDialog::populateSongList(const QList<Song>& songs)
{
    m_songList->clear();
    m_selectedSongId = 0;

    for (const Song& song : songs) {
        QString itemText = song.title();
        if (!song.author().isEmpty()) {
            itemText += " - " + song.author();
        }
        // Add CCLI# if available
        if (!song.ccliNumber().isEmpty()) {
            itemText += QString(" [#%1]").arg(song.ccliNumber());
        }

        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, song.id());

        // Add tooltip with usage info
        QString tooltip = song.title();
        if (!song.ccliNumber().isEmpty()) {
            tooltip += QString("\nCCLI #: %1").arg(song.ccliNumber());
        }
        if (song.usageCount() > 0) {
            tooltip += QString("\nUsed %1 time(s)").arg(song.usageCount());
        }
        item->setToolTip(tooltip);

        m_songList->addItem(item);
    }

    m_songCountLabel->setText(QString("Songs: %1").arg(songs.count()));

    // Update button states
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_insertButton->setEnabled(false);

    // Clear details
    showSongDetails(Song());
}

void SongLibraryDialog::onSearch()
{
    refreshSongList();
}

void SongLibraryDialog::onSongSelectionChanged()
{
    QListWidgetItem* current = m_songList->currentItem();

    if (!current) {
        m_selectedSongId = 0;
        m_editButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_insertButton->setEnabled(false);
        showSongDetails(Song());
        return;
    }

    m_selectedSongId = current->data(Qt::UserRole).toInt();
    m_editButton->setEnabled(true);
    m_deleteButton->setEnabled(true);
    m_insertButton->setEnabled(true);

    Song song = m_library->getSong(m_selectedSongId);
    showSongDetails(song);
}

void SongLibraryDialog::onSongDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item)
    if (m_selectedSongId > 0) {
        accept();  // Insert the song
    }
}

void SongLibraryDialog::showSongDetails(const Song& song)
{
    if (song.title().isEmpty()) {
        m_titleLabel->setText("-");
        m_titleLabel->setToolTip("");
        m_authorLabel->setText("-");
        m_authorLabel->setToolTip("");
        m_copyrightLabel->setText("-");
        m_copyrightLabel->setToolTip("");
        m_ccliLabel->setText("-");
        m_ccliLabel->setToolTip("");
        m_usageLabel->setText("-");
        m_usageLabel->setToolTip("");
        m_lastUsedLabel->setText("-");
        m_lastUsedLabel->setToolTip("");
        m_lyricsPreview->clear();
        return;
    }

    m_titleLabel->setText(song.title());
    m_titleLabel->setToolTip(song.title());

    QString authorText = song.author().isEmpty() ? "(Unknown)" : song.author();
    m_authorLabel->setText(authorText);
    m_authorLabel->setToolTip(authorText);

    QString copyrightText = song.copyright().isEmpty() ? "-" : song.copyright();
    m_copyrightLabel->setText(copyrightText);
    m_copyrightLabel->setToolTip(song.copyright().isEmpty() ? "" : song.copyright());

    m_ccliLabel->setText(song.ccliNumber().isEmpty() ? "-" : song.ccliNumber());
    m_ccliLabel->setToolTip(song.ccliNumber().isEmpty() ? "" : song.ccliNumber());

    // Usage statistics
    int usageCount = song.usageCount();
    m_usageLabel->setText(QString::number(usageCount));
    if (usageCount > 0) {
        // Show recent usage dates in tooltip
        QStringList recentDates;
        QList<SongUsage> history = song.usageHistory();
        int limit = qMin(5, history.count());
        for (int i = history.count() - 1; i >= history.count() - limit; --i) {
            QString dateStr = history[i].dateTime.toString("yyyy-MM-dd");
            if (!history[i].eventName.isEmpty()) {
                dateStr += " (" + history[i].eventName + ")";
            }
            recentDates.append(dateStr);
        }
        m_usageLabel->setToolTip("Recent uses:\n" + recentDates.join("\n"));
    } else {
        m_usageLabel->setToolTip("");
    }

    // Last used date
    if (song.lastUsed().isValid()) {
        QDateTime lastUsed = song.lastUsed();
        QString lastUsedStr = lastUsed.date().toString("MMM d, yyyy");

        // Add relative time
        qint64 daysAgo = lastUsed.date().daysTo(QDate::currentDate());
        if (daysAgo == 0) {
            lastUsedStr += " (today)";
        } else if (daysAgo == 1) {
            lastUsedStr += " (yesterday)";
        } else if (daysAgo < 7) {
            lastUsedStr += QString(" (%1 days ago)").arg(daysAgo);
        } else if (daysAgo < 30) {
            lastUsedStr += QString(" (%1 weeks ago)").arg(daysAgo / 7);
        } else if (daysAgo < 365) {
            lastUsedStr += QString(" (%1 months ago)").arg(daysAgo / 30);
        }

        m_lastUsedLabel->setText(lastUsedStr);
        m_lastUsedLabel->setToolTip(lastUsed.toString("yyyy-MM-dd hh:mm"));
    } else {
        m_lastUsedLabel->setText("Never");
        m_lastUsedLabel->setToolTip("");
    }

    // Build lyrics preview with section labels
    QStringList previewLines;
    for (const SongSection& section : song.sections()) {
        previewLines.append("[" + section.label + "]");
        previewLines.append(section.text);
        previewLines.append("");
    }

    m_lyricsPreview->setPlainText(previewLines.join("\n"));
}

void SongLibraryDialog::onImport()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Import Song",
        QString(),
        "Song Files (*.xml *.txt);;OpenLyrics (*.xml);;Text Files (*.txt);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    Song song = m_library->importFromFile(filePath);

    if (song.title().isEmpty() && song.sectionCount() == 0) {
        QMessageBox::warning(this, "Import Failed",
            "Could not import song from file.\nPlease check the file format.");
        return;
    }

    // Open editor to review/modify before adding
    SongEditorDialog editor(this);
    editor.setSong(song);
    editor.setWindowTitle("Review Imported Song");

    if (editor.exec() == QDialog::Accepted) {
        Song editedSong = editor.song();
        int id = m_library->addSong(editedSong);
        m_library->saveLibrary();

        refreshSongList();

        // Select the newly added song
        for (int i = 0; i < m_songList->count(); ++i) {
            if (m_songList->item(i)->data(Qt::UserRole).toInt() == id) {
                m_songList->setCurrentRow(i);
                break;
            }
        }

        QMessageBox::information(this, "Song Imported",
            QString("'%1' has been added to the library.").arg(editedSong.title()));
    }
}

void SongLibraryDialog::onNewSong()
{
    SongEditorDialog editor(this);
    editor.setWindowTitle("New Song");

    if (editor.exec() == QDialog::Accepted) {
        Song song = editor.song();

        if (song.title().isEmpty()) {
            QMessageBox::warning(this, "Missing Title",
                "Please enter a title for the song.");
            return;
        }

        int id = m_library->addSong(song);
        m_library->saveLibrary();

        refreshSongList();

        // Select the newly added song
        for (int i = 0; i < m_songList->count(); ++i) {
            if (m_songList->item(i)->data(Qt::UserRole).toInt() == id) {
                m_songList->setCurrentRow(i);
                break;
            }
        }
    }
}

void SongLibraryDialog::onEditSong()
{
    if (m_selectedSongId <= 0) {
        return;
    }

    Song song = m_library->getSong(m_selectedSongId);

    SongEditorDialog editor(this);
    editor.setSong(song);
    editor.setWindowTitle("Edit Song");

    if (editor.exec() == QDialog::Accepted) {
        Song editedSong = editor.song();
        m_library->updateSong(m_selectedSongId, editedSong);
        m_library->saveLibrary();

        refreshSongList();

        // Re-select the edited song
        for (int i = 0; i < m_songList->count(); ++i) {
            if (m_songList->item(i)->data(Qt::UserRole).toInt() == m_selectedSongId) {
                m_songList->setCurrentRow(i);
                break;
            }
        }
    }
}

void SongLibraryDialog::onDeleteSong()
{
    if (m_selectedSongId <= 0) {
        return;
    }

    Song song = m_library->getSong(m_selectedSongId);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Song",
        QString("Are you sure you want to delete '%1'?").arg(song.title()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_library->removeSong(m_selectedSongId);
        m_library->saveLibrary();
        refreshSongList();
    }
}

void SongLibraryDialog::onIncludeLabelChanged(Qt::CheckState state)
{
    Q_UNUSED(state)
    // Could update preview here if we add live preview
}

void SongLibraryDialog::setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                                         const QString& fontFamily, int fontSize)
{
    m_backgroundColor = bgColor;
    m_textColor = textColor;
    m_fontFamily = fontFamily;
    m_fontSize = fontSize;
    m_fontSizeSpinBox->setValue(fontSize);
}

int SongLibraryDialog::selectedSongId() const
{
    return m_selectedSongId;
}

bool SongLibraryDialog::includeSectionLabels() const
{
    return m_includeLabelCheck->isChecked();
}

int SongLibraryDialog::maxLinesPerSlide() const
{
    return m_maxLinesSpinBox->value();
}

SlideStyle SongLibraryDialog::slideStyle() const
{
    return SlideStyle(m_backgroundColor, m_textColor, m_fontFamily, m_fontSizeSpinBox->value());
}

QList<Slide> SongLibraryDialog::getSlides() const
{
    if (m_selectedSongId <= 0) {
        return QList<Slide>();
    }

    Song song = m_library->getSong(m_selectedSongId);
    return createSlidesFromSong(song);
}

QList<Slide> SongLibraryDialog::createSlidesFromSong(const Song& song) const
{
    SlideStyle style(m_backgroundColor, m_textColor, m_fontFamily, m_fontSizeSpinBox->value());
    bool includeTitleSlide = true;  // Always include a title slide at the start
    bool includeLabels = m_includeLabelCheck->isChecked();
    int maxLines = m_maxLinesSpinBox->value();
    return song.toSlides(style, includeTitleSlide, includeLabels, maxLines);
}

void SongLibraryDialog::onBatchImport()
{
    BatchImportDialog dialog(m_library, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshSongList();

        int importedCount = dialog.importedCount();
        if (importedCount > 0) {
            QMessageBox::information(this, "Import Complete",
                QString("%1 song(s) imported successfully.").arg(importedCount));
        }
    }
}

void SongLibraryDialog::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        // Check if any files have supported extensions
        bool hasValidFile = false;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                QString ext = path.section('.', -1).toLower();
                if (ext == "xml" || ext == "txt" || ext == "usr") {
                    hasValidFile = true;
                    break;
                }
            }
        }

        if (hasValidFile) {
            event->acceptProposedAction();
            showDropIndicator(true);
        }
    }
}

void SongLibraryDialog::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event)
    showDropIndicator(false);
}

void SongLibraryDialog::dropEvent(QDropEvent* event)
{
    showDropIndicator(false);

    QStringList filePaths;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            QString ext = path.section('.', -1).toLower();
            if (ext == "xml" || ext == "txt" || ext == "usr") {
                filePaths.append(path);
            }
        }
    }

    if (!filePaths.isEmpty()) {
        event->acceptProposedAction();
        importFiles(filePaths);
    }
}

void SongLibraryDialog::importFiles(const QStringList& filePaths)
{
    if (filePaths.count() == 1) {
        // Single file - use regular import flow with editor
        Song song = m_library->importFromFile(filePaths.first());

        if (song.title().isEmpty() && song.sectionCount() == 0) {
            QMessageBox::warning(this, "Import Failed",
                "Could not import song from file.\nPlease check the file format.");
            return;
        }

        // Open editor to review/modify before adding
        SongEditorDialog editor(this);
        editor.setSong(song);
        editor.setWindowTitle("Review Imported Song");

        if (editor.exec() == QDialog::Accepted) {
            Song editedSong = editor.song();

            // Check for duplicates by CCLI number
            if (!editedSong.ccliNumber().isEmpty()) {
                QList<Song> existing = m_library->findByCcliNumber(editedSong.ccliNumber());
                if (!existing.isEmpty()) {
                    QMessageBox::StandardButton reply = QMessageBox::question(
                        this,
                        "Duplicate Found",
                        QString("A song with CCLI# %1 already exists:\n'%2'\n\nImport anyway?")
                            .arg(editedSong.ccliNumber())
                            .arg(existing.first().title()),
                        QMessageBox::Yes | QMessageBox::No
                    );
                    if (reply != QMessageBox::Yes) {
                        return;
                    }
                }
            }

            int id = m_library->addSong(editedSong);
            m_library->saveLibrary();

            refreshSongList();

            // Select the newly added song
            for (int i = 0; i < m_songList->count(); ++i) {
                if (m_songList->item(i)->data(Qt::UserRole).toInt() == id) {
                    m_songList->setCurrentRow(i);
                    break;
                }
            }

            QMessageBox::information(this, "Song Imported",
                QString("'%1' has been added to the library.").arg(editedSong.title()));
        }
    } else {
        // Multiple files - use batch import dialog
        BatchImportDialog dialog(m_library, filePaths, this);
        if (dialog.exec() == QDialog::Accepted) {
            refreshSongList();

            int importedCount = dialog.importedCount();
            if (importedCount > 0) {
                QMessageBox::information(this, "Import Complete",
                    QString("%1 song(s) imported successfully.").arg(importedCount));
            }
        }
    }
}

void SongLibraryDialog::showDropIndicator(bool show)
{
    if (show) {
        if (!m_dropOverlay) {
            m_dropOverlay = new QWidget(this);
            m_dropOverlay->setStyleSheet(
                "background-color: rgba(30, 58, 138, 0.3);"
                "border: 3px dashed #1e3a8a;"
            );
        }
        m_dropOverlay->setGeometry(rect());
        m_dropOverlay->raise();
        m_dropOverlay->show();
    } else if (m_dropOverlay) {
        m_dropOverlay->hide();
    }
}

} // namespace Clarity
