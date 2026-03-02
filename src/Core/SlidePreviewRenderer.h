// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Slide.h"
#include <QPixmap>
#include <QSize>
#include <QColor>

namespace Clarity {

class VideoThumbnailGenerator;

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
        QString redLetterColor;            ///< Color for red letter text (Jesus' words)

        RenderOptions()
            : showSlideNumber(false)
            , slideNumber(0)
            , highlighted(false)
            , highlightColor("#3b82f6")
            , highlightBorderWidth(3)
            , isCurrentSlide(false)
            , currentSlideColor("#22c55e")
            , redLetterColor("#cc0000")
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

    /**
     * @brief Set the global video thumbnail generator
     *
     * Must be called before rendering slides with video backgrounds.
     * The caller retains ownership of the generator.
     */
    static void setVideoThumbnailGenerator(VideoThumbnailGenerator* generator);

    // Background and legibility drawing methods (public for SlideCanvasWidget reuse)
    static void drawBackground(QPainter& painter, const Slide& slide, const QRect& rect);
    static void drawGradient(QPainter& painter, const Slide& slide, const QRect& rect);
    static void drawImage(QPainter& painter, const Slide& slide, const QRect& rect);
    static void drawVideo(QPainter& painter, const Slide& slide, const QRect& rect);
    static void drawLegibilityLayers(QPainter& painter, const Slide& slide,
                                     const QRect& rect, const QRect& textRect);
    static void drawTextZoneLegibility(QPainter& painter, const Slide& slide, const QRect& rect);

private:
    static void drawText(QPainter& painter, const Slide& slide, const QRect& rect,
                         int scaledFontSize, const QString& redLetterColor = "#cc0000");
    static void drawSlideNumber(QPainter& painter, int slideNumber, const QRect& rect);
    static void drawTextZones(QPainter& painter, const Slide& slide, const QRect& rect,
                              const QString& redLetterColor);
    static void drawHighlight(QPainter& painter, const QRect& rect,
                              const QColor& color, int borderWidth);

    static VideoThumbnailGenerator* s_thumbnailGenerator;
};

} // namespace Clarity
