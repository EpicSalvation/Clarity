#include "LivePreviewPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace Clarity {

LivePreviewPanel::LivePreviewPanel(QWidget* parent)
    : QWidget(parent)
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

    // Compute a group background color midway between window and button palette colors
    QColor winColor = palette().color(QPalette::Window);
    QColor btnColor = palette().color(QPalette::Button);
    QColor groupBg((winColor.red() + btnColor.red()) / 2,
                   (winColor.green() + btnColor.green()) / 2,
                   (winColor.blue() + btnColor.blue()) / 2);
    QString groupStyle = QString("QFrame { background-color: %1; border: 1px solid palette(mid); border-radius: 4px; padding: 2px; }").arg(groupBg.name());

    // --- Output group: preview + blackout/whiteout buttons ---
    QFrame* outputGroup = new QFrame(this);
    outputGroup->setFrameShape(QFrame::StyledPanel);
    outputGroup->setStyleSheet(groupStyle);
    QVBoxLayout* outputGroupLayout = new QVBoxLayout(outputGroup);
    outputGroupLayout->setContentsMargins(4, 4, 4, 4);
    outputGroupLayout->setSpacing(4);

    m_outputPreview = new LivePreviewWidget(tr("Output"), this);
    outputGroupLayout->addWidget(m_outputPreview);

    // Blackout / Whiteout buttons
    QHBoxLayout* screenButtonLayout = new QHBoxLayout();
    screenButtonLayout->setContentsMargins(0, 0, 0, 0);
    screenButtonLayout->setSpacing(4);

    m_blackoutButton = new QPushButton(tr("Blackout"), this);
    m_blackoutButton->setCheckable(true);
    m_blackoutButton->setToolTip(tr("Toggle black screen (B)"));
    m_blackoutButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #1a1a1a; color: #ffffff; border: 1px solid #555; }"
    );
    connect(m_blackoutButton, &QPushButton::clicked, this, &LivePreviewPanel::blackoutClicked);
    screenButtonLayout->addWidget(m_blackoutButton);

    m_whiteoutButton = new QPushButton(tr("Whiteout"), this);
    m_whiteoutButton->setCheckable(true);
    m_whiteoutButton->setToolTip(tr("Toggle white screen (W)"));
    m_whiteoutButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #ffffff; color: #000000; border: 2px solid #333; }"
    );
    connect(m_whiteoutButton, &QPushButton::clicked, this, &LivePreviewPanel::whiteoutClicked);
    screenButtonLayout->addWidget(m_whiteoutButton);

    outputGroupLayout->addLayout(screenButtonLayout);
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

    m_timerPlayButton = new QPushButton(QStringLiteral("\u25B6"), this);   // ▶ play
    m_timerPauseButton = new QPushButton("||", this);                      // || pause
    m_timerResetButton = new QPushButton(QStringLiteral("\u25A0"), this);  // ■ stop/reset

    for (QPushButton* btn : {m_timerPlayButton, m_timerPauseButton, m_timerResetButton}) {
        btn->setFixedSize(36, 28);
    }
    // Play icon: larger to match playlist button icons
    QFont playFont = m_timerPlayButton->font();
    playFont.setPointSize(playFont.pointSize() + 4);
    m_timerPlayButton->setFont(playFont);
    // Pause bars: small, bold, with top padding to push down for vertical centering
    QFont pauseFont = m_timerPauseButton->font();
    pauseFont.setPointSize(pauseFont.pointSize() - 2);
    pauseFont.setBold(true);
    m_timerPauseButton->setFont(pauseFont);
    m_timerPauseButton->setStyleSheet("QPushButton { padding-top: -2px; padding-bottom: 2px; }");
    // Stop icon: keep default size
    QFont stopFont = m_timerResetButton->font();
    stopFont.setPointSize(stopFont.pointSize());
    m_timerResetButton->setFont(stopFont);
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

void LivePreviewPanel::setBlackoutActive(bool active)
{
    m_blackoutButton->setChecked(active);
}

void LivePreviewPanel::setWhiteoutActive(bool active)
{
    m_whiteoutButton->setChecked(active);
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
