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
  - [ ] Quick lookup by reference
  - [ ] Multiple translation support
  - [ ] Automatic verse formatting
  - [ ] Scripture-specific themes

- [ ] **Bible Enhancements**
  - [ ] Full Bible Importer (OSIS, USFM, Zefania XML formats)
  - [ ] Red Letter Edition (words of Jesus in red)
    - [ ] Verse-level markup in database schema
    - [ ] UI option to enable/disable red letters
    - [ ] Rich text support in slides

- [ ] **SongSelect Integration**
  - [ ] Search SongSelect catalog from within Clarity
  - [ ] Download lyrics directly (requires CCLI subscription)
  - [ ] Automatic CCLI reporting integration
  - [ ] OAuth authentication with SongSelect API
  - [ ] Cache downloaded songs in local library

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
