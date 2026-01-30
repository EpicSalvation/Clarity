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
        json["style"] = styleJson;
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
        m_hasCustomStyle = true;
    }
}

void PresentationItem::applyStyleToSlide(Slide& slide) const
{
    if (m_hasCustomStyle) {
        slide.setBackgroundColor(m_itemStyle.backgroundColor);
        slide.setTextColor(m_itemStyle.textColor);
        slide.setFontFamily(m_itemStyle.fontFamily);
        slide.setFontSize(m_itemStyle.fontSize);
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
