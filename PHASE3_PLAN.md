# Clarity - Phase 3 Implementation Plan

Detailed implementation plans for Phase 3 features, building on the solid foundation from Phases 1 and 2.

---

## Overview

Phase 3 focuses on content creation features that will make Clarity truly useful for church services. The core presentation functionality is complete; now we add the tools that help users build presentations quickly and professionally.

**Phase 3 Goals:**
- Enable rapid content creation through Bible and song integrations
- Add visual polish with transitions and themes
- Improve presenter experience with notes and shortcuts
- Expand media capabilities
- Enable remote operation

**Estimated Total Effort:** 40-60 hours of development

---

## Task 1: Scripture/Bible Integration

### Overview
Allow users to search for and insert Bible verses directly into presentations. Support multiple translations and smart verse range selection.

### Goals
- Search Bible by reference (John 3:16) or keyword ("love")
- Support multiple translations (KJV, NIV, ESV, NASB, NLT)
- Insert verses as new slides with proper formatting
- Handle verse ranges (John 3:16-17) and multiple verses
- Offline capability (embedded Bible data)

### Data Source Strategy

**Option A: Embedded SQLite Database (Recommended)**
- Ship with public domain translations (KJV, ASV, WEB)
- ~5MB database file
- No internet required
- Fast local queries

**Option B: Online API**
- Use API.Bible or similar service
- Requires API key and internet
- Access to more translations
- Rate limits and latency concerns

**Recommended Approach:** Start with embedded KJV/WEB, add API option later.

### Files to Create

#### `src/Core/BibleDatabase.h` / `BibleDatabase.cpp`
**Purpose:** SQLite interface for Bible verse lookup

```cpp
class BibleDatabase : public QObject {
    Q_OBJECT

public:
    explicit BibleDatabase(QObject* parent = nullptr);

    // Search methods
    QList<BibleVerse> lookupReference(const QString& reference);
    QList<BibleVerse> searchKeyword(const QString& keyword, int maxResults = 50);

    // Translation management
    QStringList availableTranslations() const;
    void setDefaultTranslation(const QString& translation);

    // Reference parsing
    static BibleReference parseReference(const QString& text);

private:
    QSqlDatabase m_database;
    QString m_defaultTranslation;
};

struct BibleVerse {
    QString book;
    int chapter;
    int verse;
    QString text;
    QString translation;

    QString reference() const; // "John 3:16"
    QString fullReference() const; // "John 3:16 (KJV)"
};

struct BibleReference {
    QString book;
    int startChapter;
    int startVerse;
    int endChapter;
    int endVerse;
    bool isValid;
};
```

#### `src/Control/ScriptureDialog.h` / `ScriptureDialog.cpp`
**Purpose:** UI for searching and selecting scripture

**UI Layout:**
```
+--------------------------------------------------+
| Insert Scripture                              [X] |
+--------------------------------------------------+
| Search: [John 3:16________________] [Search]     |
|                                                   |
| Translation: [KJV ▼]                             |
|                                                   |
| Results:                                          |
| +----------------------------------------------+ |
| | John 3:16                                    | |
| | For God so loved the world, that he gave    | |
| | his only begotten Son...                     | |
| +----------------------------------------------+ |
| | John 3:17                                    | |
| | For God sent not his Son into the world...  | |
| +----------------------------------------------+ |
|                                                   |
| Preview:                                          |
| +----------------------------------------------+ |
| | [Slide preview with selected verses]         | |
| +----------------------------------------------+ |
|                                                   |
| Options:                                          |
|   ☑ Include reference on slide                   |
|   ☐ One verse per slide                          |
|   Font size: [48 ▼]                              |
|                                                   |
|                         [Cancel]  [Insert]        |
+--------------------------------------------------+
```

**Implementation:**
```cpp
class ScriptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScriptureDialog(BibleDatabase* bible, QWidget* parent = nullptr);
    QList<Slide> getSlides() const;

private slots:
    void onSearch();
    void onResultSelected();
    void onTranslationChanged();
    void updatePreview();

private:
    void setupUI();
    Slide createSlideFromVerses(const QList<BibleVerse>& verses);

    BibleDatabase* m_bible;
    QLineEdit* m_searchEdit;
    QComboBox* m_translationCombo;
    QListWidget* m_resultsList;
    QLabel* m_previewLabel;
    QCheckBox* m_includeReferenceCheck;
    QCheckBox* m_onePerSlideCheck;
    QSpinBox* m_fontSizeSpinBox;

    QList<BibleVerse> m_selectedVerses;
};
```

### Database Schema

```sql
CREATE TABLE books (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    abbreviation TEXT,
    testament TEXT  -- 'OT' or 'NT'
);

CREATE TABLE verses (
    id INTEGER PRIMARY KEY,
    book_id INTEGER,
    chapter INTEGER,
    verse INTEGER,
    text TEXT,
    translation TEXT,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

CREATE INDEX idx_verses_reference ON verses(book_id, chapter, verse);
CREATE INDEX idx_verses_translation ON verses(translation);
CREATE VIRTUAL TABLE verses_fts USING fts5(text, content=verses, content_rowid=id);
```

