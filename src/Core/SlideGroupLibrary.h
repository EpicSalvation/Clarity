#pragma once

#include "Slide.h"
#include <QObject>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QJsonObject>

namespace Clarity {

/**
 * @brief A saved slide group in the library (value type, not QObject)
 */
struct LibrarySlideGroup {
    int id = 0;
    QString name;
    QList<Slide> slides;
    QDateTime dateAdded;
    QDateTime lastUsed;

    QJsonObject toJson() const;
    static LibrarySlideGroup fromJson(const QJsonObject& json);
};

/**
 * @brief Manages a persistent collection of reusable slide groups
 *
 * Stores slide groups in a JSON file in the user's config directory.
 * Follows the same pattern as SongLibrary for consistency.
 */
class SlideGroupLibrary : public QObject {
    Q_OBJECT

public:
    explicit SlideGroupLibrary(QObject* parent = nullptr);
    ~SlideGroupLibrary();

    bool loadLibrary();
    bool saveLibrary();

    QList<LibrarySlideGroup> allGroups() const { return m_groups; }
    int groupCount() const { return m_groups.count(); }

    LibrarySlideGroup getGroup(int id) const;
    int indexOf(int id) const;

    int addGroup(const LibrarySlideGroup& group);
    bool updateGroup(int id, const LibrarySlideGroup& group);
    bool renameGroup(int id, const QString& newName);
    bool removeGroup(int id);

    QList<LibrarySlideGroup> search(const QString& query) const;

    QString libraryPath() const { return m_libraryPath; }

signals:
    void groupAdded(int id);
    void groupUpdated(int id);
    void groupRemoved(int id);

private:
    int nextId();

    QList<LibrarySlideGroup> m_groups;
    QString m_libraryPath;
    int m_nextId;
};

} // namespace Clarity
