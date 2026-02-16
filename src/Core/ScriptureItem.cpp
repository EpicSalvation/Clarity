#include "ScriptureItem.h"
#include "SettingsManager.h"

namespace Clarity {

ScriptureItem::ScriptureItem(QObject* parent)
    : PresentationItem(parent)
    , m_bibleDatabase(nullptr)
    , m_settingsManager(nullptr)
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
    , m_settingsManager(nullptr)
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
            m_itemStyle.applyTo(errorSlide);
        }
        slides.append(errorSlide);
        return slides;
    }

    if (m_reference.isEmpty()) {
        Slide errorSlide(tr("No scripture reference specified"));
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(errorSlide);
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
            m_itemStyle.applyTo(errorSlide);
        }
        slides.append(errorSlide);
        return slides;
    }

    // Check if red letters are enabled
    bool useRedLetters = m_settingsManager ? m_settingsManager->redLettersEnabled() : false;

    // Helper to create and style a slide
    auto createSlide = [this](const QString& text, const QString& richText = QString()) -> Slide {
        Slide slide(text);
        if (!richText.isEmpty()) {
            slide.setRichText(richText);
        }
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(slide);
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
            QString slideRichText;

            if (m_includeVerseReferences) {
                // Format: "16 For God so loved the world..."
                slideText = QString("%1 %2").arg(verse.verse).arg(verse.text);
                // For rich text, prepend verse number outside the red letter markup
                if (useRedLetters && !verse.richText.isEmpty()) {
                    slideRichText = QString("%1 %2").arg(verse.verse).arg(verse.richText);
                }
            } else {
                slideText = verse.text;
                if (useRedLetters && !verse.richText.isEmpty()) {
                    slideRichText = verse.richText;
                }
            }
            slides.append(createSlide(slideText, slideRichText));
        }
    } else {
        // All verses on one slide
        QString combinedText;
        QString combinedRichText;
        bool hasAnyRichText = false;

        for (const BibleVerse& verse : verses) {
            if (!combinedText.isEmpty()) {
                combinedText += " ";
                combinedRichText += " ";
            }
            if (m_includeVerseReferences) {
                combinedText += QString("%1 %2").arg(verse.verse).arg(verse.text);
                if (!verse.richText.isEmpty()) {
                    combinedRichText += QString("%1 %2").arg(verse.verse).arg(verse.richText);
                    hasAnyRichText = true;
                } else {
                    combinedRichText += QString("%1 %2").arg(verse.verse).arg(verse.text.toHtmlEscaped());
                }
            } else {
                combinedText += verse.text;
                if (!verse.richText.isEmpty()) {
                    combinedRichText += verse.richText;
                    hasAnyRichText = true;
                } else {
                    combinedRichText += verse.text.toHtmlEscaped();
                }
            }
        }
        // Only use rich text if red letters are enabled and at least one verse has markup
        slides.append(createSlide(combinedText, (useRedLetters && hasAnyRichText) ? combinedRichText : QString()));
    }

    // Cascading backgrounds: scripture slides don't have explicit backgrounds by default.
    // If the item has a custom style, the first slide is explicit (it's the user's choice).
    for (int i = 0; i < slides.count(); ++i) {
        slides[i].setHasExplicitBackground(m_hasCustomStyle && i == 0);
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

void ScriptureItem::setSettingsManager(SettingsManager* settings)
{
    if (m_settingsManager != settings) {
        m_settingsManager = settings;
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

ScriptureItem* ScriptureItem::fromJson(const QJsonObject& json, BibleDatabase* database, SettingsManager* settings)
{
    if (json["type"].toString() != "scripture") {
        qWarning("ScriptureItem::fromJson: type mismatch");
        return nullptr;
    }

    ScriptureItem* item = new ScriptureItem();
    item->applyBaseJson(json);
    item->m_settingsManager = settings;
    item->m_reference = json["reference"].toString();
    item->m_translation = json["translation"].toString();
    item->m_oneVersePerSlide = json["oneVersePerSlide"].toBool(true);
    item->m_includeVerseReferences = json["includeVerseReferences"].toBool(true);
    item->m_includeHeaderSlide = json["includeHeaderSlide"].toBool(true);
    item->m_bibleDatabase = database;

    return item;
}

} // namespace Clarity
