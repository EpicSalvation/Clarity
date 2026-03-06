// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "LivePreviewPanel.h"
#include "AppStyle.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace Clarity {

LivePreviewPanel::LivePreviewPanel(QWidget* parent)
    : QWidget(parent)
    , m_outputGroup(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(10);

    // Panel title
    QLabel* titleLabel = new QLabel(tr("Live Preview"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    // Group frame style — uses palette() references so it adapts to dark/light theme
    const QString groupStyle =
        "QFrame { background-color: palette(alternate-base);"
        " border: 1px solid palette(mid);"
        " border-radius: 4px;"
        " padding: 2px; }";

    // --- Output group: preview + blackout/whiteout buttons ---
    m_outputGroup = new QFrame(this);
    QFrame* outputGroup = m_outputGroup;
    outputGroup->setFrameShape(QFrame::StyledPanel);
    outputGroup->setStyleSheet(groupStyle);
    QVBoxLayout* outputGroupLayout = new QVBoxLayout(outputGroup);
    outputGroupLayout->setContentsMargins(4, 4, 4, 4);
    outputGroupLayout->setSpacing(4);

    m_outputPreview = new LivePreviewWidget(tr("Output"), this);
    outputGroupLayout->addWidget(m_outputPreview);

    // Blackout / Whiteout buttons (row 1)
    QHBoxLayout* screenButtonRow1 = new QHBoxLayout();
    screenButtonRow1->setContentsMargins(0, 0, 0, 0);
    screenButtonRow1->setSpacing(4);

    m_blackoutButton = new QPushButton(tr("Blackout"), this);
    m_blackoutButton->setCheckable(true);
    m_blackoutButton->setToolTip(tr("Toggle black screen (B)"));
    m_blackoutButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #1a1a1a; color: #ffffff; border: 1px solid #555; }"
    );
    connect(m_blackoutButton, &QPushButton::clicked, this, &LivePreviewPanel::blackoutClicked);
    screenButtonRow1->addWidget(m_blackoutButton);

    m_whiteoutButton = new QPushButton(tr("Whiteout"), this);
    m_whiteoutButton->setCheckable(true);
    m_whiteoutButton->setToolTip(tr("Toggle white screen (W)"));
    m_whiteoutButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #ffffff; color: #000000; border: 2px solid #333; }"
    );
    connect(m_whiteoutButton, &QPushButton::clicked, this, &LivePreviewPanel::whiteoutClicked);
    screenButtonRow1->addWidget(m_whiteoutButton);

    outputGroupLayout->addLayout(screenButtonRow1);

    // Clear Text / Clear BG buttons (row 2)
    QHBoxLayout* screenButtonRow2 = new QHBoxLayout();
    screenButtonRow2->setContentsMargins(0, 0, 0, 0);
    screenButtonRow2->setSpacing(4);

    m_clearTextButton = new QPushButton(tr("Clear Text"), this);
    m_clearTextButton->setCheckable(true);
    m_clearTextButton->setToolTip(tr("Clear text, keep background (T)"));
    m_clearTextButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #2563eb; color: #ffffff; border: 1px solid #1d4ed8; }"
    );
    connect(m_clearTextButton, &QPushButton::clicked, this, &LivePreviewPanel::clearTextClicked);
    screenButtonRow2->addWidget(m_clearTextButton);

    m_clearBgButton = new QPushButton(tr("Clear BG"), this);
    m_clearBgButton->setCheckable(true);
    m_clearBgButton->setToolTip(tr("Clear background, keep text (R)"));
    m_clearBgButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #2563eb; color: #ffffff; border: 1px solid #1d4ed8; }"
    );
    connect(m_clearBgButton, &QPushButton::clicked, this, &LivePreviewPanel::clearBgClicked);
    screenButtonRow2->addWidget(m_clearBgButton);

    outputGroupLayout->addLayout(screenButtonRow2);
    layout->addWidget(outputGroup);

    // --- Confidence group: preview + timer buttons ---
    QFrame* confidenceGroup = new QFrame(this);
    confidenceGroup->setFrameShape(QFrame::StyledPanel);
    confidenceGroup->setStyleSheet(groupStyle);
    QVBoxLayout* confidenceGroupLayout = new QVBoxLayout(confidenceGroup);
    confidenceGroupLayout->setContentsMargins(4, 4, 4, 4);
    confidenceGroupLayout->setSpacing(4);

    m_confidencePreview = new ConfidencePreviewWidget(this);
    confidenceGroupLayout->addWidget(m_confidencePreview);

    // Timer control buttons (play, pause, reset icons)
    QHBoxLayout* timerButtonLayout = new QHBoxLayout();
    timerButtonLayout->setContentsMargins(0, 0, 0, 0);
    timerButtonLayout->setSpacing(4);

    m_timerPlayButton  = new QPushButton(this);
    m_timerPauseButton = new QPushButton(this);
    m_timerResetButton = new QPushButton(this);

    const QSize timerIconSz(14, 14);
    m_timerPlayButton->setIcon(AppStyle::themedIcon(":/icons/play.svg"));
    m_timerPlayButton->setIconSize(timerIconSz);
    m_timerPauseButton->setIcon(AppStyle::themedIcon(":/icons/pause.svg"));
    m_timerPauseButton->setIconSize(timerIconSz);
    m_timerResetButton->setIcon(AppStyle::themedIcon(":/icons/stop.svg"));
    m_timerResetButton->setIconSize(timerIconSz);

    for (QPushButton* btn : {m_timerPlayButton, m_timerPauseButton, m_timerResetButton}) {
        btn->setFixedSize(36, 28);
    }
    m_timerPlayButton->setToolTip(tr("Start timer"));
    m_timerPauseButton->setToolTip(tr("Pause timer"));
    m_timerResetButton->setToolTip(tr("Reset timer"));

    connect(m_timerPlayButton, &QPushButton::clicked, this, &LivePreviewPanel::timerStartClicked);
    connect(m_timerPauseButton, &QPushButton::clicked, this, &LivePreviewPanel::timerPauseClicked);
    connect(m_timerResetButton, &QPushButton::clicked, this, &LivePreviewPanel::timerResetClicked);

    timerButtonLayout->addStretch();
    timerButtonLayout->addWidget(m_timerPlayButton);
    timerButtonLayout->addWidget(m_timerPauseButton);
    timerButtonLayout->addWidget(m_timerResetButton);
    timerButtonLayout->addStretch();

    confidenceGroupLayout->addLayout(timerButtonLayout);
    layout->addWidget(confidenceGroup);

    // --- NDI output indicator ---
    m_ndiButton = new QPushButton(tr("NDI"), this);
    m_ndiButton->setCheckable(false);
    m_ndiButton->setToolTip(tr("Toggle NDI streaming output (N)"));
    m_ndiButton->setFixedHeight(32);
    m_ndiButton->setStyleSheet(
        "QPushButton {"
        "  padding: 4px 8px;"
        "  font-weight: bold;"
        "  border: 2px solid #dc2626;"
        "  border-radius: 4px;"
        "  background-color: transparent;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(128, 128, 128, 40);"
        "}"
    );
    connect(m_ndiButton, &QPushButton::clicked, this, &LivePreviewPanel::ndiClicked);
    layout->addWidget(m_ndiButton);

    // --- Auto-advance countdown indicator ---
    m_autoAdvanceWidget = new QWidget(this);
    QVBoxLayout* autoAdvanceLayout = new QVBoxLayout(m_autoAdvanceWidget);
    autoAdvanceLayout->setContentsMargins(4, 4, 4, 4);
    autoAdvanceLayout->setSpacing(2);

    m_autoAdvanceLabel = new QLabel(tr("Auto-advance"), m_autoAdvanceWidget);
    m_autoAdvanceLabel->setAlignment(Qt::AlignCenter);
    QFont autoFont = m_autoAdvanceLabel->font();
    autoFont.setPointSize(autoFont.pointSize() - 1);
    m_autoAdvanceLabel->setFont(autoFont);
    autoAdvanceLayout->addWidget(m_autoAdvanceLabel);

    m_autoAdvanceProgress = new QProgressBar(m_autoAdvanceWidget);
    m_autoAdvanceProgress->setRange(0, 100);
    m_autoAdvanceProgress->setValue(0);
    m_autoAdvanceProgress->setTextVisible(false);
    m_autoAdvanceProgress->setFixedHeight(8);
    m_autoAdvanceProgress->setStyleSheet(
        "QProgressBar { border: 1px solid palette(mid); border-radius: 3px; background: palette(base); }"
        "QProgressBar::chunk { background: #2563eb; border-radius: 2px; }"
    );
    autoAdvanceLayout->addWidget(m_autoAdvanceProgress);

    m_autoAdvanceWidget->setVisible(false);
    layout->addWidget(m_autoAdvanceWidget);

    // Wire double-click signals from child widgets to panel signals
    connect(m_outputPreview, &LivePreviewWidget::doubleClicked, this, &LivePreviewPanel::outputDoubleClicked);
    connect(m_confidencePreview, &ConfidencePreviewWidget::doubleClicked, this, &LivePreviewPanel::confidenceDoubleClicked);

    // Add stretch to push previews to the top
    layout->addStretch();

    // Set a fixed width for the panel
    setFixedWidth(200);
}

