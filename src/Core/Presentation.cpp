#include "Presentation.h"
#include <QJsonArray>

namespace Clarity {

Presentation::Presentation()
    : m_title("Untitled")
    , m_currentSlideIndex(0)
{
}

Presentation::Presentation(const QString& title)
    : m_title(title)
    , m_currentSlideIndex(0)
{
}

void Presentation::setCurrentSlideIndex(int index)
{
    if (index >= 0 && index < m_slides.count()) {
        m_currentSlideIndex = index;
    }
}

void Presentation::addSlide(const Slide& slide)
{
    m_slides.append(slide);
}

void Presentation::insertSlide(int index, const Slide& slide)
{
    if (index >= 0 && index <= m_slides.count()) {
        m_slides.insert(index, slide);
    }
}

void Presentation::removeSlide(int index)
{
    if (index >= 0 && index < m_slides.count()) {
        m_slides.removeAt(index);
        // Adjust current index if necessary
        if (m_currentSlideIndex >= m_slides.count() && m_currentSlideIndex > 0) {
            m_currentSlideIndex = m_slides.count() - 1;
        }
    }
}

void Presentation::clearSlides()
{
    m_slides.clear();
    m_currentSlideIndex = 0;
}

bool Presentation::nextSlide()
{
    if (m_currentSlideIndex < m_slides.count() - 1) {
        m_currentSlideIndex++;
        return true;
    }
    return false;
}

bool Presentation::prevSlide()
{
    if (m_currentSlideIndex > 0) {
        m_currentSlideIndex--;
        return true;
    }
    return false;
}

bool Presentation::gotoSlide(int index)
{
    if (index >= 0 && index < m_slides.count()) {
        m_currentSlideIndex = index;
        return true;
    }
    return false;
}

Slide Presentation::currentSlide() const
{
    if (m_currentSlideIndex >= 0 && m_currentSlideIndex < m_slides.count()) {
        return m_slides.at(m_currentSlideIndex);
    }
    return Slide(); // Return empty slide if index is invalid
}

QJsonObject Presentation::toJson() const
{
    QJsonObject json;
    json["title"] = m_title;
    json["currentSlideIndex"] = m_currentSlideIndex;

    QJsonArray slidesArray;
    for (const Slide& slide : m_slides) {
        slidesArray.append(slide.toJson());
    }
    json["slides"] = slidesArray;

    return json;
}

Presentation Presentation::fromJson(const QJsonObject& json)
{
    Presentation presentation;
    presentation.m_title = json["title"].toString("Untitled");
    presentation.m_currentSlideIndex = json["currentSlideIndex"].toInt(0);

    QJsonArray slidesArray = json["slides"].toArray();
    for (const QJsonValue& value : slidesArray) {
        presentation.m_slides.append(Slide::fromJson(value.toObject()));
    }

    // Validate current index
    if (presentation.m_currentSlideIndex >= presentation.m_slides.count()) {
        presentation.m_currentSlideIndex = 0;
    }

    return presentation;
}

} // namespace Clarity
