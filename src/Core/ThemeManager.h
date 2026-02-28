// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Theme.h"
#include <QObject>
#include <QList>

namespace Clarity {

/**
 * @brief Manages the collection of themes (built-in and custom)
 *
 * ThemeManager provides access to built-in themes and handles
 * persistence of custom themes. Built-in themes cannot be modified
 * or deleted.
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject* parent = nullptr);

    // Access themes
    QList<Theme> allThemes() const;
    QList<Theme> builtInThemes() const { return m_builtInThemes; }
    QList<Theme> customThemes() const { return m_customThemes; }

    // Get a specific theme by name
    Theme getTheme(const QString& name) const;
    bool hasTheme(const QString& name) const;

    // Custom theme management
    void addTheme(const Theme& theme);
    void updateTheme(const QString& name, const Theme& theme);
    void removeTheme(const QString& name);

    // Persistence
    void saveCustomThemes();
    void loadCustomThemes();

    // Get theme names for UI
    QStringList themeNames() const;
    QStringList builtInThemeNames() const;
    QStringList customThemeNames() const;

signals:
    void themeAdded(const QString& name);
    void themeUpdated(const QString& name);
    void themeRemoved(const QString& name);
    void customThemesLoaded();

private:
    void initBuiltInThemes();
    QString customThemesPath() const;

    QList<Theme> m_builtInThemes;
    QList<Theme> m_customThemes;
};

} // namespace Clarity