void LivePreviewPanel::setSettingsManager(SettingsManager* settings)
{
    m_confidencePreview->setSettingsManager(settings);
}

void LivePreviewPanel::setSlides(const Slide& currentSlide, const Slide& nextSlide,
                                  int currentIndex, int totalSlides)
{
    m_outputPreview->setSlide(currentSlide);
    m_confidencePreview->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);
}

void LivePreviewPanel::setOutputSlide(const Slide& slide)
{
    m_outputPreview->setSlide(slide);
}

void LivePreviewPanel::setConfidenceSlides(const Slide& currentSlide, const Slide& nextSlide,
                                            int currentIndex, int totalSlides)
{
    m_confidencePreview->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);
}

void LivePreviewPanel::clearOutput()
{
    m_outputPreview->clear();
}

void LivePreviewPanel::clearConfidence()
{
    m_confidencePreview->clear();
}

void LivePreviewPanel::clearAll()
{
    m_outputPreview->clear();
    m_confidencePreview->clear();
}

void LivePreviewPanel::setOutputActive(bool active)
{
    m_outputPreview->setActive(active);
}

void LivePreviewPanel::setConfidenceActive(bool active)
{
    m_confidencePreview->setActive(active);
}

QSize LivePreviewPanel::sizeHint() const
{
    return QSize(200, 450);
}

