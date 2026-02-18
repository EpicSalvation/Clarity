#include "EsvScriptureItem.h"
#include "SettingsManager.h"
#include <QJsonArray>

namespace Clarity {

EsvScriptureItem::EsvScriptureItem(QObject* parent)
    : PresentationItem(parent)
    , m_settingsManager(nullptr)
    , m_oneVersePerSlide(true)
    , m_includeVerseNumbers(true)
    , m_includeHeaderSlide(true)
    , m_isPurged(false)
{
}

EsvScriptureItem::EsvScriptureItem(const EsvPassage& passage, QObject* parent)
    : PresentationItem(parent)
    , m_reference(passage.canonical)
    , m_verses(passage.verses)
    , m_copyright(passage.copyright)
    , m_settingsManager(nullptr)
    , m_oneVersePerSlide(true)
    , m_includeVerseNumbers(true)
    , m_includeHeaderSlide(true)
    , m_isPurged(false)
{
}

QString EsvScriptureItem::displayName() const
{
    if (m_reference.isEmpty()) {
        return tr("ESV Scripture");
    }
    return m_reference;
}

QString EsvScriptureItem::displaySubtitle() const
{
    if (m_isPurged) {
        return tr("ESV (purged)");
    }
    return QStringLiteral("ESV");
}

QList<Slide> EsvScriptureItem::generateSlides() const
{
    QList<Slide> slides;

    if (m_isPurged) {
        Slide placeholder(tr("[ESV content purged]\n%1").arg(m_reference));
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(placeholder);
        }
        slides.append(placeholder);
        return slides;
    }

    if (m_verses.isEmpty()) {
        Slide errorSlide(tr("No ESV scripture loaded for: %1").arg(m_reference));
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
        QString headerText = QStringLiteral("%1 (ESV)").arg(m_reference);
        slides.append(createSlide(headerText));
    }

    if (m_oneVersePerSlide) {
        // Each verse gets its own slide
        for (const EsvVerse& verse : m_verses) {
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
        for (const EsvVerse& verse : m_verses) {
            if (m_includeVerseNumbers) {
                parts.append(QStringLiteral("%1 %2").arg(verse.number).arg(verse.text));
            } else {
                parts.append(verse.text);
            }
        }
        slides.append(createSlide(parts.join(" ")));
    }

    // Cascading backgrounds: ESV slides don't have explicit backgrounds by default.
    // If the item has a custom style, the first slide gets an explicit background.
    for (int i = 0; i < slides.count(); ++i) {
        slides[i].setHasExplicitBackground(m_hasCustomStyle && i == 0);
    }

    return slides;
}

void EsvScriptureItem::setReference(const QString& reference)
{
    if (m_reference != reference) {
        m_reference = reference;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void EsvScriptureItem::setVerses(const QList<EsvVerse>& verses)
{
    m_verses = verses;
    m_isPurged = false;
    invalidateSlideCache();
    emit itemChanged();
}

void EsvScriptureItem::setSettingsManager(SettingsManager* settings)
{
    m_settingsManager = settings;
}

void EsvScriptureItem::setOneVersePerSlide(bool onePerSlide)
{
    if (m_oneVersePerSlide != onePerSlide) {
        m_oneVersePerSlide = onePerSlide;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void EsvScriptureItem::setIncludeVerseNumbers(bool include)
{
    if (m_includeVerseNumbers != include) {
        m_includeVerseNumbers = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void EsvScriptureItem::setIncludeHeaderSlide(bool include)
{
    if (m_includeHeaderSlide != include) {
        m_includeHeaderSlide = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void EsvScriptureItem::purgeCache()
{
    m_verses.clear();
    m_isPurged = true;
    invalidateSlideCache();
    emit itemChanged();
    qDebug() << "EsvScriptureItem: Purged cached content for" << m_reference;
}

QJsonObject EsvScriptureItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["reference"] = m_reference;
    json["copyright"] = m_copyright;
    json["oneVersePerSlide"] = m_oneVersePerSlide;
    json["includeVerseNumbers"] = m_includeVerseNumbers;
    json["includeHeaderSlide"] = m_includeHeaderSlide;
    json["isPurged"] = m_isPurged;

    // Serialize cached verses (this is the ESV "cache" that must be purgeable)
    if (!m_isPurged) {
        QJsonArray versesArray;
        for (const EsvVerse& verse : m_verses) {
            QJsonObject verseJson;
            verseJson["number"] = verse.number;
            verseJson["text"] = verse.text;
            versesArray.append(verseJson);
        }
        json["verses"] = versesArray;
    }

    return json;
}

EsvScriptureItem* EsvScriptureItem::fromJson(const QJsonObject& json, SettingsManager* settings)
{
    if (json["type"].toString() != "esvScripture") {
        qWarning("EsvScriptureItem::fromJson: type mismatch");
        return nullptr;
    }

    EsvScriptureItem* item = new EsvScriptureItem();
    item->applyBaseJson(json);
    item->m_settingsManager = settings;
    item->m_reference = json["reference"].toString();
    item->m_copyright = json["copyright"].toString();
    item->m_oneVersePerSlide = json["oneVersePerSlide"].toBool(true);
    item->m_includeVerseNumbers = json["includeVerseNumbers"].toBool(true);
    item->m_includeHeaderSlide = json["includeHeaderSlide"].toBool(true);
    item->m_isPurged = json["isPurged"].toBool(false);

    // Deserialize cached verses
    if (!item->m_isPurged && json.contains("verses")) {
        QJsonArray versesArray = json["verses"].toArray();
        for (const QJsonValue& val : versesArray) {
            QJsonObject verseJson = val.toObject();
            EsvVerse verse;
            verse.number = verseJson["number"].toInt();
            verse.text = verseJson["text"].toString();
            item->m_verses.append(verse);
        }
    }

    return item;
}

} // namespace Clarity
