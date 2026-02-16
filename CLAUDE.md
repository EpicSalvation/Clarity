# Clarity - Claude Code Guidelines

## Project Overview

Clarity is a "ProPresenter lite" application for small churches. It delivers core presentation functionality (lyrics, scripture, images with backgrounds) while prioritizing reliability, speed, and simplicity.

**Philosophy**: Reliability and performance over features. Core functionality must work flawlessly every time.

## Current Phase: Phase 1 (MVP)

**Goal**: Get multi-process architecture working with basic functionality.

**Deliverables**:
- Single executable with three modes (control, output, confidence)
- IPC communication via QLocalSocket/QLocalServer
- Basic slide display (text on solid color background)
- Control UI with slide list and next/prev navigation
- Output display that updates when control navigates
- Process launching from control app

**Success Criterion**: Click "Next" in control window, see output display update.

## Architecture

### Multi-Process Design
One executable runs in three modes based on command-line arguments:

```bash
clarity              # Control mode (default)
clarity --output     # Output display mode  
clarity --confidence # Confidence monitor mode
```

- **Control app**: Qt Widgets-based main interface; acts as IPC server
- **Output display**: Qt Quick/QML fullscreen renderer; IPC client
- **Confidence monitor**: Qt Quick/QML stage display; IPC client (Phase 1: stub only)

### Technology Stack
- **Language**: C++17
- **Framework**: Qt6 (Widgets, Quick, Network modules)
- **Build System**: CMake
- **IPC**: JSON messages over QLocalSocket

### Why This Architecture
- Process isolation: output crash doesn't affect control
- Performance: dedicated rendering processes
- Reliability: output keeps showing current slide if control crashes

## Project Structure

```
clarity/
├── CMakeLists.txt
├── CLAUDE.md
├── DEVLOG.md
├── README.md
├── src/
│   ├── Main.cpp                      # Entry point, mode routing
│   ├── Core/                         # Shared business logic
│   │   ├── Slide.h / Slide.cpp
│   │   ├── Presentation.h / Presentation.cpp
│   │   ├── PresentationModel.h / PresentationModel.cpp
│   │   ├── IpcServer.h / IpcServer.cpp
│   │   └── IpcClient.h / IpcClient.cpp
│   ├── Control/                      # Control application (Qt Widgets)
│   │   ├── ControlMain.h / ControlMain.cpp
│   │   ├── ControlWindow.h / ControlWindow.cpp
│   │   └── ProcessManager.h / ProcessManager.cpp
│   ├── Output/                       # Output display (Qt Quick)
│   │   ├── OutputMain.h / OutputMain.cpp
│   │   ├── OutputDisplay.h / OutputDisplay.cpp
│   │   └── qml/
│   │       └── OutputDisplay.qml
│   └── Confidence/                   # Confidence monitor (stub for Phase 1)
│       ├── ConfidenceMain.h / ConfidenceMain.cpp
│       └── qml/
│           └── ConfidenceMonitor.qml
└── resources/
    └── Resources.qrc
```

## Coding Conventions

### General
- Standard C++17, no exotic patterns
- PascalCase for filenames (e.g., `ControlWindow.cpp`)
- PascalCase for class names
- camelCase for methods and variables
- Use Qt's parent-child memory management; avoid raw `new` without parents

### Qt-Specific
- Prefer signals/slots over callbacks
- Use `Q_OBJECT` macro in all QObject subclasses
- Use `QSharedPointer` or `std::unique_ptr` for non-QObject heap allocations
- Always check return values on file and network operations

### Error Handling
- Never silently fail; log errors with `qWarning()` or `qCritical()`
- IPC disconnection: output keeps displaying current slide, control shows status
- Validate all JSON before processing

## IPC Protocol

JSON messages over QLocalSocket. Server name: `"clarity-ipc"`

### Transport Details
- **Protocol**: Newline-delimited JSON messages (`\n` separator)
- **Format**: Compact JSON (no whitespace) via `QJsonDocument::Compact`
- **Connection**: QLocalSocket/QLocalServer
- **Error Handling**: All JSON parsing errors are logged via `qWarning()`

### Message Types

#### 1. Connection Message (Client → Server)
Sent immediately upon connection to identify the client type.

```json
{
  "type": "connect",
  "clientType": "output"
}
```

