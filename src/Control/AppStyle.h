#pragma once

#include <QApplication>
#include <QPalette>
#include <QString>

namespace Clarity {

/**
 * @brief Centralized application theme/style manager
 *
 * Applies a consistent visual theme (dark, light, or system default) to the
 * entire application via QPalette + QSS stylesheet. Call apply() once at
 * startup and again whenever the user changes the theme in Settings.
 *
 * The Qt Fusion style must be set (via QApplication::setStyle("Fusion"))
 * before calling apply() for palettes to render correctly on all platforms.
 */
class AppStyle {
public:
    enum class ThemeMode {
        System, ///< OS default (reset to Fusion defaults)
        Light,  ///< Modern light theme
        Dark    ///< ProPresenter-style dark theme
    };

    /**
     * @brief Apply the given theme to the application.
     * Sets QPalette, loads and installs a QSS stylesheet, and sets the
     * platform-appropriate UI font.
     */
    static void apply(QApplication* app, ThemeMode mode);

    /**
     * @brief Convert a settings string ("system"/"light"/"dark") to ThemeMode.
     */
    static ThemeMode fromString(const QString& str);

    /**
     * @brief Convert a ThemeMode to its settings string.
     */
    static QString toString(ThemeMode mode);

private:
    static QPalette darkPalette();
    static QPalette lightPalette();
    static QString loadStylesheet(ThemeMode mode);
    static void applyFont(QApplication* app);
};

} // namespace Clarity
