// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "Song.h"
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QDebug>

namespace Clarity {

// SongSection implementation

QJsonObject SongSection::toJson() const
{
    QJsonObject json;
    json["type"] = type;
    json["label"] = label;
    json["text"] = text;
    return json;
}

SongSection SongSection::fromJson(const QJsonObject& json)
{
    SongSection section;
    section.type = json["type"].toString();
    section.label = json["label"].toString();
    section.text = json["text"].toString();
    return section;
}

// SongUsage implementation

QJsonObject SongUsage::toJson() const
{
    QJsonObject json;
    json["dateTime"] = dateTime.toString(Qt::ISODate);
    if (!eventName.isEmpty()) {
        json["eventName"] = eventName;
    }
    return json;
}

SongUsage SongUsage::fromJson(const QJsonObject& json)
{
    SongUsage usage;
    usage.dateTime = QDateTime::fromString(json["dateTime"].toString(), Qt::ISODate);
    usage.eventName = json["eventName"].toString();
    return usage;
}

// Song implementation

Song::Song()
    : m_id(0)
    , m_addedDate(QDateTime::currentDateTime())
{
}

QString Song::allLyrics() const
{
    QStringList lyrics;
    for (const SongSection& section : m_sections) {
        lyrics.append(section.text);
    }
    return lyrics.join("\n");
}

void Song::addUsage(const SongUsage& usage)
{
    m_usageHistory.append(usage);
    // Also update lastUsed if this usage is more recent
    if (!m_lastUsed.isValid() || usage.dateTime > m_lastUsed) {
        m_lastUsed = usage.dateTime;
    }
}

int Song::usageCountInRange(const QDate& from, const QDate& to) const
{
    int count = 0;
    for (const SongUsage& usage : m_usageHistory) {
        QDate usageDate = usage.dateTime.date();
        if (usageDate >= from && usageDate <= to) {
            count++;
        }
    }
    return count;
}

QList<SongUsage> Song::usageInRange(const QDate& from, const QDate& to) const
{
    QList<SongUsage> results;
    for (const SongUsage& usage : m_usageHistory) {
        QDate usageDate = usage.dateTime.date();
        if (usageDate >= from && usageDate <= to) {
            results.append(usage);
        }
    }
    return results;
}

QList<Slide> Song::sectionToSlides(int sectionIndex, const SlideStyle& style, bool includeSectionLabel, int maxLinesPerSlide) const
{
    QList<Slide> slides;

    if (sectionIndex < 0 || sectionIndex >= m_sections.count()) {
        return slides;
    }

    const SongSection& section = m_sections[sectionIndex];
    QString sectionText = section.text;
    QStringList lines = sectionText.split('\n');

    // If max lines is set and section exceeds it, split into multiple slides
    if (maxLinesPerSlide > 0 && lines.count() > maxLinesPerSlide) {
        int slideNum = 0;
        for (int i = 0; i < lines.count(); i += maxLinesPerSlide) {
            slideNum++;
            QStringList slideLines;
            for (int j = i; j < qMin(i + maxLinesPerSlide, lines.count()); ++j) {
                slideLines.append(lines[j]);
            }

            Slide slide;
            QString text = slideLines.join("\n");

            if (includeSectionLabel && !section.label.isEmpty()) {
                QString label = section.label;
                if (lines.count() > maxLinesPerSlide) {
                    label += QString(" (%1)").arg(slideNum);
                }
                text = label + "\n\n" + text;
            }

            slide.setText(text);
            style.applyTo(slide);
            slide.setGroupLabel(section.label);
            slide.setGroupIndex(sectionIndex);

            slides.append(slide);
        }
    } else {
        // Single slide for this section
        Slide slide;

        QString text = sectionText;
        if (includeSectionLabel && !section.label.isEmpty()) {
            text = section.label + "\n\n" + text;
        }

        slide.setText(text);
        style.applyTo(slide);
        slide.setGroupLabel(section.label);
        slide.setGroupIndex(sectionIndex);

        slides.append(slide);
    }

    return slides;
}

QList<Slide> Song::toSlides(const SlideStyle& style, bool includeTitleSlide, bool includeSectionLabels, int maxLinesPerSlide) const
{
    QList<Slide> slides;

    // Add title slide at the beginning if requested
    if (includeTitleSlide && !m_title.isEmpty()) {
        Slide titleSlide;
        titleSlide.setText(m_title);
        style.applyTo(titleSlide);
        // Title slide has no group (groupIndex = -1, groupLabel = empty)
        slides.append(titleSlide);
    }

    for (int i = 0; i < m_sections.count(); ++i) {
        slides.append(sectionToSlides(i, style, includeSectionLabels, maxLinesPerSlide));
    }

    return slides;
}

QJsonObject Song::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["title"] = m_title;
    json["author"] = m_author;
    json["copyright"] = m_copyright;
    json["ccliNumber"] = m_ccliNumber;
    json["addedDate"] = m_addedDate.toString(Qt::ISODate);