**Fields**:
- `type` (string, required): Always `"connect"`
- `clientType` (string, required): Either `"output"` or `"confidence"`

**Usage**: Sent from IpcClient on connection (IpcClient.cpp:68-71)

---

#### 2. Slide Data Message (Server → Client)
Sends complete slide information to display clients.

```json
{
  "type": "slideData",
  "index": 0,
  "slide": {
    "text": "Verse 1\nAmazing grace, how sweet the sound",
    "backgroundColor": "#1e3a8a",
    "textColor": "#ffffff",
    "fontFamily": "Arial",
    "fontSize": 48
  }
}
```

**Fields**:
- `type` (string, required): Always `"slideData"`
- `index` (integer, required): Zero-based slide index in presentation
- `slide` (object, required): Complete Slide JSON object (see JSON Format Reference below)

**Usage**: Sent from ControlWindow when slides change (ControlWindow.cpp:177-180)

---

#### 3. Navigation Commands (Server → Client)

##### Next Slide
```json
{ "type": "nextSlide" }
```

##### Previous Slide
```json
{ "type": "prevSlide" }
```

##### Go to Specific Slide
```json
{
  "type": "gotoSlide",
  "index": 3
}
```

**Fields** (gotoSlide only):
- `type` (string, required): Always `"gotoSlide"`
- `index` (integer, required): Target slide index (zero-based)

**Note**: Navigation commands are reserved for future use; current implementation sends full `slideData` messages instead.

---

#### 4. Clear Output (Server → Client)
Clears the output display (black screen).

```json
{ "type": "clearOutput" }
```

**Fields**:
- `type` (string, required): Always `"clearOutput"`

**Usage**: Sent from ControlWindow clear button (ControlWindow.cpp:207-209)

## Data Models

### Slide (Phase 1 - Simplified)
```cpp
class Slide {
    QString text;
    QColor backgroundColor;  // Solid color only for Phase 1
    QColor textColor;
    QString fontFamily = "Arial";
    int fontSize = 48;
    
    QJsonObject toJson() const;
    static Slide fromJson(const QJsonObject& json);
};
```

### Presentation
```cpp
class Presentation {
    QList<Slide> slides;
    int currentSlideIndex = 0;
    QString title;
    
    QJsonObject toJson() const;
    static Presentation fromJson(const QJsonObject& json);
};
```

### PresentationModel
- Inherits `QAbstractListModel`
- Provides data to QListView in control window
- Roles: `TextRole`, `BackgroundColorRole`, `TextColorRole`

## JSON Format Reference

This section documents all JSON formats used in Clarity for serialization, IPC, and file storage.

### Slide JSON Format

Complete JSON representation of a single slide.

```json
{
  "text": "Verse 1\nAmazing grace, how sweet the sound\nThat saved a wretch like me",
  "backgroundColor": "#1e3a8a",
  "textColor": "#ffffff",
  "fontFamily": "Arial",
  "fontSize": 48
}
```

**Fields**:
- `text` (string, required): Slide content. Supports multi-line text with `\n` characters
- `backgroundColor` (string, required): Background color in hex format (e.g., `#1e3a8a`)
- `textColor` (string, required): Text color in hex format (e.g., `#ffffff`)
- `fontFamily` (string, required): Font family name (e.g., `"Arial"`, `"Helvetica"`)
- `fontSize` (integer, required): Font size in points (typically 24-72)
- `hasExplicitBackground` (boolean, optional): Whether this slide defines its own background (`true`) or inherits via cascade (`false`). Default `true`. Only serialized when `false`.

**Default Values** (when deserializing with missing fields):
- `backgroundColor`: `"#1e3a8a"` (dark blue)
- `textColor`: `"#ffffff"` (white)
- `fontFamily`: `"Arial"`
- `fontSize`: `48`
- `hasExplicitBackground`: `true`

**Implementation**: Slide.cpp:22-42
- Serialization: `Slide::toJson()`
- Deserialization: `Slide::fromJson(const QJsonObject&)`
- Color Serialization: Uses `QColor::name()` to produce hex format

---

### Presentation JSON Format

Complete JSON representation of a presentation document.

