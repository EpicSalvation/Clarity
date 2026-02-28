// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SlideGroupLibrary.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

namespace Clarity {

// --- LibrarySlideGroup ---

QJsonObject LibrarySlideGroup::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["dateAdded"] = dateAdded.toString(Qt::ISODate);
    json["lastUsed"] = lastUsed.toString(Qt::ISODate);

    QJsonArray slidesArray;
    for (const Slide& slide : slides) {
        slidesArray.append(slide.toJson());
    }
    json["slides"] = slidesArray;

    return json;
}

LibrarySlideGroup LibrarySlideGroup::fromJson(const QJsonObject& json)
{
    LibrarySlideGroup group;
    group.id = json["id"].toInt(0);
    group.name = json["name"].toString();
    group.dateAdded = QDateTime::fromString(json["dateAdded"].toString(), Qt::ISODate);
    group.lastUsed = QDateTime::fromString(json["lastUsed"].toString(), Qt::ISODate);

    QJsonArray slidesArray = json["slides"].toArray();
    for (const QJsonValue& value : slidesArray) {
        group.slides.append(Slide::fromJson(value.toObject()));
    }

    return group;
}

// --- SlideGroupLibrary ---

SlideGroupLibrary::SlideGroupLibrary(QObject* parent)
    : QObject(parent)
    , m_nextId(1)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir dir(configDir);

    if (!dir.exists("Clarity")) {
        dir.mkdir("Clarity");
    }

    m_libraryPath = configDir + "/Clarity/slidegroups.json";
    qDebug() << "Slide group library path:" << m_libraryPath;
}

SlideGroupLibrary::~SlideGroupLibrary()
{
    saveLibrary();
}

bool SlideGroupLibrary::loadLibrary()
{
    QFile file(m_libraryPath);

    if (!file.exists()) {
        qDebug() << "Slide group library file doesn't exist yet, starting with empty library";
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open slide group library:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse slide group library JSON:" << error.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QString version = root["version"].toString();

    if (version != "1.0") {
        qWarning() << "Unknown slide group library version:" << version;
    }

    m_nextId = root["nextId"].toInt(1);

    m_groups.clear();
    QJsonArray groupsArray = root["groups"].toArray();
    for (const QJsonValue& value : groupsArray) {
        LibrarySlideGroup group = LibrarySlideGroup::fromJson(value.toObject());
        m_groups.append(group);

        if (group.id >= m_nextId) {
            m_nextId = group.id + 1;
        }
    }

    qDebug() << "Loaded" << m_groups.count() << "slide groups from library";
    return true;
}

bool SlideGroupLibrary::saveLibrary()
{
    QJsonObject root;
    root["version"] = "1.0";
    root["nextId"] = m_nextId;

    QJsonArray groupsArray;
    for (const LibrarySlideGroup& group : m_groups) {
        groupsArray.append(group.toJson());
    }
    root["groups"] = groupsArray;

    QJsonDocument doc(root);

    QFile file(m_libraryPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save slide group library:" << file.errorString();
        return false;
    }

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written == -1) {
        qWarning() << "Failed to write slide group library data";
        return false;
    }

    qDebug() << "Saved" << m_groups.count() << "slide groups to library";
    return true;
}

LibrarySlideGroup SlideGroupLibrary::getGroup(int id) const
{
    for (const LibrarySlideGroup& group : m_groups) {
        if (group.id == id) {
            return group;
        }
    }
    return LibrarySlideGroup();
}

int SlideGroupLibrary::indexOf(int id) const
{
    for (int i = 0; i < m_groups.count(); ++i) {
        if (m_groups[i].id == id) {
            return i;
        }
    }
    return -1;
}

int SlideGroupLibrary::addGroup(const LibrarySlideGroup& group)
{
    LibrarySlideGroup newGroup = group;
    int id = nextId();
    newGroup.id = id;

    if (!newGroup.dateAdded.isValid()) {
        newGroup.dateAdded = QDateTime::currentDateTime();
    }

    m_groups.append(newGroup);
    emit groupAdded(id);
    return id;
}

bool SlideGroupLibrary::updateGroup(int id, const LibrarySlideGroup& group)
{
    int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    LibrarySlideGroup updated = group;
    updated.id = id;
    m_groups[index] = updated;
    emit groupUpdated(id);
    return true;
}

bool SlideGroupLibrary::renameGroup(int id, const QString& newName)
{
    int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    m_groups[index].name = newName;
    emit groupUpdated(id);
    return true;
}

bool SlideGroupLibrary::removeGroup(int id)
{
    int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    m_groups.removeAt(index);
    emit groupRemoved(id);
    return true;
}

QList<LibrarySlideGroup> SlideGroupLibrary::search(const QString& query) const
{
    QList<LibrarySlideGroup> results;

    if (query.isEmpty()) {
        return m_groups;
    }

    QString lowerQuery = query.toLower();

    for (const LibrarySlideGroup& group : m_groups) {
        if (group.name.toLower().contains(lowerQuery)) {
            results.append(group);
            continue;
        }

        // Search slide text
        for (const Slide& slide : group.slides) {
            if (slide.text().toLower().contains(lowerQuery)) {
                results.append(group);
                break;
            }
        }
    }

    return results;
}

int SlideGroupLibrary::nextId()
{
    return m_nextId++;
}

} // namespace Clarity