    if (m_lastUsed.isValid()) {
        json["lastUsed"] = m_lastUsed.toString(Qt::ISODate);
    }

    QJsonArray sectionsArray;
    for (const SongSection& section : m_sections) {
        sectionsArray.append(section.toJson());
    }
    json["sections"] = sectionsArray;

    // Usage history for CCLI reporting
    if (!m_usageHistory.isEmpty()) {
        QJsonArray usageArray;
        for (const SongUsage& usage : m_usageHistory) {
            usageArray.append(usage.toJson());
        }
        json["usageHistory"] = usageArray;
    }

    return json;
}

Song Song::fromJson(const QJsonObject& json)
{
    Song song;
    song.m_id = json["id"].toInt();
    song.m_title = json["title"].toString();
    song.m_author = json["author"].toString();
    song.m_copyright = json["copyright"].toString();
    song.m_ccliNumber = json["ccliNumber"].toString();

    if (json.contains("addedDate")) {
        song.m_addedDate = QDateTime::fromString(json["addedDate"].toString(), Qt::ISODate);
    }
    if (json.contains("lastUsed")) {
        song.m_lastUsed = QDateTime::fromString(json["lastUsed"].toString(), Qt::ISODate);
    }

    QJsonArray sectionsArray = json["sections"].toArray();
    for (const QJsonValue& value : sectionsArray) {
        song.m_sections.append(SongSection::fromJson(value.toObject()));
    }

    // Load usage history
    if (json.contains("usageHistory")) {
        QJsonArray usageArray = json["usageHistory"].toArray();
        for (const QJsonValue& value : usageArray) {
            song.m_usageHistory.append(SongUsage::fromJson(value.toObject()));
        }
    }

    return song;
}

Song Song::fromOpenLyrics(const QString& xml)
{
    Song song;

    QXmlStreamReader reader(xml);

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString name = reader.name().toString();

            // Parse title
            if (name == "title") {
                song.m_title = reader.readElementText();
            }
            // Parse author
            else if (name == "author") {
                if (song.m_author.isEmpty()) {
                    song.m_author = reader.readElementText();
                } else {
                    song.m_author += ", " + reader.readElementText();
                }
            }
            // Parse CCLI number
            else if (name == "ccliNo") {
                song.m_ccliNumber = reader.readElementText();
            }
            // Parse copyright
            else if (name == "copyright") {
                song.m_copyright = reader.readElementText();
            }
            // Parse verse sections
            else if (name == "verse") {
                QString verseName = reader.attributes().value("name").toString();
                QString verseType = "verse";
                QString verseLabel = verseName;

                // Determine type and label from name attribute
                if (verseName.startsWith("v", Qt::CaseInsensitive)) {
                    verseType = "verse";
                    QString num = verseName.mid(1);
                    verseLabel = "Verse " + (num.isEmpty() ? "1" : num);
                } else if (verseName.startsWith("c", Qt::CaseInsensitive)) {
                    verseType = "chorus";
                    QString num = verseName.mid(1);
                    verseLabel = num.isEmpty() ? "Chorus" : "Chorus " + num;
                } else if (verseName.startsWith("b", Qt::CaseInsensitive)) {
                    verseType = "bridge";
                    verseLabel = "Bridge";
                } else if (verseName.startsWith("p", Qt::CaseInsensitive)) {
                    verseType = "pre-chorus";
                    verseLabel = "Pre-Chorus";
                }

                // Read verse content (lines elements)
                QStringList lines;
                while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == QString("verse"))) {
                    reader.readNext();
                    if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == QString("lines")) {
                        lines.append(reader.readElementText());
                    }
                }

                if (!lines.isEmpty()) {
                    SongSection section(verseType, verseLabel, lines.join("\n"));
                    song.m_sections.append(section);
                }
            }
        }
    }

    if (reader.hasError()) {
        qWarning() << "OpenLyrics XML parsing error:" << reader.errorString();
    }

    return song;
}

