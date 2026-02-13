#include "SlidePreviewRenderer.h"
#include "VideoThumbnailGenerator.h"
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QtMath>

namespace Clarity {

// Initialize static member
VideoThumbnailGenerator* SlidePreviewRenderer::s_thumbnailGenerator = nullptr;

void SlidePreviewRenderer::setVideoThumbnailGenerator(VideoThumbnailGenerator* generator)
{
    s_thumbnailGenerator = generator;
}

QPixmap SlidePreviewRenderer::render(const Slide& slide, const QSize& size,
                                     const RenderOptions& options)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::black);  // Default fill

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QRect rect = pixmap.rect();

    // Draw background based on type
    switch (slide.backgroundType()) {
    case Slide::Gradient:
        drawGradient(painter, slide, rect);
        break;
    case Slide::Image:
        drawImage(painter, slide, rect);
        break;
    case Slide::Video:
        drawVideo(painter, slide, rect);
        break;
    case Slide::SolidColor:
    default:
        drawBackground(painter, slide, rect);
        break;
    }

    // Calculate scaled font size for preview
    // Original design is 1080p, so scale font proportionally
    int scaledFontSize = qMax(10, slide.fontSize() * size.height() / 1080);
    drawText(painter, slide, rect, scaledFontSize, options.redLetterColor);

    // Draw slide number if requested
    if (options.showSlideNumber) {
        drawSlideNumber(painter, options.slideNumber, rect);
    }

    // Draw current slide indicator (takes precedence over highlight)
    if (options.isCurrentSlide) {
        drawHighlight(painter, rect, options.currentSlideColor, options.highlightBorderWidth + 1);
    }
    // Draw selection highlight
    else if (options.highlighted) {
        drawHighlight(painter, rect, options.highlightColor, options.highlightBorderWidth);
    }

    painter.end();
    return pixmap;
}

void SlidePreviewRenderer::drawBackground(QPainter& painter, const Slide& slide, const QRect& rect)
{
    painter.fillRect(rect, slide.backgroundColor());
}

void SlidePreviewRenderer::drawGradient(QPainter& painter, const Slide& slide, const QRect& rect)
{
    int width = rect.width();
    int height = rect.height();
    QList<GradientStop> stops = slide.gradientStops();

    if (slide.gradientType() == RadialGradient) {
        // Radial gradient
        double cx = rect.x() + slide.radialCenterX() * width;
        double cy = rect.y() + slide.radialCenterY() * height;
        double r = slide.radialRadius() * qMax(width, height);

        QRadialGradient gradient(QPointF(cx, cy), r);
        for (const auto& stop : stops) {
            gradient.setColorAt(stop.position, stop.color);
        }

        painter.fillRect(rect, gradient);
    } else {
        // Linear gradient with angle
        int angle = slide.gradientAngle();
        double radians = angle * M_PI / 180.0;

        double centerX = width / 2.0;
        double centerY = height / 2.0;
        double diagonal = qSqrt(width * width + height * height) / 2.0;

        QPointF start(
            centerX + diagonal * qSin(radians),
            centerY - diagonal * qCos(radians)
        );
        QPointF end(
            centerX - diagonal * qSin(radians),
            centerY + diagonal * qCos(radians)
        );

        QLinearGradient gradient(start, end);
        for (const auto& stop : stops) {
            gradient.setColorAt(stop.position, stop.color);
        }

        painter.fillRect(rect, gradient);
    }
}

