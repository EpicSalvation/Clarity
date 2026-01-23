# Clarity Sample Presentations

This directory contains sample `.cly` files for testing Clarity's features.

## Sample Files

### 1. `simple-service.cly`
**Use for**: Basic functionality testing
- 7 simple slides with consistent formatting
- Single-line and two-line text
- Default blue background (#1e3a8a) with white text
- Good for testing basic navigation and display

### 2. `amazing-grace.cly`
**Use for**: Multi-line text and song lyrics
- 5 slides including title and 4 verses
- Multi-line text with blank lines for formatting
- Demonstrates typical worship song layout
- Varying font sizes (72pt title, 48pt verses)

### 3. `psalm-23.cly`
**Use for**: Scripture and custom styling
- 8 slides with the complete text of Psalm 23
- Custom color scheme: dark gray (#2d3748) with gold (#fbbf24) title
- Georgia font family for a traditional look
- Varied font sizes (48-64pt)

### 4. `announcements.cly`
**Use for**: Colorful presentations and font variety
- 5 slides with different background colors
- Multiple font families: Arial, Helvetica, Verdana
- Bright, varied color schemes:
  - Green (#059669), Blue (#0284c7), Purple (#7c3aed)
  - Red (#be123c), Orange (#ea580c)
- Demonstrates information slides with structured content

### 5. `style-demo.cly`
**Use for**: Comprehensive feature testing
- 10 slides demonstrating all style capabilities
- Font sizes: 36pt to 72pt
- Font families: Arial, Georgia, Helvetica, Verdana
- Color combinations:
  - High contrast (black/white, white/black)
  - Classic (dark blue/white)
  - Warm tones (orange/cream)
  - Cool tones (teal/light blue)
- Tests edge cases and visual variety

## Feature Coverage

| Feature | Files |
|---------|-------|
| Multi-line text | All files |
| Multiple slides | All files |
| Different font sizes | psalm-23.cly, style-demo.cly |
| Different fonts | psalm-23.cly, announcements.cly, style-demo.cly |
| Color variety | announcements.cly, style-demo.cly |
| Large text (60+pt) | amazing-grace.cly, style-demo.cly, simple-service.cly |
| Small text (36pt) | style-demo.cly |

## Testing Recommendations

1. **First-time testing**: Start with `simple-service.cly`
2. **Multi-line testing**: Use `amazing-grace.cly`
3. **Style testing**: Use `style-demo.cly`
4. **Real-world simulation**: Use `psalm-23.cly` or `announcements.cly`
5. **Full feature test**: Load all files and navigate through each

## File Format Notes

All files use:
- JSON format with `.cly` extension
- UTF-8 encoding
- Indented formatting for readability
- Version 1.0 format specification
- ISO 8601 timestamps