Song Song::fromPlainText(const QString& text, const QString& title)
{
    Song song;
    song.m_title = title;

    // Pattern to match section markers with brackets: [Verse 1], [Chorus], etc.
    static QRegularExpression bracketPattern(
        R"(^\[(Verse|Chorus|Bridge|Pre-Chorus|Tag|Intro|Outro|Ending)(?:\s*(\d+))?\]$)",
        QRegularExpression::CaseInsensitiveOption
    );

    // Pattern to match SongSelect-style section markers without brackets: "Verse 1", "Chorus 1", etc.
    // Must be on a line by itself
    static QRegularExpression songSelectPattern(
        R"(^(Verse|Chorus|Bridge|Pre-Chorus|Tag|Intro|Outro|Ending|Interlude|Vamp|Misc)(?:\s*(\d+))?$)",
        QRegularExpression::CaseInsensitiveOption
    );

    // Pattern to detect CCLI metadata lines
    static QRegularExpression ccliSongPattern(R"(^CCLI Song #(\d+))", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression ccliLicensePattern(R"(^CCLI License)", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression copyrightPattern(R"(^©|\(c\)|For use solely with)", QRegularExpression::CaseInsensitiveOption);

    QStringList lines = text.split('\n');

    // Pre-scan to find metadata section at end of file (SongSelect format)
    // Metadata starts at the author line, which is right before "CCLI Song #"
    int metadataStartLine = lines.count();  // Default: no metadata
    int ccliSongLine = -1;

    for (int i = lines.count() - 1; i >= 0; --i) {
        QString trimmed = lines[i].trimmed();
        if (ccliSongPattern.match(trimmed).hasMatch()) {
            ccliSongLine = i;
            QRegularExpressionMatch match = ccliSongPattern.match(trimmed);
            song.m_ccliNumber = match.captured(1);

            // Author line is right before CCLI Song # (skip empty lines)
            for (int j = i - 1; j >= 0; --j) {
                QString prevLine = lines[j].trimmed();
                if (prevLine.isEmpty()) {
                    continue;
                }
                // Check if this is a section marker (means no author line)
                if (songSelectPattern.match(prevLine).hasMatch() || bracketPattern.match(prevLine).hasMatch()) {
                    metadataStartLine = i;
                    break;
                }
                // This should be the author line
                song.m_author = prevLine;
                metadataStartLine = j;
                break;
            }
            break;
        }
    }

    // Extract copyright from metadata section
    for (int i = metadataStartLine; i < lines.count(); ++i) {
        QString trimmed = lines[i].trimmed();
        if (trimmed.startsWith("©") || trimmed.startsWith("(c)")) {
            song.m_copyright = trimmed;
            break;
        }
    }

    // Check if this looks like a SongSelect file (title on first line, no brackets)
    bool isSongSelectFormat = false;
    if (lines.count() >= 3) {
        QString firstLine = lines[0].trimmed();
        QString secondLine = lines[1].trimmed();
        QString thirdLine = lines[2].trimmed();

        // SongSelect: first line is title, second is empty, third starts a section
        if (!firstLine.isEmpty() && secondLine.isEmpty() && songSelectPattern.match(thirdLine).hasMatch()) {
            isSongSelectFormat = true;
            song.m_title = firstLine;  // Override title with first line
        }
    }

    int startLine = isSongSelectFormat ? 1 : 0;  // Skip title line if SongSelect format
    QString currentType = "verse";
    QString currentLabel = "Verse 1";
    QStringList currentLines;
    int verseCount = 0;
    int chorusCount = 0;

    auto saveCurrent = [&]() {
        if (!currentLines.isEmpty()) {
            QString sectionText = currentLines.join("\n").trimmed();
            if (!sectionText.isEmpty()) {
                SongSection section(currentType, currentLabel, sectionText);
                song.m_sections.append(section);
            }
            currentLines.clear();
        }
    };

    auto parseSection = [&](const QString& typeStr, const QString& numStr) {
        QString typeLower = typeStr.toLower();

        if (typeLower == "verse") {
            currentType = "verse";
            verseCount++;
            currentLabel = "Verse " + (numStr.isEmpty() ? QString::number(verseCount) : numStr);
        } else if (typeLower == "chorus") {
            currentType = "chorus";
            chorusCount++;
            currentLabel = numStr.isEmpty() && chorusCount == 1 ? "Chorus" : "Chorus " + (numStr.isEmpty() ? QString::number(chorusCount) : numStr);
        } else if (typeLower == "bridge") {
            currentType = "bridge";
            currentLabel = numStr.isEmpty() ? "Bridge" : "Bridge " + numStr;
        } else if (typeLower == "pre-chorus") {
            currentType = "pre-chorus";
            currentLabel = "Pre-Chorus";
        } else if (typeLower == "tag") {
            currentType = "tag";
            currentLabel = "Tag";
        } else if (typeLower == "intro") {
            currentType = "intro";
            currentLabel = "Intro";
        } else if (typeLower == "outro" || typeLower == "ending") {
            currentType = "outro";
            currentLabel = "Outro";
        } else if (typeLower == "interlude") {
            currentType = "interlude";
            currentLabel = "Interlude";
        } else if (typeLower == "vamp" || typeLower == "misc") {
            currentType = "misc";
            currentLabel = typeStr;  // Keep original label
        }
    };

    // Parse lyrics (stop at metadata section)
    for (int i = startLine; i < metadataStartLine; ++i) {
        QString trimmed = lines[i].trimmed();

        // Try bracket pattern first
        QRegularExpressionMatch bracketMatch = bracketPattern.match(trimmed);
        if (bracketMatch.hasMatch()) {
            saveCurrent();
            parseSection(bracketMatch.captured(1), bracketMatch.captured(2));
            continue;
        }

        // Try SongSelect pattern (no brackets)
        QRegularExpressionMatch songSelectMatch = songSelectPattern.match(trimmed);
        if (songSelectMatch.hasMatch()) {
            saveCurrent();
            parseSection(songSelectMatch.captured(1), songSelectMatch.captured(2));
            continue;
        }

        // Regular line of lyrics
        if (!trimmed.isEmpty()) {
            currentLines.append(trimmed);
        } else if (!currentLines.isEmpty()) {
            // Empty line - preserve paragraph breaks within sections
            currentLines.append("");
        }
    }

    // Save final section
    saveCurrent();

    // If no sections were created (no markers found), create one section with all text
    if (song.m_sections.isEmpty() && !text.trimmed().isEmpty()) {
        // Build text from non-metadata lines
        QStringList contentLines;
        for (int i = startLine; i < metadataStartLine; ++i) {
            contentLines.append(lines[i]);
        }
        QString content = contentLines.join("\n").trimmed();
        if (!content.isEmpty()) {
            SongSection section("verse", "Verse 1", content);
            song.m_sections.append(section);
        }
    }

    return song;
}

Song Song::fromUsrFile(const QString& content)
{
    Song song;

    // USR files are INI-style with sections like [File] and [S <CCLI#>]
    // The song data is in the [S <number>] section
    QStringList lines = content.split('\n');

    bool inSongSection = false;
    QString ccliNumber;

    // Temporary storage for parsing
    QString fieldsLine;
    QString wordsLine;

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();

        // Check for section headers
        if (line.startsWith('[') && line.endsWith(']')) {
            QString sectionName = line.mid(1, line.length() - 2);

            // Check for song section: [S <CCLI#>]
            if (sectionName.startsWith("S ") || sectionName.startsWith("s ")) {
                inSongSection = true;
                ccliNumber = sectionName.mid(2).trimmed();
                song.setCcliNumber(ccliNumber);
            } else {
                inSongSection = false;
            }
            continue;
        }

        if (!inSongSection) {
            continue;
        }

        // Parse key=value pairs in song section
        int equalsPos = line.indexOf('=');
        if (equalsPos <= 0) {
            continue;
        }

        QString key = line.left(equalsPos).trimmed();
        QString value = line.mid(equalsPos + 1);

        if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            song.setTitle(value);
        }
        else if (key.compare("Author", Qt::CaseInsensitive) == 0) {
            // Authors are pipe-delimited
            QStringList authors = value.split('|', Qt::SkipEmptyParts);
            song.setAuthor(authors.join(", "));
        }
        else if (key.compare("Copyright", Qt::CaseInsensitive) == 0) {
            // Copyright entries are pipe-delimited
            QStringList copyrights = value.split('|', Qt::SkipEmptyParts);
            song.setCopyright(copyrights.join("; "));
        }
        else if (key.compare("Fields", Qt::CaseInsensitive) == 0) {
            fieldsLine = value;
        }
        else if (key.compare("Words", Qt::CaseInsensitive) == 0) {
            wordsLine = value;
        }
    }

    // Parse sections from Fields and Words
    // Fields: tab-delimited section names like "Vers 1\tChorus 1\tVers 2"
    // Words: tab-delimited lyrics, /n for newlines within each section
    if (!fieldsLine.isEmpty() && !wordsLine.isEmpty()) {
        QStringList sectionNames = fieldsLine.split('\t', Qt::KeepEmptyParts);
        QStringList sectionLyrics = wordsLine.split('\t', Qt::KeepEmptyParts);

        for (int i = 0; i < sectionNames.count() && i < sectionLyrics.count(); ++i) {
            QString name = sectionNames[i].trimmed();
            QString lyrics = sectionLyrics[i];

            // Convert /n to actual newlines
            lyrics.replace("/n", "\n");
            lyrics = lyrics.trimmed();

            if (name.isEmpty() || lyrics.isEmpty()) {
                continue;
            }

            // Determine section type from name
            // USR uses "Vers" not "Verse", "Chor" for chorus
            QString type = "verse";
            QString label = name;

            QString nameLower = name.toLower();
            if (nameLower.startsWith("vers") || nameLower.startsWith("verse")) {
                type = "verse";
                // Extract number if present
                static QRegularExpression versNum(R"(\d+$)");
                QRegularExpressionMatch match = versNum.match(name);
                if (match.hasMatch()) {
                    label = "Verse " + match.captured(0);
                } else {
                    label = "Verse";
                }
            }
            else if (nameLower.startsWith("chor") || nameLower.startsWith("chorus")) {
                type = "chorus";
                static QRegularExpression chorusNum(R"(\d+$)");
                QRegularExpressionMatch match = chorusNum.match(name);
                if (match.hasMatch()) {
                    label = "Chorus " + match.captured(0);
                } else {
                    label = "Chorus";
                }
            }
            else if (nameLower.startsWith("bridge")) {
                type = "bridge";
                label = "Bridge";
            }
            else if (nameLower.startsWith("pre-chorus") || nameLower.startsWith("prechorus")) {
                type = "pre-chorus";
                label = "Pre-Chorus";
            }
            else if (nameLower.startsWith("tag")) {
                type = "tag";
                label = "Tag";
            }
            else if (nameLower.startsWith("intro")) {
                type = "intro";
                label = "Intro";
            }
            else if (nameLower.startsWith("outro") || nameLower.startsWith("ending")) {
                type = "outro";
                label = "Outro";
            }
            else if (nameLower.startsWith("interlude")) {
                type = "interlude";
                label = "Interlude";
            }
            else if (nameLower.startsWith("vamp") || nameLower.startsWith("misc")) {
                type = "misc";
                // Keep original label for misc sections
            }

            SongSection section(type, label, lyrics);
            song.addSection(section);
        }
    }

    if (song.title().isEmpty()) {
        qWarning() << "USR file parsing: No title found";
    }

    return song;
}

} // namespace Clarity
