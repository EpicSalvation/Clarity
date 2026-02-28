// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "ThemeManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace Clarity {

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    initBuiltInThemes();
    loadCustomThemes();
}

void ThemeManager::initBuiltInThemes()
{
    // =========================================================================
    // General-Purpose Themes
    // =========================================================================

    // Theme 1: Classic Blue — traditional church standby
    {
        Theme theme("Classic Blue", "Traditional blue gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#1e3a8a")),
            GradientStop(0.6, QColor("#1e40af")),
            GradientStop(1.0, QColor("#2563eb"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(160);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#fbbf24"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(10, 20, 60, 160));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 2: Modern Dark — sleek and contemporary
    {
        Theme theme("Modern Dark", "Dark gradient with clean white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#0a0a0a")),
            GradientStop(1.0, QColor("#1a1a2e"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.4);
        theme.setRadialRadius(0.7);
        theme.setTextColor(QColor("#f0f0f0"));
        theme.setAccentColor(QColor("#60a5fa"));
        theme.setFontFamily("Helvetica");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(0, 0, 0, 180));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(8);
        m_builtInThemes.append(theme);
    }

    // Theme 3: Warm Earth — rich and inviting
    {
        Theme theme("Warm Earth", "Earthy brown gradient with warm cream text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#78350f")),
            GradientStop(0.5, QColor("#5c2d0e")),
            GradientStop(1.0, QColor("#3b1a08"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(170);
        theme.setTextColor(QColor("#fef3c7"));
        theme.setAccentColor(QColor("#d97706"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(30, 15, 5, 170));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 4: Ocean — calming and refreshing
    {
        Theme theme("Ocean", "Deep blue ocean gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#0c4a6e")),
            GradientStop(0.4, QColor("#0e5a8a")),
            GradientStop(1.0, QColor("#0284c7"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(140);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#38bdf8"));
        theme.setFontFamily("Verdana");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(5, 30, 60, 170));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 5: Sunrise — warm and uplifting
    {
        Theme theme("Sunrise", "Warm sunrise gradient with dark text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#f97316")),
            GradientStop(0.5, QColor("#fb923c")),
            GradientStop(1.0, QColor("#fbbf24"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(45);
        theme.setTextColor(QColor("#1c1917"));
        theme.setAccentColor(QColor("#7c2d12"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(255, 240, 200, 140));
        theme.setDropShadowOffsetX(1);
        theme.setDropShadowOffsetY(2);
        theme.setDropShadowBlur(5);
        m_builtInThemes.append(theme);
    }

    // Theme 6: Forest — natural and peaceful
    {
        Theme theme("Forest", "Lush green gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#064e3b")),
            GradientStop(0.5, QColor("#065f46")),
            GradientStop(1.0, QColor("#047857"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(160);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#34d399"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(3, 30, 20, 170));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 7: Royal Purple — elegant and reverent
    {
        Theme theme("Royal Purple", "Rich purple gradient with gold accents");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#4c1d95")),
            GradientStop(0.5, QColor("#5b21b6")),
            GradientStop(1.0, QColor("#7c3aed"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(150);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#fbbf24"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(25, 10, 50, 170));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 8: Clean White — bright and crisp for well-lit rooms
    {
        Theme theme("Clean White", "Light gradient with dark text for bright rooms");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#ffffff")),
            GradientStop(1.0, QColor("#e8ecf1"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.4);
        theme.setRadialRadius(0.8);
        theme.setTextColor(QColor("#1f2937"));
        theme.setAccentColor(QColor("#2563eb"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(160, 170, 190, 80));
        theme.setDropShadowOffsetX(1);
        theme.setDropShadowOffsetY(2);
        theme.setDropShadowBlur(4);
        m_builtInThemes.append(theme);
    }

    // Theme 9: Midnight Sky — deep and contemplative
    {
        Theme theme("Midnight Sky", "Deep navy radial gradient with soft white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#1e293b")),
            GradientStop(0.6, QColor("#0f172a")),
            GradientStop(1.0, QColor("#020617"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.3);
        theme.setRadialRadius(0.7);
        theme.setTextColor(QColor("#e2e8f0"));
        theme.setAccentColor(QColor("#818cf8"));
        theme.setFontFamily("Verdana");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(0, 0, 0, 200));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(8);
        m_builtInThemes.append(theme);
    }

    // Theme 10: Golden Hour — warm and joyful
    {
        Theme theme("Golden Hour", "Soft amber radial glow with warm white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#b45309")),
            GradientStop(0.5, QColor("#92400e")),
            GradientStop(1.0, QColor("#6b2f0a"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.5);
        theme.setRadialRadius(0.65);
        theme.setTextColor(QColor("#fef9ef"));
        theme.setAccentColor(QColor("#fbbf24"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(50, 20, 5, 160));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 11: Soft Teal — fresh and welcoming
    {
        Theme theme("Soft Teal", "Gentle teal gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#134e4a")),
            GradientStop(0.5, QColor("#115e59")),
            GradientStop(1.0, QColor("#0f766e"))
        });
        theme.setGradientType(LinearGradient);
        theme.setGradientAngle(155);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#5eead4"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(5, 30, 28, 170));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(6);
        m_builtInThemes.append(theme);
    }

    // Theme 12: Plain Black — simple, for use with background images
    {
        Theme theme("Plain Black", "Solid black background for use with background images");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::SolidColor);
        theme.setBackgroundColor(QColor("#000000"));
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#a0a0a0"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        theme.setDropShadowEnabled(false);
        m_builtInThemes.append(theme);
    }

    // =========================================================================
    // Scripture-Specific Themes
    // =========================================================================

    // Theme 13: Scripture Parchment — classic Bible study look
    {
        Theme theme("Scripture Parchment", "Warm parchment gradient with serif font for Bible verses");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#f5e6d3")),
            GradientStop(0.5, QColor("#eedcc5")),
            GradientStop(1.0, QColor("#e0ccae"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.4);
        theme.setRadialRadius(0.75);
        theme.setTextColor(QColor("#3d2914"));
        theme.setAccentColor(QColor("#8b4513"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(64);
        theme.setBodyFontSize(44);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(245, 230, 200, 120));
        theme.setDropShadowOffsetX(1);
        theme.setDropShadowOffsetY(2);
        theme.setDropShadowBlur(4);
        m_builtInThemes.append(theme);
    }

    // Theme 14: Scripture Classic — elegant printed page
    {
        Theme theme("Scripture Classic", "Cream page with elegant serif typography");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#fdf8f0")),
            GradientStop(1.0, QColor("#f0e8d8"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.45);
        theme.setRadialRadius(0.8);
        theme.setTextColor(QColor("#1a1a1a"));
        theme.setAccentColor(QColor("#8b0000"));
        theme.setFontFamily("Times New Roman");
        theme.setTitleFontSize(64);
        theme.setBodyFontSize(44);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(220, 210, 190, 90));
        theme.setDropShadowOffsetX(1);
        theme.setDropShadowOffsetY(2);
        theme.setDropShadowBlur(3);
        m_builtInThemes.append(theme);
    }

    // Theme 15: Scripture Night — evening devotional
    {
        Theme theme("Scripture Night", "Dark theme for scripture in dim rooms");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStops({
            GradientStop(0.0, QColor("#1a1a2e")),
            GradientStop(0.5, QColor("#16213e")),
            GradientStop(1.0, QColor("#0f1729"))
        });
        theme.setGradientType(RadialGradient);
        theme.setRadialCenterX(0.5);
        theme.setRadialCenterY(0.4);
        theme.setRadialRadius(0.7);
        theme.setTextColor(QColor("#eae7dc"));
        theme.setAccentColor(QColor("#d4a574"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(64);
        theme.setBodyFontSize(44);
        theme.setDropShadowEnabled(true);
        theme.setDropShadowColor(QColor(0, 0, 15, 180));
        theme.setDropShadowOffsetX(2);
        theme.setDropShadowOffsetY(3);
        theme.setDropShadowBlur(7);
        m_builtInThemes.append(theme);
    }

    qDebug() << "Initialized" << m_builtInThemes.count() << "built-in themes";
}

QList<Theme> ThemeManager::allThemes() const
{
    QList<Theme> all;
    all.append(m_builtInThemes);
    all.append(m_customThemes);
    return all;
}

Theme ThemeManager::getTheme(const QString& name) const
{
    // Check built-in themes first
    for (const Theme& theme : m_builtInThemes) {
        if (theme.name() == name) {
            return theme;
        }
    }

    // Check custom themes
    for (const Theme& theme : m_customThemes) {
        if (theme.name() == name) {
            return theme;
        }
    }

    // Return default theme if not found
    qWarning() << "Theme not found:" << name;
    return Theme();
}

bool ThemeManager::hasTheme(const QString& name) const
{
    for (const Theme& theme : m_builtInThemes) {
        if (theme.name() == name) {
            return true;
        }
    }
    for (const Theme& theme : m_customThemes) {
        if (theme.name() == name) {
            return true;
        }
    }
    return false;
}

void ThemeManager::addTheme(const Theme& theme)
{
    // Don't allow adding themes with duplicate names
    if (hasTheme(theme.name())) {
        qWarning() << "Cannot add theme: name already exists:" << theme.name();
        return;
    }

    // Ensure custom themes are not marked as built-in
    Theme newTheme = theme;
    newTheme.setBuiltIn(false);

    m_customThemes.append(newTheme);
    emit themeAdded(newTheme.name());

    qDebug() << "Added custom theme:" << newTheme.name();
}

void ThemeManager::updateTheme(const QString& name, const Theme& theme)
{
    // Cannot update built-in themes
    for (const Theme& builtIn : m_builtInThemes) {
        if (builtIn.name() == name) {
            qWarning() << "Cannot update built-in theme:" << name;
            return;
        }
    }

    // Find and update custom theme
    for (int i = 0; i < m_customThemes.count(); i++) {
        if (m_customThemes[i].name() == name) {
            Theme updatedTheme = theme;
            updatedTheme.setBuiltIn(false);
            m_customThemes[i] = updatedTheme;
            emit themeUpdated(name);
            qDebug() << "Updated custom theme:" << name;
            return;
        }
    }

    qWarning() << "Theme not found for update:" << name;
}

void ThemeManager::removeTheme(const QString& name)
{
    // Cannot remove built-in themes
    for (const Theme& builtIn : m_builtInThemes) {
        if (builtIn.name() == name) {
            qWarning() << "Cannot remove built-in theme:" << name;
            return;
        }
    }

    // Find and remove custom theme
    for (int i = 0; i < m_customThemes.count(); i++) {
        if (m_customThemes[i].name() == name) {
            m_customThemes.removeAt(i);
            emit themeRemoved(name);
            qDebug() << "Removed custom theme:" << name;
            return;
        }
    }

    qWarning() << "Theme not found for removal:" << name;
}

QString ThemeManager::customThemesPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return configDir + "/Clarity/themes.json";
}

void ThemeManager::saveCustomThemes()
{
    QString path = customThemesPath();

    // Ensure directory exists
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical() << "Failed to create directory:" << dir.path();
            return;
        }
    }

    // Build JSON array of custom themes
    QJsonArray themesArray;
    for (const Theme& theme : m_customThemes) {
        themesArray.append(theme.toJson());
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["themes"] = themesArray;

    QJsonDocument doc(root);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Failed to open themes file for writing:" << path << file.errorString();
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Saved" << m_customThemes.count() << "custom themes to:" << path;
}

void ThemeManager::loadCustomThemes()
{
    QString path = customThemesPath();

    QFile file(path);
    if (!file.exists()) {
        qDebug() << "No custom themes file found at:" << path;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open themes file:" << path << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse themes file:" << parseError.errorString();
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray themesArray = root["themes"].toArray();

    m_customThemes.clear();
    for (const QJsonValue& value : themesArray) {
        Theme theme = Theme::fromJson(value.toObject());
        theme.setBuiltIn(false);  // Ensure loaded themes are not marked built-in
        m_customThemes.append(theme);
    }

    emit customThemesLoaded();
    qDebug() << "Loaded" << m_customThemes.count() << "custom themes from:" << path;
}

QStringList ThemeManager::themeNames() const
{
    QStringList names;
    for (const Theme& theme : m_builtInThemes) {
        names.append(theme.name());
    }
    for (const Theme& theme : m_customThemes) {
        names.append(theme.name());
    }
    return names;
}

QStringList ThemeManager::builtInThemeNames() const
{
    QStringList names;
    for (const Theme& theme : m_builtInThemes) {
        names.append(theme.name());
    }
    return names;
}

QStringList ThemeManager::customThemeNames() const
{
    QStringList names;
    for (const Theme& theme : m_customThemes) {
        names.append(theme.name());
    }
    return names;
}

} // namespace Clarity
