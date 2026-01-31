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

    // UI behavior settings
    bool scrollWheelChangesInputs() const;  // Whether mouse wheel changes combo boxes/spin boxes without focus
    void setScrollWheelChangesInputs(bool enabled);

    bool showAllSlidesInGrid() const;  // Whether to show all slides or just selected item's slides
    void setShowAllSlidesInGrid(bool showAll);

    // Remote control settings
    bool remoteControlEnabled() const;
    void setRemoteControlEnabled(bool enabled);

    quint16 remoteControlPort() const;
    void setRemoteControlPort(quint16 port);

    bool remoteControlPinEnabled() const;
    void setRemoteControlPinEnabled(bool enabled);

    QString remoteControlPin() const;
    void setRemoteControlPin(const QString& pin);

    // Language settings
    QString language() const;  // Language code: "en", "es", "de", "fr", or "system"
    void setLanguage(const QString& languageCode);

    // Bible settings
    QString preferredBibleTranslation() const;  // Preferred/default translation code (e.g., "KJV")
    void setPreferredBibleTranslation(const QString& translationCode);

    bool rememberLastBibleTranslation() const;  // If true, use last used; if false, use preferred
    void setRememberLastBibleTranslation(bool remember);

    QString lastBibleTranslation() const;  // Last used translation code
    void setLastBibleTranslation(const QString& translationCode);

    /**
     * @brief Get the translation to use (respects remember setting)
     * @return Either last used or preferred translation based on settings
     */
    QString effectiveBibleTranslation() const;

    bool scriptureOneVersePerSlide() const;  // Whether to create one slide per verse
    void setScriptureOneVersePerSlide(bool onePerSlide);

    // Reset all settings to defaults
    void resetToDefaults();

signals:
    void outputScreenIndexChanged(int index);
    void confidenceScreenIndexChanged(int index);
    void confidenceDisplaySettingsChanged();
    void transitionSettingsChanged();
    void remoteControlSettingsChanged();
    void languageChanged(const QString& languageCode);
    void slideGridModeChanged(bool showAll);

private:
    QSettings* m_settings;

    // Default values
    static constexpr int DEFAULT_OUTPUT_SCREEN_INDEX = 0;
    static constexpr int DEFAULT_CONFIDENCE_SCREEN_INDEX = 0;
    static constexpr int DEFAULT_CONFIDENCE_FONT_SIZE = 32;
    static constexpr int DEFAULT_TRANSITION_DURATION = 500;
    static constexpr quint16 DEFAULT_REMOTE_CONTROL_PORT = 8080;
};

} // namespace Clarity
