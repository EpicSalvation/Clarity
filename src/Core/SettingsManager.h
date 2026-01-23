#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

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

    // Reset all settings to defaults
    void resetToDefaults();

signals:
    void outputScreenIndexChanged(int index);

private:
    QSettings* m_settings;

    // Default values
    static constexpr int DEFAULT_OUTPUT_SCREEN_INDEX = 0;
};

} // namespace Clarity
