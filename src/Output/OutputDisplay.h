#pragma once

#include "Core/IpcClient.h"
#include "Core/Slide.h"
#include <QObject>
#include <QColor>
#include <QFile>
#include <QElapsedTimer>

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
    Q_PROPERTY(QString slideRichText READ slideRichText NOTIFY slideRichTextChanged)
    Q_PROPERTY(bool useRichText READ useRichText NOTIFY useRichTextChanged)
    Q_PROPERTY(QString redLetterColor READ redLetterColor NOTIFY redLetterColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY textColorChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(int fontSize READ fontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool isCleared READ isCleared NOTIFY isClearedChanged)
    Q_PROPERTY(QString backgroundType READ backgroundType NOTIFY backgroundTypeChanged)
    Q_PROPERTY(QByteArray backgroundImageData READ backgroundImageData NOTIFY backgroundImageDataChanged)
    Q_PROPERTY(QString backgroundImageDataBase64 READ backgroundImageDataBase64 NOTIFY backgroundImageDataChanged)
    Q_PROPERTY(QString gradientStopsJson READ gradientStopsJson NOTIFY gradientStopsJsonChanged)
    Q_PROPERTY(QString gradientType READ gradientType NOTIFY gradientTypeChanged)
    Q_PROPERTY(int gradientAngle READ gradientAngle NOTIFY gradientAngleChanged)
    Q_PROPERTY(double radialCenterX READ radialCenterX NOTIFY radialCenterXChanged)
    Q_PROPERTY(double radialCenterY READ radialCenterY NOTIFY radialCenterYChanged)
    Q_PROPERTY(double radialRadius READ radialRadius NOTIFY radialRadiusChanged)
    Q_PROPERTY(QString backgroundVideoSource READ backgroundVideoSource NOTIFY backgroundVideoSourceChanged)
    Q_PROPERTY(bool videoLoop READ videoLoop NOTIFY videoLoopChanged)

    // Text legibility: Drop shadow
    Q_PROPERTY(bool dropShadowEnabled READ dropShadowEnabled NOTIFY dropShadowEnabledChanged)
    Q_PROPERTY(QColor dropShadowColor READ dropShadowColor NOTIFY dropShadowColorChanged)
    Q_PROPERTY(int dropShadowOffsetX READ dropShadowOffsetX NOTIFY dropShadowOffsetXChanged)
    Q_PROPERTY(int dropShadowOffsetY READ dropShadowOffsetY NOTIFY dropShadowOffsetYChanged)
    Q_PROPERTY(int dropShadowBlur READ dropShadowBlur NOTIFY dropShadowBlurChanged)

    // Text legibility: Background overlay
    Q_PROPERTY(bool overlayEnabled READ overlayEnabled NOTIFY overlayEnabledChanged)
    Q_PROPERTY(QColor overlayColor READ overlayColor NOTIFY overlayColorChanged)
    Q_PROPERTY(int overlayBlur READ overlayBlur NOTIFY overlayBlurChanged)

    // Text legibility: Text container
    Q_PROPERTY(bool textContainerEnabled READ textContainerEnabled NOTIFY textContainerEnabledChanged)
    Q_PROPERTY(QColor textContainerColor READ textContainerColor NOTIFY textContainerColorChanged)
    Q_PROPERTY(int textContainerPadding READ textContainerPadding NOTIFY textContainerPaddingChanged)
    Q_PROPERTY(int textContainerRadius READ textContainerRadius NOTIFY textContainerRadiusChanged)
    Q_PROPERTY(int textContainerBlur READ textContainerBlur NOTIFY textContainerBlurChanged)

    // Text legibility: Text band
    Q_PROPERTY(bool textBandEnabled READ textBandEnabled NOTIFY textBandEnabledChanged)
    Q_PROPERTY(QColor textBandColor READ textBandColor NOTIFY textBandColorChanged)
    Q_PROPERTY(int textBandBlur READ textBandBlur NOTIFY textBandBlurChanged)

    // Transition properties
    Q_PROPERTY(QString transitionType READ transitionType NOTIFY transitionTypeChanged)
    Q_PROPERTY(int transitionDuration READ transitionDuration NOTIFY transitionDurationChanged)

public:
    explicit OutputDisplay(QObject* parent = nullptr);
    explicit OutputDisplay(const QString& clientType, QObject* parent = nullptr);

    // Property getters
    QString slideText() const { return m_slideText; }
    QString slideRichText() const { return m_slideRichText; }
    bool useRichText() const { return m_useRichText; }
    QString redLetterColor() const { return m_redLetterColor; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor textColor() const { return m_textColor; }
    QString fontFamily() const { return m_fontFamily; }
    int fontSize() const { return m_fontSize; }
    bool isCleared() const { return m_isCleared; }
    QString backgroundType() const { return m_backgroundType; }
    QByteArray backgroundImageData() const { return m_backgroundImageData; }
    QString backgroundImageDataBase64() const { return QString(m_backgroundImageData.toBase64()); }
    QString gradientStopsJson() const { return m_gradientStopsJson; }
    QString gradientType() const { return m_gradientType; }
    int gradientAngle() const { return m_gradientAngle; }
    double radialCenterX() const { return m_radialCenterX; }
    double radialCenterY() const { return m_radialCenterY; }
    double radialRadius() const { return m_radialRadius; }
    QString backgroundVideoSource() const { return m_backgroundVideoSource; }
    bool videoLoop() const { return m_videoLoop; }

    // Text legibility: Drop shadow getters
    bool dropShadowEnabled() const { return m_dropShadowEnabled; }
    QColor dropShadowColor() const { return m_dropShadowColor; }
    int dropShadowOffsetX() const { return m_dropShadowOffsetX; }
    int dropShadowOffsetY() const { return m_dropShadowOffsetY; }
    int dropShadowBlur() const { return m_dropShadowBlur; }

    // Text legibility: Overlay getters
    bool overlayEnabled() const { return m_overlayEnabled; }
    QColor overlayColor() const { return m_overlayColor; }
    int overlayBlur() const { return m_overlayBlur; }

    // Text legibility: Text container getters
    bool textContainerEnabled() const { return m_textContainerEnabled; }
    QColor textContainerColor() const { return m_textContainerColor; }
    int textContainerPadding() const { return m_textContainerPadding; }
    int textContainerRadius() const { return m_textContainerRadius; }
    int textContainerBlur() const { return m_textContainerBlur; }

    // Text legibility: Text band getters
    bool textBandEnabled() const { return m_textBandEnabled; }
    QColor textBandColor() const { return m_textBandColor; }
    int textBandBlur() const { return m_textBandBlur; }

    // Transition getters
    QString transitionType() const { return m_transitionType; }
    int transitionDuration() const { return m_transitionDuration; }

    // Called by QML when transition is complete
    Q_INVOKABLE void transitionComplete();

    // Called by QML to log blur performance status
    Q_INVOKABLE void logBlurStatus(const QString& details);

signals:
    void slideTextChanged();
    void slideRichTextChanged();
    void useRichTextChanged();
    void redLetterColorChanged();
    void backgroundColorChanged();
    void textColorChanged();
    void fontFamilyChanged();
    void fontSizeChanged();
    void isClearedChanged();
    void backgroundTypeChanged();
    void backgroundImageDataChanged();
    void gradientStopsJsonChanged();
    void gradientTypeChanged();
    void gradientAngleChanged();
    void radialCenterXChanged();
    void radialCenterYChanged();
    void radialRadiusChanged();
    void backgroundVideoSourceChanged();
    void videoLoopChanged();

    // Text legibility signals
    void dropShadowEnabledChanged();
    void dropShadowColorChanged();
    void dropShadowOffsetXChanged();
    void dropShadowOffsetYChanged();
    void dropShadowBlurChanged();
    void overlayEnabledChanged();
    void overlayColorChanged();
    void overlayBlurChanged();
    void textContainerEnabledChanged();
    void textContainerColorChanged();
    void textContainerPaddingChanged();
    void textContainerRadiusChanged();
    void textContainerBlurChanged();
    void textBandEnabledChanged();
    void textBandColorChanged();
    void textBandBlurChanged();

    void transitionTypeChanged();
    void transitionDurationChanged();

    // Signal to trigger transition in QML
    void startTransition();

    // Signal for immediate update (cut transition)
    void cutTransition();

    // Signal to toggle fullscreen mode
    void toggleFullscreen();

    // Signal to toggle visibility
    void toggleVisibility();

private slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QJsonObject& message);

private:
    void updateSlide(const Slide& slide);
    void clearDisplay();
    void initBlurLog();
    void logBlurConfig(const Slide& slide);

    IpcClient* m_ipcClient;
    QFile* m_blurLogFile = nullptr;
    QElapsedTimer m_slideUpdateTimer;

    QString m_slideText;
    QString m_slideRichText;
    bool m_useRichText;
    QString m_redLetterColor;
    QColor m_backgroundColor;
    QColor m_textColor;
    QString m_fontFamily;
    int m_fontSize;
    bool m_isCleared;
    QString m_backgroundType;
    QByteArray m_backgroundImageData;
    QString m_gradientStopsJson;
    QString m_gradientType;
    int m_gradientAngle;
    double m_radialCenterX;
    double m_radialCenterY;
    double m_radialRadius;
    QString m_backgroundVideoSource;
    bool m_videoLoop;

    // Text legibility: Drop shadow
    bool m_dropShadowEnabled;
    QColor m_dropShadowColor;
    int m_dropShadowOffsetX;
    int m_dropShadowOffsetY;
    int m_dropShadowBlur;

    // Text legibility: Overlay
    bool m_overlayEnabled;
    QColor m_overlayColor;
    int m_overlayBlur;

    // Text legibility: Text container
    bool m_textContainerEnabled;
    QColor m_textContainerColor;
    int m_textContainerPadding;
    int m_textContainerRadius;
    int m_textContainerBlur;

    // Text legibility: Text band
    bool m_textBandEnabled;
    QColor m_textBandColor;
    int m_textBandBlur;

    // Transition properties
    QString m_transitionType;
    int m_transitionDuration;
};

} // namespace Clarity