### Reference Parsing

Support multiple formats:
- `John 3:16` - Single verse
- `John 3:16-17` - Verse range
- `John 3:16-4:3` - Cross-chapter range
- `Jn 3:16` - Abbreviations
- `1 John 3:16` - Books with numbers
- `Psalm 23` - Entire chapter

```cpp
BibleReference BibleDatabase::parseReference(const QString& text) {
    // Regex patterns for different formats
    static QRegularExpression singleVerse(
        R"((\d?\s*\w+)\s+(\d+):(\d+))"
    );
    static QRegularExpression verseRange(
        R"((\d?\s*\w+)\s+(\d+):(\d+)-(\d+))"
    );
    // ... additional patterns
}
```

### Integration Points

**ControlWindow.cpp:**
- Add "Insert Scripture" button or menu item
- Connect to ScriptureDialog
- Insert returned slides into presentation

**Slide Menu:**
```cpp
slideMenu->addAction("Insert &Scripture...", this, &ControlWindow::insertScripture,
                     QKeySequence("Ctrl+B")); // B for Bible
```

### Testing Checklist

- [ ] Search by reference returns correct verses
- [ ] Search by keyword finds relevant verses
- [ ] Verse ranges parsed correctly
- [ ] Multiple translations work
- [ ] Slides created with proper formatting
- [ ] Reference included when option checked
- [ ] One-per-slide option creates multiple slides
- [ ] Database performs well (< 100ms queries)

### Dependencies
- Qt6 SQL module
- SQLite (bundled with Qt)
- Bible data in public domain (KJV, WEB, ASV)

### Estimated Complexity
**Medium-High** - Database setup and reference parsing require care

---

## Task 2: Song Lyrics Database

### Overview
Import and manage song lyrics for worship. Support common formats (OpenLyrics, CCLI SongSelect export, plain text) and organize songs in a searchable library.

### Goals
- Import songs from files (OpenLyrics XML, ChordPro, plain text)
- Searchable song library
- Quick insert of songs into presentations
- Automatic slide breaking by verse/chorus
- CCLI license number tracking (for compliance)

### Files to Create

#### `src/Core/Song.h` / `Song.cpp`
**Purpose:** Data model for songs

```cpp
class Song {
public:
    QString title;
    QString author;
    QString copyright;
    QString ccliNumber;
    QList<SongSection> sections;

    QJsonObject toJson() const;
    static Song fromJson(const QJsonObject& json);
    static Song fromOpenLyrics(const QString& xml);
    static Song fromChordPro(const QString& text);
    static Song fromPlainText(const QString& text, const QString& title);

    QList<Slide> toSlides(const SlideStyle& style) const;
};

struct SongSection {
    QString type;  // "verse", "chorus", "bridge", "pre-chorus", "tag"
    QString label; // "Verse 1", "Chorus", etc.
    QString text;
    int repeatCount = 1;
};

struct SlideStyle {
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int fontSize;
    // Future: background image, gradient
};
```

#### `src/Core/SongLibrary.h` / `SongLibrary.cpp`
**Purpose:** Manage collection of songs with persistence

```cpp
class SongLibrary : public QObject {
    Q_OBJECT

public:
    explicit SongLibrary(QObject* parent = nullptr);

    // Library management
    void addSong(const Song& song);
    void updateSong(int id, const Song& song);
    void removeSong(int id);
    Song getSong(int id) const;

    // Search
    QList<Song> search(const QString& query) const;
    QList<Song> allSongs() const;
    QList<Song> recentSongs(int count = 10) const;

    // Import/Export
    bool importFromFile(const QString& filePath);
    bool exportToFile(int songId, const QString& filePath);

    // Persistence
    void saveLibrary();
    void loadLibrary();

signals:
    void songAdded(int id);
    void songUpdated(int id);
    void songRemoved(int id);
    void libraryLoaded();

private:
    QList<Song> m_songs;
    QString m_libraryPath;
};
```

#### `src/Control/SongLibraryDialog.h` / `SongLibraryDialog.cpp`
**Purpose:** UI for browsing and managing songs

**UI Layout:**
```
+--------------------------------------------------+
| Song Library                                  [X] |
+--------------------------------------------------+
| Search: [________________________] [🔍]          |
|                                                   |
| +---------------------+-------------------------+ |
| | Songs               | Amazing Grace           | |
| |---------------------|-------------------------| |
| | Amazing Grace     > | Verse 1:                | |
| | How Great Thou Art  | Amazing grace, how      | |
| | 10,000 Reasons      | sweet the sound...      | |
| | Blessed Be Your...  |                         | |
| | Great Is Thy...     | Verse 2:                | |
| |                     | 'Twas grace that        | |
| |                     | taught my heart...      | |
| |                     |                         | |
| |                     | [Author: John Newton]   | |
| |                     | [CCLI: 1234567]         | |
| +---------------------+-------------------------+ |
|                                                   |
| [Import...] [New Song] [Edit] [Delete]           |
|                                                   |
|                         [Cancel]  [Insert]        |
+--------------------------------------------------+
```

