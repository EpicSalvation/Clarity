// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "TourOverlay.h"

#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

namespace Clarity {

// ------------------------------------------------------------------
// Layout constants
// ------------------------------------------------------------------
static constexpr int SPOTLIGHT_MARGIN = 10;   // Extra px around the target widget
static constexpr int CALLOUT_WIDTH    = 340;
static constexpr int CALLOUT_GAP      = 16;   // Gap between spotlight edge and callout

// ------------------------------------------------------------------
TourOverlay::TourOverlay(QWidget* parentWindow, const QList<Step>& steps)
    : QWidget(parentWindow)
    , m_steps(steps)
{
    Q_ASSERT(parentWindow);
    Q_ASSERT(!steps.isEmpty());

    // Cover the entire parent window
    setGeometry(parentWindow->rect());
    setAttribute(Qt::WA_NoSystemBackground);

    // Intercept parent resize events so we stay full-size
    parentWindow->installEventFilter(this);

    // ------------------------------------------------------------------
    // Build the callout popup
    // ------------------------------------------------------------------
    m_callout = new QFrame(this);
    m_callout->setFixedWidth(CALLOUT_WIDTH);
    m_callout->setObjectName("tourCallout");
    m_callout->setStyleSheet(
        "QFrame#tourCallout {"
        "  background: palette(window);"
        "  border: 1px solid palette(mid);"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
    );

    QVBoxLayout* calloutLayout = new QVBoxLayout(m_callout);
    calloutLayout->setContentsMargins(16, 14, 16, 14);
    calloutLayout->setSpacing(8);

    m_titleLabel = new QLabel(m_callout);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 13pt; }");

    m_descLabel = new QLabel(m_callout);
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("QLabel { font-size: 10pt; }");

    m_stepCountLabel = new QLabel(m_callout);
    m_stepCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_stepCountLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");

    // Navigation row
    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setSpacing(6);

    m_skipButton = new QPushButton(tr("Skip Tour"), m_callout);
    m_backButton = new QPushButton(tr("Back"), m_callout);
    m_nextButton = new QPushButton(tr("Next"), m_callout);
    m_nextButton->setDefault(true);

    // Style the Next button to stand out
    m_nextButton->setStyleSheet(
        "QPushButton {"
        "  background: #3b82f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 5px 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:pressed { background: #1d4ed8; }"
    );

    navLayout->addWidget(m_skipButton);
    navLayout->addStretch();
    navLayout->addWidget(m_backButton);
    navLayout->addWidget(m_nextButton);

    calloutLayout->addWidget(m_titleLabel);
    calloutLayout->addWidget(m_descLabel);
    calloutLayout->addWidget(m_stepCountLabel);
    calloutLayout->addLayout(navLayout);

    connect(m_backButton, &QPushButton::clicked, this, &TourOverlay::onBack);
    connect(m_nextButton, &QPushButton::clicked, this, &TourOverlay::onNext);
    connect(m_skipButton, &QPushButton::clicked, this, &TourOverlay::onSkip);

    hide();
}

// ------------------------------------------------------------------
void TourOverlay::start()
{
    m_currentStep = 0;
    show();
    raise();
    showStep(0);
}

// ------------------------------------------------------------------
void TourOverlay::showStep(int index)
{
    if (index < 0 || index >= m_steps.size())
        return;

    m_currentStep = index;
    const Step& step = m_steps[index];

    // Run any pre-show action (e.g. switching a settings tab)
    if (step.beforeShow)
        step.beforeShow();

    // Update callout text
    m_titleLabel->setText(step.title);
    m_descLabel->setText(step.description);
    m_stepCountLabel->setText(tr("Step %1 of %2").arg(index + 1).arg(m_steps.size()));

    // Update button labels
    m_backButton->setVisible(index > 0);
    bool isLast = (index == m_steps.size() - 1);
    m_nextButton->setText(isLast ? tr("Finish") : tr("Next"));
    m_skipButton->setVisible(!isLast);

    // Repaint overlay with new spotlight
    update();

    // Position callout near the spotlight
    m_callout->adjustSize();
    QRect spot = spotlightRect();
    positionCallout(spot);
    m_callout->show();
    m_callout->raise();
}

// ------------------------------------------------------------------
void TourOverlay::onNext()
{
    if (m_currentStep < m_steps.size() - 1) {
        showStep(m_currentStep + 1);
    } else {
        hide();
        emit completed();
        deleteLater();
    }
}

void TourOverlay::onBack()
{
    if (m_currentStep > 0)
        showStep(m_currentStep - 1);
}

void TourOverlay::onSkip()
{
    hide();
    emit skipped();
    deleteLater();
}

// ------------------------------------------------------------------
void TourOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect spot = spotlightRect();

    if (!spot.isNull()) {
        // Draw 4 darkened regions around the spotlight, leaving it clear
        const QColor ov = QColor(0, 0, 0, 170);
        // Top
        p.fillRect(0, 0, width(), spot.top(), ov);
        // Bottom
        p.fillRect(0, spot.bottom() + 1, width(), height() - spot.bottom() - 1, ov);
        // Left (clamped to spotlight's top/bottom)
        p.fillRect(0, spot.top(), spot.left(), spot.height(), ov);
        // Right
        p.fillRect(spot.right() + 1, spot.top(), width() - spot.right() - 1, spot.height(), ov);

        // Draw glow border around the spotlight
        QPen pen(QColor(59, 130, 246, 200), 2);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(spot, 5, 5);
    } else {
        // No target — dim the entire window (intro/outro steps)
        p.fillRect(rect(), QColor(0, 0, 0, 170));
    }
}

// ------------------------------------------------------------------
bool TourOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        if (isVisible()) {
            update();
            QRect spot = spotlightRect();
            positionCallout(spot);
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ------------------------------------------------------------------
QRect TourOverlay::spotlightRect() const
{
    if (m_currentStep >= m_steps.size())
        return {};

    QWidget* target = m_steps[m_currentStep].target;
    if (!target || !target->isVisible())
        return {};

    // Map target's global rect into overlay-local coordinates
    QPoint topLeft = target->mapToGlobal(QPoint(0, 0));
    topLeft = mapFromGlobal(topLeft);
    QRect r(topLeft, target->size());
    return r.adjusted(-SPOTLIGHT_MARGIN, -SPOTLIGHT_MARGIN,
                       SPOTLIGHT_MARGIN,  SPOTLIGHT_MARGIN);
}

// ------------------------------------------------------------------
void TourOverlay::positionCallout(const QRect& spotRect)
{
    const int calloutH = m_callout->sizeHint().height();
    const int calloutW = m_callout->width();

    int x, y;

    if (spotRect.isNull()) {
        // Centre in the overlay
        x = (width()  - calloutW) / 2;
        y = (height() - calloutH) / 2;
    } else {
        // Horizontally: centre on spotlight, clamped to stay in bounds
        x = spotRect.left() + (spotRect.width() - calloutW) / 2;
        x = qBound(8, x, width() - calloutW - 8);

        // Vertically: prefer below, fallback to above
        int belowY = spotRect.bottom() + CALLOUT_GAP;
        int aboveY = spotRect.top() - calloutH - CALLOUT_GAP;

        if (belowY + calloutH <= height() - 8) {
            y = belowY;
        } else if (aboveY >= 8) {
            y = aboveY;
        } else {
            // Not enough room above or below — place at bottom of overlay
            y = height() - calloutH - 8;
        }
    }

    m_callout->move(x, y);
}

} // namespace Clarity
