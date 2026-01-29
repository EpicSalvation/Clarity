# Phase 4 Plan - Advanced Features

## Overview

Phase 4 focuses on polish, performance improvements, and advanced media features to enhance the presentation experience.

## Planned Tasks

### 1. Transition Refinements
**Priority: High**

Improve slide transition behavior:
- If a slide change is requested during an active transition, respect the new slide change instead of ignoring it
- Current behavior: transitions block new navigation until complete
- Desired behavior: new navigation interrupts current transition and starts transition to new slide
- Consider: immediate cut to new slide vs. smooth redirect to new target

### 2. Audio Playback Support
**Priority: Medium**

Add audio playback capabilities:
- Background music for slides
- Audio cues/sound effects
- Volume control
- Fade in/out options
- Continue across slides option

### 3. Background Blur Effects
**Priority: Low**

Add blur effect options:
- Blur behind text for legibility
- Frosted glass effect for text containers
- Variable blur radius

### 4. Ken Burns Effect
**Priority: Low**

Animated pan/zoom for image backgrounds:
- Configurable start and end positions
- Adjustable duration
- Direction presets (zoom in, zoom out, pan left, etc.)

### 5. Video Playback Controls
**Priority: Medium**

Enhanced video background options:
- Start time / end time trimming
- Playback speed control
- Audio track enable/disable per slide
- Loop point configuration

### 6. Lower-Third Animations
**Priority: Low**

Animated text entry/exit:
- Slide in from side
- Fade in/out
- Typewriter effect
- Configurable timing

### 7. Scripture Lookup Integration
**Priority: Medium**

Improved scripture features:
- Quick lookup by reference
- Multiple translation support
- Automatic verse formatting
- Scripture-specific themes

### 8. Bible Enhancements
**Priority: Medium**

- **Full Bible Importer**: Tool/UI to import complete Bible translations from standard formats (OSIS, USFM, Zefania XML)
- **Red Letter Edition**: Format words of Jesus in red text on slides
  - Requires verse-level markup in database schema
  - UI option to enable/disable red letters
  - May need rich text support in slides

### 9. Song Library Enhancements
**Priority: Medium**

- **SongSelect Integration**: Direct integration with CCLI SongSelect service
  - Search SongSelect catalog from within Clarity
  - Download lyrics directly (requires CCLI subscription)
  - Automatic CCLI reporting integration
  - OAuth authentication with SongSelect API
  - Cache downloaded songs in local library

### 10. Presentation Structure Refactor (Playlist Model)
**Priority: High**

Restructure presentations to be playlists of "items" rather than flat slide lists.

**Problem**: Currently presentations are flat lists of slides. This makes it difficult to:
- Apply a theme to "all slides in this song" or "all slides in this scripture passage"
- Reorder entire songs/scripture blocks as units
- Track which slides belong together logically
- Show song/scripture metadata in confidence monitor

**Proposed Solution**: PresentationItem base class with derived types:
- `SongItem` - references a Song from the library, generates slides on demand
- `ScriptureItem` - references a scripture passage, generates slides on demand
- `CustomSlideItem` - standalone slides (announcements, images, etc.)
- `SlideGroupItem` - arbitrary group of slides that belong together

**Benefits**:
- Each item can have its own theme/style settings
- Items track their source (song ID, scripture reference) for updates
- "Apply theme to item" applies to all slides in that item
- Drag-and-drop reorders entire items, not individual slides
- Confidence monitor can show "Now playing: Amazing Grace (Verse 2 of 4)"

**Migration path**:
- Existing .cly files load as a single SlideGroupItem
- New presentations use item-based structure
- Version field in JSON handles format detection

### 11. Other Potential Features
**Priority: Low**

- Radial gradients
- Multi-stop gradients
- Drag-and-drop slide reordering (item-level and slide-level)
- Undo/redo for edits
- Cloud sync for presentations
- Presentation templates marketplace

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