#### `src/Control/SongEditorDialog.h` / `SongEditorDialog.cpp`
**Purpose:** Create and edit songs

```cpp
class SongEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SongEditorDialog(QWidget* parent = nullptr);
    explicit SongEditorDialog(const Song& song, QWidget* parent = nullptr);
    Song getSong() const;

private:
    void setupUI();
    void loadSong(const Song& song);

    QLineEdit* m_titleEdit;
    QLineEdit* m_authorEdit;
    QLineEdit* m_copyrightEdit;
    QLineEdit* m_ccliEdit;
    QTextEdit* m_lyricsEdit;  // With section markers
    QListWidget* m_sectionList;
};
```

### OpenLyrics Format Support

OpenLyrics is an open XML format for song lyrics:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<song xmlns="http://openlyrics.info/namespace/2009/song">
  <properties>
    <titles><title>Amazing Grace</title></titles>
    <authors><author>John Newton</author></authors>
    <ccliNo>1234567</ccliNo>
  </properties>
  <lyrics>
    <verse name="v1">
      <lines>Amazing grace, how sweet the sound</lines>
      <lines>That saved a wretch like me</lines>
    </verse>
    <verse name="c">
      <lines>My chains are gone, I've been set free</lines>
    </verse>
  </lyrics>
</song>
```

### Song Library Storage

**File: `~/.config/Clarity/songs.json`**
```json
{
  "version": "1.0",
  "songs": [
    {
      "id": 1,
      "title": "Amazing Grace",
      "author": "John Newton",
      "copyright": "Public Domain",
      "ccliNumber": "",
      "sections": [
        {
          "type": "verse",
          "label": "Verse 1",
          "text": "Amazing grace, how sweet the sound\nThat saved a wretch like me"
        }
      ],
      "addedDate": "2026-01-24T10:00:00",
      "lastUsed": "2026-01-24T14:30:00"
    }
  ]
}
```

### Slide Generation

When inserting a song, generate slides with configurable options:
- Lines per slide (2, 4, 6, or auto-fit)
- Section labels on slides (optional)
- Repeat chorus after each verse (optional)
- Blank slide between songs (optional)

```cpp
QList<Slide> Song::toSlides(const SlideStyle& style) const {
    QList<Slide> slides;

    for (const SongSection& section : m_sections) {
        Slide slide;
        slide.setText(section.text);
        slide.setBackgroundColor(style.backgroundColor);
        slide.setTextColor(style.textColor);
        slide.setFontFamily(style.fontFamily);
        slide.setFontSize(style.fontSize);

        // Optionally prepend section label
        if (m_includeSectionLabels) {
            slide.setText(section.label + "\n\n" + section.text);
        }

        slides.append(slide);
    }

    return slides;
}
```

### Integration Points

**ControlWindow:**
- Add "Song Library" button to toolbar
- Add "Insert Song" menu item (Ctrl+L for Lyrics)
- Quick-insert recent songs

**Slide Menu:**
```cpp
slideMenu->addAction("Insert &Song...", this, &ControlWindow::insertSong,
                     QKeySequence("Ctrl+L"));
```

### Testing Checklist

- [ ] Import OpenLyrics XML files
- [ ] Import plain text with manual section marking
- [ ] Search finds songs by title, author, lyrics
- [ ] Edit existing songs
- [ ] Delete songs with confirmation
- [ ] Generate slides with correct formatting
- [ ] CCLI numbers stored and displayed
- [ ] Library persists between sessions
- [ ] Recent songs list works

### Dependencies
- Qt6 XML module (for OpenLyrics parsing)
- Qt6 Core (JSON for library storage)

### Estimated Complexity
**Medium** - File parsing and library management are straightforward

---

## Task 3: Slide Transitions

### Overview
Add visual transitions between slides for a more polished presentation. Support common transition types with configurable duration.

### Goals
- Fade transition (cross-dissolve)
- Slide transitions (left, right, up, down)
- Cut (instant, no animation)
- Configurable duration (0.25s - 2s)
- Per-slide transition override (optional)
- Default transition in settings

### Implementation Approach

**QML Animation Strategy:**
Use Qt Quick's animation system with two layers:
1. Current slide (fading out / sliding out)
2. Next slide (fading in / sliding in)

### Files to Modify

#### `src/Core/SettingsManager.h` / `SettingsManager.cpp`
**Add transition settings:**
```cpp
// Transition settings
QString defaultTransition() const;  // "fade", "slideLeft", "cut", etc.
void setDefaultTransition(const QString& transition);

int transitionDuration() const;  // milliseconds
void setTransitionDuration(int ms);
```

#### `src/Output/OutputDisplay.h` / `OutputDisplay.cpp`
**Add transition properties:**
```cpp
Q_PROPERTY(QString transitionType READ transitionType NOTIFY transitionTypeChanged)
Q_PROPERTY(int transitionDuration READ transitionDuration NOTIFY transitionDurationChanged)
Q_PROPERTY(bool isTransitioning READ isTransitioning NOTIFY isTransitioningChanged)

// Transition trigger
Q_INVOKABLE void startTransition();
```

#### `src/Output/qml/OutputDisplay.qml`
**Add transition animations:**

```qml
import QtQuick

