// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "ApiBibleScriptureItem.h"
#include "SettingsManager.h"
#include <QJsonArray>

namespace Clarity {

ApiBibleScriptureItem::ApiBibleScriptureItem(QObject* parent)
    : PresentationItem(parent)
    , m_oneVersePerSlide(true)
    , m_includeVerseNumbers(true)
    , m_includeVerseReferences(false)
    , m_includeHeaderSlide(true)
    , m_settingsManager(nullptr)
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
    , m_includeVerseReferences(false)
    , m_includeHeaderSlide(true)
    , m_settingsManager(nullptr)
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

    bool useRedLetters = m_settingsManager ? m_settingsManager->redLettersEnabled() : false;

    // Check scripture reference position setting
    bool refAtBottom = m_settingsManager
        && m_settingsManager->scriptureReferencePosition() == "bottom";

    // Translation label for reference display
    QString translationLabel = m_bibleAbbreviation.isEmpty()
        ? QStringLiteral("API.bible") : m_bibleAbbreviation;

    // Helper to create and style a slide (with optional rich text for red letters)
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

    // Helper to create a scripture template slide with reference in its own zone
    auto createScriptureSlide = [&](const QString& refText, const QString& bodyText,
                                    const QString& bodyRichText = QString()) -> Slide {
        Slide slide;
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(slide);
        }
        slide.setSlideTemplate(SlideTemplate::Scripture);
        auto zones = Slide::createTemplateZones(SlideTemplate::Scripture, refAtBottom);
        for (auto& zone : zones) {
            if (zone.id == "reference") {
                zone.text = refText;
                zone.textColor = slide.textColor();
                zone.fontFamily = slide.fontFamily();
            } else if (zone.id == "body") {
                zone.text = bodyText;
                if (!bodyRichText.isEmpty()) {
                    zone.richText = bodyRichText;
                }
                zone.textColor = slide.textColor();
                zone.fontFamily = slide.fontFamily();
                zone.fontSize = slide.fontSize();
            }
        }
        slide.setTextZones(zones);
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
            QString bodyText;
            QString bodyRichText;

            if (m_includeVerseNumbers) {
                bodyText = QStringLiteral("%1 %2").arg(verse.number).arg(verse.text);
                if (useRedLetters && !verse.richText.isEmpty()) {
                    bodyRichText = QStringLiteral("%1 %2").arg(verse.number).arg(verse.richText);
                }
            } else {
                bodyText = verse.text;
                if (useRedLetters && !verse.richText.isEmpty()) {
                    bodyRichText = verse.richText;
                }
            }

            if (m_includeVerseReferences) {
                QString refText = QStringLiteral("%1:%2 (%3)")
                    .arg(m_reference, QString::number(verse.number), translationLabel);
                slides.append(createScriptureSlide(refText, bodyText, bodyRichText));
            } else {
                slides.append(createSlide(bodyText, bodyRichText));
            }
        }
    } else {
        // All verses on one slide
        QString combinedText;
        QString combinedRichText;
        bool hasAnyRichText = false;

        for (const ApiBibleVerse& verse : m_verses) {
            if (!combinedText.isEmpty()) {
                combinedText += " ";
                combinedRichText += " ";
            }
            if (m_includeVerseNumbers) {
                combinedText += QStringLiteral("%1 %2").arg(verse.number).arg(verse.text);
                if (!verse.richText.isEmpty()) {
                    combinedRichText += QStringLiteral("%1 %2").arg(verse.number).arg(verse.richText);
                    hasAnyRichText = true;
                } else {
                    combinedRichText += QStringLiteral("%1 %2").arg(verse.number).arg(verse.text.toHtmlEscaped());
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

        QString richTextStr = (useRedLetters && hasAnyRichText) ? combinedRichText : QString();

        if (m_includeVerseReferences) {
            QString refText = QStringLiteral("%1 (%2)").arg(m_reference, translationLabel);
            slides.append(createScriptureSlide(refText, combinedText, richTextStr));
        } else {
            slides.append(createSlide(combinedText, richTextStr));
        }
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

void ApiBibleScriptureItem::setIncludeVerseReferences(bool include)
{
    if (m_includeVerseReferences != include) {
        m_includeVerseReferences = include;
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

void ApiBibleScriptureItem::setSettingsManager(SettingsManager* settings)
{
    m_settingsManager = settings;
}

QJsonObject ApiBibleScriptureItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["reference"] = m_reference;
    json["copyright"] = m_copyright;
    json["bibleAbbreviation"] = m_bibleAbbreviation;
    json["oneVersePerSlide"] = m_oneVersePerSlide;
    json["includeVerseNumbers"] = m_includeVerseNumbers;
    json["includeVerseReferences"] = m_includeVerseReferences;
    json["includeHeaderSlide"] = m_includeHeaderSlide;

    // Serialize cached verses
    QJsonArray versesArray;
    for (const ApiBibleVerse& verse : m_verses) {
        QJsonObject verseJson;
        verseJson["number"] = verse.number;
        verseJson["text"] = verse.text;
        if (!verse.richText.isEmpty()) {
            verseJson["richText"] = verse.richText;
        }
        versesArray.append(verseJson);
    }
    json["verses"] = versesArray;

    return json;
}

ApiBibleScriptureItem* ApiBibleScriptureItem::fromJson(const QJsonObject& json, SettingsManager* settings)
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
    item->m_includeVerseReferences = json["includeVerseReferences"].toBool(false);
    item->m_includeHeaderSlide = json["includeHeaderSlide"].toBool(true);
    item->m_settingsManager = settings;

    // Deserialize cached verses
    if (json.contains("verses")) {
        QJsonArray versesArray = json["verses"].toArray();
        for (const QJsonValue& val : versesArray) {
            QJsonObject verseJson = val.toObject();
            ApiBibleVerse verse;
            verse.number = verseJson["number"].toInt();
            verse.text = verseJson["text"].toString();
            verse.richText = verseJson["richText"].toString();
            item->m_verses.append(verse);
        }
    }

    return item;
}

} // namespace Clarity
