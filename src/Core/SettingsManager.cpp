#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDebug>

namespace Clarity {

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings(new QSettings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName(),
                               this))
{
    qDebug() << "SettingsManager: Settings file:" << m_settings->fileName();
}

SettingsManager::~SettingsManager()
{
    // QSettings is a child object, will be deleted automatically
}

int SettingsManager::outputScreenIndex() const
{
    return m_settings->value("Display/OutputScreenIndex", DEFAULT_OUTPUT_SCREEN_INDEX).toInt();
}

void SettingsManager::setOutputScreenIndex(int index)
{
    if (index < 0) {
        qWarning() << "SettingsManager: Invalid screen index:" << index;
        return;
    }

    int currentIndex = outputScreenIndex();
    if (currentIndex != index) {
        m_settings->setValue("Display/OutputScreenIndex", index);
        m_settings->sync(); // Ensure written to disk immediately
        emit outputScreenIndexChanged(index);
        qDebug() << "SettingsManager: Output screen index set to" << index;
    }
}

int SettingsManager::confidenceScreenIndex() const
{
    return m_settings->value("Display/ConfidenceScreenIndex", DEFAULT_CONFIDENCE_SCREEN_INDEX).toInt();
}

void SettingsManager::setConfidenceScreenIndex(int index)
{
    if (index < 0) {
        qWarning() << "SettingsManager: Invalid screen index:" << index;
        return;
    }

    int currentIndex = confidenceScreenIndex();
    if (currentIndex != index) {
        m_settings->setValue("Display/ConfidenceScreenIndex", index);
        m_settings->sync(); // Ensure written to disk immediately
        emit confidenceScreenIndexChanged(index);
        qDebug() << "SettingsManager: Confidence screen index set to" << index;
    }
}

QString SettingsManager::confidenceFontFamily() const
{
    return m_settings->value("ConfidenceMonitor/FontFamily", "Arial").toString();
}

void SettingsManager::setConfidenceFontFamily(const QString& fontFamily)
{
    if (confidenceFontFamily() != fontFamily) {
        m_settings->setValue("ConfidenceMonitor/FontFamily", fontFamily);
        m_settings->sync();
        emit confidenceDisplaySettingsChanged();
        qDebug() << "SettingsManager: Confidence font family set to" << fontFamily;
    }
}

int SettingsManager::confidenceFontSize() const
{
    return m_settings->value("ConfidenceMonitor/FontSize", DEFAULT_CONFIDENCE_FONT_SIZE).toInt();
}

void SettingsManager::setConfidenceFontSize(int size)
{
    if (size < 8 || size > 200) {
        qWarning() << "SettingsManager: Invalid font size:" << size;
        return;
    }

    if (confidenceFontSize() != size) {
        m_settings->setValue("ConfidenceMonitor/FontSize", size);
        m_settings->sync();
        emit confidenceDisplaySettingsChanged();
        qDebug() << "SettingsManager: Confidence font size set to" << size;
    }
}

QColor SettingsManager::confidenceTextColor() const
{
    return QColor(m_settings->value("ConfidenceMonitor/TextColor", "#ffffff").toString());
}

void SettingsManager::setConfidenceTextColor(const QColor& color)
{
    if (confidenceTextColor() != color) {
        m_settings->setValue("ConfidenceMonitor/TextColor", color.name());
        m_settings->sync();
        emit confidenceDisplaySettingsChanged();
        qDebug() << "SettingsManager: Confidence text color set to" << color.name();
    }
}

QColor SettingsManager::confidenceBackgroundColor() const
{
    return QColor(m_settings->value("ConfidenceMonitor/BackgroundColor", "#2a2a2a").toString());
}

void SettingsManager::setConfidenceBackgroundColor(const QColor& color)
{
    if (confidenceBackgroundColor() != color) {
        m_settings->setValue("ConfidenceMonitor/BackgroundColor", color.name());
        m_settings->sync();
        emit confidenceDisplaySettingsChanged();
        qDebug() << "SettingsManager: Confidence background color set to" << color.name();
    }
}

QString SettingsManager::transitionType() const
{
    return m_settings->value("Transitions/Type", "fade").toString();
}

void SettingsManager::setTransitionType(const QString& type)
{
    QStringList validTypes = {"cut", "fade", "slideLeft", "slideRight", "slideUp", "slideDown"};
    if (!validTypes.contains(type)) {
        qWarning() << "SettingsManager: Invalid transition type:" << type;
        return;
    }

    if (transitionType() != type) {
        m_settings->setValue("Transitions/Type", type);
        m_settings->sync();
        emit transitionSettingsChanged();
        qDebug() << "SettingsManager: Transition type set to" << type;
    }
}