Window {
    id: root
    visible: false
    color: "black"

    // Current slide (will animate out)
    Item {
        id: currentSlideContainer
        anchors.fill: parent
        opacity: 1

        Loader {
            id: currentSlideLoader
            anchors.fill: parent
            sourceComponent: slideComponent
        }
    }

    // Next slide (will animate in)
    Item {
        id: nextSlideContainer
        anchors.fill: parent
        opacity: 0

        Loader {
            id: nextSlideLoader
            anchors.fill: parent
            sourceComponent: slideComponent
        }
    }

    // Slide content component
    Component {
        id: slideComponent

        Rectangle {
            id: slideBackground
            anchors.fill: parent
            color: slideData.backgroundColor

            // Gradient background
            Rectangle {
                visible: slideData.backgroundType === "gradient"
                // ... gradient rendering
            }

            // Image background
            Image {
                visible: slideData.backgroundType === "image"
                // ... image rendering
            }

            // Text overlay
            Text {
                anchors.centerIn: parent
                text: slideData.text
                color: slideData.textColor
                // ... text properties
            }
        }
    }

    // Transition states
    states: [
        State {
            name: "showCurrent"
            PropertyChanges { target: currentSlideContainer; opacity: 1; x: 0 }
            PropertyChanges { target: nextSlideContainer; opacity: 0; x: root.width }
        },
        State {
            name: "showNext"
            PropertyChanges { target: currentSlideContainer; opacity: 0; x: -root.width }
            PropertyChanges { target: nextSlideContainer; opacity: 1; x: 0 }
        }
    ]

    // Fade transition
    transitions: [
        Transition {
            to: "showNext"

            // Fade transition
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation {
                        target: currentSlideContainer
                        property: "opacity"
                        duration: displayController.transitionDuration
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        target: nextSlideContainer
                        property: "opacity"
                        duration: displayController.transitionDuration
                        easing.type: Easing.InOutQuad
                    }
                }
                ScriptAction {
                    script: displayController.transitionComplete()
                }
            }
        }
    ]

    // Transition type selection
    function getTransition() {
        switch (displayController.transitionType) {
            case "fade":
                return fadeTransition
            case "slideLeft":
                return slideLeftTransition
            case "slideRight":
                return slideRightTransition
            case "cut":
                return cutTransition
            default:
                return fadeTransition
        }
    }
}
```

### Transition Types

1. **Cut** - Instant switch, no animation
2. **Fade** - Cross-dissolve (both slides visible during transition)
3. **Slide Left** - Current slides out left, next slides in from right
4. **Slide Right** - Current slides out right, next slides in from left
5. **Slide Up** - Current slides up, next slides in from bottom
6. **Slide Down** - Current slides down, next slides in from top

### Settings UI Addition

**SettingsDialog - Display Page:**
```
Transitions:
  Default transition: [Fade ▼]
  Duration: [500 ms ▼]  (250, 500, 750, 1000, 1500, 2000)

  ☐ Preview transitions in control window
```

### Testing Checklist

- [ ] Fade transition smooth and complete
- [ ] Slide transitions move in correct direction
- [ ] Cut transition is instant
- [ ] Duration setting affects all transitions
- [ ] Transition completes before next navigation allowed
- [ ] Rapid navigation doesn't break transitions
- [ ] Clear output doesn't leave transition artifacts
- [ ] Confidence monitor doesn't show transitions

### Dependencies
- Qt6 Quick (animations built-in)

### Estimated Complexity
**Medium** - QML animations are powerful but require careful state management

---

## Task 4: Themes/Templates

### Overview
Create reusable visual themes that can be applied to slides or entire presentations. Include preset themes and allow custom theme creation.

### Goals
- Preset themes (Classic, Modern, Nature, etc.)
- Custom theme creation
- Apply theme to single slide or all slides
- Theme includes: colors, fonts, default background
- Save/load custom themes

### Files to Create

#### `src/Core/Theme.h` / `Theme.cpp`
**Purpose:** Theme data model

```cpp
class Theme {
public:
    QString name;
    QString description;
    bool isBuiltIn;  // Cannot delete built-in themes

    // Colors
    QColor backgroundColor;
    QColor textColor;
    QColor accentColor;

    // Typography
    QString fontFamily;
    int titleFontSize;
    int bodyFontSize;

    // Background
    Slide::BackgroundType backgroundType;
    QColor gradientStartColor;
    QColor gradientEndColor;
    int gradientAngle;
    QByteArray backgroundImageData;

    // Methods
    void applyToSlide(Slide& slide) const;
    Slide createSlide(const QString& text) const;

    QJsonObject toJson() const;
    static Theme fromJson(const QJsonObject& json);
};
```

#### `src/Core/ThemeManager.h` / `ThemeManager.cpp`
**Purpose:** Manage theme collection

```cpp
class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject* parent = nullptr);

    QList<Theme> allThemes() const;
    QList<Theme> builtInThemes() const;
    QList<Theme> customThemes() const;

    Theme getTheme(const QString& name) const;
    void addTheme(const Theme& theme);
    void updateTheme(const QString& name, const Theme& theme);
    void removeTheme(const QString& name);

    void saveCustomThemes();
    void loadCustomThemes();

