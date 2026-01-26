#pragma once

#include "Core/IpcClient.h"
#include "Core/Slide.h"
#include "Core/Presentation.h"
#include <QObject>
#include <QColor>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QSettings>

namespace Clarity {

/**
 * @brief Controller for confidence monitor display
 *
 * Manages IPC connection and exposes current and next slide properties to QML.
 * The confidence monitor shows the presenter:
 * - Current slide (what the audience sees)
 * - Next slide preview (what's coming up)
 * - Timer (future enhancement)
 * - Notes (future enhancement)
 */
class ConfidenceDisplay : public QObject {
    Q_OBJECT

    // Current slide properties (what's on screen now)
    Q_PROPERTY(QString currentSlideText READ currentSlideText NOTIFY currentSlideChanged)
    Q_PROPERTY(QColor currentBackgroundColor READ currentBackgroundColor NOTIFY currentSlideChanged)
    Q_PROPERTY(QColor currentTextColor READ currentTextColor NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentFontFamily READ currentFontFamily NOTIFY currentSlideChanged)
    Q_PROPERTY(int currentFontSize READ currentFontSize NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentBackgroundType READ currentBackgroundType NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentBackgroundImageDataBase64 READ currentBackgroundImageDataBase64 NOTIFY currentSlideChanged)
    Q_PROPERTY(QColor currentGradientStartColor READ currentGradientStartColor NOTIFY currentSlideChanged)
    Q_PROPERTY(QColor currentGradientEndColor READ currentGradientEndColor NOTIFY currentSlideChanged)
    Q_PROPERTY(int currentGradientAngle READ currentGradientAngle NOTIFY currentSlideChanged)

    // Next slide properties (preview)
    Q_PROPERTY(QString nextSlideText READ nextSlideText NOTIFY nextSlideChanged)
    Q_PROPERTY(QColor nextBackgroundColor READ nextBackgroundColor NOTIFY nextSlideChanged)
    Q_PROPERTY(QColor nextTextColor READ nextTextColor NOTIFY nextSlideChanged)
    Q_PROPERTY(QString nextFontFamily READ nextFontFamily NOTIFY nextSlideChanged)
    Q_PROPERTY(int nextFontSize READ nextFontSize NOTIFY nextSlideChanged)
    Q_PROPERTY(QString nextBackgroundType READ nextBackgroundType NOTIFY nextSlideChanged)
    Q_PROPERTY(QString nextBackgroundImageDataBase64 READ nextBackgroundImageDataBase64 NOTIFY nextSlideChanged)
    Q_PROPERTY(QColor nextGradientStartColor READ nextGradientStartColor NOTIFY nextSlideChanged)
    Q_PROPERTY(QColor nextGradientEndColor READ nextGradientEndColor NOTIFY nextSlideChanged)
    Q_PROPERTY(int nextGradientAngle READ nextGradientAngle NOTIFY nextSlideChanged)

    // State properties
    Q_PROPERTY(bool hasNextSlide READ hasNextSlide NOTIFY nextSlideChanged)
    Q_PROPERTY(bool isCleared READ isCleared NOTIFY isClearedChanged)
    Q_PROPERTY(int currentSlideIndex READ currentSlideIndex NOTIFY currentSlideIndexChanged)
    Q_PROPERTY(int totalSlides READ totalSlides NOTIFY totalSlidesChanged)

    // Presenter notes (shown only on confidence monitor)
    Q_PROPERTY(QString currentNotes READ currentNotes NOTIFY currentSlideChanged)

    // Timer and clock properties
    Q_PROPERTY(QString elapsedTime READ elapsedTime NOTIFY elapsedTimeChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerRunningChanged)

    // Display settings (from settings file)
    Q_PROPERTY(QString settingsFontFamily READ settingsFontFamily NOTIFY settingsChanged)
    Q_PROPERTY(int settingsFontSize READ settingsFontSize NOTIFY settingsChanged)
    Q_PROPERTY(QColor settingsTextColor READ settingsTextColor NOTIFY settingsChanged)
    Q_PROPERTY(QColor settingsBackgroundColor READ settingsBackgroundColor NOTIFY settingsChanged)

public:
    explicit ConfidenceDisplay(QObject* parent = nullptr);

    // Current slide getters
    QString currentSlideText() const { return m_currentSlide.text(); }
    QColor currentBackgroundColor() const { return m_currentSlide.backgroundColor(); }
    QColor currentTextColor() const { return m_currentSlide.textColor(); }
    QString currentFontFamily() const { return m_currentSlide.fontFamily(); }
    int currentFontSize() const { return m_currentSlide.fontSize(); }
    QString currentBackgroundType() const;
    QString currentBackgroundImageDataBase64() const;
    QColor currentGradientStartColor() const { return m_currentSlide.gradientStartColor(); }
    QColor currentGradientEndColor() const { return m_currentSlide.gradientEndColor(); }
    int currentGradientAngle() const { return m_currentSlide.gradientAngle(); }

    // Next slide getters
    QString nextSlideText() const { return m_nextSlide.text(); }
    QColor nextBackgroundColor() const { return m_nextSlide.backgroundColor(); }
    QColor nextTextColor() const { return m_nextSlide.textColor(); }
    QString nextFontFamily() const { return m_nextSlide.fontFamily(); }
    int nextFontSize() const { return m_nextSlide.fontSize(); }
    QString nextBackgroundType() const;
    QString nextBackgroundImageDataBase64() const;
    QColor nextGradientStartColor() const { return m_nextSlide.gradientStartColor(); }
    QColor nextGradientEndColor() const { return m_nextSlide.gradientEndColor(); }
    int nextGradientAngle() const { return m_nextSlide.gradientAngle(); }

    // State getters
    bool hasNextSlide() const { return m_hasNextSlide; }
    bool isCleared() const { return m_isCleared; }
    int currentSlideIndex() const { return m_currentSlideIndex; }
    int totalSlides() const { return m_totalSlides; }

    // Presenter notes getter
    QString currentNotes() const { return m_currentSlide.notes(); }

    // Timer and clock getters
    QString elapsedTime() const;
    QString currentTime() const;
    bool timerRunning() const { return m_timerRunning; }

    // Settings getters
    QString settingsFontFamily() const;
    int settingsFontSize() const;
    QColor settingsTextColor() const;
    QColor settingsBackgroundColor() const;

    // Timer control methods (can be called from QML)
    Q_INVOKABLE void startTimer();
    Q_INVOKABLE void pauseTimer();
    Q_INVOKABLE void resetTimer();

signals:
    void currentSlideChanged();
    void nextSlideChanged();
    void isClearedChanged();
    void currentSlideIndexChanged();
    void totalSlidesChanged();
    void elapsedTimeChanged();
    void currentTimeChanged();
    void timerRunningChanged();
    void settingsChanged();

    // Signal to toggle visibility of the window
    void toggleVisibility();

private slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QJsonObject& message);
    void onTimerTick();

private:
    void clearDisplay();

    IpcClient* m_ipcClient;

    Slide m_currentSlide;
    Slide m_nextSlide;
    bool m_hasNextSlide;
    bool m_isCleared;
    int m_currentSlideIndex;
    int m_totalSlides;

    // Timer and clock
    QTimer* m_updateTimer;       // Updates the display every second
    QElapsedTimer m_elapsedTimer; // Tracks elapsed presentation time
    qint64 m_pausedElapsedMs;    // Elapsed time when paused
    bool m_timerRunning;
};

} // namespace Clarity
