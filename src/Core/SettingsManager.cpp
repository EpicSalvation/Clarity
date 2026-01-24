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

void SettingsManager::resetToDefaults()
{
    qDebug() << "SettingsManager: Resetting all settings to defaults";
    m_settings->clear();
    emit outputScreenIndexChanged(DEFAULT_OUTPUT_SCREEN_INDEX);
    emit confidenceScreenIndexChanged(DEFAULT_CONFIDENCE_SCREEN_INDEX);
    emit confidenceDisplaySettingsChanged();
}

} // namespace Clarity