signals:
    void themeAdded(const QString& name);
    void themeRemoved(const QString& name);

private:
    void initBuiltInThemes();

    QList<Theme> m_builtInThemes;
    QList<Theme> m_customThemes;
};
```

### Built-In Themes

1. **Classic Blue** - Dark blue background, white text, Arial
2. **Modern Dark** - Near-black background, white text, Helvetica
3. **Warm Earth** - Brown gradient, cream text, Georgia
4. **Ocean** - Blue gradient, white text, Verdana
5. **Sunrise** - Orange/yellow gradient, dark text
6. **Forest** - Green gradient, white text
7. **Royal Purple** - Purple gradient, gold accents
8. **Clean White** - White background, dark text (for projectors in bright rooms)

### Theme Editor Dialog

```
+--------------------------------------------------+
| Edit Theme                                    [X] |
+--------------------------------------------------+
| Name: [My Custom Theme_______________]           |
|                                                   |
| Colors:                                           |
|   Background: [■ #1e3a8a] Text: [■ #ffffff]     |
|   Accent: [■ #fbbf24]                            |
|                                                   |
| Typography:                                       |
|   Font: [Arial ▼]                                |
|   Title size: [72 ▼]  Body size: [48 ▼]         |
|                                                   |
| Background Type:                                  |
|   ○ Solid Color  ● Gradient  ○ Image            |
|                                                   |
| Gradient:                                         |
|   Start: [■ #1e3a8a]  End: [■ #60a5fa]          |
|   Angle: [135°___]                               |
|                                                   |
| Preview:                                          |
| +----------------------------------------------+ |
| |     [Live preview of theme]                  | |
| +----------------------------------------------+ |
|                                                   |
|                         [Cancel]  [Save]          |
+--------------------------------------------------+
```

### Integration

**Apply Theme Options:**
- Right-click slide → Apply Theme → [theme list]
- Edit menu → Apply Theme to All Slides
- New slide inherits current theme
- Theme selector in slide editor

### Testing Checklist

- [ ] Built-in themes load correctly
- [ ] Apply theme to single slide
- [ ] Apply theme to all slides
- [ ] Create custom theme
- [ ] Edit custom theme
- [ ] Delete custom theme (not built-in)
- [ ] Custom themes persist between sessions
- [ ] Preview updates live in editor

### Dependencies
- None (uses existing Qt components)

### Estimated Complexity
**Low-Medium** - Straightforward data model and UI

---

## Task 5: Presenter Notes

### Overview
Add a notes field to slides that appears only on the confidence monitor, helping presenters remember key points, scripture references, or cues.

### Goals
- Notes field on each slide
- Notes displayed on confidence monitor
- Notes NOT displayed on output
- Notes included in slide editor
- Notes searchable (future)

### Files to Modify

#### `src/Core/Slide.h` / `Slide.cpp`
**Add notes field:**
```cpp
// Existing fields...
QString m_notes;  // Presenter notes, not shown on output

// Getters/setters
QString notes() const { return m_notes; }
void setNotes(const QString& notes) { m_notes = notes; }

// Update toJson/fromJson to include notes
```

#### `src/Control/SlideEditorDialog.cpp`
**Add notes editor:**
```cpp
// In setupUI(), add Notes section
QGroupBox* notesGroup = new QGroupBox("Presenter Notes", this);
QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);

m_notesEdit = new QTextEdit(notesGroup);
m_notesEdit->setPlaceholderText("Notes for the presenter (not shown on output)...");
m_notesEdit->setMaximumHeight(100);
notesLayout->addWidget(m_notesEdit);

mainLayout->addWidget(notesGroup);
```

#### `src/Confidence/ConfidenceDisplay.h` / `ConfidenceDisplay.cpp`
**Add notes property:**
```cpp
Q_PROPERTY(QString currentNotes READ currentNotes NOTIFY currentSlideChanged)

QString currentNotes() const { return m_currentSlide.notes(); }
```

#### `src/Confidence/qml/ConfidenceMonitor.qml`
**Add notes display:**
```qml
// Notes panel (below or beside next slide)
Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 150
    color: "#1a1a1a"
    border.color: "#4a4a4a"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Text {
            text: "Notes"
            color: "#888888"
            font.pixelSize: 14
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                width: parent.width
                text: confidenceDisplay.currentNotes || "(No notes for this slide)"
                color: confidenceDisplay.currentNotes ? "#ffffff" : "#666666"
                font.pixelSize: 16
                wrapMode: Text.WordWrap
                font.italic: !confidenceDisplay.currentNotes
            }
        }
    }
}
```

### IPC Update

Include notes in confidence data message:
```json
{
  "type": "confidenceData",
  "currentSlide": {
    "text": "...",
    "notes": "Remember to mention the potluck after service"
  }
}
```

### Testing Checklist

- [ ] Add notes to slide in editor
- [ ] Notes appear on confidence monitor
- [ ] Notes do NOT appear on output display
- [ ] Notes saved/loaded with presentation
- [ ] Empty notes show placeholder text
- [ ] Long notes are scrollable

### Dependencies
- None

### Estimated Complexity
**Low** - Simple text field addition

---

## Task 6: Keyboard Shortcuts

### Overview
Add comprehensive keyboard shortcuts for efficient presentation control without mouse interaction.

### Goals
- Navigation shortcuts (arrows, page up/down)
- Quick actions (clear, black screen)
- Slide management shortcuts
- Customizable shortcuts (future)

### Shortcut Map

**Navigation:**
| Key | Action |
|-----|--------|
| Right Arrow / Page Down / Space | Next slide |
| Left Arrow / Page Up | Previous slide |
| Home | First slide |
| End | Last slide |
| 1-9 | Go to slide 1-9 |
| G | Go to slide (prompt for number) |

**Display Control:**
| Key | Action |
|-----|--------|
| B | Black screen (clear output) |
| W | White screen |
| Escape | Clear output / Exit fullscreen |
| O | Toggle output display |
| F | Toggle output fullscreen |
| C | Toggle confidence monitor |

**Slide Management:**
| Key | Action |
|-----|--------|
| Ctrl+N | New presentation |
| Ctrl+O | Open presentation |
| Ctrl+S | Save presentation |
| Ctrl+Shift+N | New slide |
| Ctrl+E | Edit current slide |
| Delete | Delete selected slide |
| Ctrl+Up | Move slide up |
| Ctrl+Down | Move slide down |

**Content:**
| Key | Action |
|-----|--------|
| Ctrl+B | Insert Bible verse |
| Ctrl+L | Insert song lyrics |

### Implementation

**ControlWindow.cpp:**
```cpp
void ControlWindow::setupShortcuts() {
    // Navigation
    new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(onNextSlide()));
    new QShortcut(QKeySequence(Qt::Key_PageDown), this, SLOT(onNextSlide()));
    new QShortcut(QKeySequence(Qt::Key_Space), this, SLOT(onNextSlide()));

    new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(onPrevSlide()));
    new QShortcut(QKeySequence(Qt::Key_PageUp), this, SLOT(onPrevSlide()));

    new QShortcut(QKeySequence(Qt::Key_Home), this, SLOT(gotoFirstSlide()));
    new QShortcut(QKeySequence(Qt::Key_End), this, SLOT(gotoLastSlide()));

    // Display control
    new QShortcut(QKeySequence(Qt::Key_B), this, SLOT(blackScreen()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(onClearOutput()));

    // Number keys for quick navigation
    for (int i = 1; i <= 9; i++) {
        QShortcut* shortcut = new QShortcut(
            QKeySequence(Qt::Key_0 + i), this
        );
        connect(shortcut, &QShortcut::activated, [this, i]() {
            gotoSlide(i - 1);  // 0-indexed
        });
    }
}
```

### Shortcut Reference Dialog

Add Help → Keyboard Shortcuts menu item showing all available shortcuts.

### Testing Checklist

- [ ] All navigation shortcuts work
- [ ] Number keys go to correct slides
- [ ] B key blacks out screen
- [ ] Escape clears output
- [ ] Ctrl+key combinations work
- [ ] Shortcuts work when slide list has focus
- [ ] Shortcuts don't interfere with text editing

### Dependencies
- Qt6 Widgets (QShortcut)

### Estimated Complexity
**Low** - Qt's QShortcut makes this straightforward

---

## Task 7: Media Support

### Overview
Add support for video backgrounds and audio playback during presentations.

### Goals
- Video backgrounds on slides
- Audio playback (background music, sound effects)
- Media controls (play, pause, volume)
- Common format support (MP4, MP3, WAV)

### Video Backgrounds

#### `src/Core/Slide.h` / `Slide.cpp`
**Add video background support:**
```cpp
enum BackgroundType { SolidColor, Image, Gradient, Video };

