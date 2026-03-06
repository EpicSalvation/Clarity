// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Slide.h"
#include "LivePreviewWidget.h"
#include "ConfidencePreviewWidget.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>

namespace Clarity {

/**
 * @brief Container panel for Output and Confidence monitor previews
 *
 * Displays live previews of what's being shown on the output display
 * and confidence monitor in a vertical stack.
 */
class LivePreviewPanel : public QWidget {
    Q_OBJECT

public:
    explicit LivePreviewPanel(QWidget* parent = nullptr);

    /**
     * @brief Set the settings manager for confidence monitor settings
     */
    void setSettingsManager(SettingsManager* settings);

    /**
     * @brief Set slides for both output and confidence previews
     * @param currentSlide The current slide being displayed
     * @param nextSlide The next slide (for confidence monitor)
     * @param currentIndex Current slide index (0-based)
     * @param totalSlides Total number of slides
     */
    void setSlides(const Slide& currentSlide, const Slide& nextSlide,
                   int currentIndex, int totalSlides);

    /**
     * @brief Set only the output preview slide
     */
    void setOutputSlide(const Slide& slide);

    /**
     * @brief Set the confidence preview with current and next slides
     */
    void setConfidenceSlides(const Slide& currentSlide, const Slide& nextSlide,
                             int currentIndex, int totalSlides);

    /**
     * @brief Clear the output preview
     */
    void clearOutput();

    /**
     * @brief Clear the confidence preview
     */
    void clearConfidence();

    /**
     * @brief Clear both previews
     */
    void clearAll();

    /**
     * @brief Set the output preview active state (green/red border)
     */
    void setOutputActive(bool active);

    /**
     * @brief Set the confidence preview active state (green/red border)
     */
    void setConfidenceActive(bool active);

    /**
     * @brief Returns the output group frame (preview + control buttons).
     * Used by the welcome tour to spotlight this area.
     */
    QFrame* outputGroupFrame() const { return m_outputGroup; }

    /**
     * @brief Returns the output preview widget.
     * Used by the welcome tour to spotlight the live preview area.
     */
    LivePreviewWidget* outputPreviewWidget() const { return m_outputPreview; }

    /**
     * @brief Get the preferred size for this panel
     */
    QSize sizeHint() const override;

    /**
     * @brief Get the minimum size for this panel
     */
    QSize minimumSizeHint() const override;

    /**
     * @brief Set the NDI output active state (green/red border)
     */
    void setNdiActive(bool active);

    /**
     * @brief Set the blackout button active state
     */
    void setBlackoutActive(bool active);

    /**
     * @brief Set the whiteout button active state
     */
    void setWhiteoutActive(bool active);

    /**
     * @brief Set the clear text button active state
     */
    void setClearTextActive(bool active);

    /**
     * @brief Set the clear background button active state
     */
    void setClearBgActive(bool active);

    /**
     * @brief Update the auto-advance countdown display
     * @param seconds Remaining seconds (0 = hide countdown)
     * @param total Total duration in seconds
     */
    void setAutoAdvanceCountdown(int seconds, int total);

    /**
     * @brief Set whether the auto-advance timer is active
     */
    void setAutoAdvanceActive(bool active);

    /**
     * @brief Set whether the auto-advance timer is paused
     */
    void setAutoAdvancePaused(bool paused);

signals:
    /**
     * @brief Emitted when user double-clicks the output preview to toggle display
     */
    void outputDoubleClicked();

    /**
     * @brief Emitted when user double-clicks the confidence preview to toggle display
     */
    void confidenceDoubleClicked();

    /**
     * @brief Emitted when user clicks the Blackout button
     */
    void blackoutClicked();

    /**
     * @brief Emitted when user clicks the Whiteout button
     */
    void whiteoutClicked();

    /**
     * @brief Emitted when user clicks the Clear Text button
     */
    void clearTextClicked();

    /**
     * @brief Emitted when user clicks the Clear BG button
     */
    void clearBgClicked();

    /**
     * @brief Emitted when user clicks the NDI indicator to toggle NDI output
     */
    void ndiClicked();

    /**
     * @brief Emitted when user clicks the timer Play button
     */
    void timerStartClicked();

    /**
     * @brief Emitted when user clicks the timer Pause button
     */
    void timerPauseClicked();

    /**
     * @brief Emitted when user clicks the timer Reset button
     */
    void timerResetClicked();

private:
    QFrame* m_outputGroup;                         ///< Output group frame (preview + control buttons)
    LivePreviewWidget* m_outputPreview;            ///< Output display preview
    ConfidencePreviewWidget* m_confidencePreview;  ///< Confidence monitor preview
    QPushButton* m_blackoutButton;                 ///< Blackout toggle button
    QPushButton* m_whiteoutButton;                 ///< Whiteout toggle button
    QPushButton* m_clearTextButton;                ///< Clear text toggle button
    QPushButton* m_clearBgButton;                  ///< Clear background toggle button
    QPushButton* m_timerPlayButton;                ///< Timer play button
    QPushButton* m_timerPauseButton;               ///< Timer pause button
    QPushButton* m_timerResetButton;               ///< Timer reset button

    // NDI status indicator
    QPushButton* m_ndiButton;                      ///< Clickable NDI status button

    // Auto-advance countdown indicator
    QWidget* m_autoAdvanceWidget;                  ///< Container for auto-advance UI
    QLabel* m_autoAdvanceLabel;                    ///< Countdown text label
    QProgressBar* m_autoAdvanceProgress;           ///< Countdown progress bar
};

} // namespace Clarity
