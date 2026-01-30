#pragma once

#include "Core/Song.h"
#include "Core/SongLibrary.h"
#include "Core/Slide.h"
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>

namespace Clarity {

/**
 * @brief Dialog for browsing and managing the song library
 *
 * Provides UI for:
 * - Searching songs by title, author, or lyrics
 * - Browsing the complete song library
 * - Viewing song details and preview
 * - Importing songs from files
 * - Creating and editing songs
 * - Inserting songs as slides
 */
class SongLibraryDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct song library dialog
     * @param library Pointer to SongLibrary (must remain valid during dialog lifetime)
     * @param parent Parent widget
     */
    explicit SongLibraryDialog(SongLibrary* library, QWidget* parent = nullptr);

    /**
     * @brief Get the slides created from selected song
     * @return List of slides (empty if cancelled or no selection)
     */
    QList<Slide> getSlides() const;

    /**
     * @brief Get the ID of the selected song
     * @return Song ID, or 0 if no selection
     */
    int selectedSongId() const;

    /**
     * @brief Get whether to include section labels
     */
    bool includeSectionLabels() const;

    /**
     * @brief Get the maximum lines per slide setting
     */
    int maxLinesPerSlide() const;

    /**
     * @brief Get the slide style settings
     */
    SlideStyle slideStyle() const;

    /**
     * @brief Set default slide style for generated slides
     */
    void setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                         const QString& fontFamily, int fontSize);

private slots:
    void onSearch();
    void onSongSelectionChanged();
    void onSongDoubleClicked(QListWidgetItem* item);
    void onImport();
    void onNewSong();
    void onEditSong();
    void onDeleteSong();
    void onIncludeLabelChanged(Qt::CheckState state);
    void refreshSongList();

private:
    void setupUI();
    void populateSongList(const QList<Song>& songs);
    void showSongDetails(const Song& song);
    QList<Slide> createSlidesFromSong(const Song& song) const;

    // Library reference
    SongLibrary* m_library;

    // Search controls
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;

    // Song list
    QListWidget* m_songList;
    QLabel* m_songCountLabel;

    // Song details
    QLabel* m_titleLabel;
    QLabel* m_authorLabel;
    QLabel* m_copyrightLabel;
    QLabel* m_ccliLabel;
    QTextEdit* m_lyricsPreview;

    // Library management buttons
    QPushButton* m_importButton;
    QPushButton* m_newButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;

    // Insert options
    QCheckBox* m_includeLabelCheck;
    QSpinBox* m_fontSizeSpinBox;
    QSpinBox* m_maxLinesSpinBox;

    // Dialog buttons
    QPushButton* m_insertButton;
    QPushButton* m_cancelButton;

    // Selected song
    int m_selectedSongId;

    // Style settings for generated slides
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;
};

} // namespace Clarity
