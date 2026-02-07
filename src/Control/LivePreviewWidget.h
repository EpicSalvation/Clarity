#pragma once

#include "Core/Slide.h"
#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>

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
     * @brief Set whether this preview's display process is active (connected via IPC)
     */
    void setActive(bool active);

    /**
     * @brief Check if this preview's display process is active
     */
    bool isActive() const { return m_active; }

    /**
     * @brief Get the preferred size for this widget
     */
    QSize sizeHint() const override;

    /**
     * @brief Get the minimum size for this widget
     */
    QSize minimumSizeHint() const override;

signals:
    /**
     * @brief Emitted when the user double-clicks to toggle the display
     */
    void doubleClicked();

protected:
    /**
     * @brief Paint the preview
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handle resize to update cached preview
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Handle double-click to toggle display
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    /**
     * @brief Update the cached preview pixmap
     */
    void updateCachedPreview();

    QString m_title;             ///< Display title
    Slide m_currentSlide;        ///< Currently displayed slide
    bool m_hasSlide;             ///< Whether a slide is being displayed
    bool m_active;               ///< Whether the display process is connected
    QPixmap m_cachedPreview;     ///< Cached rendered preview
    bool m_cacheValid;           ///< Whether the cache needs updating
};

} // namespace Clarity
