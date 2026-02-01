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

- [ ] **Audio Playback Support**
  - [ ] Background music for slides
  - [ ] Audio cues/sound effects
  - [ ] Volume control
  - [ ] Fade in/out options
  - [ ] Continue across slides option

- [ ] **Video Playback Controls**
  - [ ] Start time / end time trimming
  - [ ] Playback speed control
  - [ ] Audio track enable/disable per slide
  - [ ] Loop point configuration

- [ ] **Scripture Lookup Integration**
  - [x] Quick lookup by reference
  - [x] Multiple translation support
  - [x] Automatic verse formatting
  - [ ] Scripture-specific themes

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
  - [ ] Direct API integration (requires CCLI developer account - future)

### Low Priority

- [ ] **Background Blur Effects**
  - [ ] Blur behind text for legibility
  - [ ] Frosted glass effect for text containers
  - [ ] Variable blur radius

- [ ] **Ken Burns Effect**
  - [ ] Configurable start and end positions
  - [ ] Adjustable duration
  - [ ] Direction presets (zoom in, zoom out, pan left, etc.)

- [ ] **Lower-Third Animations**
  - [ ] Slide in from side
  - [ ] Fade in/out
  - [ ] Typewriter effect
  - [ ] Configurable timing

- [ ] **Other Potential Features**
  - [ ] Radial gradients
  - [ ] Multi-stop gradients
  - [ ] Drag-and-drop slide reordering (item-level and slide-level)
  - [ ] Undo/redo for edits
  - [ ] Cloud sync for presentations
  - [ ] Presentation templates marketplace

## Technical Considerations

### Transition Interruption
The transition interruption feature requires careful handling:
1. Track current transition state (source slide, target slide, progress)
2. On new navigation during transition:
   - Option A: Immediately cut to current frame, then transition to new target
   - Option B: Redirect transition mid-flight to new target
   - Option C: Queue the new target and transition after current completes
3. Recommended: Option A for simplicity and predictable behavior

### Audio Architecture
- Consider using Qt Multimedia for audio playback
- Separate audio player from video player for background music
- Audio state should persist across slide changes when configured

## Success Criteria

- Transitions feel responsive even during rapid navigation
- Audio playback is reliable and doesn't affect slide performance
- Advanced effects don't impact presentation stability
- All features maintain the "reliability first" philosophy

## Notes

- Phase 4 features are enhancements; core functionality from Phases 1-3 must remain stable
- Performance testing required for Ken Burns and blur effects on lower-end hardware
- Consider feature flags for experimental features
