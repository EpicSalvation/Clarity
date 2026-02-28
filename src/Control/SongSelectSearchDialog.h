// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Clarity {

/**
 * @brief Dialog for searching SongSelect via DuckDuckGo and opening results in browser
 *
 * Searches DuckDuckGo with site:songselect.ccli.com restriction,
 * extracts song URLs, and allows users to click to open
 * in their browser (where they can download if logged in to CCLI).
 */
class SongSelectSearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit SongSelectSearchDialog(QWidget* parent = nullptr);

    /**
     * @brief Get the CCLI number if user selected a result
     * @return CCLI number string, or empty if none selected
     */
    QString selectedCcliNumber() const { return m_selectedCcliNumber; }

private slots:
    void onSearch();
    void onResultClicked(QListWidgetItem* item);
    void onResultDoubleClicked(QListWidgetItem* item);
    void onOpenInBrowser();

private:
    void setupUI();
    void openSearch(const QString& urlTemplate);
    void performSearch(const QString& query);
    void parseSearchResults(const QString& html);

    // UI components
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    QListWidget* m_resultsList;
    QLabel* m_statusLabel;
    QPushButton* m_openButton;
    QPushButton* m_closeButton;

    // Network
    QNetworkAccessManager* m_networkManager;

    // Results data
    struct SearchResult {
        QString title;
        QString ccliNumber;
        QString url;
    };
    QList<SearchResult> m_results;
    QString m_selectedCcliNumber;
};

} // namespace Clarity