QString m_backgroundVideoPath;
bool m_videoLoop = true;
bool m_videoMuted = true;  // Usually muted for backgrounds
```

#### `src/Output/qml/OutputDisplay.qml`
**Add video background:**
```qml
import QtMultimedia

// Video background component
Component {
    id: videoBackground

    Video {
        anchors.fill: parent
        source: displayController.backgroundVideoSource
        loops: displayController.videoLoop ? MediaPlayer.Infinite : 1
        muted: displayController.videoMuted
        fillMode: VideoOutput.PreserveAspectCrop
        autoPlay: true
    }
}
```

### Audio Playback

#### `src/Core/AudioPlayer.h` / `AudioPlayer.cpp`
**Purpose:** Manage background audio

```cpp
class AudioPlayer : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)

public:
    explicit AudioPlayer(QObject* parent = nullptr);

    void play(const QString& filePath);
    void pause();
    void stop();
    void setVolume(int volume);  // 0-100

    bool isPlaying() const;
    int volume() const;
    qint64 position() const;
    qint64 duration() const;

signals:
    void isPlayingChanged();
    void volumeChanged();
    void positionChanged();
    void durationChanged();
    void finished();

private:
    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;
};
```

### Media Library

Store media references (not embedded like images due to size):
- Video/audio files referenced by path
- Warn if files are moved/missing
- Option to copy to presentation folder

### UI Integration

**Control Window - Media Panel:**
```
+---------------------------+
| Media                     |
|---------------------------|
| ▶ [Now Playing: hymn.mp3] |
| [▮▮] Volume: [████░░░] 70%|
| [◀◀] [▶/▮▮] [▶▶] [🔁]    |
+---------------------------+
```

### Testing Checklist

- [ ] Video backgrounds play smoothly
- [ ] Video loops correctly
- [ ] Audio playback works
- [ ] Volume control responsive
- [ ] Pause/resume works
- [ ] Different formats supported (MP4, MP3, WAV)
- [ ] Missing media files handled gracefully

### Dependencies
- Qt6 Multimedia module

### Estimated Complexity
**Medium-High** - Multimedia requires careful handling and format support

---

## Task 8: Remote Control

### Overview
Enable control of presentations from mobile devices or secondary computers via a web interface.

### Goals
- Web-based remote control
- Show current/next slide
- Navigation controls
- QR code for easy connection
- Optional: Native mobile app (future)

### Architecture

**Embedded Web Server:**
- Use Qt's HTTP server capabilities or embed a lightweight server
- Serve simple HTML/JS interface
- WebSocket for real-time updates

### Files to Create

#### `src/Core/RemoteServer.h` / `RemoteServer.cpp`
**Purpose:** HTTP/WebSocket server for remote control

```cpp
class RemoteServer : public QObject {
    Q_OBJECT

public:
    explicit RemoteServer(quint16 port = 8080, QObject* parent = nullptr);

