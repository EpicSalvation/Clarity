// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SongSelectSearchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

namespace Clarity {

SongSelectSearchDialog::SongSelectSearchDialog(QWidget* parent)
    : QDialog(parent)
    , m_networkManager(nullptr)
{
    setupUI();
    setWindowTitle(tr("Search SongSelect"));
    resize(450, 280);
}

void SongSelectSearchDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Instructions
    QLabel* instructionsLabel = new QLabel(
        "Search for songs on CCLI SongSelect. Your search will open in your browser.\n"
        "You'll need to be logged in to SongSelect to download lyric sheets.",
        this);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
    mainLayout->addWidget(instructionsLabel);

    // Search input
    QHBoxLayout* searchLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Enter song title, artist, or lyrics...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SongSelectSearchDialog::onSearch);
    searchLayout->addWidget(m_searchEdit);

    m_searchButton = new QPushButton("Search", this);
    connect(m_searchButton, &QPushButton::clicked, this, &SongSelectSearchDialog::onSearch);
    searchLayout->addWidget(m_searchButton);

    mainLayout->addLayout(searchLayout);

    // Search options
    QGroupBox* optionsGroup = new QGroupBox("Search Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    QPushButton* googleButton = new QPushButton("Search on Google", this);
    googleButton->setToolTip("Search for songs on SongSelect via Google (recommended)");
    connect(googleButton, &QPushButton::clicked, this, [this]() {
        openSearch("https://www.google.com/search?q=site:songselect.ccli.com/Songs+%1");
    });
    optionsLayout->addWidget(googleButton);

    QPushButton* directButton = new QPushButton("Search on SongSelect", this);
    directButton->setToolTip("Open SongSelect's own search (requires login)");
    connect(directButton, &QPushButton::clicked, this, [this]() {
        openSearch("https://songselect.ccli.com/search/results?search=%1&cat=all");
    });
    optionsLayout->addWidget(directButton);

    mainLayout->addWidget(optionsGroup);

    // Status
    m_statusLabel = new QLabel("Enter a search term and click Search or choose an option", this);
    m_statusLabel->setStyleSheet("color: #666;");
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    m_searchEdit->setFocus();
}

void SongSelectSearchDialog::openSearch(const QString& urlTemplate)
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) {
        m_statusLabel->setText("Please enter a search term");
        return;
    }

    // Encode the query for URL
    QString encodedQuery = QString::fromUtf8(QUrl::toPercentEncoding(query));
    QString searchUrl = QString(urlTemplate).arg(encodedQuery);

    QDesktopServices::openUrl(QUrl(searchUrl));
    m_statusLabel->setText("Opened search in browser");
}

void SongSelectSearchDialog::onSearch()
{
    // Default: open Google search
    openSearch("https://www.google.com/search?q=site:songselect.ccli.com/Songs+%1");
}

void SongSelectSearchDialog::performSearch(const QString& query)
{
    Q_UNUSED(query)
}

void SongSelectSearchDialog::parseSearchResults(const QString& html)
{
    Q_UNUSED(html)
}

void SongSelectSearchDialog::onResultClicked(QListWidgetItem* item)
{
    Q_UNUSED(item)
}

void SongSelectSearchDialog::onResultDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item)
}

void SongSelectSearchDialog::onOpenInBrowser()
{
}

} // namespace Clarity