```json
{
  "version": "1.0",
  "title": "Sunday Service - January 23, 2026",
  "currentSlideIndex": 0,
  "createdDate": "2026-01-23T10:30:45",
  "modifiedDate": "2026-01-23T11:45:30",
  "slides": [
    {
      "text": "Amazing Grace",
      "backgroundColor": "#1e3a8a",
      "textColor": "#ffffff",
      "fontFamily": "Arial",
      "fontSize": 48
    },
    {
      "text": "Verse 1\nAmazing grace, how sweet the sound",
      "backgroundColor": "#1e3a8a",
      "textColor": "#ffffff",
      "fontFamily": "Arial",
      "fontSize": 48
    }
  ]
}
```

**Fields**:
- `version` (string, required): Format version for future compatibility. Currently `"1.0"`
- `title` (string, required): Presentation title
- `currentSlideIndex` (integer, required): Current slide position (zero-based)
- `createdDate` (string, required): ISO 8601 timestamp of creation (e.g., `"2026-01-23T10:30:45"`)
- `modifiedDate` (string, required): ISO 8601 timestamp of last modification
- `slides` (array, required): Array of Slide JSON objects (may be empty)

**Default Values** (when deserializing with missing fields):
- `title`: `"Untitled"`
- `currentSlideIndex`: Validated to not exceed slide count; clamped to valid range

**Validation**:
- Version must be `"1.0"` (future versions may support migration)
- `currentSlideIndex` is validated against slide array length
- Timestamps are auto-generated on save, preserved on load

**Implementation**: Presentation.cpp:124-164
- Serialization: `Presentation::toJson()`
- Deserialization: `Presentation::fromJson(const QJsonObject&)`
- Timestamp Format: Qt's `QDateTime::toString(Qt::ISODate)` produces ISO 8601

---

### File Format Specification

#### File Extension
`.cly` (Clarity Presentation)

#### File Format
Human-readable indented JSON using the Presentation JSON format.

**Example .cly File**:
```json
{
  "version": "1.0",
  "title": "Sunday Service",
  "currentSlideIndex": 0,
  "createdDate": "2026-01-23T10:30:45",
  "modifiedDate": "2026-01-23T11:45:30",
  "slides": [
    {
      "text": "Welcome",
      "backgroundColor": "#1e3a8a",
      "textColor": "#ffffff",
      "fontFamily": "Arial",
      "fontSize": 48
    }
  ]
}
```

**Format Details**:
- **Encoding**: UTF-8
- **Whitespace**: Indented for readability (`QJsonDocument::Indented`)
- **Structure**: Direct serialization of Presentation::toJson()

**File Operations** (ControlWindow.cpp):
- **Save** (lines 417-438):
  - Opens file in WriteOnly mode
  - Serializes Presentation to indented JSON
  - Writes to disk and marks presentation as clean
  - Auto-appends `.cly` extension if missing (line 456)

- **Load** (lines 380-407):
  - Opens file in ReadOnly mode
  - Reads entire file content
  - Parses JSON with error handling
  - Deserializes via `Presentation::fromJson()`
  - Validates structure before loading into UI

**File Dialog Configuration**:
- Default filter: `"Clarity Presentations (*.cly);;All Files (*)"`
- Default location: User's home directory (`QDir::homePath()`)

**Compatibility**:
- Version field enables future format migrations
- Unknown fields are ignored (forward compatibility)
- Missing optional fields use default values (backward compatibility)

---

### JSON Serialization Architecture

**Qt Classes Used**:
- `QJsonObject`: Primary data structure for messages and objects
- `QJsonArray`: Slide collections in presentations
- `QJsonDocument`: Conversion to/from byte arrays with formatting options
- `QJsonParseError`: Error detection during deserialization
- `QColor::name()`: Color serialization to hex format (e.g., `#ffffff`)

**Serialization Format**:
- **IPC Messages**: Compact (no whitespace) - `QJsonDocument::Compact`
- **File Storage**: Indented (human-readable) - `QJsonDocument::Indented`
- **Newline Delimiter**: IPC messages are newline-delimited (`\n`)

**Error Handling**:
- All JSON parsing uses `QJsonParseError` validation
- Invalid JSON logged with `qWarning()` but doesn't crash
- Silent failures are logged, not thrown as exceptions
- Missing fields fall back to documented default values