int SettingsManager::transitionDuration() const
{
    return m_settings->value("Transitions/Duration", DEFAULT_TRANSITION_DURATION).toInt();
}

void SettingsManager::setTransitionDuration(int ms)
{
    if (ms < 0 || ms > 2000) {
        qWarning() << "SettingsManager: Invalid transition duration:" << ms;
        return;
    }

    if (transitionDuration() != ms) {
        m_settings->setValue("Transitions/Duration", ms);
        m_settings->sync();
        emit transitionSettingsChanged();
        qDebug() << "SettingsManager: Transition duration set to" << ms << "ms";
    }
}

bool SettingsManager::scrollWheelChangesInputs() const
{
    // Default is false - require click/focus before scroll wheel changes values
    return m_settings->value("UI/ScrollWheelChangesInputs", false).toBool();
}

void SettingsManager::setScrollWheelChangesInputs(bool enabled)
{
    if (scrollWheelChangesInputs() != enabled) {
        m_settings->setValue("UI/ScrollWheelChangesInputs", enabled);
        m_settings->sync();
        qDebug() << "SettingsManager: Scroll wheel changes inputs set to" << enabled;
    }
}

bool SettingsManager::remoteControlEnabled() const
{
    return m_settings->value("RemoteControl/Enabled", true).toBool();
}

void SettingsManager::setRemoteControlEnabled(bool enabled)
{
    if (remoteControlEnabled() != enabled) {
        m_settings->setValue("RemoteControl/Enabled", enabled);
        m_settings->sync();
        emit remoteControlSettingsChanged();
        qDebug() << "SettingsManager: Remote control enabled set to" << enabled;
    }
}

quint16 SettingsManager::remoteControlPort() const
{
    return static_cast<quint16>(m_settings->value("RemoteControl/Port", DEFAULT_REMOTE_CONTROL_PORT).toUInt());
}

void SettingsManager::setRemoteControlPort(quint16 port)
{
    if (port < 1024 || port > 65535) {
        qWarning() << "SettingsManager: Invalid port number:" << port;
        return;
    }

    if (remoteControlPort() != port) {
        m_settings->setValue("RemoteControl/Port", port);
        m_settings->sync();
        emit remoteControlSettingsChanged();
        qDebug() << "SettingsManager: Remote control port set to" << port;
    }
}

bool SettingsManager::remoteControlPinEnabled() const
{
    return m_settings->value("RemoteControl/PinEnabled", false).toBool();
}

void SettingsManager::setRemoteControlPinEnabled(bool enabled)
{
    if (remoteControlPinEnabled() != enabled) {
        m_settings->setValue("RemoteControl/PinEnabled", enabled);
        m_settings->sync();
        emit remoteControlSettingsChanged();
        qDebug() << "SettingsManager: Remote control PIN enabled set to" << enabled;
    }
}

QString SettingsManager::remoteControlPin() const
{
    return m_settings->value("RemoteControl/Pin", "").toString();
}

void SettingsManager::setRemoteControlPin(const QString& pin)
{
    // Allow 4-8 digit PINs only
    if (!pin.isEmpty() && (pin.length() < 4 || pin.length() > 8)) {
        qWarning() << "SettingsManager: PIN must be 4-8 digits";
        return;
    }

    // Validate that PIN contains only digits
    for (const QChar& c : pin) {
        if (!c.isDigit()) {
            qWarning() << "SettingsManager: PIN must contain only digits";
            return;
        }
    }

    if (remoteControlPin() != pin) {
        m_settings->setValue("RemoteControl/Pin", pin);
        m_settings->sync();
        emit remoteControlSettingsChanged();
        qDebug() << "SettingsManager: Remote control PIN updated";
    }
}

bool SettingsManager::showAllSlidesInGrid() const
{
    // Default is false - show only the selected item's slides
    return m_settings->value("UI/ShowAllSlidesInGrid", false).toBool();
}

void SettingsManager::setShowAllSlidesInGrid(bool showAll)
{
    if (showAllSlidesInGrid() != showAll) {
        m_settings->setValue("UI/ShowAllSlidesInGrid", showAll);
        m_settings->sync();
        emit slideGridModeChanged(showAll);
        qDebug() << "SettingsManager: Show all slides in grid set to" << showAll;
    }
}

QString SettingsManager::slidePreviewSize() const
{
    return m_settings->value("UI/SlidePreviewSize", "small").toString();
}

