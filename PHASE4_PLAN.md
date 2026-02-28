# Phase 4 Plan - Advanced Features

## Overview

Phase 4 focuses on polish, performance improvements, and advanced media features to enhance the presentation experience.

## Task Checklist (sorted by priority)

### High Priority

- [x] **Presentation Structure Refactor (Playlist Model)**
  - [x] PresentationItem base class with SongItem, ScriptureItem, CustomSlideItem, SlideGroupItem
  - [x] Presentation refactored to use QList<PresentationItem*>
  - [x] ItemListModel for left panel showing items
  - [x] SlideFilterProxyModel for filtering grid by selected item
  - [x] v2.0 file format with automatic v1.0 migration
  - [x] IPC updates with item info for confidence monitor
  - [x] Settings option to toggle between showing all slides or item slides only

- [x] **Transition Refinements**
  - [x] Interrupt active transitions when new slide change is requested
  - [x] New navigation interrupts current transition and starts transition to new slide
  - [x] Implementation: immediate snap to end state, then begin new transition (Option A)

### Medium Priority

- [x] **Slide Auto-Advance Timer**
  - [x] Per-slide timer duration setting (seconds) to auto-advance to next slide
  - [x] Group-level default timer that applies to all slides in a SlideGroupItem
  - [x] Per-slide override to customize individual durations within a group
  - [x] Visual countdown indicator in control window (optional)
  - [x] Pause/resume auto-advance during presentation
  - [x] Timer resets when manually navigating away from a timed slide
  - [x] Works across item boundaries (last slide in group advances to first slide of next item)

- [x] **Scripture Lookup Integration**
  - [x] Quick lookup by reference
  - [x] Multiple translation support
  - [x] Automatic verse formatting
  - [x] Scripture-specific themes
  - [x] Unified tabbed scripture dialog (Local Bible, ESV API, API.bible)

- [x] **Bible Translation Importer**
  - [x] Multi-format support: OSIS XML, USFM, USX, USFX, Zefania XML, TSV
  - [x] Auto-detection of format by file content
  - [x] Folder import for multi-file USFM Bibles
  - [x] Import dialog with preview and progress tracking
  - [x] Translation management in Settings (import/delete)
  - [x] Persistent translation preferences (preferred vs. remember last)
  - [x] One verse per slide setting persistence

- [x] **Bible Enhancements (Red Letter Edition)**
  - [x] Words of Jesus displayed in red
  - [x] Verse-level markup in database schema
  - [x] UI option to enable/disable red letters
  - [x] Rich text support in slides
  - [x] API.bible red letter support (via `<span class="wj">` → `<span class="jesus">` conversion)

- [x] **SongSelect Integration**
  - [x] USR file format import (SongSelect's native format)
  - [x] Plain text import with SongSelect format auto-detection
  - [x] Batch import with duplicate detection by CCLI number
  - [x] Drag & drop file import
  - [x] SongSelect search (opens browser to Google or SongSelect)
  - [x] CCLI usage tracking when songs are displayed
  - [x] CCLI Report dialog with date range, CSV/text export
  - [x] Usage statistics in song library (times used, last used)
  - [x] Quick filters (used this month/year, never used, has CCLI#)
  - [x] Copyright compliance: CCLI info on song title slides, auto-generated copyright slide
  - [x] Dedicated Copyright settings page with CCLI report access

### Slide Templates

- [ ] **Slide Template System**
  - [ ] Template engine supporting multiple text zones with independent font, size, and color
  - [ ] Title slide template (large title + subtitle)
  - [ ] Title & body template (heading + paragraph text)
  - [ ] Scripture template (reference heading + verse body)
  - [ ] Blank template (single text zone, current default behavior)
  - [ ] Template selection in slide editor
  - [ ] Templates work with existing theme system (colors, backgrounds)

### Low Priority

- [x] **Background Blur Effects**
  - [x] Blur behind text for legibility (overlay blur with ShaderEffectSource downsampling)
  - [x] Frosted glass effect for text containers (text container blur + text band blur)
  - [x] Variable blur radius (0-50 range, maps to texture downsample divisor 1-16x)
  - [x] Performance logging to file (clarity_blur_perf.log in app data directory)

- [x] **NDI Streaming Output**
  - [x] Offscreen QML rendering via QQuickRenderControl + OpenGL FBO
  - [x] Dynamic NDI DLL loading (no SDK dependency at build time)
  - [x] Configurable resolution, frame rate, and NDI source name
  - [x] Double-buffered frame readback with vertical flip
  - [x] IPC integration — receives slideData, clearOutput, whiteout messages
  - [x] Toggle from control window (N shortcut) or LivePreviewPanel button
  - [x] Shared OutputDisplayContent.qml between output window and NDI

- [ ] **Other Potential Features**
  - [x] Radial gradients
  - [x] Multi-stop gradients
  - [x] Drag-and-drop slide reordering (item-level and slide-level)
  - [x] Undo/redo for edits

## Technical Considerations

### Transition Interruption
The transition interruption feature requires careful handling:
1. Track current transition state (source slide, target slide, progress)
2. On new navigation during transition:
   - Option A: Immediately cut to current frame, then transition to new target
   - Option B: Redirect transition mid-flight to new target
   - Option C: Queue the new target and transition after current completes
3. Recommended: Option A for simplicity and predictable behavior

### Blur Architecture
- Uses ShaderEffectSource with reduced textureSize for background blur (no external Qt modules needed)
- Bilinear filtering when displaying the downsampled texture at full size creates the blur appearance
- Four blur types: overlay (full-screen), text container (box behind text), text band (strip), drop shadow (soft shadow via layer rendering)
- Blur radius 0-50 maps to downsample divisor via `1 + radius * 0.3` (range 1x to 16x)
- Performance logging writes to `clarity_blur_perf.log` in the app data directory
- hideSource on overlay blur ShaderEffectSource hides original background when blur replaces it

## Success Criteria

- Transitions feel responsive even during rapid navigation
- Blur effects render correctly for overlay, text container, text band, and drop shadow
- Slide templates render correctly with multiple text zones
- Advanced effects don't impact presentation stability
- All features maintain the "reliability first" philosophy

## Notes

- Phase 4 features are enhancements; core functionality from Phases 1-3 must remain stable
- Performance testing required for blur effects on lower-end hardware
- Consider feature flags for experimental features
