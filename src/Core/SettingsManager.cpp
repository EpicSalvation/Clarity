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

void SettingsManager::resetToDefaults()
{
    qDebug() << "SettingsManager: Resetting all settings to defaults";
    m_settings->clear();
    emit outputScreenIndexChanged(DEFAULT_OUTPUT_SCREEN_INDEX);
    emit confidenceScreenIndexChanged(DEFAULT_CONFIDENCE_SCREEN_INDEX);
    emit confidenceDisplaySettingsChanged();
    emit transitionSettingsChanged();
}

} // namespace Clarity
