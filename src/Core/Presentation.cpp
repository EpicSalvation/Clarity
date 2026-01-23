#include "Presentation.h"
#include <QJsonArray>
#include <QDateTime>

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

void Presentation::updateSlide(int index, const Slide& slide)
{
    if (index >= 0 && index < m_slides.count()) {
        m_slides[index] = slide;
    }
}

void Presentation::moveSlide(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_slides.count() ||
        toIndex < 0 || toIndex >= m_slides.count()) {
        return;
    }

    m_slides.move(fromIndex, toIndex);

    // Update current index if needed
    if (m_currentSlideIndex == fromIndex) {
        m_currentSlideIndex = toIndex;
    } else if (fromIndex < m_currentSlideIndex && toIndex >= m_currentSlideIndex) {
        m_currentSlideIndex--;
    } else if (fromIndex > m_currentSlideIndex && toIndex <= m_currentSlideIndex) {
        m_currentSlideIndex++;
    }
}

Slide Presentation::getSlide(int index) const
{
    if (index >= 0 && index < m_slides.count()) {
        return m_slides.at(index);
    }
    return Slide(); // Return default slide if invalid
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
    json["version"] = "1.0";
    json["title"] = m_title;
    json["currentSlideIndex"] = m_currentSlideIndex;
    json["createdDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json["modifiedDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);

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

    // Check version for future compatibility
    QString version = json["version"].toString("1.0");
    // For now, we only support version 1.0, but this allows future migration

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
