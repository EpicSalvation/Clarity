# Clarity

A "ProPresenter lite" presentation application for small churches, focusing on reliability, performance, and simplicity.

## Current Status: Phase 4 - Advanced Features

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
  - T to clear text (keep background), R to clear background (keep text)
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

#### Copyright & CCLI
- **Dedicated Copyright settings page** for all licensing options
- **CCLI license number** display on song title slides
- **Auto-generated copyright slide** at end of presentation
- **CCLI Usage Report** accessible directly from settings

#### Scripture Support
- **Unified scripture dialog** — single tabbed interface for all scripture sources (Ctrl+B)
- **Built-in Bible database** with multiple translations
- **ESV API** integration for English Standard Version text
- **API.bible** integration with 2500+ Bible versions across 1600+ languages
- **Quick verse lookup** by book, chapter, verse
- **Import additional translations** (OSIS, USFM, USX, USFX, Zefania, TSV)
- **Red Letter Edition** - words of Jesus in red (local Bibles and supported API.bible versions)
- **One verse per slide** option

#### NDI Output
- **NDI streaming** for routing output to video switchers, capture software, and NDI monitors
- Offscreen rendering via `QQuickRenderControl` — no extra display needed
- Configurable resolution (`--width`, `--height`), frame rate (`--fps`), and source name (`--ndi-name`)
- Dynamic DLL loading — builds without the NDI SDK; only the runtime is needed
- Toggle on/off from control window or with **N** keyboard shortcut

#### Cascading Backgrounds
- **Cascading backgrounds** — set a background on one slide and it persists for all subsequent slides until another explicit background is encountered
- **Scripture theme override** — optionally apply a chosen theme's background to all scripture slides
- **"Use own background" toggle** in the slide editor to control whether a slide inherits or defines its own background
- Enabled by default; can be toggled in Settings > General > Backgrounds

#### Import
- **Import PowerPoint** (Ctrl+Shift+P) — exports slides as images via PowerShell COM automation (Windows)
- **Import Slide Images** (Ctrl+Shift+I) — import PNG/JPG/BMP files as slide backgrounds

#### Additional Features
- **Themes** for consistent slide styling (15 built-in themes with gradients)
- **Global undo/redo** for all presentation edits
- **Presenter notes** for each slide
- **Confidence monitor** support
- **Slide auto-advance** with per-slide and group-level timers
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
        ├──────────────────────►┌──────────────────┐
        │                      │ Confidence Mon.  │
        │                      │  (Qt Quick/QML)  │
        │                      │  IPC Client      │
        │                      └──────────────────┘
        │
        └──────────────────────►┌──────────────────┐
                               │ NDI Output       │
                               │  (Offscreen QML) │
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
- Qt6::OpenGL
- Qt5Compat::GraphicalEffects

**Optional runtime dependencies:**
- **NDI Tools** (for NDI output) — install from https://ndi.video/tools/

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
./Clarity --ndi        # NDI streaming mode (requires NDI runtime)
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
│   │   ├── ScriptureInsertDialog.*
│   │   ├── MediaLibraryDialog.*
│   │   └── ProcessManager.*
│   ├── Output/            # Output display (Qt Quick)
│   │   ├── OutputDisplay.*
│   │   └── qml/
│   │       ├── OutputDisplay.qml
│   │       └── OutputDisplayContent.qml
│   ├── Ndi/               # NDI streaming output
│   │   ├── NdiMain.*
│   │   ├── NdiSender.*
│   │   └── NdiTypes.h
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

### Phase 4 (In Progress)
- [x] Presentation structure refactor (playlist model)
- [x] Transition refinements
- [x] Slide auto-advance timer
- [x] Scripture lookup & Bible translation importer
- [x] Red Letter Edition
- [x] SongSelect integration & CCLI reporting
- [x] Background blur effects
- [x] Global undo/redo
- [x] Multi-stop & radial gradients, updated themes
- [x] NDI streaming output
- [x] Cascading backgrounds & scripture theme override
- [x] ESV API & API.bible scripture integration
- [x] Unified tabbed scripture dialog
- [x] API.bible red letter support
- [x] Copyright compliance (CCLI title slides, auto-generated copyright slide)
- [x] Dedicated Copyright settings page with CCLI report access
- [x] Import PowerPoint & Import Slide Images
- [ ] Audio playback support
- [ ] Video playback controls (start/end times)

See [PHASE4_PLAN.md](PHASE4_PLAN.md) for detailed planning.

## License

This project is licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.en.html). See the [LICENSE](LICENSE) file for details.

This application uses the [Qt framework](https://www.qt.io/), which is licensed under the LGPL v3.0. Qt libraries are dynamically linked. See the [NOTICE](NOTICE) file for full attribution.

## Contributing

This is currently a personal project. Contribution guidelines will be established in a future release.
