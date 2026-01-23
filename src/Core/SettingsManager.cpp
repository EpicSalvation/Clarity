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

void SettingsManager::resetToDefaults()
{
    qDebug() << "SettingsManager: Resetting all settings to defaults";
    m_settings->clear();
    emit outputScreenIndexChanged(DEFAULT_OUTPUT_SCREEN_INDEX);
}

} // namespace Clarity
