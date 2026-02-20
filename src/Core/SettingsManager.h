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

    QString slidePreviewSize() const;  // "small", "medium", "large"
    void setSlidePreviewSize(const QString& size);

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

    // Red Letter Edition settings
    bool redLettersEnabled() const;          // Whether to show words of Jesus in red
    void setRedLettersEnabled(bool enabled);

    QString redLetterColor() const;          // Color for words of Jesus (default: #cc0000)
    void setRedLetterColor(const QString& color);

    // CCLI Usage Tracking settings
    bool usageTrackingEnabled() const;       // Whether to automatically track song usage
    void setUsageTrackingEnabled(bool enabled);

    bool promptForEventName() const;         // Whether to prompt for event name when recording usage
    void setPromptForEventName(bool prompt);

    QString defaultEventName() const;        // Default event name (e.g., "Sunday Service")
    void setDefaultEventName(const QString& name);

    // Copyright & CCLI display settings
    bool showCcliOnTitleSlides() const;        // Show CCLI song# and license# on song title slides
    void setShowCcliOnTitleSlides(bool enabled);

    QString ccliLicenseNumber() const;         // Church's CCLI license number
    void setCcliLicenseNumber(const QString& number);

    bool showCopyrightSlide() const;           // Auto-generate copyright slide at end
    void setShowCopyrightSlide(bool enabled);

    // Cascading background settings
    bool cascadingBackgrounds() const;
    void setCascadingBackgrounds(bool enabled);

    bool scriptureThemeOverride() const;
    void setScriptureThemeOverride(bool enabled);

    QString scriptureThemeOverrideName() const;
    void setScriptureThemeOverrideName(const QString& name);

    // Library settings
    bool autoSyncLibraryGroups() const;
    void setAutoSyncLibraryGroups(bool enabled);

    // Appearance / theme settings
    QString themeMode() const;       // "system", "light", "dark"
    void setThemeMode(const QString& mode);

    // ESV API settings
    /**
     * @brief Get the ESV API key (user must obtain from api.esv.org)
     */
    QString esvApiKey() const;

    /**
     * @brief Set the ESV API key
     */
    void setEsvApiKey(const QString& key);

    /**
     * @brief Check if an ESV API key is configured
     */
    bool hasEsvApiKey() const;

    /**
     * @brief Get the number of ESV verses currently cached across all presentations
     *
     * Per ESV API terms, applications may not cache more than 500 verses.
     */
    int esvCachedVerseCount() const;

    /**
     * @brief Set the ESV cached verse count
     */
    void setEsvCachedVerseCount(int count);

    // API.bible settings
    /**
     * @brief Get the API.bible API key
     */
    QString apiBibleApiKey() const;

    /**
     * @brief Set the API.bible API key
     */
    void setApiBibleApiKey(const QString& key);

    /**
     * @brief Check if an API.bible API key is configured
     */
    bool hasApiBibleApiKey() const;

    /**
     * @brief Get the last selected API.bible Bible version ID
     */
    QString apiBibleLastBibleId() const;

    /**
     * @brief Set the last selected API.bible Bible version ID
     */
    void setApiBibleLastBibleId(const QString& bibleId);

    /**
     * @brief Get the last selected API.bible language code
     */
    QString apiBibleLastLanguage() const;

    /**
     * @brief Set the last selected API.bible language code
     */
    void setApiBibleLastLanguage(const QString& languageCode);

    // Recent files
    QStringList recentFiles() const;
    void addRecentFile(const QString& path);
    void removeRecentFile(const QString& path);
    void clearRecentFiles();

    // Reset all settings to defaults
    void resetToDefaults();

signals:
    void recentFilesChanged();
    void outputScreenIndexChanged(int index);
    void confidenceScreenIndexChanged(int index);
    void confidenceDisplaySettingsChanged();
    void transitionSettingsChanged();
    void remoteControlSettingsChanged();
    void languageChanged(const QString& languageCode);
    void slideGridModeChanged(bool showAll);
    void slidePreviewSizeChanged(const QString& size);
    void esvApiKeyChanged();
    void apiBibleApiKeyChanged();
    void themeModeChanged(const QString& mode);

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