    void start();
    void stop();
    bool isRunning() const;

    QString serverUrl() const;
    QImage qrCode() const;  // QR code for easy connection

    // Send updates to connected clients
    void broadcastSlideUpdate(const Slide& current, const Slide& next);

signals:
    void clientConnected();
    void clientDisconnected();
    void navigationRequested(const QString& action);  // "next", "prev", "goto:5"

private:
    void handleHttpRequest(QTcpSocket* client);
    void handleWebSocketMessage(const QString& message);

    QTcpServer* m_httpServer;
    QList<QWebSocket*> m_webSocketClients;
    quint16 m_port;
};
```

### Web Interface

**`resources/remote/index.html`:**
```html
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Clarity Remote</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            background: #1a1a1a;
            color: white;
            margin: 0;
            padding: 20px;
        }
        .slide-preview {
            background: #2a2a2a;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            text-align: center;
        }
        .controls {
            display: flex;
            justify-content: center;
            gap: 20px;
        }
        button {
            background: #3b82f6;
            color: white;
            border: none;
            padding: 20px 40px;
            font-size: 24px;
            border-radius: 8px;
            cursor: pointer;
        }
        button:active {
            background: #2563eb;
        }
    </style>
</head>
<body>
    <div class="slide-preview">
        <h3>Current Slide</h3>
        <p id="currentSlide">Connecting...</p>
    </div>

    <div class="slide-preview">
        <h3>Next Slide</h3>
        <p id="nextSlide">-</p>
    </div>

    <div class="controls">
        <button onclick="send('prev')">◀ Prev</button>
        <button onclick="send('next')">Next ▶</button>
    </div>

    <script>
        const ws = new WebSocket(`ws://${location.host}/ws`);

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            document.getElementById('currentSlide').textContent = data.current;
            document.getElementById('nextSlide').textContent = data.next || '(End)';
        };

        function send(action) {
            ws.send(JSON.stringify({ action }));
        }
    </script>
</body>
</html>
```

### Settings Integration

**SettingsDialog - Remote Control Page:**
```
Remote Control:
  ☑ Enable remote control server
  Port: [8080]

  Server URL: http://192.168.1.100:8080
  [Show QR Code]

  Connected clients: 2
```

### Security Considerations

- Local network only (no internet exposure)
- Optional PIN protection
- Show connected clients in UI
- Ability to disconnect clients

### Testing Checklist

- [ ] Web server starts/stops correctly
- [ ] Web interface loads on mobile browser
- [ ] Navigation commands work
- [ ] Slide updates appear in real-time
- [ ] QR code scans correctly
- [ ] Multiple clients supported
- [ ] Server handles disconnections gracefully

### Dependencies
- Qt6 Network (QTcpServer)
- Qt6 WebSockets (optional, for real-time)
- QR code library (qrencode or similar)

### Estimated Complexity
**High** - Web server and real-time sync require careful implementation

---

## Task 9: Multi-Language Support

### Overview
Internationalize the UI to support multiple languages.

### Goals
- Extract all user-facing strings
- Use Qt's translation system (lupdate/lrelease)
- Initial languages: English, Spanish
- Easy to add more languages

### Implementation

**Qt Translation System:**
1. Wrap all strings in `tr()` calls
2. Run `lupdate` to extract strings
3. Translate in Qt Linguist
4. Run `lrelease` to compile translations
5. Load translations at runtime

### Files to Modify

**All UI files:**
```cpp
// Before
button->setText("Next");

