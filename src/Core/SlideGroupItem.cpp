#include "SlideGroupItem.h"
#include <QJsonArray>

namespace Clarity {

SlideGroupItem::SlideGroupItem(QObject* parent)
    : PresentationItem(parent)
    , m_name(tr("Slide Group"))
{
}

SlideGroupItem::SlideGroupItem(const QString& name, QObject* parent)
    : PresentationItem(parent)
    , m_name(name)
{
}

SlideGroupItem::SlideGroupItem(const QString& name, const QList<Slide>& slides, QObject* parent)
    : PresentationItem(parent)
    , m_name(name)
    , m_slides(slides)
{
}

QString SlideGroupItem::displayName() const
{
    if (m_name.isEmpty()) {
        return tr("Slide Group (%n slide(s))", "", m_slides.count());
    }
    return m_name;
}

QList<Slide> SlideGroupItem::generateSlides() const
{
    if (!m_hasCustomStyle) {
        // No custom style - return slides as-is
        return m_slides;
    }

    // Apply custom style to copies of slides
    QList<Slide> styledSlides;
    styledSlides.reserve(m_slides.size());

    for (const Slide& slide : m_slides) {
        Slide styledSlide = slide;
        m_itemStyle.applyTo(styledSlide);
        styledSlides.append(styledSlide);
    }

    return styledSlides;
}

void SlideGroupItem::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        emit itemChanged();
    }
}

void SlideGroupItem::setSlides(const QList<Slide>& slides)
{
    m_slides = slides;
    invalidateSlideCache();
    emit itemChanged();
}

void SlideGroupItem::addSlide(const Slide& slide)
{
    m_slides.append(slide);
    invalidateSlideCache();
    emit itemChanged();
}

void SlideGroupItem::insertSlide(int index, const Slide& slide)
{
    if (index >= 0 && index <= m_slides.count()) {
        m_slides.insert(index, slide);
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SlideGroupItem::bakeCustomStyle()
{
    if (!m_hasCustomStyle) return;

    // Permanently apply group style to each individual slide
    for (int i = 0; i < m_slides.count(); ++i) {
        m_itemStyle.applyTo(m_slides[i]);
    }

    // Clear the group-level style so generateSlides() returns m_slides as-is
    m_hasCustomStyle = false;
    m_itemStyle = SlideStyle();
    invalidateSlideCache();
}

void SlideGroupItem::updateSlide(int index, const Slide& slide)
{
    if (index >= 0 && index < m_slides.count()) {
        m_slides[index] = slide;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SlideGroupItem::removeSlide(int index)
{
    if (index >= 0 && index < m_slides.count()) {
        m_slides.removeAt(index);
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SlideGroupItem::moveSlide(int from, int to)
{
    if (from >= 0 && from < m_slides.count() &&
        to >= 0 && to < m_slides.count() &&
        from != to) {
        m_slides.move(from, to);
        invalidateSlideCache();
        emit itemChanged();
    }
}

Slide SlideGroupItem::slideAt(int index) const
{
    if (index >= 0 && index < m_slides.count()) {
        return m_slides.at(index);
    }
    return Slide();
}

QJsonObject SlideGroupItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["name"] = m_name;

    QJsonArray slidesArray;
    for (const Slide& slide : m_slides) {
        slidesArray.append(slide.toJson());
    }
    json["slides"] = slidesArray;

    return json;
}

SlideGroupItem* SlideGroupItem::fromJson(const QJsonObject& json)
{
    if (json["type"].toString() != "slideGroup") {
        qWarning("SlideGroupItem::fromJson: type mismatch");
        return nullptr;
    }

    SlideGroupItem* item = new SlideGroupItem();
    item->applyBaseJson(json);
    item->m_name = json["name"].toString(tr("Slide Group"));

    QJsonArray slidesArray = json["slides"].toArray();
    for (const QJsonValue& value : slidesArray) {
        item->m_slides.append(Slide::fromJson(value.toObject()));
    }

    return item;
}

} // namespace Clarity
