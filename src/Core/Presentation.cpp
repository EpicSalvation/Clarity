// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "Presentation.h"
#include "CustomSlideItem.h"
#include "SlideGroupItem.h"
#include "SongItem.h"
#include "ScriptureItem.h"
#include "EsvScriptureItem.h"
#include "ApiBibleScriptureItem.h"
#include "SettingsManager.h"
#include "ThemeManager.h"
#include "Theme.h"
#include <QJsonArray>
#include <QDateTime>
#include <algorithm>

namespace Clarity {

Presentation::Presentation(QObject* parent)
    : QObject(parent)
    , m_title("Untitled")
    , m_currentSlideIndex(0)
    , m_cachedTotalSlides(0)
    , m_flatIndexValid(false)
{
}

Presentation::Presentation(const QString& title, QObject* parent)
    : QObject(parent)
    , m_title(title)
    , m_currentSlideIndex(0)
    , m_cachedTotalSlides(0)
    , m_flatIndexValid(false)
{
}

Presentation::~Presentation()
{
    // Items are QObject children, so they'll be deleted automatically
    // But let's be explicit for clarity
    qDeleteAll(m_items);
    m_items.clear();
}

void Presentation::setTitle(const QString& title)
{
    if (m_title != title) {
        m_title = title;
        emit presentationModified();
    }
}

// === Item management ===

PresentationItem* Presentation::itemAt(int index) const
{
    if (index >= 0 && index < m_items.count()) {
        return m_items.at(index);
    }
    return nullptr;
}

void Presentation::addItem(PresentationItem* item)
{
    if (!item) return;

    item->setParent(this);
    connectItemSignals(item);
    m_items.append(item);
    m_flatIndexValid = false;

    emit itemAdded(m_items.count() - 1);
    emit itemsChanged();
    emit slidesChanged();
    emit presentationModified();
}

void Presentation::insertItem(int index, PresentationItem* item)
{
    if (!item) return;
    if (index < 0) index = 0;
    if (index > m_items.count()) index = m_items.count();

    item->setParent(this);
    connectItemSignals(item);
    m_items.insert(index, item);
    m_flatIndexValid = false;

    // Adjust current slide index if needed
    int insertedSlides = item->slideCount();
    if (insertedSlides > 0) {
        rebuildFlatIndex();
        int insertFlatIndex = (index < m_itemStartIndices.count())
            ? m_itemStartIndices[index] - insertedSlides
            : m_cachedTotalSlides - insertedSlides;

        if (m_currentSlideIndex >= insertFlatIndex) {
            m_currentSlideIndex += insertedSlides;
        }
    }

    emit itemAdded(index);
    emit itemsChanged();
    emit slidesChanged();
    emit presentationModified();
}

void Presentation::removeItem(int index)
{
    if (index < 0 || index >= m_items.count()) return;

    PresentationItem* item = m_items.at(index);
    int removedSlides = item->slideCount();

    // Calculate flat index of removed item's first slide
    rebuildFlatIndex();
    int removedFlatIndex = (index < m_itemStartIndices.count())
        ? m_itemStartIndices[index] : 0;

    m_items.removeAt(index);
    m_flatIndexValid = false;

    // Adjust current slide index
    if (removedSlides > 0) {
        if (m_currentSlideIndex >= removedFlatIndex + removedSlides) {
            // Current slide was after removed slides
            m_currentSlideIndex -= removedSlides;
        } else if (m_currentSlideIndex >= removedFlatIndex) {
            // Current slide was within removed slides
            m_currentSlideIndex = removedFlatIndex;
            // Clamp to valid range
            rebuildFlatIndex();
            if (m_currentSlideIndex >= m_cachedTotalSlides && m_cachedTotalSlides > 0) {
                m_currentSlideIndex = m_cachedTotalSlides - 1;
            } else if (m_cachedTotalSlides == 0) {
                m_currentSlideIndex = 0;
            }
        }
    }

    delete item;

    emit itemRemoved(index);
    emit itemsChanged();
    emit slidesChanged();
    emit presentationModified();
}

void Presentation::moveItem(int from, int to)
{
    if (from < 0 || from >= m_items.count() ||
        to < 0 || to >= m_items.count() ||
        from == to) {
        return;
    }

    // Save current slide position info before move
    SlidePosition currentPos = positionForFlatIndex(m_currentSlideIndex);

    m_items.move(from, to);
    m_flatIndexValid = false;

    // Update current slide index to follow the same slide
    if (currentPos.isValid()) {
        // Adjust item index if it was affected by the move
        int newItemIndex = currentPos.itemIndex;
        if (currentPos.itemIndex == from) {
            newItemIndex = to;
        } else if (from < currentPos.itemIndex && to >= currentPos.itemIndex) {
            newItemIndex--;
        } else if (from > currentPos.itemIndex && to <= currentPos.itemIndex) {
            newItemIndex++;
        }
        m_currentSlideIndex = flatIndexForPosition(newItemIndex, currentPos.slideInItem);
    }

    emit itemMoved(from, to);
    emit itemsChanged();
    emit slidesChanged();
    emit presentationModified();
}

void Presentation::clearItems()
{
    if (m_items.isEmpty()) return;

    qDeleteAll(m_items);
    m_items.clear();
    m_currentSlideIndex = 0;
    m_flatIndexValid = false;
    m_itemStartIndices.clear();
    m_cachedTotalSlides = 0;

    emit itemsChanged();
    emit slidesChanged();
    emit presentationModified();
}

// === Flat slide access ===

void Presentation::rebuildFlatIndex() const
{
    if (m_flatIndexValid) return;

    m_itemStartIndices.clear();
    m_itemStartIndices.reserve(m_items.count());

    int flatIndex = 0;
    for (PresentationItem* item : m_items) {
        m_itemStartIndices.append(flatIndex);
        flatIndex += item->slideCount();
    }
    m_cachedTotalSlides = flatIndex;
    m_flatIndexValid = true;
}

int Presentation::totalSlideCount() const
{
    rebuildFlatIndex();
    return m_cachedTotalSlides + (hasCopyrightSlide() ? 1 : 0);
}

Slide Presentation::slideAt(int flatIndex) const
{
    rebuildFlatIndex();
    // Check if this is the virtual copyright slide
    if (hasCopyrightSlide() && flatIndex == m_cachedTotalSlides) {
        return generateCopyrightSlide();
    }

    SlidePosition pos = positionForFlatIndex(flatIndex);
    if (!pos.isValid()) {
        return Slide();
    }

    PresentationItem* item = m_items.at(pos.itemIndex);
    QList<Slide> slides = item->cachedSlides();
    if (pos.slideInItem < slides.count()) {
        return slides.at(pos.slideInItem);
    }
    return Slide();
}

// Copy all background properties from source to target
static void copyBackgroundTo(const Slide& source, Slide& target)
{
    target.setBackgroundType(source.backgroundType());
    target.setBackgroundColor(source.backgroundColor());
    target.setBackgroundImagePath(source.backgroundImagePath());
    target.setBackgroundImageData(source.backgroundImageData());
    target.setGradientStops(source.gradientStops());
    target.setGradientType(source.gradientType());
    target.setGradientAngle(source.gradientAngle());
    target.setRadialCenterX(source.radialCenterX());
    target.setRadialCenterY(source.radialCenterY());
    target.setRadialRadius(source.radialRadius());
    target.setBackgroundVideoPath(source.backgroundVideoPath());
    target.setVideoLoop(source.videoLoop());
}

Slide Presentation::resolvedSlideAt(int flatIndex) const
{
    rebuildFlatIndex();
    // Copyright slide has an explicit background — no cascading needed
    if (hasCopyrightSlide() && flatIndex == m_cachedTotalSlides) {
        return generateCopyrightSlide();
    }

    Slide slide = slideAt(flatIndex);
    if (!m_settingsManager) {
        return slide;
    }

    // Scripture theme override: if enabled, apply chosen theme to all scripture slides
    if (m_settingsManager->scriptureThemeOverride() && m_themeManager) {
        SlidePosition pos = positionForFlatIndex(flatIndex);
        if (pos.isValid()) {
            PresentationItem* item = m_items.at(pos.itemIndex);
            if (qobject_cast<ScriptureItem*>(item)) {
                QString themeName = m_settingsManager->scriptureThemeOverrideName();
                Theme theme;
                if (!themeName.isEmpty() && m_themeManager->hasTheme(themeName)) {
                    theme = m_themeManager->getTheme(themeName);
                } else {
                    // Fall back to first available theme
                    QList<Theme> all = m_themeManager->allThemes();
                    if (!all.isEmpty()) {
                        theme = all.first();
                    }
                }
                if (!theme.name().isEmpty()) {
                    theme.applyToSlide(slide);
                    // Scripture-overridden slides do NOT feed the cascade
                    slide.setHasExplicitBackground(false);
                }
                return slide;
            }
        }
    }

    // Cascading backgrounds: if enabled and slide has no explicit background,
    // walk backward to find the nearest slide with an explicit background
    if (m_settingsManager->cascadingBackgrounds() && !slide.hasExplicitBackground()) {
        for (int i = flatIndex - 1; i >= 0; --i) {
            Slide prev = slideAt(i);
            if (prev.hasExplicitBackground()) {
                copyBackgroundTo(prev, slide);
                return slide;
            }
        }
    }

    return slide;
}

SlidePosition Presentation::positionForFlatIndex(int flatIndex) const
{
    rebuildFlatIndex();

    if (flatIndex < 0 || flatIndex >= m_cachedTotalSlides || m_items.isEmpty()) {
        return SlidePosition();
    }

    // Binary search to find the item containing this flat index
    // m_itemStartIndices[i] is the flat index of item i's first slide
    auto it = std::upper_bound(m_itemStartIndices.begin(), m_itemStartIndices.end(), flatIndex);
    int itemIndex = static_cast<int>(it - m_itemStartIndices.begin()) - 1;

    if (itemIndex < 0) itemIndex = 0;
    if (itemIndex >= m_items.count()) itemIndex = m_items.count() - 1;

    int slideInItem = flatIndex - m_itemStartIndices[itemIndex];
    return SlidePosition(itemIndex, slideInItem, flatIndex);
}

int Presentation::flatIndexForPosition(int itemIndex, int slideInItem) const
{
    rebuildFlatIndex();

    if (itemIndex < 0 || itemIndex >= m_items.count()) {
        return -1;
    }

    int flatIndex = m_itemStartIndices[itemIndex] + slideInItem;
    if (flatIndex >= m_cachedTotalSlides) {
        return -1;
    }
    return flatIndex;
}

// === Navigation ===

void Presentation::setCurrentSlideIndex(int index)
{
    int total = totalSlideCount();
    if (index >= 0 && index < total) {
        if (m_currentSlideIndex != index) {
            m_currentSlideIndex = index;
            emit currentSlideChanged(index);
        }
    }
}

bool Presentation::nextSlide()
{
    int total = totalSlideCount();
    if (m_currentSlideIndex < total - 1) {
        m_currentSlideIndex++;
        emit currentSlideChanged(m_currentSlideIndex);
        return true;
    }
    return false;
}

bool Presentation::prevSlide()
{
    if (m_currentSlideIndex > 0) {
        m_currentSlideIndex--;
        emit currentSlideChanged(m_currentSlideIndex);
        return true;
    }
    return false;
}

bool Presentation::gotoSlide(int index)
{
    int total = totalSlideCount();
    if (index >= 0 && index < total) {
        if (m_currentSlideIndex != index) {
            m_currentSlideIndex = index;
            emit currentSlideChanged(m_currentSlideIndex);
        }
        return true;
    }
    return false;
}

Slide Presentation::currentSlide() const
{
    return slideAt(m_currentSlideIndex);
}

// === Current item info ===

PresentationItem* Presentation::currentItem() const
{
    SlidePosition pos = positionForFlatIndex(m_currentSlideIndex);
    if (pos.isValid() && pos.itemIndex < m_items.count()) {
        return m_items.at(pos.itemIndex);
    }
    return nullptr;
}

int Presentation::currentSlideInItem() const
{
    SlidePosition pos = positionForFlatIndex(m_currentSlideIndex);
    return pos.isValid() ? pos.slideInItem : -1;
}

int Presentation::slidesInCurrentItem() const
{
    PresentationItem* item = currentItem();
    return item ? item->slideCount() : 0;
}

// === Legacy flat slide API ===

QList<Slide> Presentation::slides() const
{
    QList<Slide> allSlides;
    for (PresentationItem* item : m_items) {
        allSlides.append(item->cachedSlides());
    }
    return allSlides;
}

void Presentation::addSlide(const Slide& slide)
{
    // Wrap single slide in CustomSlideItem
    auto* item = new CustomSlideItem(slide, this);
    addItem(item);
}

void Presentation::insertSlide(int flatIndex, const Slide& slide)
{
    rebuildFlatIndex();

    // Find where this flat index falls
    SlidePosition pos = positionForFlatIndex(flatIndex);

    // Create a new CustomSlideItem
    auto* newItem = new CustomSlideItem(slide, this);

    if (!pos.isValid() || m_items.isEmpty()) {
        // Insert at end
        addItem(newItem);
    } else if (pos.slideInItem == 0) {
        // Insert before this item
        insertItem(pos.itemIndex, newItem);
    } else {
        // Insert after this item
        insertItem(pos.itemIndex + 1, newItem);
    }
}

void Presentation::updateSlide(int flatIndex, const Slide& slide)
{
    SlidePosition pos = positionForFlatIndex(flatIndex);
    if (!pos.isValid()) return;

    PresentationItem* item = m_items.at(pos.itemIndex);

    // Handle different item types
    if (auto* customItem = qobject_cast<CustomSlideItem*>(item)) {
        customItem->setSlide(slide);
    } else if (auto* groupItem = qobject_cast<SlideGroupItem*>(item)) {
        // Bake group style into individual slides so this change sticks
        if (groupItem->hasCustomStyle()) {
            groupItem->bakeCustomStyle();
        }
        groupItem->updateSlide(pos.slideInItem, slide);
    } else {
        // For SongItem/ScriptureItem, apply style as a per-slide override
        // (text edits are ignored since slides are generated from source data)
        SlideStyle style = SlideStyle::fromSlide(slide);
        item->setSlideStyleOverride(pos.slideInItem, style);
    }

    emit presentationModified();
}

void Presentation::removeSlide(int flatIndex)
{
    SlidePosition pos = positionForFlatIndex(flatIndex);
    if (!pos.isValid()) return;

    PresentationItem* item = m_items.at(pos.itemIndex);

    // Handle different item types
    if (auto* customItem = qobject_cast<CustomSlideItem*>(item)) {
        // CustomSlideItem has only one slide, so remove the whole item
        removeItem(pos.itemIndex);
    } else if (auto* groupItem = qobject_cast<SlideGroupItem*>(item)) {
        if (groupItem->slideCount() <= 1) {
            // Last slide in group - remove the whole item
            removeItem(pos.itemIndex);
        } else {
            // Block signals to prevent itemChanged() from triggering a nested
            // model reset while PresentationModel::removeSlide has beginRemoveRows active
            groupItem->blockSignals(true);
            groupItem->removeSlide(pos.slideInItem);
            groupItem->blockSignals(false);
            m_flatIndexValid = false;
            // Adjust current index if needed
            if (m_currentSlideIndex > flatIndex) {
                m_currentSlideIndex--;
            } else if (m_currentSlideIndex == flatIndex) {
                rebuildFlatIndex();
                if (m_currentSlideIndex >= m_cachedTotalSlides && m_cachedTotalSlides > 0) {
                    m_currentSlideIndex = m_cachedTotalSlides - 1;
                }
            }
            emit slidesChanged();
            emit presentationModified();
        }
    } else {
        // For SongItem/ScriptureItem, remove the whole item
        qWarning("Removing entire item because individual slides cannot be removed from SongItem/ScriptureItem");
        removeItem(pos.itemIndex);
    }
}

void Presentation::moveSlide(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex) return;

