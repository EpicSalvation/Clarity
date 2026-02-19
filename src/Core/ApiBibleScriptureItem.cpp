#include "ApiBibleScriptureItem.h"
#include <QJsonArray>

namespace Clarity {

ApiBibleScriptureItem::ApiBibleScriptureItem(QObject* parent)
    : PresentationItem(parent)
    , m_oneVersePerSlide(true)
    , m_includeVerseNumbers(true)
    , m_includeHeaderSlide(true)
{
}

ApiBibleScriptureItem::ApiBibleScriptureItem(const ApiBiblePassage& passage, QObject* parent)
    : PresentationItem(parent)
    , m_reference(passage.reference)
    , m_verses(passage.verses)
    , m_copyright(passage.copyright)
    , m_bibleAbbreviation(passage.bibleAbbreviation)
    , m_oneVersePerSlide(true)
    , m_includeVerseNumbers(true)
    , m_includeHeaderSlide(true)
{
}

QString ApiBibleScriptureItem::displayName() const
{
    if (m_reference.isEmpty()) {
        return tr("API.bible Scripture");
    }
    return m_reference;
}

QString ApiBibleScriptureItem::displaySubtitle() const
{
    if (!m_bibleAbbreviation.isEmpty()) {
        return m_bibleAbbreviation;
    }
    return QStringLiteral("API.bible");
}

QList<Slide> ApiBibleScriptureItem::generateSlides() const
{
    QList<Slide> slides;

    if (m_verses.isEmpty()) {
        Slide errorSlide(tr("No scripture loaded for: %1").arg(m_reference));
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(errorSlide);
        }
        slides.append(errorSlide);
        return slides;
    }

    // Helper to create and style a slide
    auto createSlide = [this](const QString& text) -> Slide {
        Slide slide(text);
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(slide);
        }
        return slide;
    };

    // Add header slide if requested
    if (m_includeHeaderSlide) {
        QString headerText;
        if (!m_bibleAbbreviation.isEmpty()) {
            headerText = QStringLiteral("%1 (%2)").arg(m_reference, m_bibleAbbreviation);
        } else {
            headerText = m_reference;
        }
        slides.append(createSlide(headerText));
    }

    if (m_oneVersePerSlide) {
        // Each verse gets its own slide
        for (const ApiBibleVerse& verse : m_verses) {
            QString slideText;
            if (m_includeVerseNumbers) {
                slideText = QStringLiteral("%1 %2").arg(verse.number).arg(verse.text);
            } else {
                slideText = verse.text;
            }
            slides.append(createSlide(slideText));
        }
    } else {
        // All verses on one slide
        QStringList parts;
        for (const ApiBibleVerse& verse : m_verses) {
            if (m_includeVerseNumbers) {
                parts.append(QStringLiteral("%1 %2").arg(verse.number).arg(verse.text));
            } else {
                parts.append(verse.text);
            }
        }
        slides.append(createSlide(parts.join(" ")));
    }

    // Cascading backgrounds: slides don't have explicit backgrounds by default.
    // If the item has a custom style, the first slide gets an explicit background.
    for (int i = 0; i < slides.count(); ++i) {
        slides[i].setHasExplicitBackground(m_hasCustomStyle && i == 0);
    }

    return slides;
}

void ApiBibleScriptureItem::setReference(const QString& reference)
{
    if (m_reference != reference) {
        m_reference = reference;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ApiBibleScriptureItem::setVerses(const QList<ApiBibleVerse>& verses)
{
    m_verses = verses;
    invalidateSlideCache();
    emit itemChanged();
}

void ApiBibleScriptureItem::setBibleAbbreviation(const QString& abbr)
{
    if (m_bibleAbbreviation != abbr) {
        m_bibleAbbreviation = abbr;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ApiBibleScriptureItem::setOneVersePerSlide(bool onePerSlide)
{
    if (m_oneVersePerSlide != onePerSlide) {
        m_oneVersePerSlide = onePerSlide;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ApiBibleScriptureItem::setIncludeVerseNumbers(bool include)
{
    if (m_includeVerseNumbers != include) {
        m_includeVerseNumbers = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void ApiBibleScriptureItem::setIncludeHeaderSlide(bool include)
{
    if (m_includeHeaderSlide != include) {
        m_includeHeaderSlide = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

QJsonObject ApiBibleScriptureItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["reference"] = m_reference;
    json["copyright"] = m_copyright;
    json["bibleAbbreviation"] = m_bibleAbbreviation;
    json["oneVersePerSlide"] = m_oneVersePerSlide;
    json["includeVerseNumbers"] = m_includeVerseNumbers;
    json["includeHeaderSlide"] = m_includeHeaderSlide;

    // Serialize cached verses
    QJsonArray versesArray;
    for (const ApiBibleVerse& verse : m_verses) {
        QJsonObject verseJson;
        verseJson["number"] = verse.number;
        verseJson["text"] = verse.text;
        versesArray.append(verseJson);
    }
    json["verses"] = versesArray;

    return json;
}

ApiBibleScriptureItem* ApiBibleScriptureItem::fromJson(const QJsonObject& json)
{
    if (json["type"].toString() != "apiBibleScripture") {
        qWarning("ApiBibleScriptureItem::fromJson: type mismatch");
        return nullptr;
    }

    ApiBibleScriptureItem* item = new ApiBibleScriptureItem();
    item->applyBaseJson(json);
    item->m_reference = json["reference"].toString();
    item->m_copyright = json["copyright"].toString();
    item->m_bibleAbbreviation = json["bibleAbbreviation"].toString();
    item->m_oneVersePerSlide = json["oneVersePerSlide"].toBool(true);
    item->m_includeVerseNumbers = json["includeVerseNumbers"].toBool(true);
    item->m_includeHeaderSlide = json["includeHeaderSlide"].toBool(true);

    // Deserialize cached verses
    if (json.contains("verses")) {
        QJsonArray versesArray = json["verses"].toArray();
        for (const QJsonValue& val : versesArray) {
            QJsonObject verseJson = val.toObject();
            ApiBibleVerse verse;
            verse.number = verseJson["number"].toInt();
            verse.text = verseJson["text"].toString();
            item->m_verses.append(verse);
        }
    }

    return item;
}

} // namespace Clarity
