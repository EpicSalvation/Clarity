#include "CCLIReportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

namespace Clarity {

CCLIReportDialog::CCLIReportDialog(SongLibrary* library, QWidget* parent)
    : QDialog(parent)
    , m_library(library)
{
    setupUI();
    setWindowTitle(tr("CCLI Usage Report"));
    resize(700, 500);

    // Generate initial report
    generateReport();
}

void CCLIReportDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Date range section
    QGroupBox* dateGroup = new QGroupBox("Date Range", this);
    QHBoxLayout* dateLayout = new QHBoxLayout(dateGroup);

    dateLayout->addWidget(new QLabel("Preset:"));
    m_dateRangePreset = new QComboBox(this);
    m_dateRangePreset->addItem("This Month", "month");
    m_dateRangePreset->addItem("This Quarter", "quarter");
    m_dateRangePreset->addItem("This Year", "year");
    m_dateRangePreset->addItem("Last 30 Days", "30days");
    m_dateRangePreset->addItem("Last 90 Days", "90days");
    m_dateRangePreset->addItem("Custom", "custom");
    connect(m_dateRangePreset, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CCLIReportDialog::onDateRangePresetChanged);
    dateLayout->addWidget(m_dateRangePreset);

    dateLayout->addSpacing(20);

    dateLayout->addWidget(new QLabel("From:"));
    m_fromDate = new QDateEdit(this);
    m_fromDate->setCalendarPopup(true);
    m_fromDate->setDate(QDate::currentDate().addMonths(-1));
    connect(m_fromDate, &QDateEdit::dateChanged, this, &CCLIReportDialog::onDateRangeChanged);
    dateLayout->addWidget(m_fromDate);

    dateLayout->addWidget(new QLabel("To:"));
    m_toDate = new QDateEdit(this);
    m_toDate->setCalendarPopup(true);
    m_toDate->setDate(QDate::currentDate());
    connect(m_toDate, &QDateEdit::dateChanged, this, &CCLIReportDialog::onDateRangeChanged);
    dateLayout->addWidget(m_toDate);

    dateLayout->addStretch();

    mainLayout->addWidget(dateGroup);

    // Report table
    m_reportTable = new QTableWidget(this);
    m_reportTable->setColumnCount(5);
    m_reportTable->setHorizontalHeaderLabels({"Title", "CCLI #", "Author", "Uses", "Dates"});
    m_reportTable->horizontalHeader()->setStretchLastSection(true);
    m_reportTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_reportTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reportTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reportTable->setSortingEnabled(true);
    mainLayout->addWidget(m_reportTable, 1);

    // Summary
    m_summaryLabel = new QLabel(this);
    mainLayout->addWidget(m_summaryLabel);

    // Export buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_exportCsvButton = new QPushButton("Export CSV...", this);
    connect(m_exportCsvButton, &QPushButton::clicked, this, &CCLIReportDialog::onExportCsv);
    buttonLayout->addWidget(m_exportCsvButton);

    m_exportTextButton = new QPushButton("Export Text...", this);
    connect(m_exportTextButton, &QPushButton::clicked, this, &CCLIReportDialog::onExportText);
    buttonLayout->addWidget(m_exportTextButton);

    m_copyButton = new QPushButton("Copy to Clipboard", this);
    connect(m_copyButton, &QPushButton::clicked, this, &CCLIReportDialog::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    buttonLayout->addStretch();

    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    // Set initial date range (this month)
    onDateRangePresetChanged(0);
}

void CCLIReportDialog::onDateRangePresetChanged(int index)
{
    QString preset = m_dateRangePreset->itemData(index).toString();
    QDate today = QDate::currentDate();
    QDate from, to;

    if (preset == "month") {
        from = QDate(today.year(), today.month(), 1);
        to = today;
    } else if (preset == "quarter") {
        int quarter = (today.month() - 1) / 3;
        from = QDate(today.year(), quarter * 3 + 1, 1);
        to = today;
    } else if (preset == "year") {
        from = QDate(today.year(), 1, 1);
        to = today;
    } else if (preset == "30days") {
        from = today.addDays(-30);
        to = today;
    } else if (preset == "90days") {
        from = today.addDays(-90);
        to = today;
    } else {
        // Custom - don't change dates
        return;
    }

    // Block signals while updating to avoid double-regeneration
    m_fromDate->blockSignals(true);
    m_toDate->blockSignals(true);
    m_fromDate->setDate(from);
    m_toDate->setDate(to);
    m_fromDate->blockSignals(false);
    m_toDate->blockSignals(false);

    generateReport();
}

void CCLIReportDialog::onDateRangeChanged()
{
    // Switch to custom preset when dates are manually changed
    m_dateRangePreset->blockSignals(true);
    m_dateRangePreset->setCurrentIndex(m_dateRangePreset->count() - 1);  // "Custom"
    m_dateRangePreset->blockSignals(false);

    generateReport();
}

void CCLIReportDialog::generateReport()
{
    m_reportData.clear();
    m_reportTable->setRowCount(0);

    QDate from = m_fromDate->date();
    QDate to = m_toDate->date();

    int totalUses = 0;

    for (const Song& song : m_library->allSongs()) {
        QList<SongUsage> usages = song.usageInRange(from, to);

        if (usages.isEmpty()) {
            continue;
        }

        ReportEntry entry;
        entry.title = song.title();
        entry.ccliNumber = song.ccliNumber();
        entry.author = song.author();
        entry.usageCount = usages.count();

        for (const SongUsage& usage : usages) {
            QString dateStr = usage.dateTime.date().toString("yyyy-MM-dd");
            if (!usage.eventName.isEmpty()) {
                dateStr += " (" + usage.eventName + ")";
            }
            entry.usageDates.append(dateStr);
        }

        m_reportData.append(entry);
        totalUses += entry.usageCount;
    }

    // Sort by usage count (descending)
    std::sort(m_reportData.begin(), m_reportData.end(),
              [](const ReportEntry& a, const ReportEntry& b) {
                  return a.usageCount > b.usageCount;
              });

    // Populate table
    m_reportTable->setRowCount(m_reportData.count());
    for (int i = 0; i < m_reportData.count(); ++i) {
        const ReportEntry& entry = m_reportData[i];

        m_reportTable->setItem(i, 0, new QTableWidgetItem(entry.title));
        m_reportTable->setItem(i, 1, new QTableWidgetItem(entry.ccliNumber));
        m_reportTable->setItem(i, 2, new QTableWidgetItem(entry.author));

        QTableWidgetItem* countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, entry.usageCount);
        m_reportTable->setItem(i, 3, countItem);

        m_reportTable->setItem(i, 4, new QTableWidgetItem(entry.usageDates.join(", ")));
    }

