#pragma once

#include "Slide.h"
#include <QPixmap>
#include <QSize>
#include <QColor>

namespace Clarity {

/**
 * @brief Centralized utility for rendering slide previews to QPixmap
 *
 * Used by SlideGridDelegate and LivePreviewWidget to render consistent
 * slide thumbnails with proper background handling (solid, gradient, image).
 */
class SlidePreviewRenderer {
public:
    /**
     * @brief Options for customizing preview rendering
     */
    struct RenderOptions {
        bool showSlideNumber;              ///< Show slide number in corner
        int slideNumber;                   ///< Slide number to display (1-based)
        bool highlighted;                  ///< Draw highlight border
        QColor highlightColor;             ///< Highlight border color
        int highlightBorderWidth;          ///< Highlight border width in pixels
        bool isCurrentSlide;               ///< Special indicator for "live" slide
        QColor currentSlideColor;          ///< "Live" indicator color (green)

        RenderOptions()
            : showSlideNumber(false)
            , slideNumber(0)
            , highlighted(false)
            , highlightColor("#3b82f6")
            , highlightBorderWidth(3)
            , isCurrentSlide(false)
            , currentSlideColor("#22c55e")
        {}
    };

    /**
     * @brief Render a slide to a pixmap
     * @param slide The slide to render
     * @param size Target size for the preview
     * @param options Rendering options
     * @return Rendered pixmap
     */
    static QPixmap render(const Slide& slide, const QSize& size,
                          const RenderOptions& options = RenderOptions());

private:
    /**
     * @brief Draw solid color background
     */
    static void drawBackground(QPainter& painter, const Slide& slide, const QRect& rect);

    /**
     * @brief Draw gradient background
     */
    static void drawGradient(QPainter& painter, const Slide& slide, const QRect& rect);

    /**
     * @brief Draw image background
     */
    static void drawImage(QPainter& painter, const Slide& slide, const QRect& rect);

    /**
     * @brief Draw slide text with scaled font
     */
    static void drawText(QPainter& painter, const Slide& slide, const QRect& rect, int scaledFontSize);

    /**
     * @brief Draw slide number indicator
     */
    static void drawSlideNumber(QPainter& painter, int slideNumber, const QRect& rect);

    /**
     * @brief Draw highlight border around slide
     */
    static void drawHighlight(QPainter& painter, const QRect& rect,
                              const QColor& color, int borderWidth);
};

} // namespace Clarity