    SlidePosition fromPos = positionForFlatIndex(fromIndex);
    SlidePosition toPos = positionForFlatIndex(toIndex);

    if (!fromPos.isValid()) return;

    PresentationItem* fromItem = m_items.at(fromPos.itemIndex);

    // Only SlideGroupItem supports internal slide moves
    if (auto* groupItem = qobject_cast<SlideGroupItem*>(fromItem)) {
        if (toPos.isValid() && toPos.itemIndex == fromPos.itemIndex) {
            // Move within the same SlideGroupItem
            groupItem->moveSlide(fromPos.slideInItem, toPos.slideInItem);
            m_flatIndexValid = false;

            // Update current index if needed
            if (m_currentSlideIndex == fromIndex) {
                m_currentSlideIndex = toIndex;
            } else if (fromIndex < m_currentSlideIndex && toIndex >= m_currentSlideIndex) {
                m_currentSlideIndex--;
            } else if (fromIndex > m_currentSlideIndex && toIndex <= m_currentSlideIndex) {
                m_currentSlideIndex++;
            }

            emit slidesChanged();
            emit presentationModified();
            return;
        }
    }

    // Cross-item move or non-group item: extract slide and insert as new CustomSlideItem
    Slide slide = slideAt(fromIndex);
    removeSlide(fromIndex);

