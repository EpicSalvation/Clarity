#pragma once

#include "Core/IpcClient.h"
#include "Core/Slide.h"
#include <QObject>
#include <QColor>

namespace Clarity {

/**
 * @brief Controller for output display
 *
 * Manages IPC connection and exposes slide properties to QML via Q_PROPERTY.
 * This class acts as the bridge between the IPC messages and the QML UI.
 */
class OutputDisplay : public QObject {
    Q_OBJECT

    // Properties exposed to QML
    Q_PROPERTY(QString slideText READ slideText NOTIFY slideTextChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY textColorChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(int fontSize READ fontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool isCleared READ isCleared NOTIFY isClearedChanged)

public:
    explicit OutputDisplay(QObject* parent = nullptr);

    // Property getters
    QString slideText() const { return m_slideText; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor textColor() const { return m_textColor; }
    QString fontFamily() const { return m_fontFamily; }
    int fontSize() const { return m_fontSize; }
    bool isCleared() const { return m_isCleared; }

signals:
    void slideTextChanged();
    void backgroundColorChanged();
    void textColorChanged();
    void fontFamilyChanged();
    void fontSizeChanged();
    void isClearedChanged();

private slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QJsonObject& message);

private:
    void updateSlide(const Slide& slide);
    void clearDisplay();

    IpcClient* m_ipcClient;

    QString m_slideText;
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;
    bool m_isCleared;
};

} // namespace Clarity
