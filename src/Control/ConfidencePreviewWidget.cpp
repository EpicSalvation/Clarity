#include "ConfidencePreviewWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QDateTime>

namespace Clarity {

ConfidencePreviewWidget::ConfidencePreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_settings(nullptr)
    , m_hasCurrentSlide(false)
    , m_hasNextSlide(false)
    , m_currentIndex(0)
    , m_totalSlides(0)
{
    setMinimumSize(160, 100);
}

void ConfidencePreviewWidget::setSettingsManager(SettingsManager* settings)
{
    m_settings = settings;
    update();
}

void ConfidencePreviewWidget::setSlides(const Slide& currentSlide, const Slide& nextSlide,
                                         int currentIndex, int totalSlides)
{
    m_currentSlide = currentSlide;
    m_nextSlide = nextSlide;
    m_hasCurrentSlide = true;
    m_hasNextSlide = true;
    m_currentIndex = currentIndex;
    m_totalSlides = totalSlides;
    update();
}

void ConfidencePreviewWidget::setCurrentSlide(const Slide& slide, int currentIndex, int totalSlides)
{
    m_currentSlide = slide;
    m_hasCurrentSlide = true;
    m_hasNextSlide = false;
    m_currentIndex = currentIndex;
    m_totalSlides = totalSlides;
    update();
}

void ConfidencePreviewWidget::clear()
{
    m_hasCurrentSlide = false;
    m_hasNextSlide = false;
    update();
}

QSize ConfidencePreviewWidget::sizeHint() const
{
    return QSize(180, 110);
}

QSize ConfidencePreviewWidget::minimumSizeHint() const
{
    return QSize(140, 90);
}

void ConfidencePreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Title bar
    int titleHeight = 18;
    QRect titleRect(0, 0, width(), titleHeight);
    painter.fillRect(titleRect, QColor("#1f2937"));

    QFont titleFont("Arial", 8);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(titleRect, Qt::AlignCenter, "Confidence");

    // Content area - render the confidence monitor layout
    QRect contentRect(0, titleHeight, width(), height() - titleHeight);
    renderConfidenceMonitor(painter, contentRect);

    // Draw border
    painter.setPen(QColor("#374151"));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(contentRect.adjusted(0, 0, -1, -1));
}

void ConfidencePreviewWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
}