**Key Files**:
- `/home/user/Clarity/src/Core/Slide.cpp` - Slide JSON serialization
- `/home/user/Clarity/src/Core/Presentation.cpp` - Presentation JSON serialization
- `/home/user/Clarity/src/Core/IpcServer.cpp` - Message transmission
- `/home/user/Clarity/src/Core/IpcClient.cpp` - Message reception
- `/home/user/Clarity/src/Control/ControlWindow.cpp` - File I/O operations

## Qt Quick Notes

The developer has moderate Qt experience but is new to Qt Quick/QML.

When writing QML:
- Add clear comments explaining QML concepts
- Keep QML files focused and simple
- Expose C++ data via Q_PROPERTY or context properties
- For Phase 1, OutputDisplay.qml just needs: Rectangle background + centered Text

Example OutputDisplay.qml structure:
```qml
import QtQuick

Window {
    id: root
    visibility: Window.FullScreen
    color: displayController.backgroundColor  // Exposed from C++
    
    Text {
        anchors.centerIn: parent
        text: displayController.slideText
        color: displayController.textColor
        font.pixelSize: displayController.fontSize
    }
}
```

## Build Instructions

### Prerequisites
- Qt 6.5+ with: Core, Widgets, Quick, Network
- CMake 3.16+
- C++17 compatible compiler

### CMake Structure
```cmake
cmake_minimum_required(VERSION 3.16)
project(Clarity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick Network)

# Core library (shared between all modes)
add_library(ClarityCore STATIC ...)
target_link_libraries(ClarityCore Qt6::Core Qt6::Network)

# Main executable
add_executable(Clarity ...)
target_link_libraries(Clarity ClarityCore Qt6::Widgets Qt6::Quick)
```

## Development Workflow

1. **Scaffold first**: Create project structure and stub files
2. **Core library**: Implement Slide, Presentation, IPC classes
3. **Output display**: Get QML window showing hardcoded text
4. **Control window**: Basic UI with slide list and buttons
5. **Integration**: Wire up IPC so control drives output
6. **Process management**: Launch output from control app

## Development Log (DEVLOG.md)

**IMPORTANT**: The development log must be updated for every significant task or work session on this project.

### When to Update the Devlog

Update DEVLOG.md after completing:
- Any feature implementation or enhancement
- Bug fixes (especially non-trivial ones)
- Architectural changes or refactoring
- Performance optimizations
- Build system or dependency updates
- Documentation updates (if substantial)
- Each work session (at minimum, daily if working continuously)

### What to Include in Each Entry

Each devlog entry should contain:

1. **Date and Title**: Clear identifier (e.g., "2026-01-23 - Phase 1 Scaffolding Complete")
2. **Summary**: One paragraph overview of what was accomplished
3. **Work Completed**: Detailed list of changes, implementations, and fixes
4. **Technical Decisions**: Any significant architectural or implementation choices made
5. **Testing**: Testing approach and results (if applicable)
6. **Issues/Blockers**: Any problems encountered or unresolved issues
7. **Next Steps**: What to work on next
8. **Commit(s)**: Branch and commit hash/message

### Devlog Format

Follow the template provided at the bottom of DEVLOG.md. Entries should be:
- **Chronological**: Newest entries at the top (after the header)
- **Detailed**: Include enough context for someone to understand what was done and why
- **Honest**: Document problems and limitations, not just successes
- **Actionable**: Include clear next steps

### Benefits

The devlog serves multiple purposes:
- **Project history**: Track decision-making and evolution
- **Onboarding**: Help new developers (or future you) understand the codebase
- **Debugging**: Reference what changed when tracking down issues
- **Communication**: Share progress with stakeholders
- **Learning**: Reflect on what worked and what didn't

**Never skip the devlog** - it takes 2-5 minutes but saves hours later when trying to remember why something was done a certain way.

## Key Constraints

- **Windows primary development**, but keep code cross-platform (no Windows-specific APIs)
- **Performance**: Slide transitions must feel instant (<50ms)
- **Reliability**: Output must keep displaying if control crashes
- **Simplicity**: Phase 1 is minimal; resist adding features not listed

## Testing Approach

For Phase 1, manual testing is acceptable:
1. Run control app, verify UI appears
2. Run output app separately, verify it connects
3. Navigate slides in control, verify output updates
4. Kill control app, verify output keeps showing last slide

## Questions to Ask

If requirements are ambiguous, ask before implementing. Prefer simple solutions that can be enhanced later over complex solutions that anticipate future needs.
