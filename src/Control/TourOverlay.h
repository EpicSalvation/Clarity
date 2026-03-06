// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <functional>

class QLabel;
class QPushButton;
class QFrame;

namespace Clarity {

/**
 * @brief Spotlight-style guided tour overlay widget
 *
 * Renders a semi-transparent dark overlay over the parent window with a
 * highlighted "spotlight" region around the current target widget. A floating
 * callout box shows the step title, description, and navigation buttons.
 *
 * Usage:
 * @code
 *   QList<TourOverlay::Step> steps;
 *   steps.append({someWidget, tr("Title"), tr("Description.")});
 *   auto* tour = new TourOverlay(parentWindow, steps);
 *   connect(tour, &TourOverlay::completed, ...);
 *   tour->start();
 * @endcode
 */
class TourOverlay : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief A single step in the tour
     *
     * @param target   Widget to spotlight. If nullptr the entire window is dimmed
     *                 (useful for an intro or outro step).
     * @param title    Bold heading shown in the callout.
     * @param description  Body text shown in the callout.
     * @param beforeShow   Optional lambda called right before this step is shown
     *                     (useful for switching pages/tabs so the target is visible).
     */
    struct Step {
        QWidget* target = nullptr;
        QString title;
        QString description;
        std::function<void()> beforeShow;  ///< Optional — called before the step is displayed
    };

    /**
     * @param parentWindow  The window to cover. The overlay is reparented here and
     *                      fills it completely. Must not be nullptr.
     * @param steps         Ordered list of tour steps. Must not be empty.
     */
    explicit TourOverlay(QWidget* parentWindow, const QList<Step>& steps);

    /** Start the tour at step 0. Shows the overlay and positions the callout. */
    void start();

signals:
    /** Emitted when the user reaches the last step and clicks the finish button. */
    void completed();

    /** Emitted when the user clicks Skip at any point. */
    void skipped();

protected:
    void paintEvent(QPaintEvent* event) override;

    /**
     * Event filter installed on the parent window so we can resize the overlay
     * whenever the parent is resized.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onNext();
    void onBack();
    void onSkip();

private:
    void showStep(int index);

    /**
     * @brief Compute the spotlight rectangle in overlay-local coordinates.
     *
     * The spotlight is the target widget's global rect mapped to overlay coords,
     * expanded by a small margin so the widget isn't clipped right at its edge.
     */
    QRect spotlightRect() const;

    /**
     * @brief Position the callout frame near the spotlight rectangle.
     *
     * Prefers placing the callout below the spotlight if there is sufficient
     * room; otherwise places it above. Horizontally centred on the spotlight,
     * clamped so the callout stays within the overlay bounds.
     */
    void positionCallout(const QRect& spotRect);

    QList<Step> m_steps;
    int m_currentStep = 0;

    // Callout popup — child of this overlay
    QFrame*       m_callout;
    QLabel*       m_titleLabel;
    QLabel*       m_descLabel;
    QLabel*       m_stepCountLabel;
    QPushButton*  m_backButton;
    QPushButton*  m_nextButton;
    QPushButton*  m_skipButton;
};

} // namespace Clarity
