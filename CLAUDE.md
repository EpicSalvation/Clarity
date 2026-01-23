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

### Message Types (Phase 1)

```json
// Client identifies itself on connection
{ "type": "connect", "clientType": "output" }

// Server sends full slide data
{ "type": "slideData", "index": 0, "slide": { "text": "...", "backgroundColor": "#1e3a8a", "textColor": "#ffffff" } }

// Navigation commands
{ "type": "gotoSlide", "index": 3 }
{ "type": "nextSlide" }
{ "type": "prevSlide" }

// Clear to black
{ "type": "clearOutput" }
```

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