void SlidePreviewRenderer::drawImage(QPainter& painter, const Slide& slide, const QRect& rect)
{
    // First fill with background color as fallback
    painter.fillRect(rect, slide.backgroundColor());

    // Load image from embedded data (already raw bytes, not base64)
    QByteArray imageData = slide.backgroundImageData();
    if (imageData.isEmpty()) {
        return;
    }

    QImage image;
    if (image.loadFromData(imageData)) {
        // Scale image to fill the rect while maintaining aspect ratio
        QImage scaled = image.scaled(rect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        // Center the scaled image
        int x = (rect.width() - scaled.width()) / 2;
        int y = (rect.height() - scaled.height()) / 2;

        painter.drawImage(x, y, scaled);
    }
}

void SlidePreviewRenderer::drawVideo(QPainter& painter, const Slide& slide, const QRect& rect)
{
    // First fill with dark background as fallback
    painter.fillRect(rect, QColor(30, 30, 30));

    QString videoPath = slide.backgroundVideoPath();
    if (videoPath.isEmpty()) {
        return;
    }

    // Use thumbnail generator if available
    if (s_thumbnailGenerator) {
        QPixmap thumbnail = s_thumbnailGenerator->getThumbnail(videoPath, rect.size());
        if (!thumbnail.isNull()) {
            // Center the thumbnail
            int x = rect.x() + (rect.width() - thumbnail.width()) / 2;
            int y = rect.y() + (rect.height() - thumbnail.height()) / 2;
            painter.drawPixmap(x, y, thumbnail);
            return;
        }
    }

    // Fallback: draw placeholder with video icon
    QPixmap placeholder = VideoThumbnailGenerator::placeholderThumbnail(rect.size());
    painter.drawPixmap(rect.topLeft(), placeholder);
}

void SlidePreviewRenderer::drawText(QPainter& painter, const Slide& slide, const QRect& rect,
                                    int scaledFontSize, const QString& redLetterColor)
{
    QString text = slide.text();
    if (text.isEmpty()) {
        return;
    }

    // Use a margin for text
    int margin = qMax(4, rect.width() / 20);
    QRect textRect = rect.adjusted(margin, margin, -margin, -margin);

    // If slide has rich text (red letter markup), use QTextDocument for rendering
    if (slide.hasRichText()) {
        QTextDocument doc;
        doc.setTextWidth(textRect.width());

        // Build HTML with CSS for styling
        QString css = QString(
            "<style>"
            "body { color: %1; font-family: %2; font-size: %3px; text-align: center; }"
            ".jesus { color: %4; }"
            "</style>"
        ).arg(slide.textColor().name())
         .arg(slide.fontFamily())
         .arg(scaledFontSize)
         .arg(redLetterColor);

        // Wrap in centered paragraph
        QString html = css + "<body><p style='text-align: center;'>" + slide.richText() + "</p></body>";
        doc.setHtml(html);

        // Calculate vertical centering
        int docHeight = doc.size().height();
        int yOffset = textRect.top() + (textRect.height() - docHeight) / 2;
        yOffset = qMax(textRect.top(), yOffset);

        painter.save();
        painter.translate(textRect.left(), yOffset);
        doc.drawContents(&painter, QRectF(0, 0, textRect.width(), textRect.height()));
        painter.restore();
    } else {
        // Plain text rendering
        QFont font(slide.fontFamily());
        font.setPixelSize(scaledFontSize);
        painter.setFont(font);
        painter.setPen(slide.textColor());

        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
}

void SlidePreviewRenderer::drawSlideNumber(QPainter& painter, int slideNumber, const QRect& rect)
{
    // Draw slide number in bottom-left corner with semi-transparent background
    QString numberText = QString::number(slideNumber);

    QFont font("Arial", 10);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int padding = 4;
    int textWidth = fm.horizontalAdvance(numberText);
    int textHeight = fm.height();

    QRect bgRect(
        rect.left() + 4,
        rect.bottom() - textHeight - padding * 2 - 4,
        textWidth + padding * 2,
        textHeight + padding * 2
    );

    // Semi-transparent background
    painter.fillRect(bgRect, QColor(0, 0, 0, 128));

    // White text
    painter.setPen(Qt::white);
    painter.drawText(bgRect, Qt::AlignCenter, numberText);
}

void SlidePreviewRenderer::drawHighlight(QPainter& painter, const QRect& rect,
                                         const QColor& color, int borderWidth)
{
    QPen pen(color);
    pen.setWidth(borderWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Draw border inside the rect
    int offset = borderWidth / 2;
    painter.drawRect(rect.adjusted(offset, offset, -offset, -offset));
}

} // namespace Clarity
