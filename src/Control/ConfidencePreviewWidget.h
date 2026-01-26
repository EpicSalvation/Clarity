#pragma once

#include "Core/Slide.h"
#include "Core/SettingsManager.h"
#include <QWidget>
#include <QPixmap>
#include <QString>

namespace Clarity {

/**
 * @brief Widget for displaying a miniature confidence monitor preview
 *
 * Renders an exact miniature replica of the actual confidence monitor layout:
 * - Dark background (#1a1a1a)
 * - Left side (65%): Current slide with green border and header
 * - Right side (35%): Next slide with orange/gray border and header
 * - Bottom bar with timer and clock placeholders
 */
class ConfidencePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConfidencePreviewWidget(QWidget* parent = nullptr);

    /**
     * @brief Set the settings manager for reading confidence display settings
     */
    void setSettingsManager(SettingsManager* settings);

    /**
     * @brief Set the current and next slides to display
     * @param currentSlide The currently displayed slide
     * @param nextSlide The upcoming slide (can be empty if at end)
     * @param currentIndex Current slide index (0-based)
     * @param totalSlides Total number of slides
     */
    void setSlides(const Slide& currentSlide, const Slide& nextSlide,
                   int currentIndex, int totalSlides);

    /**
     * @brief Set only the current slide (no next slide available)
     */
    void setCurrentSlide(const Slide& slide, int currentIndex, int totalSlides);

    /**
     * @brief Clear the preview (show "No slide displayed" state)
     */
    void clear();

    /**
     * @brief Get the preferred size for this widget
     */
    QSize sizeHint() const override;

    /**
     * @brief Get the minimum size for this widget
     */
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    /**
     * @brief Draw the confidence monitor layout to a pixmap
     */
    void renderConfidenceMonitor(QPainter& painter, const QRect& rect);

    /**
     * @brief Check if current slide has notes
     */
    bool hasNotes() const { return m_hasCurrentSlide && !m_currentSlide.notes().isEmpty(); }

    SettingsManager* m_settings;
    Slide m_currentSlide;
    Slide m_nextSlide;
    bool m_hasCurrentSlide;
    bool m_hasNextSlide;
    int m_currentIndex;
    int m_totalSlides;
};

} // namespace Clarity
