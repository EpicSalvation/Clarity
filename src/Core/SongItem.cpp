#include "SongItem.h"
#include "SettingsManager.h"
#include <QJsonArray>

namespace Clarity {

SongItem::SongItem(QObject* parent)
    : PresentationItem(parent)
    , m_songId(-1)
    , m_songLibrary(nullptr)
    , m_settingsManager(nullptr)
    , m_includeTitleSlide(true)
    , m_includeSectionLabels(false)
    , m_maxLinesPerSlide(0)
    , m_hasCustomSectionOrder(false)
{
}

SongItem::SongItem(int songId, SongLibrary* library, QObject* parent)
    : PresentationItem(parent)
    , m_songId(songId)
    , m_songLibrary(library)
    , m_settingsManager(nullptr)
    , m_includeTitleSlide(true)
    , m_includeSectionLabels(false)
    , m_maxLinesPerSlide(0)
    , m_hasCustomSectionOrder(false)
{
    if (m_songLibrary) {
        connect(m_songLibrary, &SongLibrary::songUpdated,
                this, &SongItem::onSongUpdated);
        connect(m_songLibrary, &SongLibrary::songRemoved,
                this, &SongItem::onSongRemoved);
    }
}

QString SongItem::displayName() const
{
    if (!m_songLibrary) {
        return tr("Song (no library)");
    }

    Song s = m_songLibrary->getSong(m_songId);
    if (s.title().isEmpty()) {
        return tr("Song (not found)");
    }

    return s.title();
}

QString SongItem::displaySubtitle() const
{
    if (!m_songLibrary) {
        return QString();
    }

    Song s = m_songLibrary->getSong(m_songId);
    if (!s.author().isEmpty()) {
        return tr("by %1").arg(s.author());
    }

    return QString();
}

QList<Slide> SongItem::generateSlides() const
{
    if (!m_songLibrary) {
        Slide errorSlide(tr("Song library not available"));
        return QList<Slide>() << errorSlide;
    }

    Song s = m_songLibrary->getSong(m_songId);
    if (s.title().isEmpty()) {
        Slide errorSlide(tr("Song not found (ID: %1)").arg(m_songId));
        return QList<Slide>() << errorSlide;
    }

    SlideStyle style;
    if (m_hasCustomStyle) {
        style = m_itemStyle;
    }

    QList<Slide> slides;

    // Add title slide if requested
    if (m_includeTitleSlide && !s.title().isEmpty()) {
        Slide titleSlide;
        QString titleText = s.title();

        // Append CCLI info if enabled and song has a CCLI number
        if (m_settingsManager && m_settingsManager->showCcliOnTitleSlides()
            && !s.ccliNumber().isEmpty()) {
            QString ccliLine = tr("CCLI Song #%1").arg(s.ccliNumber());
            QString licenseNum = m_settingsManager->ccliLicenseNumber();
            if (!licenseNum.isEmpty()) {
                ccliLine += tr(" | CCLI License #%1").arg(licenseNum);
            }
            titleText += "\n" + ccliLine;
        }

        titleSlide.setText(titleText);
        style.applyTo(titleSlide);
        // Title slide: groupIndex = -1, groupLabel = empty (defaults)
        slides.append(titleSlide);
    }

    // Generate slides in section order
    QList<int> order = sectionOrder();
    for (int orderPos = 0; orderPos < order.count(); ++orderPos) {
        int sectionIdx = order[orderPos];
        QList<Slide> sectionSlides = s.sectionToSlides(sectionIdx, style, m_includeSectionLabels, m_maxLinesPerSlide);
        // Override groupIndex to reflect the order position (not the source section index)
        for (Slide& slide : sectionSlides) {
            slide.setGroupIndex(orderPos);
        }
        slides.append(sectionSlides);
    }

    // Cascading backgrounds: song slides don't have explicit backgrounds by default.
    // If the item has a custom style, the first slide is explicit (it's the user's choice).
    for (int i = 0; i < slides.count(); ++i) {
        slides[i].setHasExplicitBackground(m_hasCustomStyle && i == 0);
    }

    return slides;
}

void SongItem::setSongId(int songId)
{
    if (m_songId != songId) {
        m_songId = songId;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SongItem::setSongLibrary(SongLibrary* library)
{
    if (m_songLibrary != library) {
        // Disconnect from old library
        if (m_songLibrary) {
            disconnect(m_songLibrary, nullptr, this, nullptr);
        }

        m_songLibrary = library;

        // Connect to new library
        if (m_songLibrary) {
            connect(m_songLibrary, &SongLibrary::songUpdated,
                    this, &SongItem::onSongUpdated);
            connect(m_songLibrary, &SongLibrary::songRemoved,
                    this, &SongItem::onSongRemoved);
        }

        invalidateSlideCache();
        emit itemChanged();
    }
}

bool SongItem::songExists() const
{
    if (!m_songLibrary) {
        return false;
    }
    return !m_songLibrary->getSong(m_songId).title().isEmpty();
}

Song SongItem::song() const
{
    if (!m_songLibrary) {
        return Song();
    }
    return m_songLibrary->getSong(m_songId);
}

void SongItem::setIncludeTitleSlide(bool include)
{
    if (m_includeTitleSlide != include) {
        m_includeTitleSlide = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SongItem::setIncludeSectionLabels(bool include)
{
    if (m_includeSectionLabels != include) {
        m_includeSectionLabels = include;
        invalidateSlideCache();
        emit itemChanged();
    }
}

void SongItem::setSettingsManager(SettingsManager* settings)
{
    m_settingsManager = settings;
    invalidateSlideCache();
}

void SongItem::setMaxLinesPerSlide(int maxLines)
{
    if (m_maxLinesPerSlide != maxLines) {
        m_maxLinesPerSlide = maxLines;
        invalidateSlideCache();
        emit itemChanged();
    }
}

QList<int> SongItem::sectionOrder() const
{
    if (m_hasCustomSectionOrder && !m_sectionOrder.isEmpty()) {
        return m_sectionOrder;
    }
    // Natural order: [0, 1, 2, ..., N-1]
    Song s = song();
    QList<int> order;
    for (int i = 0; i < s.sectionCount(); ++i) {
        order.append(i);
    }
    return order;
}

int SongItem::sectionCount() const
{
    return sectionOrder().count();
}

int SongItem::sectionOrderIndexForSlide(int slideInItem) const
{
    // Account for title slide offset
    int slideOffset = slideInItem;
    if (m_includeTitleSlide) {
        if (slideInItem == 0) {
            return -1;  // Title slide has no section
        }
        slideOffset = slideInItem - 1;
    }

    // Walk through sections to find which one this slide belongs to
    Song s = song();
    if (s.title().isEmpty()) return -1;

    SlideStyle style;
    if (m_hasCustomStyle) {
        style = m_itemStyle;
    }

    QList<int> order = sectionOrder();
    int slidesSoFar = 0;
    for (int orderPos = 0; orderPos < order.count(); ++orderPos) {
        int sectionIdx = order[orderPos];
        QList<Slide> sectionSlides = s.sectionToSlides(sectionIdx, style, m_includeSectionLabels, m_maxLinesPerSlide);
        slidesSoFar += sectionSlides.count();
        if (slideOffset < slidesSoFar) {
            return orderPos;
        }
    }

    return -1;
}

QString SongItem::sectionLabelAt(int orderIndex) const
{
    QList<int> order = sectionOrder();
    if (orderIndex < 0 || orderIndex >= order.count()) {
        return QString();
    }

    Song s = song();
    int sectionIdx = order[orderIndex];
    if (sectionIdx < 0 || sectionIdx >= s.sectionCount()) {
        return QString();
    }

    return s.sections()[sectionIdx].label;
}

void SongItem::moveSongSection(int from, int to)
{
    QList<int> order = sectionOrder();
    if (from < 0 || from >= order.count() || to < 0 || to >= order.count() || from == to) {
        return;
    }

    order.move(from, to);
    m_sectionOrder = order;
    m_hasCustomSectionOrder = true;
    invalidateSlideCache();
    emit itemChanged();
}

void SongItem::duplicateSongSection(int orderIndex)
{
    QList<int> order = sectionOrder();
    if (orderIndex < 0 || orderIndex >= order.count()) {
        return;
    }

    order.insert(orderIndex + 1, order[orderIndex]);
    m_sectionOrder = order;
    m_hasCustomSectionOrder = true;
    invalidateSlideCache();
    emit itemChanged();
}

bool SongItem::removeSongSection(int orderIndex)
{
    QList<int> order = sectionOrder();
    if (order.count() <= 1 || orderIndex < 0 || orderIndex >= order.count()) {
        return false;
    }

    order.removeAt(orderIndex);
    m_sectionOrder = order;
    m_hasCustomSectionOrder = true;
    invalidateSlideCache();
    emit itemChanged();
    return true;
}

QJsonObject SongItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["songId"] = m_songId;
    json["includeTitleSlide"] = m_includeTitleSlide;
    json["includeSectionLabels"] = m_includeSectionLabels;
    json["maxLinesPerSlide"] = m_maxLinesPerSlide;

    if (m_hasCustomSectionOrder && !m_sectionOrder.isEmpty()) {
        QJsonArray orderArray;
        for (int idx : m_sectionOrder) {
            orderArray.append(idx);
        }
        json["sectionOrder"] = orderArray;
    }

    return json;
}

SongItem* SongItem::fromJson(const QJsonObject& json, SongLibrary* library, SettingsManager* settings)
{
    if (json["type"].toString() != "song") {
        qWarning("SongItem::fromJson: type mismatch");
        return nullptr;
    }

    SongItem* item = new SongItem();
    item->applyBaseJson(json);
    item->m_songId = json["songId"].toInt(-1);
    item->m_includeTitleSlide = json["includeTitleSlide"].toBool(true);
    item->m_includeSectionLabels = json["includeSectionLabels"].toBool(false);
    item->m_maxLinesPerSlide = json["maxLinesPerSlide"].toInt(0);
    item->m_settingsManager = settings;

    // Restore custom section order
    if (json.contains("sectionOrder")) {
        QJsonArray orderArray = json["sectionOrder"].toArray();
        for (const QJsonValue& val : orderArray) {
            item->m_sectionOrder.append(val.toInt());
        }
        item->m_hasCustomSectionOrder = !item->m_sectionOrder.isEmpty();
    }

    // Set library (which also connects signals)
    item->setSongLibrary(library);

    return item;
}

void SongItem::onSongUpdated(int id)
{
    if (id == m_songId) {
        // Validate section order against current section count
        if (m_hasCustomSectionOrder) {
            Song s = song();
            int count = s.sectionCount();
            bool valid = true;
            for (int idx : m_sectionOrder) {
                if (idx < 0 || idx >= count) {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                // Section structure changed — reset to natural order
                m_sectionOrder.clear();
                m_hasCustomSectionOrder = false;
            }
        }

        invalidateSlideCache();
        emit itemChanged();
    }
}

void SongItem::onSongRemoved(int id)
{
    if (id == m_songId) {
        // Our song was removed - invalidate cache
        // The generateSlides() method will show an error state
        invalidateSlideCache();
        emit itemChanged();
    }
}

} // namespace Clarity