void SettingsManager::setSlidePreviewSize(const QString& size)
{
    QStringList validSizes = {"small", "medium", "large"};
    if (!validSizes.contains(size)) {
        qWarning() << "SettingsManager: Invalid slide preview size:" << size;
        return;
    }

    if (slidePreviewSize() != size) {
        m_settings->setValue("UI/SlidePreviewSize", size);
        m_settings->sync();
        emit slidePreviewSizeChanged(size);
        qDebug() << "SettingsManager: Slide preview size set to" << size;
    }
}

QString SettingsManager::language() const
{
    return m_settings->value("General/Language", "system").toString();
}

void SettingsManager::setLanguage(const QString& languageCode)
{
    QStringList validLanguages = {"system", "en", "es", "de", "fr"};
    if (!validLanguages.contains(languageCode)) {
        qWarning() << "SettingsManager: Invalid language code:" << languageCode;
        return;
    }

    if (language() != languageCode) {
        m_settings->setValue("General/Language", languageCode);
        m_settings->sync();
        emit languageChanged(languageCode);
        qDebug() << "SettingsManager: Language set to" << languageCode;
    }
}

QString SettingsManager::preferredBibleTranslation() const
{
    return m_settings->value("Bible/PreferredTranslation", "KJV").toString();
}

void SettingsManager::setPreferredBibleTranslation(const QString& translationCode)
{
    if (preferredBibleTranslation() != translationCode) {
        m_settings->setValue("Bible/PreferredTranslation", translationCode);
        m_settings->sync();
    }
}

bool SettingsManager::rememberLastBibleTranslation() const
{
    return m_settings->value("Bible/RememberLastTranslation", true).toBool();
}

void SettingsManager::setRememberLastBibleTranslation(bool remember)
{
    if (rememberLastBibleTranslation() != remember) {
        m_settings->setValue("Bible/RememberLastTranslation", remember);
        m_settings->sync();
    }
}

QString SettingsManager::lastBibleTranslation() const
{
    return m_settings->value("Bible/LastTranslation", "KJV").toString();
}

void SettingsManager::setLastBibleTranslation(const QString& translationCode)
{
    if (lastBibleTranslation() != translationCode) {
        m_settings->setValue("Bible/LastTranslation", translationCode);
        m_settings->sync();
    }
}

QString SettingsManager::effectiveBibleTranslation() const
{
    if (rememberLastBibleTranslation()) {
        return lastBibleTranslation();
    }
    return preferredBibleTranslation();
}

bool SettingsManager::scriptureOneVersePerSlide() const
{
    return m_settings->value("Bible/OneVersePerSlide", false).toBool();
}

void SettingsManager::setScriptureOneVersePerSlide(bool onePerSlide)
{
    if (scriptureOneVersePerSlide() != onePerSlide) {
        m_settings->setValue("Bible/OneVersePerSlide", onePerSlide);
        m_settings->sync();
    }
}

bool SettingsManager::redLettersEnabled() const
{
    return m_settings->value("Bible/RedLettersEnabled", true).toBool();
}

void SettingsManager::setRedLettersEnabled(bool enabled)
{
    if (redLettersEnabled() != enabled) {
        m_settings->setValue("Bible/RedLettersEnabled", enabled);
        m_settings->sync();
    }
}

QString SettingsManager::redLetterColor() const
{
    return m_settings->value("Bible/RedLetterColor", "#cc0000").toString();
}

void SettingsManager::setRedLetterColor(const QString& color)
{
    // Validate color string
    if (!QColor::isValidColorName(color)) {
        qWarning() << "SettingsManager: Invalid red letter color:" << color;
        return;
    }

    if (redLetterColor() != color) {
        m_settings->setValue("Bible/RedLetterColor", color);
        m_settings->sync();
    }
}

bool SettingsManager::usageTrackingEnabled() const
{
    return m_settings->value("CCLI/UsageTrackingEnabled", true).toBool();
}

void SettingsManager::setUsageTrackingEnabled(bool enabled)
{
    if (usageTrackingEnabled() != enabled) {
        m_settings->setValue("CCLI/UsageTrackingEnabled", enabled);
        m_settings->sync();
        qDebug() << "SettingsManager: Usage tracking enabled set to" << enabled;
    }
}

bool SettingsManager::promptForEventName() const
{
    return m_settings->value("CCLI/PromptForEventName", false).toBool();
}

void SettingsManager::setPromptForEventName(bool prompt)
{
    if (promptForEventName() != prompt) {
        m_settings->setValue("CCLI/PromptForEventName", prompt);
        m_settings->sync();
        qDebug() << "SettingsManager: Prompt for event name set to" << prompt;
    }
}

