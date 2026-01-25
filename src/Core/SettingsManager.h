#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QColor>

namespace Clarity {

/**
 * @brief Manages application settings with persistent storage
 *
 * Uses QSettings to store and retrieve user preferences.
 * Settings are automatically persisted to the platform-appropriate location.
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager();

    // Screen settings
    int outputScreenIndex() const;
    void setOutputScreenIndex(int index);

    int confidenceScreenIndex() const;
    void setConfidenceScreenIndex(int index);

    // Confidence monitor display settings
    QString confidenceFontFamily() const;
    void setConfidenceFontFamily(const QString& fontFamily);

    int confidenceFontSize() const;
    void setConfidenceFontSize(int size);

    QColor confidenceTextColor() const;
    void setConfidenceTextColor(const QColor& color);

    QColor confidenceBackgroundColor() const;
    void setConfidenceBackgroundColor(const QColor& color);

    // Transition settings
    QString transitionType() const;  // "cut", "fade", "slideLeft", "slideRight", "slideUp", "slideDown"
    void setTransitionType(const QString& type);

    int transitionDuration() const;  // milliseconds
    void setTransitionDuration(int ms);

    // Reset all settings to defaults
    void resetToDefaults();

signals:
    void outputScreenIndexChanged(int index);
    void confidenceScreenIndexChanged(int index);
    void confidenceDisplaySettingsChanged();
    void transitionSettingsChanged();

private:
    QSettings* m_settings;

    // Default values
    static constexpr int DEFAULT_OUTPUT_SCREEN_INDEX = 0;
    static constexpr int DEFAULT_CONFIDENCE_SCREEN_INDEX = 0;
    static constexpr int DEFAULT_CONFIDENCE_FONT_SIZE = 32;
    static constexpr int DEFAULT_TRANSITION_DURATION = 500;
};

} // namespace Clarity