    // Adjust toIndex if it was after the removed slide
    if (toIndex > fromIndex) {
        toIndex--;
    }

    insertSlide(toIndex, slide);
}

// === Item signals ===

void Presentation::connectItemSignals(PresentationItem* item)
{
    connect(item, &PresentationItem::slideCacheInvalidated, this, &Presentation::onItemChanged);
    connect(item, &PresentationItem::itemChanged, this, &Presentation::onItemChanged);
}

void Presentation::onItemChanged()
{
    m_flatIndexValid = false;
    emit slidesChanged();
    emit presentationModified();
}

// === Copyright slide ===

bool Presentation::hasCopyrightSlide() const
{
    if (!m_settingsManager || !m_settingsManager->showCopyrightSlide()) {
        return false;
    }

    // Check if at least one item has copyright data
    for (PresentationItem* item : m_items) {
        if (auto* songItem = qobject_cast<SongItem*>(item)) {
            Song s = songItem->song();
            if (!s.copyright().isEmpty() || !s.author().isEmpty() || !s.ccliNumber().isEmpty()) {
                return true;
            }
        } else if (auto* esvItem = qobject_cast<EsvScriptureItem*>(item)) {
            if (!esvItem->copyright().isEmpty()) {
                return true;
            }
        } else if (auto* apiItem = qobject_cast<ApiBibleScriptureItem*>(item)) {
            if (!apiItem->copyright().isEmpty()) {
                return true;
            }
        }
    }

    return false;
}

