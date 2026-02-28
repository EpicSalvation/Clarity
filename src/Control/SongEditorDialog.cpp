// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SongEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

namespace Clarity {

SongEditorDialog::SongEditorDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentSectionIndex(-1)
{
    setupUI();
    setWindowTitle(tr("Song Editor"));
    resize(700, 600);
}

void SongEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Metadata section
    QGroupBox* metadataGroup = new QGroupBox(tr("Song Information"), this);
    QFormLayout* metadataLayout = new QFormLayout(metadataGroup);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Enter song title...");
    metadataLayout->addRow("Title:", m_titleEdit);

    m_authorEdit = new QLineEdit(this);
    m_authorEdit->setPlaceholderText("Enter author/composer...");
    metadataLayout->addRow("Author:", m_authorEdit);

    m_copyrightEdit = new QLineEdit(this);
    m_copyrightEdit->setPlaceholderText("e.g., Public Domain, (c) 2020 Publisher");
    metadataLayout->addRow("Copyright:", m_copyrightEdit);

    m_ccliEdit = new QLineEdit(this);
    m_ccliEdit->setPlaceholderText("CCLI song number (optional)");
    metadataLayout->addRow("CCLI #:", m_ccliEdit);

    mainLayout->addWidget(metadataGroup);

    // Sections editor
    QGroupBox* sectionsGroup = new QGroupBox("Lyrics Sections", this);
    QHBoxLayout* sectionsLayout = new QHBoxLayout(sectionsGroup);

    // Left: Section list
    QVBoxLayout* listLayout = new QVBoxLayout();

    m_sectionList = new QListWidget(this);
    m_sectionList->setMinimumWidth(150);
    m_sectionList->setMaximumWidth(200);
    connect(m_sectionList, &QListWidget::currentRowChanged,
            this, [this](int) { onSectionSelectionChanged(); });
    listLayout->addWidget(m_sectionList);

    // Section management buttons
    QHBoxLayout* sectionButtonLayout = new QHBoxLayout();

    m_addSectionButton = new QPushButton("+", this);
    m_addSectionButton->setToolTip("Add new section");
    m_addSectionButton->setMaximumWidth(40);
    connect(m_addSectionButton, &QPushButton::clicked, this, &SongEditorDialog::onAddSection);
    sectionButtonLayout->addWidget(m_addSectionButton);

    m_removeSectionButton = new QPushButton("-", this);
    m_removeSectionButton->setToolTip("Remove selected section");
    m_removeSectionButton->setMaximumWidth(40);
    m_removeSectionButton->setEnabled(false);
    connect(m_removeSectionButton, &QPushButton::clicked, this, &SongEditorDialog::onRemoveSection);
    sectionButtonLayout->addWidget(m_removeSectionButton);

    m_moveUpButton = new QPushButton("\u2191", this);  // Up arrow
    m_moveUpButton->setToolTip("Move section up");
    m_moveUpButton->setMaximumWidth(40);
    m_moveUpButton->setEnabled(false);
    connect(m_moveUpButton, &QPushButton::clicked, this, &SongEditorDialog::onMoveUp);
    sectionButtonLayout->addWidget(m_moveUpButton);

    m_moveDownButton = new QPushButton("\u2193", this);  // Down arrow
    m_moveDownButton->setToolTip("Move section down");
    m_moveDownButton->setMaximumWidth(40);
    m_moveDownButton->setEnabled(false);
    connect(m_moveDownButton, &QPushButton::clicked, this, &SongEditorDialog::onMoveDown);
    sectionButtonLayout->addWidget(m_moveDownButton);

    listLayout->addLayout(sectionButtonLayout);
    sectionsLayout->addLayout(listLayout);

    // Right: Section editor
    QVBoxLayout* editorLayout = new QVBoxLayout();

    // Section type and label
    QHBoxLayout* sectionHeaderLayout = new QHBoxLayout();

