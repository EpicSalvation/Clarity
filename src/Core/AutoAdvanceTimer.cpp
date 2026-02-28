// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "AutoAdvanceTimer.h"
#include <QtMath>

namespace Clarity {

static const int TICK_INTERVAL_MS = 1000;  // 1 second tick

AutoAdvanceTimer::AutoAdvanceTimer(QObject* parent)
    : QObject(parent)
    , m_totalDuration(0)
    , m_remainingMs(0)
    , m_paused(false)
    , m_enabled(true)
{
    m_tickTimer.setInterval(TICK_INTERVAL_MS);
    connect(&m_tickTimer, &QTimer::timeout, this, &AutoAdvanceTimer::onTick);
}

void AutoAdvanceTimer::startCountdown(int durationSeconds)
{
    // Stop any existing countdown
    m_tickTimer.stop();
    m_paused = false;

    if (!m_enabled || durationSeconds <= 0) {
        m_totalDuration = 0;
        m_remainingMs = 0;
        emit stateChanged();
        return;
    }

    m_totalDuration = durationSeconds;
    m_remainingMs = durationSeconds * 1000;

    m_tickTimer.start();
    emit tick(remainingSeconds());
    emit stateChanged();
}

void AutoAdvanceTimer::stop()
{
    m_tickTimer.stop();
    m_totalDuration = 0;
    m_remainingMs = 0;
    m_paused = false;
    emit stateChanged();
}

void AutoAdvanceTimer::pause()
{
    if (!m_paused && m_tickTimer.isActive()) {
        m_tickTimer.stop();
        m_paused = true;
        emit stateChanged();
    }
}

void AutoAdvanceTimer::resume()
{
    if (m_paused && m_remainingMs > 0) {
        m_paused = false;
        m_tickTimer.start();
        emit stateChanged();
    }
}

void AutoAdvanceTimer::togglePause()
{
    if (m_paused) {
        resume();
    } else {
        pause();
    }
}

int AutoAdvanceTimer::remainingSeconds() const
{
    return qCeil(static_cast<double>(m_remainingMs) / 1000.0);
}

double AutoAdvanceTimer::progress() const
{
    if (m_totalDuration <= 0) {
        return 0.0;
    }
    double totalMs = m_totalDuration * 1000.0;
    return 1.0 - (static_cast<double>(m_remainingMs) / totalMs);
}

void AutoAdvanceTimer::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    if (!m_enabled) {
        stop();
    }
    emit stateChanged();
}

void AutoAdvanceTimer::onTick()
{
    m_remainingMs -= TICK_INTERVAL_MS;

    if (m_remainingMs <= 0) {
        m_remainingMs = 0;
        m_tickTimer.stop();
        emit tick(0);
        emit stateChanged();
        emit expired();
        return;
    }

    emit tick(remainingSeconds());
}

} // namespace Clarity
