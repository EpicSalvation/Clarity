// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/MediaLibrary.h"
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

namespace Clarity {

class VideoThumbnailGenerator;

/**
 * @brief Dialog for browsing and selecting media from the library
 *
 * Shows a grid of thumbnails for images or a list for videos.
 * Allows selecting existing media or importing new files.
 */
class MediaLibraryDialog : public QDialog {
    Q_OBJECT

public:
    explicit MediaLibraryDialog(MediaLibrary* library,
                                MediaLibrary::MediaType type,
                                VideoThumbnailGenerator* thumbnailGen = nullptr,
                                QWidget* parent = nullptr);
    ~MediaLibraryDialog();

    /**
     * @brief Get the selected media path
     * @return Library path of selected media, or empty if cancelled
     */
    QString selectedPath() const { return m_selectedPath; }

private slots:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onImportClicked();
    void onDeleteClicked();
    void onSelectClicked();
    void onThumbnailReady(const QString& videoPath, const QImage& thumbnail);

private:
    void setupUI();
    void populateList();
    void updatePreview();

    MediaLibrary* m_library;
    MediaLibrary::MediaType m_mediaType;
    VideoThumbnailGenerator* m_thumbnailGen;
    QString m_selectedPath;

    QListWidget* m_listWidget;
    QLabel* m_previewLabel;
    QLabel* m_infoLabel;
    QPushButton* m_importButton;
    QPushButton* m_deleteButton;
    QPushButton* m_selectButton;
    QPushButton* m_cancelButton;
};

} // namespace Clarity