void ConfidencePreviewWidget::renderConfidenceMonitor(QPainter& painter, const QRect& rect)
{
    // Colors matching the actual confidence monitor QML
    const QColor bgColor("#1a1a1a");
    const QColor panelColor("#2a2a2a");
    const QColor greenColor("#00ff00");
    const QColor orangeColor("#ffaa00");
    const QColor grayColor("#666666");

    // Get settings colors (or defaults)
    QColor settingsBgColor = panelColor;
    QColor settingsTextColor("#ffffff");
    if (m_settings) {
        settingsBgColor = m_settings->confidenceBackgroundColor();
        settingsTextColor = m_settings->confidenceTextColor();
    }

    // Fill background
    painter.fillRect(rect, bgColor);

    if (!m_hasCurrentSlide) {
        // "No slide displayed" state
        QFont noSlideFont("Arial", 7);
        painter.setFont(noSlideFont);
        painter.setPen(grayColor);
        painter.drawText(rect, Qt::AlignCenter, "No slide displayed");
        return;
    }

    // Calculate layout dimensions (matching QML proportions)
    int margin = 2;
    int spacing = 2;
    int timerBarHeight = 10;

    QRect contentArea = rect.adjusted(margin, margin, -margin, -margin - timerBarHeight);

    // Left side: Current slide (65% of width)
    int leftWidth = static_cast<int>(contentArea.width() * 0.65);
    int rightWidth = contentArea.width() - leftWidth - spacing;

    QRect leftRect(contentArea.left(), contentArea.top(), leftWidth, contentArea.height());
    QRect rightRect(contentArea.left() + leftWidth + spacing, contentArea.top(), rightWidth, contentArea.height());

    // Header heights (scaled)
    int leftHeaderHeight = 8;
    int rightHeaderHeight = 7;

    // Draw left panel (current slide)
    {
        // Green border
        painter.setPen(QPen(greenColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(leftRect);

        // Green header
        QRect headerRect(leftRect.left(), leftRect.top(), leftRect.width(), leftHeaderHeight);
        painter.fillRect(headerRect, greenColor);

        // Header text
        QFont headerFont("Arial", 5);
        headerFont.setBold(true);
        painter.setFont(headerFont);
        painter.setPen(Qt::black);
        QString headerText = QString("CURRENT (%1/%2)").arg(m_currentIndex + 1).arg(m_totalSlides);
        painter.drawText(headerRect, Qt::AlignCenter, headerText);

        // Content area
        QRect slideRect(leftRect.left() + 1, leftRect.top() + leftHeaderHeight,
                        leftRect.width() - 2, leftRect.height() - leftHeaderHeight - 1);
        painter.fillRect(slideRect, settingsBgColor);

        // Slide text (scaled down)
        QFont textFont("Arial", 6);
        painter.setFont(textFont);
        painter.setPen(settingsTextColor);
        QRect textRect = slideRect.adjusted(2, 2, -2, -2);
        QString displayText = m_currentSlide.text();
        if (displayText.length() > 30) {
            displayText = displayText.left(27) + "...";
        }
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, displayText);
    }

    // Draw right panel (next slide)
    {
        QColor borderColor = m_hasNextSlide ? orangeColor : grayColor;

        // Border
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rightRect);

        // Header
        QRect headerRect(rightRect.left(), rightRect.top(), rightRect.width(), rightHeaderHeight);
        painter.fillRect(headerRect, borderColor);

        // Header text
        QFont headerFont("Arial", 4);
        headerFont.setBold(true);
        painter.setFont(headerFont);
        painter.setPen(Qt::black);
        painter.drawText(headerRect, Qt::AlignCenter, m_hasNextSlide ? "NEXT" : "END");

        // Content area
        QRect slideRect(rightRect.left() + 1, rightRect.top() + rightHeaderHeight,
                        rightRect.width() - 2, rightRect.height() - rightHeaderHeight - 1);
        painter.fillRect(slideRect, settingsBgColor);

        // Slide text or end message
        QFont textFont("Arial", 5);
        painter.setFont(textFont);
        QRect textRect = slideRect.adjusted(1, 1, -1, -1);

        if (m_hasNextSlide) {
            painter.setPen(settingsTextColor);
            QString displayText = m_nextSlide.text();
            if (displayText.length() > 20) {
                displayText = displayText.left(17) + "...";
            }
            painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, displayText);
        } else {
            painter.setPen(grayColor);
            QFont italicFont("Arial", 4);
            italicFont.setItalic(true);
            painter.setFont(italicFont);
            painter.drawText(textRect, Qt::AlignCenter, "End");
        }
    }

    // Draw notes bar if notes exist
    int notesBarHeight = 0;
    if (hasNotes()) {
        notesBarHeight = 12;
        QRect notesRect(rect.left() + margin, rect.bottom() - timerBarHeight - notesBarHeight - 2,
                        rect.width() - margin * 2, notesBarHeight);
        painter.fillRect(notesRect, panelColor);

        // Notes indicator
        QFont notesFont("Arial", 4);
        painter.setFont(notesFont);
        painter.setPen(QColor("#4a90d9"));
        painter.drawText(notesRect.adjusted(2, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "NOTES");

        // Draw border
        painter.setPen(QColor("#4a90d9"));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(notesRect);
    }

    // Draw timer bar at bottom
    QRect timerRect(rect.left(), rect.bottom() - timerBarHeight, rect.width(), timerBarHeight);
    painter.fillRect(timerRect, panelColor);

    // Timer placeholder (left) and clock (right)
    QFont timerFont("Consolas", 5);
    painter.setFont(timerFont);

    // Timer (left side)
    painter.setPen(Qt::white);
    painter.drawText(timerRect.adjusted(3, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "00:00:00");

    // Clock (right side) - show actual time
    QString timeStr = QDateTime::currentDateTime().toString("h:mm AP");
    painter.drawText(timerRect.adjusted(0, 0, -3, 0), Qt::AlignRight | Qt::AlignVCenter, timeStr);
}

} // namespace Clarity