Slide Presentation::generateCopyrightSlide() const
{
    QStringList songLines;
    QStringList scriptureLines;

    for (PresentationItem* item : m_items) {
        if (auto* songItem = qobject_cast<SongItem*>(item)) {
            Song s = songItem->song();
            if (s.copyright().isEmpty() && s.author().isEmpty() && s.ccliNumber().isEmpty()) {
                continue;
            }

            QString entry;
            // "Title" by Author
            if (!s.author().isEmpty()) {
                entry = QString("\"%1\" by %2").arg(s.title(), s.author());
            } else {
                entry = QString("\"%1\"").arg(s.title());
            }
            if (!s.copyright().isEmpty()) {
                entry += "\n  " + s.copyright();
            }
            if (!s.ccliNumber().isEmpty()) {
                entry += "\n  CCLI Song #" + s.ccliNumber();
            }
            songLines.append(entry);

        } else if (auto* esvItem = qobject_cast<EsvScriptureItem*>(item)) {
            if (!esvItem->copyright().isEmpty()) {
                scriptureLines.append(esvItem->copyright());
            }

        } else if (auto* apiItem = qobject_cast<ApiBibleScriptureItem*>(item)) {
            if (!apiItem->copyright().isEmpty()) {
                QString entry;
                if (!apiItem->reference().isEmpty()) {
                    entry = apiItem->reference();
                    if (!apiItem->bibleAbbreviation().isEmpty()) {
                        entry += " (" + apiItem->bibleAbbreviation() + ")";
                    }
                    entry += ": " + apiItem->copyright();
                } else {
                    entry = apiItem->copyright();
                }
                scriptureLines.append(entry);
            }
        }
    }

    // Build the copyright slide text
    QStringList parts;

    // License line at top if configured
    if (m_settingsManager) {
        QString licenseNum = m_settingsManager->ccliLicenseNumber();
        if (!licenseNum.isEmpty()) {
            parts.append(QObject::tr("Used by Permission. CCLI License #%1").arg(licenseNum));
        }
    }

    if (!songLines.isEmpty()) {
        parts.append("");
        parts.append(QObject::tr("Songs:"));
        for (const QString& line : songLines) {
            parts.append("  " + line);
        }
    }

    if (!scriptureLines.isEmpty()) {
        parts.append("");
        parts.append(QObject::tr("Scripture:"));
        for (const QString& line : scriptureLines) {
            parts.append("  " + line);
        }
    }

    Slide slide;
    slide.setText(parts.join("\n").trimmed());
    slide.setBackgroundColor(QColor("#000000"));
    slide.setTextColor(QColor("#ffffff"));
    slide.setFontFamily("Arial");
    slide.setFontSize(28);
    slide.setHasExplicitBackground(true);

    return slide;
}

