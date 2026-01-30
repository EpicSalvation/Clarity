#include "ScriptureItem.h"

namespace Clarity {

ScriptureItem::ScriptureItem(QObject* parent)
    : PresentationItem(parent)
    , m_bibleDatabase(nullptr)
    , m_oneVersePerSlide(true)
    , m_includeVerseReferences(true)
    , m_includeHeaderSlide(true)
{
}

ScriptureItem::ScriptureItem(const QString& reference, const QString& translation,
                             BibleDatabase* database, QObject* parent)
    : PresentationItem(parent)
    , m_reference(reference)
    , m_translation(translation)
    , m_bibleDatabase(database)
    , m_oneVersePerSlide(true)
    , m_includeVerseReferences(true)
    , m_includeHeaderSlide(true)
{
}

QString ScriptureItem::displayName() const
{
    if (m_reference.isEmpty()) {
        return tr("Scripture");
    }
    return m_reference;
}

QString ScriptureItem::displaySubtitle() const
{
    if (!m_translation.isEmpty()) {
        return m_translation;
    }
    return QString();
}

QList<Slide> ScriptureItem::generateSlides() const
{
    QList<Slide> slides;

    if (!m_bibleDatabase || !m_bibleDatabase->isValid()) {
        // No database - return a placeholder slide
        Slide errorSlide(tr("Bible database not available"));
        if (m_hasCustomStyle) {
            errorSlide.setBackgroundColor(m_itemStyle.backgroundColor);
            errorSlide.setTextColor(m_itemStyle.textColor);
            errorSlide.setFontFamily(m_itemStyle.fontFamily);
            errorSlide.setFontSize(m_itemStyle.fontSize);
        }
        slides.append(errorSlide);
        return slides;
    }

    if (m_reference.isEmpty()) {
        Slide errorSlide(tr("No scripture reference specified"));
        if (m_hasCustomStyle) {
            errorSlide.setBackgroundColor(m_itemStyle.backgroundColor);
            errorSlide.setTextColor(m_itemStyle.textColor);
            errorSlide.setFontFamily(m_itemStyle.fontFamily);
            errorSlide.setFontSize(m_itemStyle.fontSize);
        }
        slides.append(errorSlide);
        return slides;
    }

    // Set translation if specified
    if (!m_translation.isEmpty()) {
        // Note: BibleDatabase uses a default translation; we might need to
        // temporarily set it or add a method that takes translation as parameter
        // For now, we work with the database's current default
    }

    // Look up the verses
    QList<BibleVerse> verses = m_bibleDatabase->lookupReference(m_reference);

    if (verses.isEmpty()) {
        Slide errorSlide(tr("Scripture not found: %1").arg(m_reference));
        if (m_hasCustomStyle) {
            errorSlide.setBackgroundColor(m_itemStyle.backgroundColor);
            errorSlide.setTextColor(m_itemStyle.textColor);
            errorSlide.setFontFamily(m_itemStyle.fontFamily);
            errorSlide.setFontSize(m_itemStyle.fontSize);
        }
        slides.append(errorSlide);
        return slides;
    }

    // Helper to create and style a slide
    auto createSlide = [this](const QString& text) -> Slide {
        Slide slide(text);
        if (m_hasCustomStyle) {
            slide.setBackgroundColor(m_itemStyle.backgroundColor);
            slide.setTextColor(m_itemStyle.textColor);
            slide.setFontFamily(m_itemStyle.fontFamily);
            slide.setFontSize(m_itemStyle.fontSize);
        }
        return slide;
    };

    // Add header slide if requested
    if (m_includeHeaderSlide) {
        QString headerText = m_reference;
        if (!m_translation.isEmpty()) {
            headerText += QString(" (%1)").arg(m_translation);
        }
        slides.append(createSlide(headerText));
    }

    if (m_oneVersePerSlide) {
        // Each verse gets its own slide
        for (const BibleVerse& verse : verses) {
            QString slideText;
            if (m_includeVerseReferences) {
                // Format: "16 For God so loved the world..."
                slideText = QString("%1 %2").arg(verse.verse).arg(verse.text);
            } else {
                slideText = verse.text;
            }
            slides.append(createSlide(slideText));
        }
    } else {
        // All verses on one slide
        QString combinedText;
        for (const BibleVerse& verse : verses) {
            if (!combinedText.isEmpty()) {
                combinedText += " ";
            }
            if (m_includeVerseReferences) {
                combinedText += QString("%1 %2").arg(verse.verse).arg(verse.text);
            } else {
                combinedText += verse.text;
            }
        }
        slides.append(createSlide(combinedText));
    }

    return slides;
}

void ScriptureItem::setReference(const QString& reference)
{
    if (m_reference != reference) {
        m_reference = reference;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ScriptureItem::setTranslation(const QString& translation)
{
    if (m_translation != translation) {
        m_translation = translation;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ScriptureItem::setBibleDatabase(BibleDatabase* database)
{
    if (m_bibleDatabase != database) {
        m_bibleDatabase = database;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ScriptureItem::setOneVersePerSlide(bool onePerSlide)
{
    if (m_oneVersePerSlide != onePerSlide) {
        m_oneVersePerSlide = onePerSlide;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ScriptureItem::setIncludeVerseReferences(bool include)
{
    if (m_includeVerseReferences != include) {
        m_includeVerseReferences = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ScriptureItem::setIncludeHeaderSlide(bool include)
{
    if (m_includeHeaderSlide != include) {
        m_includeHeaderSlide = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

QJsonObject ScriptureItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["reference"] = m_reference;
    json["translation"] = m_translation;
    json["oneVersePerSlide"] = m_oneVersePerSlide;
    json["includeVerseReferences"] = m_includeVerseReferences;
    json["includeHeaderSlide"] = m_includeHeaderSlide;
    return json;
}

ScriptureItem* ScriptureItem::fromJson(const QJsonObject& json, BibleDatabase* database)
{
    if (json["type"].toString() != "scripture") {
        qWarning("ScriptureItem::fromJson: type mismatch");
        return nullptr;
    }

    ScriptureItem* item = new ScriptureItem();
    item->applyBaseJson(json);
    item->m_reference = json["reference"].toString();
    item->m_translation = json["translation"].toString();
    item->m_oneVersePerSlide = json["oneVersePerSlide"].toBool(true);
    item->m_includeVerseReferences = json["includeVerseReferences"].toBool(true);
    item->m_includeHeaderSlide = json["includeHeaderSlide"].toBool(true);
    item->m_bibleDatabase = database;

    return item;
}

} // namespace Clarity
