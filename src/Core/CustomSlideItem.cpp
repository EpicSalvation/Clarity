// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "CustomSlideItem.h"

namespace Clarity {

CustomSlideItem::CustomSlideItem(QObject* parent)
    : PresentationItem(parent)
{
}

CustomSlideItem::CustomSlideItem(const Slide& slide, QObject* parent)
    : PresentationItem(parent)
    , m_slide(slide)
{
}

QString CustomSlideItem::displayName() const
{
    // Use the first line of text as the display name, or "Empty Slide" if blank
    QString text = m_slide.text().trimmed();
    if (text.isEmpty()) {
        return tr("Empty Slide");
    }

    // Get first line, truncate if too long
    int newlinePos = text.indexOf('\n');
    if (newlinePos > 0) {
        text = text.left(newlinePos);
    }

    const int maxLength = 40;
    if (text.length() > maxLength) {
        text = text.left(maxLength - 3) + "...";
    }

    return text;
}

QList<Slide> CustomSlideItem::generateSlides() const
{
    QList<Slide> slides;
    Slide slide = m_slide;

    // Apply custom style if set
    if (m_hasCustomStyle) {
        m_itemStyle.applyTo(slide);
    }

    slides.append(slide);
    return slides;
}

void CustomSlideItem::setSlide(const Slide& slide)
{
    m_slide = slide;
    invalidateSlideCache();
    emit itemChanged();
}

QJsonObject CustomSlideItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["slide"] = m_slide.toJson();
    return json;
}

CustomSlideItem* CustomSlideItem::fromJson(const QJsonObject& json)
{
    if (json["type"].toString() != "customSlide") {
        qWarning("CustomSlideItem::fromJson: type mismatch");
        return nullptr;
    }

    CustomSlideItem* item = new CustomSlideItem();
    item->applyBaseJson(json);

    if (json.contains("slide")) {
        item->m_slide = Slide::fromJson(json["slide"].toObject());
    }

    return item;
}

} // namespace Clarity
