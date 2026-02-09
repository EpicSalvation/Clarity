#pragma once

#include <QObject>
#include <QTimer>

namespace Clarity {

/**
 * @brief Timer engine for automatic slide advancement
 *
 * Manages a countdown timer that fires when it's time to advance to the next slide.
 * Features:
 * - Starts countdown when a timed slide is displayed
 * - Supports pause/resume
 * - Resets when manually navigating away from a timed slide
 * - Emits secondsRemaining updates for visual countdown display
 * - Globally pausable (user can pause all auto-advance during presentation)
 */
class AutoAdvanceTimer : public QObject {
    Q_OBJECT

public:
    explicit AutoAdvanceTimer(QObject* parent = nullptr);

    /**
     * @brief Start the auto-advance countdown for a slide
     * @param durationSeconds Duration in seconds until auto-advance (0 = stop timer)
     *
     * If durationSeconds is 0, stops any active countdown.
     * Replaces any existing active countdown.
     */
    void startCountdown(int durationSeconds);

    /**
     * @brief Stop the countdown timer without triggering advance
     *
     * Called when user manually navigates away from a timed slide.
     */
    void stop();

    /**
     * @brief Pause the countdown (preserves remaining time)
     */
    void pause();

    /**
     * @brief Resume a paused countdown
     */
    void resume();

    /**
     * @brief Toggle pause/resume state
     */
    void togglePause();

    /**
     * @brief Check if the timer is actively counting down
     */
    bool isRunning() const { return m_tickTimer.isActive() && !m_paused; }

    /**
     * @brief Check if the timer is paused
     */
    bool isPaused() const { return m_paused; }

    /**
     * @brief Check if an auto-advance countdown is active (running or paused)
     */
    bool isActive() const { return m_totalDuration > 0 && m_remainingMs > 0; }

    /**
     * @brief Get the remaining time in seconds (rounded up)
     */
    int remainingSeconds() const;

    /**
     * @brief Get the total duration of the current countdown in seconds
     */
    int totalDuration() const { return m_totalDuration; }

    /**
     * @brief Get the remaining time as a fraction (0.0 to 1.0) of total duration
     */
    double progress() const;

    /**
     * @brief Set whether auto-advance is globally enabled
     *
     * When disabled, startCountdown() calls are ignored. If a countdown is
     * active when disabled, it is stopped.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if auto-advance is globally enabled
     */
    bool isEnabled() const { return m_enabled; }

signals:
    /**
     * @brief Emitted when the countdown expires (time to advance to next slide)
     */
    void expired();

    /**
     * @brief Emitted every second with the remaining seconds
     * @param seconds Remaining seconds (rounded up)
     */
    void tick(int seconds);

    /**
     * @brief Emitted when the timer state changes (started, stopped, paused, resumed)
     */
    void stateChanged();

private slots:
    void onTick();

private:
    QTimer m_tickTimer;          ///< 1-second interval tick timer
    int m_totalDuration;         ///< Total countdown duration in seconds
    int m_remainingMs;           ///< Remaining time in milliseconds
    bool m_paused;               ///< Whether countdown is paused
    bool m_enabled;              ///< Whether auto-advance is globally enabled
};

} // namespace Clarity
