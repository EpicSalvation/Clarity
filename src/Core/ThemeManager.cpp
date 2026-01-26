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
    // Theme 1: Classic Blue
    {
        Theme theme("Classic Blue", "Traditional dark blue background with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::SolidColor);
        theme.setBackgroundColor(QColor("#1e3a8a"));
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#fbbf24"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 2: Modern Dark
    {
        Theme theme("Modern Dark", "Near-black background with clean white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::SolidColor);
        theme.setBackgroundColor(QColor("#0f0f0f"));
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#60a5fa"));
        theme.setFontFamily("Helvetica");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 3: Warm Earth
    {
        Theme theme("Warm Earth", "Earthy brown gradient with cream text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStartColor(QColor("#78350f"));
        theme.setGradientEndColor(QColor("#451a03"));
        theme.setGradientAngle(180);
        theme.setTextColor(QColor("#fef3c7"));
        theme.setAccentColor(QColor("#d97706"));
        theme.setFontFamily("Georgia");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 4: Ocean
    {
        Theme theme("Ocean", "Blue ocean gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStartColor(QColor("#0c4a6e"));
        theme.setGradientEndColor(QColor("#0369a1"));
        theme.setGradientAngle(135);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#38bdf8"));
        theme.setFontFamily("Verdana");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 5: Sunrise
    {
        Theme theme("Sunrise", "Warm orange/yellow gradient for bright themes");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStartColor(QColor("#ea580c"));
        theme.setGradientEndColor(QColor("#facc15"));
        theme.setGradientAngle(45);
        theme.setTextColor(QColor("#1c1917"));
        theme.setAccentColor(QColor("#7c2d12"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 6: Forest
    {
        Theme theme("Forest", "Natural green gradient with white text");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStartColor(QColor("#064e3b"));
        theme.setGradientEndColor(QColor("#065f46"));
        theme.setGradientAngle(180);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#34d399"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 7: Royal Purple
    {
        Theme theme("Royal Purple", "Elegant purple gradient with gold accents");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::Gradient);
        theme.setGradientStartColor(QColor("#581c87"));
        theme.setGradientEndColor(QColor("#7e22ce"));
        theme.setGradientAngle(135);
        theme.setTextColor(QColor("#ffffff"));
        theme.setAccentColor(QColor("#fbbf24"));
        theme.setFontFamily("Times New Roman");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
        m_builtInThemes.append(theme);
    }

    // Theme 8: Clean White
    {
        Theme theme("Clean White", "White background with dark text - ideal for bright rooms");
        theme.setBuiltIn(true);
        theme.setBackgroundType(Slide::SolidColor);
        theme.setBackgroundColor(QColor("#ffffff"));
        theme.setTextColor(QColor("#1f2937"));
        theme.setAccentColor(QColor("#2563eb"));
        theme.setFontFamily("Arial");
        theme.setTitleFontSize(72);
        theme.setBodyFontSize(48);
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
