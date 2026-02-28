// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Song.h"
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>

namespace Clarity {

/**
 * @brief Dialog for creating and editing songs
 *
 * Provides UI for:
 * - Song metadata (title, author, copyright, CCLI)
 * - Section-based lyrics editing
 * - Adding, removing, and reordering sections
 */
class SongEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SongEditorDialog(QWidget* parent = nullptr);

    /**
     * @brief Set the song to edit
     */
    void setSong(const Song& song);

    /**
     * @brief Get the edited song
     */
    Song song() const;

private slots:
    void onSectionSelectionChanged();
    void onSectionTextChanged();
    void onAddSection();
    void onRemoveSection();
    void onMoveUp();
    void onMoveDown();

private:
    void setupUI();
    void updateSectionList();
    void saveCurrentSection();

    // Metadata fields
    QLineEdit* m_titleEdit;
    QLineEdit* m_authorEdit;
    QLineEdit* m_copyrightEdit;
    QLineEdit* m_ccliEdit;

    // Section management
    QListWidget* m_sectionList;
    QComboBox* m_sectionTypeCombo;
    QLineEdit* m_sectionLabelEdit;
    QTextEdit* m_sectionTextEdit;

    // Section buttons
    QPushButton* m_addSectionButton;
    QPushButton* m_removeSectionButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Working data
    QList<SongSection> m_sections;
    int m_currentSectionIndex;
    Song m_originalSong;
};

} // namespace Clarity
