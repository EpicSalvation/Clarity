#include "SongItem.h"

namespace Clarity {

SongItem::SongItem(QObject* parent)
    : PresentationItem(parent)
    , m_songId(-1)
    , m_songLibrary(nullptr)
    , m_includeTitleSlide(true)
    , m_includeSectionLabels(false)
    , m_maxLinesPerSlide(0)
{
}

SongItem::SongItem(int songId, SongLibrary* library, QObject* parent)
    : PresentationItem(parent)
    , m_songId(songId)
    , m_songLibrary(library)
    , m_includeTitleSlide(true)
    , m_includeSectionLabels(false)
    , m_maxLinesPerSlide(0)
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
        // No library - return a placeholder slide
        Slide errorSlide(tr("Song library not available"));
        return QList<Slide>() << errorSlide;
    }

    Song s = m_songLibrary->getSong(m_songId);
    if (s.title().isEmpty()) {
        // Song not found - return a placeholder slide
        Slide errorSlide(tr("Song not found (ID: %1)").arg(m_songId));
        return QList<Slide>() << errorSlide;
    }

    // Determine style to use
    SlideStyle style;
    if (m_hasCustomStyle) {
        style = m_itemStyle;
    }
    // Otherwise use the default style from SlideStyle constructor

    // Generate slides from song
    return s.toSlides(style, m_includeTitleSlide, m_includeSectionLabels, m_maxLinesPerSlide);
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

void SongItem::setMaxLinesPerSlide(int maxLines)
{
    if (m_maxLinesPerSlide != maxLines) {
        m_maxLinesPerSlide = maxLines;
        invalidateSlideCache();
        emit itemChanged();
    }
}

QJsonObject SongItem::toJson() const
{
    QJsonObject json = baseToJson();
    json["songId"] = m_songId;
    json["includeTitleSlide"] = m_includeTitleSlide;
    json["includeSectionLabels"] = m_includeSectionLabels;
    json["maxLinesPerSlide"] = m_maxLinesPerSlide;
    return json;
}

SongItem* SongItem::fromJson(const QJsonObject& json, SongLibrary* library)
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

    // Set library (which also connects signals)
    item->setSongLibrary(library);

    return item;
}

void SongItem::onSongUpdated(int id)
{
    if (id == m_songId) {
        // Our song was updated - invalidate cache
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
