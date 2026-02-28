// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Song.h"
#include "Core/SongLibrary.h"
#include <QDialog>
#include <QDateEdit>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

namespace Clarity {

/**
 * @brief Dialog for generating CCLI usage reports
 *
 * Features:
 * - Date range selector (this month, this quarter, custom)
 * - Export formats: CSV, plain text
 * - Shows song title, CCLI number, usage count, dates used
 * - Suitable for manual entry into CCLI reporting system
 */
class CCLIReportDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct CCLI report dialog
     * @param library Song library to generate report from
     * @param parent Parent widget
     */
    explicit CCLIReportDialog(SongLibrary* library, QWidget* parent = nullptr);

private slots:
    void onDateRangePresetChanged(int index);
    void onDateRangeChanged();
    void onExportCsv();
    void onExportText();
    void onCopyToClipboard();

private:
    void setupUI();
    void generateReport();
    QString generateCsvContent() const;
    QString generateTextContent() const;

    SongLibrary* m_library;

    // Date range selection
    QComboBox* m_dateRangePreset;
    QDateEdit* m_fromDate;
    QDateEdit* m_toDate;

    // Report table
    QTableWidget* m_reportTable;
    QLabel* m_summaryLabel;

    // Export buttons
    QPushButton* m_exportCsvButton;
    QPushButton* m_exportTextButton;
    QPushButton* m_copyButton;
    QPushButton* m_closeButton;

    // Report data
    struct ReportEntry {
        QString title;
        QString ccliNumber;
        QString author;
        int usageCount;
        QStringList usageDates;
    };
    QList<ReportEntry> m_reportData;
};

} // namespace Clarity