    // Update summary
    m_summaryLabel->setText(QString("Total: %1 song(s), %2 use(s) in selected period")
                            .arg(m_reportData.count())
                            .arg(totalUses));

    // Enable/disable export buttons
    bool hasData = !m_reportData.isEmpty();
    m_exportCsvButton->setEnabled(hasData);
    m_exportTextButton->setEnabled(hasData);
    m_copyButton->setEnabled(hasData);
}

QString CCLIReportDialog::generateCsvContent() const
{
    QStringList lines;

    // Header
    lines.append("\"Title\",\"CCLI #\",\"Author\",\"Uses\",\"Dates\"");

    // Data rows
    for (const ReportEntry& entry : m_reportData) {
        // Escape double quotes in CSV by doubling them
        QString escapedTitle = entry.title;
        escapedTitle.replace("\"", "\"\"");
        QString escapedAuthor = entry.author;
        escapedAuthor.replace("\"", "\"\"");

        QString line = QString("\"%1\",\"%2\",\"%3\",%4,\"%5\"")
            .arg(escapedTitle)
            .arg(entry.ccliNumber)
            .arg(escapedAuthor)
            .arg(entry.usageCount)
            .arg(entry.usageDates.join("; "));
        lines.append(line);
    }

    return lines.join("\n");
}

QString CCLIReportDialog::generateTextContent() const
{
    QStringList lines;

    // Header
    lines.append("CCLI Usage Report");
    lines.append(QString("Period: %1 to %2")
                 .arg(m_fromDate->date().toString("yyyy-MM-dd"))
                 .arg(m_toDate->date().toString("yyyy-MM-dd")));
    lines.append(QString("Generated: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm")));
    lines.append("");
    lines.append("----------------------------------------");

    // Data rows
    for (const ReportEntry& entry : m_reportData) {
        lines.append("");
        lines.append(QString("Title: %1").arg(entry.title));
        if (!entry.ccliNumber.isEmpty()) {
            lines.append(QString("CCLI #: %1").arg(entry.ccliNumber));
        }
        if (!entry.author.isEmpty()) {
            lines.append(QString("Author: %1").arg(entry.author));
        }
        lines.append(QString("Uses: %1").arg(entry.usageCount));
        if (!entry.usageDates.isEmpty()) {
            lines.append(QString("Dates: %1").arg(entry.usageDates.join(", ")));
        }
    }

    lines.append("");
    lines.append("----------------------------------------");
    lines.append(QString("Total: %1 song(s)").arg(m_reportData.count()));

    return lines.join("\n");
}

void CCLIReportDialog::onExportCsv()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export CSV",
        QString("ccli-report-%1.csv").arg(QDate::currentDate().toString("yyyy-MM-dd")),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed",
            QString("Could not create file:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream << generateCsvContent();
    file.close();

    QMessageBox::information(this, "Export Complete",
        QString("Report exported to:\n%1").arg(filePath));
}

void CCLIReportDialog::onExportText()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Text",
        QString("ccli-report-%1.txt").arg(QDate::currentDate().toString("yyyy-MM-dd")),
        "Text Files (*.txt);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed",
            QString("Could not create file:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream << generateTextContent();
    file.close();

    QMessageBox::information(this, "Export Complete",
        QString("Report exported to:\n%1").arg(filePath));
}

void CCLIReportDialog::onCopyToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(generateTextContent());

    QMessageBox::information(this, "Copied",
        "Report copied to clipboard.");
}

} // namespace Clarity