QSize LivePreviewPanel::minimumSizeHint() const
{
    return QSize(180, 380);
}

void LivePreviewPanel::setNdiActive(bool active)
{
    QString borderColor = active ? "#16a34a" : "#dc2626";  // green-600 / red-600
    m_ndiButton->setStyleSheet(
        QString(
            "QPushButton {"
            "  padding: 4px 8px;"
            "  font-weight: bold;"
            "  border: 2px solid %1;"
            "  border-radius: 4px;"
            "  background-color: transparent;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(128, 128, 128, 40);"
            "}"
        ).arg(borderColor)
    );
}

void LivePreviewPanel::setBlackoutActive(bool active)
{
    m_blackoutButton->setChecked(active);
}

void LivePreviewPanel::setWhiteoutActive(bool active)
{
    m_whiteoutButton->setChecked(active);
}

void LivePreviewPanel::setClearTextActive(bool active)
{
    m_clearTextButton->setChecked(active);
}

void LivePreviewPanel::setClearBgActive(bool active)
{
    m_clearBgButton->setChecked(active);
}

void LivePreviewPanel::setAutoAdvanceCountdown(int seconds, int total)
{
    if (seconds <= 0 || total <= 0) {
        m_autoAdvanceLabel->setText(tr("Auto-advance"));
        m_autoAdvanceProgress->setValue(0);
        return;
    }

    m_autoAdvanceLabel->setText(tr("Auto-advance: %1s").arg(seconds));

    // Progress goes from 0 (full time remaining) to 100 (expired)
    int elapsed = total - seconds;
    int percent = (total > 0) ? qBound(0, (elapsed * 100) / total, 100) : 0;
    m_autoAdvanceProgress->setValue(percent);
}

void LivePreviewPanel::setAutoAdvanceActive(bool active)
{
    m_autoAdvanceWidget->setVisible(active);
}

void LivePreviewPanel::setAutoAdvancePaused(bool paused)
{
    if (paused) {
        m_autoAdvanceProgress->setStyleSheet(
            "QProgressBar { border: 1px solid palette(mid); border-radius: 3px; background: palette(base); }"
            "QProgressBar::chunk { background: #f59e0b; border-radius: 2px; }"
        );
        // Append "(paused)" to label if not already there
        QString text = m_autoAdvanceLabel->text();
        if (!text.contains(tr("paused"))) {
            m_autoAdvanceLabel->setText(text + tr(" (paused)"));
        }
    } else {
        m_autoAdvanceProgress->setStyleSheet(
            "QProgressBar { border: 1px solid palette(mid); border-radius: 3px; background: palette(base); }"
            "QProgressBar::chunk { background: #2563eb; border-radius: 2px; }"
        );
    }
}

} // namespace Clarity
