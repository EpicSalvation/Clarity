# Clarity

A "ProPresenter lite" presentation application for small churches, focusing on reliability, performance, and simplicity.

## Current Status: Phase 3 - Feature Complete

Clarity now includes comprehensive presentation features suitable for worship services, with a focus on ease of use and reliability.

### Features

#### Slide Backgrounds
- **Solid colors** with color picker
- **Gradients** with customizable start/end colors and angle
- **Images** with automatic scaling and centering
- **Videos** with loop control (muted for background use)

#### Text & Legibility
- Customizable font family, size, and color
- **Drop shadows** for text visibility on busy backgrounds
- **Background overlay** - semi-transparent layer over background
- **Text container** - box behind text with padding and rounded corners
- **Text band** - full-width horizontal strip behind text

#### Media Library
- Centralized storage for images and videos
- Automatic import when selecting backgrounds
- **Video thumbnails** generated via FFmpeg
- Clean grid-based browser with previews
- **Drag-and-drop** media onto slides to set backgrounds
  - Left-drag applies to a single slide
  - Right-drag applies to all slides in a group/item

#### Presentation Controls
- Grid and list view modes for slides
- Drag-and-drop slide reordering
- **Keyboard shortcuts** for efficient control:
  - Arrow keys, Page Up/Down, Space for navigation
  - Home/End for first/last slide
  - B for blackout, W for whiteout
  - D for output disable toggle
  - F1 for shortcuts reference
- **Slide transitions** with per-slide overrides
- Output preview in control window

#### Remote Control
- **Mobile web interface** for controlling presentations from phone/tablet
- **QR code** for easy connection (click URL in status bar)
- **PIN protection** to prevent unauthorized access
- Real-time slide preview and navigation controls

#### Multi-Language Support
- **Language selection** in Settings (General page)
- Supported languages: English, Spanish, German, French
- System language auto-detection option
- Mobile remote control page also translated

#### Song Library
- **Song management** with title, author, copyright, CCLI number
- **Multiple import formats**: OpenLyrics XML, SongSelect TXT, SongSelect USR
- **Batch import** with duplicate detection by CCLI number
- **Drag & drop** import support
- **Search** by title, author, lyrics, or CCLI number
- **Usage tracking** for CCLI reporting
- **CCLI Report generator** with date range filtering and CSV/text export
- **SongSelect search** opens browser search on Google or SongSelect directly

#### Scripture Support
- **Built-in Bible database** with multiple translations
- **Quick verse lookup** by book, chapter, verse
- **Import additional translations** (OSIS, USFM, USX, USFX, Zefania, TSV)
- **Red Letter Edition** - words of Jesus in red (configurable)
- **One verse per slide** option

#### Additional Features
- **Themes** for consistent slide styling
- **Presenter notes** for each slide
- **Confidence monitor** support
- Save/load presentations (.cly format)

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
│  IPC Client      │
└──────────────────┘
```

## Building

### Prerequisites

- Qt 6.5 or later
- CMake 3.16 or later
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **FFmpeg** (optional, for video thumbnails)

**Required Qt modules:**
- Qt6::Core
- Qt6::Widgets
- Qt6::Quick
- Qt6::Network
- Qt6::Multimedia
- Qt5Compat::GraphicalEffects

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

### FFmpeg Setup (for video thumbnails)

Video thumbnails require FFmpeg to be installed:

1. Download from https://www.gyan.dev/ffmpeg/builds/ (essentials build)
2. Extract and either:
   - Add the `bin` folder to your system PATH, or
   - Place `ffmpeg.exe` in the same folder as `Clarity.exe`

Without FFmpeg, video backgrounds will show placeholder thumbnails but still play correctly.

## Usage

1. **Start the control app**: Run `./Clarity` (or double-click the executable)

2. **Create slides**: Click "Add Slide" to create new slides
   - Set background type (color, gradient, image, or video)
   - Enter text content
   - Adjust text legibility settings as needed

3. **Launch output display**: Click "Launch Output Display"
   - Opens fullscreen on the selected monitor
   - Automatically connects via IPC

4. **Present**: Use keyboard shortcuts or click slides to navigate
   - Press F1 for keyboard shortcuts reference

5. **Save/Load**: Use File menu to save and load presentations (.cly files)

## Project Structure

```
clarity/
├── CMakeLists.txt
├── CLAUDE.md              # Development guidelines
├── DEVLOG.md              # Development history
├── README.md
├── src/
│   ├── Main.cpp           # Entry point with mode routing
│   ├── Core/              # Shared business logic
│   │   ├── Slide.*
│   │   ├── Presentation.*
│   │   ├── MediaLibrary.*
│   │   ├── VideoThumbnailGenerator.*
│   │   ├── IpcServer.*
│   │   └── IpcClient.*
│   ├── Control/           # Control application (Qt Widgets)
│   │   ├── ControlWindow.*
│   │   ├── SlideEditorDialog.*
│   │   ├── MediaLibraryDialog.*
│   │   └── ProcessManager.*
│   ├── Output/            # Output display (Qt Quick)
│   │   ├── OutputDisplay.*
│   │   └── qml/OutputDisplay.qml
│   └── Confidence/        # Confidence monitor
│       ├── ConfidenceMain.*
│       └── qml/ConfidenceMonitor.qml
└── resources/
    └── Resources.qrc
```

## Development

See [CLAUDE.md](CLAUDE.md) for detailed development guidelines, coding conventions, and architecture decisions.

See [DEVLOG.md](DEVLOG.md) for development history and technical decisions.

## Roadmap

### Phase 4 (Planned)
- **Transition refinements** - respect slide changes during transitions
- Audio playback support
- Background blur effects
- Ken Burns effect for images
- Video playback controls (start/end times)
- Lower-third animations
- Scripture lookup integration

See [PHASE4_PLAN.md](PHASE4_PLAN.md) for detailed planning.

## License

TBD

## Contributing

This is currently a personal project. Contribution guidelines will be established in a future release.