QString SettingsManager::defaultEventName() const
{
    return m_settings->value("CCLI/DefaultEventName", "Sunday Service").toString();
}

void SettingsManager::setDefaultEventName(const QString& name)
{
    if (defaultEventName() != name) {
        m_settings->setValue("CCLI/DefaultEventName", name);
        m_settings->sync();
        qDebug() << "SettingsManager: Default event name set to" << name;
    }
}

bool SettingsManager::cascadingBackgrounds() const
{
    return m_settings->value("Presentation/CascadingBackgrounds", true).toBool();
}

void SettingsManager::setCascadingBackgrounds(bool enabled)
{
    if (cascadingBackgrounds() != enabled) {
        m_settings->setValue("Presentation/CascadingBackgrounds", enabled);
        m_settings->sync();
    }
}

bool SettingsManager::scriptureThemeOverride() const
{
    return m_settings->value("Presentation/ScriptureThemeOverride", false).toBool();
}

void SettingsManager::setScriptureThemeOverride(bool enabled)
{
    if (scriptureThemeOverride() != enabled) {
        m_settings->setValue("Presentation/ScriptureThemeOverride", enabled);
        m_settings->sync();
    }
}

QString SettingsManager::scriptureThemeOverrideName() const
{
    return m_settings->value("Presentation/ScriptureThemeOverrideName", "").toString();
}

void SettingsManager::setScriptureThemeOverrideName(const QString& name)
{
    if (scriptureThemeOverrideName() != name) {
        m_settings->setValue("Presentation/ScriptureThemeOverrideName", name);
        m_settings->sync();
    }
}

bool SettingsManager::autoSyncLibraryGroups() const
{
    return m_settings->value("Library/AutoSyncGroups", false).toBool();
}

void SettingsManager::setAutoSyncLibraryGroups(bool enabled)
{
    if (autoSyncLibraryGroups() != enabled) {
        m_settings->setValue("Library/AutoSyncGroups", enabled);
        m_settings->sync();
    }
}

QString SettingsManager::esvApiKey() const
{
    return m_settings->value("ESV/ApiKey", "").toString();
}

void SettingsManager::setEsvApiKey(const QString& key)
{
    if (esvApiKey() != key) {
        m_settings->setValue("ESV/ApiKey", key);
        m_settings->sync();
        emit esvApiKeyChanged();
        qDebug() << "SettingsManager: ESV API key updated";
    }
}

bool SettingsManager::hasEsvApiKey() const
{
    return !esvApiKey().trimmed().isEmpty();
}

int SettingsManager::esvCachedVerseCount() const
{
    return m_settings->value("ESV/CachedVerseCount", 0).toInt();
}

void SettingsManager::setEsvCachedVerseCount(int count)
{
    m_settings->setValue("ESV/CachedVerseCount", qMax(0, count));
    m_settings->sync();
}

QString SettingsManager::apiBibleApiKey() const
{
    return m_settings->value("ApiBible/ApiKey", "").toString();
}

void SettingsManager::setApiBibleApiKey(const QString& key)
{
    if (apiBibleApiKey() != key) {
        m_settings->setValue("ApiBible/ApiKey", key);
        m_settings->sync();
        emit apiBibleApiKeyChanged();
        qDebug() << "SettingsManager: API.bible API key updated";
    }
}

bool SettingsManager::hasApiBibleApiKey() const
{
    return !apiBibleApiKey().trimmed().isEmpty();
}

QString SettingsManager::apiBibleLastBibleId() const
{
    return m_settings->value("ApiBible/LastBibleId", "").toString();
}

void SettingsManager::setApiBibleLastBibleId(const QString& bibleId)
{
    if (apiBibleLastBibleId() != bibleId) {
        m_settings->setValue("ApiBible/LastBibleId", bibleId);
        m_settings->sync();
    }
}

void SettingsManager::resetToDefaults()
{
    qDebug() << "SettingsManager: Resetting all settings to defaults";
    m_settings->clear();
    emit outputScreenIndexChanged(DEFAULT_OUTPUT_SCREEN_INDEX);
    emit confidenceScreenIndexChanged(DEFAULT_CONFIDENCE_SCREEN_INDEX);
    emit confidenceDisplaySettingsChanged();
    emit transitionSettingsChanged();
    emit remoteControlSettingsChanged();
    emit languageChanged("system");
    emit slidePreviewSizeChanged("small");
}

} // namespace Clarity