// === JSON serialization ===

QJsonObject Presentation::toJson() const
{
    QJsonObject json;
    json["version"] = "2.0";
    json["title"] = m_title;
    json["currentSlideIndex"] = m_currentSlideIndex;
    json["createdDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json["modifiedDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray itemsArray;
    for (PresentationItem* item : m_items) {
        itemsArray.append(item->toJson());
    }
    json["items"] = itemsArray;

    return json;
}

Presentation* Presentation::fromJson(const QJsonObject& json,
                                      SongLibrary* songLibrary,
                                      BibleDatabase* bibleDatabase,
                                      SettingsManager* settingsManager,
                                      QObject* parent)
{
    Presentation* presentation = new Presentation(parent);

    QString version = json["version"].toString("1.0");
    presentation->m_title = json["title"].toString("Untitled");
    presentation->m_currentSlideIndex = json["currentSlideIndex"].toInt(0);

    if (version == "1.0") {
        // v1.0 migration: wrap all slides in a single SlideGroupItem
        QJsonArray slidesArray = json["slides"].toArray();
        if (!slidesArray.isEmpty()) {
            QList<Slide> slides;
            for (const QJsonValue& value : slidesArray) {
                slides.append(Slide::fromJson(value.toObject()));
            }

            auto* groupItem = new SlideGroupItem(
                QObject::tr("Imported Slides"),
                slides,
                presentation
            );
            presentation->m_items.append(groupItem);
            presentation->connectItemSignals(groupItem);
        }
    } else {
        // v2.0 format: load items
        QJsonArray itemsArray = json["items"].toArray();
        for (const QJsonValue& value : itemsArray) {
            QJsonObject itemJson = value.toObject();
            QString typeName = itemJson["type"].toString();

            PresentationItem* item = nullptr;

            if (typeName == "song") {
                item = SongItem::fromJson(itemJson, songLibrary, settingsManager);
            } else if (typeName == "scripture") {
                item = ScriptureItem::fromJson(itemJson, bibleDatabase, settingsManager);
            } else if (typeName == "customSlide") {
                item = CustomSlideItem::fromJson(itemJson);
            } else if (typeName == "slideGroup") {
                item = SlideGroupItem::fromJson(itemJson);
            } else if (typeName == "esvScripture") {
                item = EsvScriptureItem::fromJson(itemJson, settingsManager);
            } else if (typeName == "apiBibleScripture") {
                item = ApiBibleScriptureItem::fromJson(itemJson, settingsManager);
            } else {
                qWarning("Unknown item type: %s", qPrintable(typeName));
                continue;
            }

            if (item) {
                item->setParent(presentation);
                presentation->m_items.append(item);
                presentation->connectItemSignals(item);
            }
        }
    }

    // Validate current index
    presentation->rebuildFlatIndex();
    if (presentation->m_currentSlideIndex >= presentation->m_cachedTotalSlides) {
        presentation->m_currentSlideIndex = 0;
    }

    return presentation;
}

} // namespace Clarity
