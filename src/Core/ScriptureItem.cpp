// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

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

    // Check scripture reference position setting
    bool refAtBottom = m_settingsManager
        && m_settingsManager->scriptureReferencePosition() == "bottom";

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

    // Helper to create a scripture template slide with reference in its own zone
    auto createScriptureSlide = [&](const QString& refText, const QString& bodyText,
                                    const QString& bodyRichText = QString()) -> Slide {
        Slide slide;
        if (m_hasCustomStyle) {
            m_itemStyle.applyTo(slide);
        }
        slide.setSlideTemplate(SlideTemplate::Scripture);
        auto zones = Slide::createTemplateZones(SlideTemplate::Scripture, refAtBottom);
        // Populate zones: reference zone first, then body zone
        for (auto& zone : zones) {
            if (zone.id == "reference") {
                zone.text = refText;
                // Inherit slide-level styling for the zone
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
        QString headerText = m_reference;
        if (!m_translation.isEmpty()) {
            headerText += QString(" (%1)").arg(m_translation);
        }
        slides.append(createSlide(headerText));
    }

    if (m_oneVersePerSlide) {
        // Each verse gets its own slide
        for (const BibleVerse& verse : verses) {
            QString bodyText;
            QString bodyRichText;

            if (m_includeVerseReferences) {
                // Verse number prefix on body text
                bodyText = QString("%1 %2").arg(verse.verse).arg(verse.text);
                if (useRedLetters && !verse.richText.isEmpty()) {
                    bodyRichText = QString("%1 %2").arg(verse.verse).arg(verse.richText);
                }
                // Reference in its own zone
                QString refText = verse.fullReference();
                slides.append(createScriptureSlide(refText, bodyText, bodyRichText));
            } else {
                bodyText = verse.text;
                if (useRedLetters && !verse.richText.isEmpty()) {
                    bodyRichText = verse.richText;
                }
                slides.append(createSlide(bodyText, bodyRichText));
            }
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

        QString richTextStr = (useRedLetters && hasAnyRichText) ? combinedRichText : QString();

        if (m_includeVerseReferences && !verses.isEmpty()) {
            // Build reference string for the range
            const BibleVerse& first = verses.first();
            const BibleVerse& last = verses.last();
            QString refStr;
            if (verses.count() == 1) {
                refStr = first.fullReference();
            } else if (first.book == last.book && first.chapter == last.chapter) {
                refStr = QString("%1 %2:%3-%4 (%5)")
                    .arg(first.book).arg(first.chapter).arg(first.verse)
                    .arg(last.verse).arg(first.translation);
            } else if (first.book == last.book) {
                refStr = QString("%1 %2:%3-%4:%5 (%6)")
                    .arg(first.book).arg(first.chapter).arg(first.verse)
                    .arg(last.chapter).arg(last.verse).arg(first.translation);
            } else {
                refStr = QString("%1 - %2").arg(first.fullReference(), last.fullReference());
            }
            slides.append(createScriptureSlide(refStr, combinedText, richTextStr));
        } else {
            slides.append(createSlide(combinedText, richTextStr));
        }
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
