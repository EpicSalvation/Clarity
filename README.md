# Clarity

A "ProPresenter lite" presentation application for small churches, focusing on reliability, performance, and simplicity.

## Current Status: Phase 1 - MVP

Phase 1 implements the core multi-process architecture with basic slide presentation functionality.

### Features Implemented

- ✅ Multi-process architecture (control, output, confidence)
- ✅ Single executable with three modes
- ✅ IPC communication via QLocalSocket/QLocalServer
- ✅ Basic slide display (text on solid color backgrounds)
- ✅ Control UI with slide list and navigation
- ✅ Output display with real-time updates
- ✅ Process management for launching displays

### Success Criterion

Click "Next" in the control window → see output display update in real-time.

## Architecture

```
┌─────────────────┐         IPC          ┌──────────────────┐
│  Control App    │◄────────────────────►│ Output Display   │
│  (Qt Widgets)   │   QLocalSocket       │  (Qt Quick/QML)  │
│  IPC Server     │                      │  IPC Client      │
└─────────────────┘                      └──────────────────┘
        │
        │ Launches
        ▼
┌──────────────────┐
│ Confidence Mon.  │
│  (Qt Quick/QML)  │
│  (Stub - Phase 1)│
└──────────────────┘
```

## Building

### Prerequisites

- Qt 6.5 or later
- CMake 3.16 or later
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

**Required Qt modules:**
- Qt6::Core
- Qt6::Widgets
- Qt6::Quick
- Qt6::Network

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run
./Clarity              # Control mode (default)
./Clarity --output     # Output display mode
./Clarity --confidence # Confidence monitor mode
```

### Windows

```powershell
# Using Qt from official installer
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" ..
cmake --build . --config Release

# Run
.\Release\Clarity.exe
```

## Usage

1. **Start the control app**: Run `./Clarity` (or just double-click the executable)

2. **Launch output display**: Click "Launch Output Display" button in the control window
   - Output display will open fullscreen on the primary monitor
   - It automatically connects to the control app via IPC

3. **Navigate slides**: Use the "Previous" and "Next" buttons or click slides in the list
   - Output display updates in real-time

4. **Clear output**: Click "Clear Output" to show black screen

## Project Structure

```
clarity/
├── CMakeLists.txt
├── CLAUDE.md              # Development guidelines
├── README.md
├── src/
│   ├── Main.cpp          # Entry point with mode routing
│   ├── Core/             # Shared business logic
│   │   ├── Slide.*
│   │   ├── Presentation.*
│   │   ├── PresentationModel.*
│   │   ├── IpcServer.*
│   │   └── IpcClient.*
│   ├── Control/          # Control application (Qt Widgets)
│   │   ├── ControlMain.*
│   │   ├── ControlWindow.*
│   │   └── ProcessManager.*
│   ├── Output/           # Output display (Qt Quick)
│   │   ├── OutputMain.*
│   │   ├── OutputDisplay.*
│   │   └── qml/OutputDisplay.qml
│   └── Confidence/       # Confidence monitor (stub)
│       ├── ConfidenceMain.*
│       └── qml/ConfidenceMonitor.qml
└── resources/
    └── Resources.qrc
```

## IPC Protocol

Messages are JSON objects sent over QLocalSocket, newline-delimited.

### Server name: `clarity-ipc`

### Message types:

```json
// Client identification
{ "type": "connect", "clientType": "output" }

// Slide data broadcast
{
  "type": "slideData",
  "index": 0,
  "slide": {
    "text": "Slide text here",
    "backgroundColor": "#1e3a8a",
    "textColor": "#ffffff",
    "fontFamily": "Arial",
    "fontSize": 48
  }
}

// Clear output to black
{ "type": "clearOutput" }
```

## Development

See [CLAUDE.md](CLAUDE.md) for detailed development guidelines, coding conventions, and architecture decisions.

## Roadmap

### Phase 2 (Planned)
- Background images
- Image slides
- Basic transitions
- File save/load (JSON format)

### Phase 3 (Planned)
- Confidence monitor implementation
- Scripture lookup
- Multi-monitor support
- Playlist management

## License

TBD

## Contributing

This is currently a personal project. Contribution guidelines will be established in a future release.
