#include "LivePreviewWidget.h"
#include "Core/SlidePreviewRenderer.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

namespace Clarity {

LivePreviewWidget::LivePreviewWidget(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_hasSlide(false)
    , m_active(false)
    , m_cacheValid(false)
{
    // Set a default minimum size maintaining 16:9 aspect ratio
    setMinimumSize(160, 110);  // 160x90 preview + 20 for title
}

void LivePreviewWidget::setSlide(const Slide& slide)
{
    m_currentSlide = slide;
    m_hasSlide = true;
    m_cacheValid = false;
    update();
}

void LivePreviewWidget::setActive(bool active)
{
    m_active = active;
    update();
}

void LivePreviewWidget::clear()
{
    m_hasSlide = false;
    m_cacheValid = false;
    update();
}

QSize LivePreviewWidget::sizeHint() const
{
    // Preferred size: 180x120 (16:9 preview + title)
    return QSize(180, 125);
}

QSize LivePreviewWidget::minimumSizeHint() const
{
    return QSize(120, 90);
}

void LivePreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    int titleHeight = 20;
    QRect titleRect(0, 0, width(), titleHeight);
    QRect previewRect(0, titleHeight, width(), height() - titleHeight);

    // Draw title background
    painter.fillRect(titleRect, QColor("#1f2937"));

    // Draw title text
    QFont titleFont("Arial", 9);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(titleRect, Qt::AlignCenter, m_title);

    // Draw preview area
    if (m_hasSlide) {
        // Update cache if needed
        if (!m_cacheValid || m_cachedPreview.size() != previewRect.size()) {
            updateCachedPreview();
        }

        if (!m_cachedPreview.isNull()) {
            painter.drawPixmap(previewRect.topLeft(), m_cachedPreview);
        }
    } else {
        // Draw black rectangle with "No Signal" text
        painter.fillRect(previewRect, Qt::black);

        QFont noSignalFont("Arial", 10);
        painter.setFont(noSignalFont);
        painter.setPen(QColor("#6b7280"));  // Gray text
        painter.drawText(previewRect, Qt::AlignCenter, tr("No Signal"));
    }

    // Draw status border around entire widget (green=active, red=inactive)
    QColor borderColor = m_active ? QColor("#22c55e") : QColor("#ef4444");
    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
}

void LivePreviewWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    emit doubleClicked();
}

void LivePreviewWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    m_cacheValid = false;
}

void LivePreviewWidget::updateCachedPreview()
{
    int titleHeight = 20;
    QSize previewSize(width(), height() - titleHeight);

    if (previewSize.width() <= 0 || previewSize.height() <= 0) {
        return;
    }

    // Render the preview using the shared renderer
    m_cachedPreview = SlidePreviewRenderer::render(m_currentSlide, previewSize);
    m_cacheValid = true;
}

} // namespace Clarity