    sectionHeaderLayout->addWidget(new QLabel("Type:"));
    m_sectionTypeCombo = new QComboBox(this);
    m_sectionTypeCombo->addItem("Verse", "verse");
    m_sectionTypeCombo->addItem("Chorus", "chorus");
    m_sectionTypeCombo->addItem("Bridge", "bridge");
    m_sectionTypeCombo->addItem("Pre-Chorus", "pre-chorus");
    m_sectionTypeCombo->addItem("Tag", "tag");
    m_sectionTypeCombo->addItem("Intro", "intro");
    m_sectionTypeCombo->addItem("Outro", "outro");
    m_sectionTypeCombo->setEnabled(false);
    sectionHeaderLayout->addWidget(m_sectionTypeCombo);

    sectionHeaderLayout->addSpacing(20);

    sectionHeaderLayout->addWidget(new QLabel("Label:"));
    m_sectionLabelEdit = new QLineEdit(this);
    m_sectionLabelEdit->setPlaceholderText("e.g., Verse 1, Chorus");
    m_sectionLabelEdit->setEnabled(false);
    sectionHeaderLayout->addWidget(m_sectionLabelEdit);

    editorLayout->addLayout(sectionHeaderLayout);

    // Section text
    QLabel* textLabel = new QLabel("Lyrics:", this);
    editorLayout->addWidget(textLabel);

    m_sectionTextEdit = new QTextEdit(this);
    m_sectionTextEdit->setPlaceholderText("Enter section lyrics here...\n\nEach section becomes one slide.");
    m_sectionTextEdit->setEnabled(false);
    connect(m_sectionTextEdit, &QTextEdit::textChanged, this, &SongEditorDialog::onSectionTextChanged);
    editorLayout->addWidget(m_sectionTextEdit);

    sectionsLayout->addLayout(editorLayout, 1);
    mainLayout->addWidget(sectionsGroup, 1);