// After
button->setText(tr("Next"));
```

**Main.cpp:**
```cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Load translations
    QTranslator translator;
    QString locale = QLocale::system().name();  // e.g., "es_ES"

    if (translator.load("clarity_" + locale, ":/translations")) {
        app.installTranslator(&translator);
    }

    // ... rest of initialization
}
```

### Translation Files

**`translations/clarity_es.ts`** (Spanish)
**`translations/clarity_de.ts`** (German)
**`translations/clarity_fr.ts`** (French)

### Settings Integration

**SettingsDialog - General Page:**
```
Language:
  [English ▼]

  Note: Restart required for language change
```

### Testing Checklist

- [ ] All UI strings wrapped in tr()
- [ ] Translation files generated
- [ ] Spanish translation complete
- [ ] Language setting works
- [ ] Fallback to English for missing translations
- [ ] Right-to-left languages handled (future)

### Dependencies
- Qt6 Linguist tools (lupdate, lrelease)

### Estimated Complexity
**Medium** - Tedious string extraction but straightforward process

---

## Implementation Order Summary

| # | Task | Complexity | Est. Hours | Dependencies |
|---|------|------------|------------|--------------|
| 1 | Scripture/Bible Integration | Medium-High | 8-12 | None |
| 2 | Song Lyrics Database | Medium | 6-10 | None |
| 3 | Slide Transitions | Medium | 4-6 | None |
| 4 | Themes/Templates | Low-Medium | 4-6 | None |
| 5 | Presenter Notes | Low | 2-3 | None |
| 6 | Keyboard Shortcuts | Low | 2-3 | None |
| 7 | Media Support | Medium-High | 8-12 | Qt Multimedia |
| 8 | Remote Control | High | 10-14 | Qt Network |
| 9 | Multi-Language Support | Medium | 4-6 | Qt Linguist |

**Total Estimated Time:** 48-72 hours

---

## Phase 3 Complete Checklist

- [ ] Scripture lookup and insertion working
- [ ] Song library with import/export
- [ ] Slide transitions (fade, slide, cut)
- [ ] Theme system with presets and custom themes
- [ ] Presenter notes on slides
- [ ] Keyboard shortcuts for all common actions
- [ ] Video backgrounds
- [ ] Audio playback
- [ ] Remote control web interface
- [ ] Spanish translation (at minimum)
- [ ] Update DEVLOG.md with all changes
- [ ] Performance testing with large presentations
- [ ] Documentation updated

---

## Risk Mitigation

### Potential Issues

1. **Bible Database Size**: ~5MB acceptable for desktop app
2. **Video Performance**: May need hardware acceleration
3. **Remote Security**: Keep to local network, add PIN option
4. **Translation Quality**: Use professional translators or community

### Testing Strategy

- Test each feature in isolation
- Integration testing with full workflow
- Performance testing (100+ slides, video backgrounds)
- Cross-platform testing (Windows primary, Linux secondary)

---

## Phase 4 Planning Notes

Features to consider for Phase 4:

### Bible Enhancements
- **Full Bible Importer**: Tool/UI to import complete Bible translations from standard formats (OSIS, USFM, Zefania XML)
- **Red Letter Edition**: Format words of Jesus in red text on slides
  - Requires verse-level markup in database schema
  - UI option to enable/disable red letters
  - May need rich text support in slides

### Song Library Enhancements
- **SongSelect Integration**: Direct integration with CCLI SongSelect service
  - Search SongSelect catalog from within Clarity
  - Download lyrics directly (requires CCLI subscription)
  - Automatic CCLI reporting integration
  - OAuth authentication with SongSelect API
  - Cache downloaded songs in local library

### Presentation Structure Refactor (Playlist Model)
- **Problem**: Currently presentations are flat lists of slides. This makes it difficult to:
  - Apply a theme to "all slides in this song" or "all slides in this scripture passage"
  - Reorder entire songs/scripture blocks as units
  - Track which slides belong together logically
  - Show song/scripture metadata in confidence monitor

- **Proposed Solution**: Restructure Presentation to be a playlist of "items"
  - **PresentationItem** base class with derived types:
    - `SongItem` - references a Song from the library, generates slides on demand
    - `ScriptureItem` - references a scripture passage, generates slides on demand
    - `CustomSlideItem` - standalone slides (announcements, images, etc.)
    - `SlideGroupItem` - arbitrary group of slides that belong together
  - Each item can have its own theme/style settings
  - Items track their source (song ID, scripture reference) for updates
  - "Apply theme to item" applies to all slides in that item
  - Drag-and-drop reorders entire items, not individual slides
  - Confidence monitor can show "Now playing: Amazing Grace (Verse 2 of 4)"

- **Migration path**:
  - Existing .cly files load as a single SlideGroupItem
  - New presentations use item-based structure
  - Version field in JSON handles format detection

### Other Potential Features
- Radial gradients
- Multi-stop gradients
- Drag-and-drop slide reordering (item-level and slide-level)
- Undo/redo for edits
- Cloud sync for presentations
- Presentation templates marketplace

---

**End of Phase 3 Implementation Plan**
