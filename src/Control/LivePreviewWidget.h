#pragma once

#include "Core/Slide.h"
#include <QWidget>
#include <QPixmap>
#include <QString>

namespace Clarity {

/**
 * @brief Widget for displaying a live preview of a slide
 *
 * Shows a miniature version of what's displayed on the output
 * or confidence monitor with a title label.
 */
class LivePreviewWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct a live preview widget
     * @param title Display title (e.g., "Output" or "Confidence")
     * @param parent Parent widget
     */
    explicit LivePreviewWidget(const QString& title, QWidget* parent = nullptr);

    /**
     * @brief Set the slide to display
     */
    void setSlide(const Slide& slide);

    /**
     * @brief Clear the preview (show black/empty)
     */
    void clear();

    /**
     * @brief Check if a slide is currently being displayed
     */
    bool hasSlide() const { return m_hasSlide; }

    /**
     * @brief Get the current slide
     */
    Slide currentSlide() const { return m_currentSlide; }

    /**
     * @brief Get the preferred size for this widget
     */
    QSize sizeHint() const override;

    /**
     * @brief Get the minimum size for this widget
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Paint the preview
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handle resize to update cached preview
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    /**
     * @brief Update the cached preview pixmap
     */
    void updateCachedPreview();

    QString m_title;             ///< Display title
    Slide m_currentSlide;        ///< Currently displayed slide
    bool m_hasSlide;             ///< Whether a slide is being displayed
    QPixmap m_cachedPreview;     ///< Cached rendered preview
    bool m_cacheValid;           ///< Whether the cache needs updating
};

} // namespace Clarity