    // Help text
    QLabel* helpLabel = new QLabel(
        "Tip: Each section becomes one slide. Create separate sections for each verse, chorus, etc.",
        this
    );
    helpLabel->setStyleSheet("color: #666;");
    helpLabel->setWordWrap(true);
    mainLayout->addWidget(helpLabel);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton("OK", this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void SongEditorDialog::setSong(const Song& song)
{
    m_originalSong = song;

    // Set metadata
    m_titleEdit->setText(song.title());
    m_authorEdit->setText(song.author());
    m_copyrightEdit->setText(song.copyright());
    m_ccliEdit->setText(song.ccliNumber());

    // Copy sections
    m_sections = song.sections();
    m_currentSectionIndex = -1;

    updateSectionList();

    // Select first section if available
    if (!m_sections.isEmpty()) {
        m_sectionList->setCurrentRow(0);
    }
}

Song SongEditorDialog::song() const
{
    Song result = m_originalSong;

    result.setTitle(m_titleEdit->text().trimmed());
    result.setAuthor(m_authorEdit->text().trimmed());
    result.setCopyright(m_copyrightEdit->text().trimmed());
    result.setCcliNumber(m_ccliEdit->text().trimmed());

    // Need to save current section if editing
    // Note: This is const, so we need to work around it
    QList<SongSection> sections = m_sections;

    // Save current section if one is selected
    if (m_currentSectionIndex >= 0 && m_currentSectionIndex < sections.count()) {
        sections[m_currentSectionIndex].type = m_sectionTypeCombo->currentData().toString();
        sections[m_currentSectionIndex].label = m_sectionLabelEdit->text().trimmed();
        sections[m_currentSectionIndex].text = m_sectionTextEdit->toPlainText().trimmed();
    }

    result.setSections(sections);

    return result;
}

void SongEditorDialog::updateSectionList()
{
    m_sectionList->clear();

    for (const SongSection& section : m_sections) {
        m_sectionList->addItem(section.label);
    }
}

void SongEditorDialog::saveCurrentSection()
{
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= m_sections.count()) {
        return;
    }

    m_sections[m_currentSectionIndex].type = m_sectionTypeCombo->currentData().toString();
    m_sections[m_currentSectionIndex].label = m_sectionLabelEdit->text().trimmed();
    m_sections[m_currentSectionIndex].text = m_sectionTextEdit->toPlainText().trimmed();

    // Update list item text
    if (m_sectionList->item(m_currentSectionIndex)) {
        m_sectionList->item(m_currentSectionIndex)->setText(m_sections[m_currentSectionIndex].label);
    }
}

void SongEditorDialog::onSectionSelectionChanged()
{
    // Save current section before switching
    saveCurrentSection();

    int newIndex = m_sectionList->currentRow();

    // Update button states
    bool hasSelection = newIndex >= 0;
    m_removeSectionButton->setEnabled(hasSelection);
    m_moveUpButton->setEnabled(hasSelection && newIndex > 0);
    m_moveDownButton->setEnabled(hasSelection && newIndex < m_sections.count() - 1);

    // Enable/disable editor controls
    m_sectionTypeCombo->setEnabled(hasSelection);
    m_sectionLabelEdit->setEnabled(hasSelection);
    m_sectionTextEdit->setEnabled(hasSelection);

    if (!hasSelection) {
        m_currentSectionIndex = -1;
        m_sectionTypeCombo->setCurrentIndex(0);
        m_sectionLabelEdit->clear();
        m_sectionTextEdit->clear();
        return;
    }

    m_currentSectionIndex = newIndex;
    const SongSection& section = m_sections[newIndex];

    // Find and select the type in combo
    int typeIndex = m_sectionTypeCombo->findData(section.type);
    if (typeIndex >= 0) {
        m_sectionTypeCombo->setCurrentIndex(typeIndex);
    }

    m_sectionLabelEdit->setText(section.label);
    m_sectionTextEdit->setPlainText(section.text);
}

void SongEditorDialog::onSectionTextChanged()
{
    // Auto-save as user types (optional, could be removed for explicit save)
    // Currently we save on selection change
}

void SongEditorDialog::onAddSection()
{
    // Save current section first
    saveCurrentSection();

    // Determine default label based on existing sections
    int verseCount = 0;
    for (const SongSection& s : m_sections) {
        if (s.type == "verse") verseCount++;
    }

    SongSection newSection;
    newSection.type = "verse";
    newSection.label = QString("Verse %1").arg(verseCount + 1);
    newSection.text = "";

    m_sections.append(newSection);
    updateSectionList();

    // Select the new section
    m_sectionList->setCurrentRow(m_sections.count() - 1);
}

void SongEditorDialog::onRemoveSection()
{
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= m_sections.count()) {
        return;
    }

    m_sections.removeAt(m_currentSectionIndex);
    m_currentSectionIndex = -1;

    updateSectionList();

    // Select previous or first section
    if (!m_sections.isEmpty()) {
        int newIndex = qMin(m_currentSectionIndex, m_sections.count() - 1);
        if (newIndex < 0) newIndex = 0;
        m_sectionList->setCurrentRow(newIndex);
    } else {
        onSectionSelectionChanged();  // Clear the editor
    }
}

void SongEditorDialog::onMoveUp()
{
    if (m_currentSectionIndex <= 0) {
        return;
    }

    saveCurrentSection();

    // Swap with previous
    m_sections.swapItemsAt(m_currentSectionIndex, m_currentSectionIndex - 1);
    updateSectionList();

    m_sectionList->setCurrentRow(m_currentSectionIndex - 1);
}

void SongEditorDialog::onMoveDown()
{
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= m_sections.count() - 1) {
        return;
    }

    saveCurrentSection();

    // Swap with next
    m_sections.swapItemsAt(m_currentSectionIndex, m_currentSectionIndex + 1);
    updateSectionList();

    m_sectionList->setCurrentRow(m_currentSectionIndex + 1);
}

} // namespace Clarity
