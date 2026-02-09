#include "PresentationItem.h"
#include <QJsonArray>

namespace Clarity {

PresentationItem::PresentationItem(QObject* parent)
    : QObject(parent)
    , m_uuid(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_hasCustomStyle(false)
    , m_cacheValid(false)
{
}

QString PresentationItem::typeName() const
{
    switch (type()) {
        case SongItemType:        return QStringLiteral("song");
        case ScriptureItemType:   return QStringLiteral("scripture");
        case CustomSlideItemType: return QStringLiteral("customSlide");
        case SlideGroupItemType:  return QStringLiteral("slideGroup");
    }
    return QStringLiteral("unknown");
}

QList<Slide> PresentationItem::cachedSlides() const
{
    if (!m_cacheValid) {
        m_cachedSlides = generateSlides();

        // Apply per-slide style overrides on top of generated slides
        for (auto it = m_perSlideStyles.constBegin(); it != m_perSlideStyles.constEnd(); ++it) {
            if (it.key() >= 0 && it.key() < m_cachedSlides.count()) {
                it.value().applyTo(m_cachedSlides[it.key()]);
            }
        }

        m_cacheValid = true;
    }
    return m_cachedSlides;
}

int PresentationItem::slideCount() const
{
    return cachedSlides().count();
}

void PresentationItem::invalidateSlideCache()
{
    m_cacheValid = false;
    m_cachedSlides.clear();
    emit slideCacheInvalidated();
}

void PresentationItem::setItemStyle(const SlideStyle& style)
{
    m_itemStyle = style;
    m_hasCustomStyle = true;
    m_perSlideStyles.clear();  // Group-level style supersedes per-slide overrides
    invalidateSlideCache();
    emit itemChanged();
}

void PresentationItem::clearCustomStyle()
{
    m_hasCustomStyle = false;
    m_itemStyle = SlideStyle();  // Reset to defaults
    invalidateSlideCache();
    emit itemChanged();
}

void PresentationItem::setSlideStyleOverride(int slideIndex, const SlideStyle& style)
{
    m_perSlideStyles[slideIndex] = style;
    invalidateSlideCache();
    emit itemChanged();
}

void PresentationItem::clearSlideStyleOverride(int slideIndex)
{
    if (m_perSlideStyles.remove(slideIndex) > 0) {
        invalidateSlideCache();
        emit itemChanged();
    }
}

QJsonObject PresentationItem::toJson() const
{
    return baseToJson();
}

QJsonObject PresentationItem::baseToJson() const
{
    QJsonObject json;
    json["type"] = typeName();
    json["uuid"] = m_uuid;

    if (m_hasCustomStyle) {
        QJsonObject styleJson;
        styleJson["backgroundColor"] = m_itemStyle.backgroundColor.name();
        styleJson["textColor"] = m_itemStyle.textColor.name();
        styleJson["fontFamily"] = m_itemStyle.fontFamily;
        styleJson["fontSize"] = m_itemStyle.fontSize;

        // Background type
        QString bgTypeStr = "solidColor";
        if (m_itemStyle.backgroundType == Slide::Gradient) {
            bgTypeStr = "gradient";
        } else if (m_itemStyle.backgroundType == Slide::Image) {
            bgTypeStr = "image";
        } else if (m_itemStyle.backgroundType == Slide::Video) {
            bgTypeStr = "video";
        }
        styleJson["backgroundType"] = bgTypeStr;

        if (m_itemStyle.backgroundType == Slide::Gradient) {
            styleJson["gradientStartColor"] = m_itemStyle.gradientStartColor.name();
            styleJson["gradientEndColor"] = m_itemStyle.gradientEndColor.name();
            styleJson["gradientAngle"] = m_itemStyle.gradientAngle;
        } else if (m_itemStyle.backgroundType == Slide::Image) {
            styleJson["backgroundImagePath"] = m_itemStyle.backgroundImagePath;
            if (!m_itemStyle.backgroundImageData.isEmpty()) {
                styleJson["backgroundImageData"] = QString::fromLatin1(m_itemStyle.backgroundImageData.toBase64());
            }
        } else if (m_itemStyle.backgroundType == Slide::Video) {
            styleJson["backgroundVideoPath"] = m_itemStyle.backgroundVideoPath;
            styleJson["videoLoop"] = m_itemStyle.videoLoop;
        }

        json["style"] = styleJson;
    }

    // Serialize per-slide style overrides
    if (!m_perSlideStyles.isEmpty()) {
        QJsonObject slideStylesJson;
        for (auto it = m_perSlideStyles.constBegin(); it != m_perSlideStyles.constEnd(); ++it) {
            QJsonObject ssJson;
            ssJson["backgroundColor"] = it.value().backgroundColor.name();
            ssJson["textColor"] = it.value().textColor.name();
            ssJson["fontFamily"] = it.value().fontFamily;
            ssJson["fontSize"] = it.value().fontSize;

            QString bgTypeStr = "solidColor";
            if (it.value().backgroundType == Slide::Gradient) {
                bgTypeStr = "gradient";
                ssJson["gradientStartColor"] = it.value().gradientStartColor.name();
                ssJson["gradientEndColor"] = it.value().gradientEndColor.name();
                ssJson["gradientAngle"] = it.value().gradientAngle;
            } else if (it.value().backgroundType == Slide::Image) {
                bgTypeStr = "image";
                ssJson["backgroundImagePath"] = it.value().backgroundImagePath;
                if (!it.value().backgroundImageData.isEmpty()) {
                    ssJson["backgroundImageData"] = QString::fromLatin1(it.value().backgroundImageData.toBase64());
                }
            } else if (it.value().backgroundType == Slide::Video) {
                bgTypeStr = "video";
                ssJson["backgroundVideoPath"] = it.value().backgroundVideoPath;
                ssJson["videoLoop"] = it.value().videoLoop;
            }
            ssJson["backgroundType"] = bgTypeStr;

            slideStylesJson[QString::number(it.key())] = ssJson;
        }
        json["slideStyles"] = slideStylesJson;
    }

    return json;
}

void PresentationItem::applyBaseJson(const QJsonObject& json)
{
    m_uuid = json["uuid"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));

    if (json.contains("style")) {
        QJsonObject styleJson = json["style"].toObject();
        m_itemStyle.backgroundColor = QColor(styleJson["backgroundColor"].toString("#1e3a8a"));
        m_itemStyle.textColor = QColor(styleJson["textColor"].toString("#ffffff"));
        m_itemStyle.fontFamily = styleJson["fontFamily"].toString("Arial");
        m_itemStyle.fontSize = styleJson["fontSize"].toInt(48);

        // Background type
        QString bgTypeStr = styleJson["backgroundType"].toString("solidColor");
        if (bgTypeStr == "gradient") {
            m_itemStyle.backgroundType = Slide::Gradient;
            m_itemStyle.gradientStartColor = QColor(styleJson["gradientStartColor"].toString("#1e3a8a"));
            m_itemStyle.gradientEndColor = QColor(styleJson["gradientEndColor"].toString("#60a5fa"));
            m_itemStyle.gradientAngle = styleJson["gradientAngle"].toInt(135);
        } else if (bgTypeStr == "image") {
            m_itemStyle.backgroundType = Slide::Image;
            m_itemStyle.backgroundImagePath = styleJson["backgroundImagePath"].toString();
            if (styleJson.contains("backgroundImageData")) {
                m_itemStyle.backgroundImageData = QByteArray::fromBase64(
                    styleJson["backgroundImageData"].toString().toLatin1());
            }
        } else if (bgTypeStr == "video") {
            m_itemStyle.backgroundType = Slide::Video;
            m_itemStyle.backgroundVideoPath = styleJson["backgroundVideoPath"].toString();
            m_itemStyle.videoLoop = styleJson["videoLoop"].toBool(true);
        } else {
            m_itemStyle.backgroundType = Slide::SolidColor;
        }

        m_hasCustomStyle = true;
    }

    // Deserialize per-slide style overrides
    if (json.contains("slideStyles")) {
        QJsonObject slideStylesJson = json["slideStyles"].toObject();
        for (auto it = slideStylesJson.constBegin(); it != slideStylesJson.constEnd(); ++it) {
            bool ok;
            int slideIndex = it.key().toInt(&ok);
            if (!ok) continue;

            QJsonObject ssJson = it.value().toObject();
            SlideStyle style;
            style.backgroundColor = QColor(ssJson["backgroundColor"].toString("#1e3a8a"));
            style.textColor = QColor(ssJson["textColor"].toString("#ffffff"));
            style.fontFamily = ssJson["fontFamily"].toString("Arial");
            style.fontSize = ssJson["fontSize"].toInt(48);

            QString bgTypeStr = ssJson["backgroundType"].toString("solidColor");
            if (bgTypeStr == "gradient") {
                style.backgroundType = Slide::Gradient;
                style.gradientStartColor = QColor(ssJson["gradientStartColor"].toString("#1e3a8a"));
                style.gradientEndColor = QColor(ssJson["gradientEndColor"].toString("#60a5fa"));
                style.gradientAngle = ssJson["gradientAngle"].toInt(135);
            } else if (bgTypeStr == "image") {
                style.backgroundType = Slide::Image;
                style.backgroundImagePath = ssJson["backgroundImagePath"].toString();
                if (ssJson.contains("backgroundImageData")) {
                    style.backgroundImageData = QByteArray::fromBase64(
                        ssJson["backgroundImageData"].toString().toLatin1());
                }
            } else if (bgTypeStr == "video") {
                style.backgroundType = Slide::Video;
                style.backgroundVideoPath = ssJson["backgroundVideoPath"].toString();
                style.videoLoop = ssJson["videoLoop"].toBool(true);
            } else {
                style.backgroundType = Slide::SolidColor;
            }

            m_perSlideStyles[slideIndex] = style;
        }
    }
}

void PresentationItem::applyStyleToSlide(Slide& slide) const
{
    if (m_hasCustomStyle) {
        m_itemStyle.applyTo(slide);
    }
}

// Static factory method - implemented after subclasses are defined
// Forward declarations are used here; full implementation requires subclass headers
PresentationItem* PresentationItem::fromJson(const QJsonObject& json,
                                              SongLibrary* songLibrary,
                                              BibleDatabase* bibleDatabase)
{
    Q_UNUSED(songLibrary)
    Q_UNUSED(bibleDatabase)

    QString typeName = json["type"].toString();

    // This factory method will be completed after subclass implementations
    // For now, return nullptr - subclasses register themselves
    // The actual implementation uses dynamic type registration

    // Note: Full implementation is in a separate compilation unit to avoid
    // circular dependencies. See PresentationItemFactory.cpp
    qWarning("PresentationItem::fromJson called - subclass factory not yet linked");
    Q_UNUSED(typeName)

    return nullptr;
}

} // namespace Clarity
