#include "StartupWidget.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QFont>

namespace Clarity {

StartupWidget::StartupWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignCenter);

    // Centered column container
    QWidget* column = new QWidget(this);
    column->setFixedWidth(450);
    QVBoxLayout* layout = new QVBoxLayout(column);
    layout->setSpacing(24);
    layout->setContentsMargins(0, 0, 0, 0);

    // Title + subtitle group (tight spacing)
    QVBoxLayout* titleGroup = new QVBoxLayout();
    titleGroup->setSpacing(4);

    QLabel* title = new QLabel(tr("Clarity"), column);
    QFont titleFont = title->font();
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    titleGroup->addWidget(title);

    QLabel* subtitle = new QLabel(tr("Presentation Software for Churches"), column);
    subtitle->setAlignment(Qt::AlignCenter);
    QPalette subtitlePal = subtitle->palette();
    subtitlePal.setColor(QPalette::WindowText, subtitle->palette().color(QPalette::PlaceholderText));
    subtitle->setPalette(subtitlePal);
    titleGroup->addWidget(subtitle);

    layout->addLayout(titleGroup);

    // Action buttons group
    QVBoxLayout* buttonGroup = new QVBoxLayout();
    buttonGroup->setSpacing(8);

    QPushButton* newBtn = new QPushButton(tr("New Presentation"), column);
    QPushButton* openBtn = new QPushButton(tr("Open Presentation"), column);

    for (QPushButton* btn : {newBtn, openBtn}) {
        btn->setMinimumHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
    }

    connect(newBtn, &QPushButton::clicked, this, &StartupWidget::newPresentationRequested);
    connect(openBtn, &QPushButton::clicked, this, &StartupWidget::openPresentationRequested);

    buttonGroup->addWidget(newBtn);
    buttonGroup->addWidget(openBtn);

    layout->addLayout(buttonGroup);

    // Recent presentations group (label + list in a wrapper widget so they stay together)
    m_recentWidget = new QWidget(column);
    QVBoxLayout* recentLayout = new QVBoxLayout(m_recentWidget);
    recentLayout->setSpacing(4);
    recentLayout->setContentsMargins(0, 0, 0, 0);

    m_recentLabel = new QLabel(tr("Recent Presentations"), m_recentWidget);
    QFont recentFont = m_recentLabel->font();
    recentFont.setBold(true);
    m_recentLabel->setFont(recentFont);
    recentLayout->addWidget(m_recentLabel);

    m_recentList = new QListWidget(m_recentWidget);
    m_recentList->setMinimumHeight(120);
    m_recentList->setMaximumHeight(250);
    m_recentList->setCursor(Qt::PointingHandCursor);
    recentLayout->addWidget(m_recentList);

    layout->addWidget(m_recentWidget);
    layout->addStretch();

    connect(m_recentList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            emit openRecentRequested(path);
        }
    });

    // Hide recent section by default
    m_recentWidget->setVisible(false);

    outerLayout->addWidget(column);
}

void StartupWidget::setRecentFiles(const QStringList& files)
{
    m_recentList->clear();

    bool hasFiles = !files.isEmpty();
    m_recentWidget->setVisible(hasFiles);

    for (const QString& path : files) {
        QFileInfo info(path);
        QListWidgetItem* item = new QListWidgetItem(info.fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        m_recentList->addItem(item);
    }
}

} // namespace Clarity
