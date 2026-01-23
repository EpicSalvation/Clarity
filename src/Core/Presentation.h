#pragma once

#include "Slide.h"
#include <QList>
#include <QString>
#include <QJsonObject>

namespace Clarity {

/**
 * @brief Represents a complete presentation with multiple slides
 *
 * Manages slide collection and tracks current slide index
 */
class Presentation {
public:
    Presentation();
    explicit Presentation(const QString& title);

    // Getters
    QString title() const { return m_title; }
    QList<Slide> slides() const { return m_slides; }
    int currentSlideIndex() const { return m_currentSlideIndex; }
    int slideCount() const { return m_slides.count(); }

    // Setters
    void setTitle(const QString& title) { m_title = title; }
    void setCurrentSlideIndex(int index);

    // Slide management
    void addSlide(const Slide& slide);
    void insertSlide(int index, const Slide& slide);
    void removeSlide(int index);
    void clearSlides();

    // Navigation
    bool nextSlide();
    bool prevSlide();
    bool gotoSlide(int index);
    Slide currentSlide() const;

    // JSON serialization
    QJsonObject toJson() const;
    static Presentation fromJson(const QJsonObject& json);

private:
    QString m_title;
    QList<Slide> m_slides;
    int m_currentSlideIndex;
};

} // namespace Clarity
