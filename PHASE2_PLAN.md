# Clarity - Phase 2 Implementation Plan

Detailed implementation plans for Phase 2 features, ordered by dependency and logical progression.

---

## Task 1: Save/Load Presentations (JSON Files)

### Overview
Replace hardcoded demo presentations with file-based persistence. Users can create, save, and load presentation files (.cly JSON format).

### Goals
- Load presentations from JSON files
- Save presentations to JSON files
- Handle file I/O errors gracefully
- Support "Save" vs "Save As" workflows
- Track "dirty" state (unsaved changes)

### Files to Modify

#### `src/Control/ControlWindow.h` / `ControlWindow.cpp`
**Changes:**
- Add File menu to menu bar (New, Open, Save, Save As, Exit)
- Add member variable: `QString m_currentFilePath` (tracks loaded file)
- Add member variable: `bool m_isDirty` (tracks unsaved changes)
- Add slot: `newPresentation()` - Clear current presentation, reset to blank
- Add slot: `openPresentation()` - QFileDialog, load JSON, populate model
- Add slot: `savePresentation()` - Save to current file (or prompt if new)
- Add slot: `saveAsPresentation()` - QFileDialog, save to new file
- Add slot: `onPresentationModified()` - Set m_isDirty = true
- Add method: `updateWindowTitle()` - Show filename and dirty indicator (*) in title
- Add method: `promptSaveIfDirty()` - Ask user to save before New/Open/Exit

**UI Changes:**
```cpp
// In constructor
QMenuBar* menuBar = new QMenuBar(this);
QMenu* fileMenu = menuBar->addMenu("&File");

fileMenu->addAction("&New", this, &ControlWindow::newPresentation, QKeySequence::New);
fileMenu->addAction("&Open...", this, &ControlWindow::openPresentation, QKeySequence::Open);
fileMenu->addAction("&Save", this, &ControlWindow::savePresentation, QKeySequence::Save);
fileMenu->addAction("Save &As...", this, &ControlWindow::saveAsPresentation, QKeySequence::SaveAs);
fileMenu->addSeparator();
fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

setMenuBar(menuBar);
```

**File I/O Logic:**
```cpp
void ControlWindow::openPresentation() {
    if (!promptSaveIfDirty()) return; // User cancelled

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Presentation",
        QDir::homePath(),
        "Clarity Presentations (*.cly);;All Files (*.*)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file: " + file.errorString());
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid presentation file format.");
        return;
    }

    Presentation loaded = Presentation::fromJson(doc.object());
    m_presentation = loaded;
    m_model->setPresentation(m_presentation);
    m_currentFilePath = filePath;
    m_isDirty = false;
    updateWindowTitle();
}
```

#### `src/Core/PresentationModel.h` / `PresentationModel.cpp`
**Changes:**
- Add method: `setPresentation(const Presentation& presentation)` - Replace entire presentation
- Emit `dataChanged()` signals after updates
- Add signal: `presentationModified()` - Emit when slides are added/removed/edited

#### `src/Core/Presentation.h` / `Presentation.cpp`
**Verify/Update:**
- Ensure `toJson()` serializes all slide data correctly
- Ensure `fromJson()` deserializes all slide data correctly
- Add `title` field if not present (default to "Untitled Presentation")
- Consider adding metadata: `createdDate`, `modifiedDate`, `author`

### File Format

**Example .cly file structure:**
```json
{
  "version": "1.0",
  "title": "Sunday Service - January 26",
  "createdDate": "2026-01-23T10:30:00Z",
  "modifiedDate": "2026-01-23T11:45:00Z",
  "slides": [
    {
      "text": "Welcome to Our Church",
      "backgroundColor": "#1e3a8a",
      "textColor": "#ffffff",
      "fontFamily": "Arial",
      "fontSize": 72
    },
    {
      "text": "Amazing Grace\nHow sweet the sound...",
      "backgroundColor": "#1e3a8a",
      "textColor": "#ffffff",
      "fontFamily": "Arial",
      "fontSize": 48
    }
  ]
}
```

### UI/UX Details

**Window Title Format:**
- Unsaved new presentation: `Clarity - Untitled*`
- Saved presentation: `Clarity - Sunday Service.cly`
- Dirty presentation: `Clarity - Sunday Service.cly*`

**Unsaved Changes Prompt:**
```
"Presentation has unsaved changes. Save before closing?"
[Save] [Don't Save] [Cancel]
```

### Error Handling

**Scenarios to handle:**
- File doesn't exist (Open)
- File is not readable (permissions)
- File is not valid JSON
- File is valid JSON but wrong structure
- Disk full (Save)
- Write permissions denied (Save)
- File path contains invalid characters

**Strategy:**
- Use `QMessageBox::critical()` for errors
- Log errors with `qCritical()`
- Never crash - always fail gracefully
- Show user-friendly error messages (not JSON parse errors)

### Testing Checklist

- [ ] Create new presentation, save as `test.cly`, verify file exists
- [ ] Close and reopen Clarity, open `test.cly`, verify data loads correctly
- [ ] Make changes, verify `*` appears in title
- [ ] Press Save, verify `*` disappears
- [ ] Close without saving, verify prompt appears
- [ ] Cancel prompt, verify window stays open
- [ ] Try to open non-existent file, verify error message
- [ ] Try to open invalid JSON file, verify error message
- [ ] Try to save to read-only directory, verify error message
- [ ] Test keyboard shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S)

### Dependencies
- Qt6 Core (QFile, QJsonDocument)
- Qt6 Widgets (QFileDialog, QMessageBox, QMenuBar)

### Estimated Complexity
**Low-Medium** - Straightforward file I/O, well-established patterns

---

## Task 2: Image Backgrounds

### Overview
Allow slides to have image backgrounds instead of just solid colors. Users can import images (PNG, JPG) as slide backgrounds with text overlays.

### Goals
- Import images as slide backgrounds
- Display images correctly in output display
- Send images over IPC efficiently
- Support common formats (PNG, JPG, BMP)
- Handle aspect ratio and scaling

### Files to Modify

#### `src/Core/Slide.h` / `Slide.cpp`
**Changes:**
- Add enum: `BackgroundType { SolidColor, Image, Gradient }`
- Add member: `BackgroundType backgroundType = SolidColor`
- Add member: `QString backgroundImagePath` (file path to image)
- Add member: `QByteArray backgroundImageData` (base64-encoded image for IPC)
- Update `toJson()`: Serialize background type and image data
- Update `fromJson()`: Deserialize background type and image data

**Example JSON structure:**
```json
{
  "text": "Welcome",
  "textColor": "#ffffff",
  "fontFamily": "Arial",
  "fontSize": 72,
  "backgroundType": "image",
  "backgroundImagePath": "/home/user/backgrounds/sunset.jpg",
  "backgroundImageData": "iVBORw0KGgoAAAANS..." // base64 encoded
}
```

**Image Encoding for IPC:**
```cpp
// When preparing slide for IPC transmission
QImage image(slide.backgroundImagePath);
QByteArray byteArray;
QBuffer buffer(&byteArray);
buffer.open(QIODevice::WriteOnly);
image.save(&buffer, "PNG"); // Always use PNG for quality
slide.backgroundImageData = byteArray.toBase64();
```

#### `src/Control/ControlWindow.h` / `ControlWindow.cpp`
**Changes:**
- Add menu item: **Slide → Import Image Slide**
- Add slot: `importImageSlide()`
- In `importImageSlide()`:
  - Show QFileDialog for image selection
  - Load image file
  - Create new Slide with backgroundType = Image
  - Encode image to base64 for storage
  - Add slide to presentation
  - Mark presentation as dirty

**Import Logic:**
```cpp
void ControlWindow::importImageSlide() {
    QString imagePath = QFileDialog::getOpenFileName(
        this,
        "Select Image",
        QDir::homePath(),
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*.*)"
    );

    if (imagePath.isEmpty()) return;

    QImage image(imagePath);
    if (image.isNull()) {
        QMessageBox::critical(this, "Error", "Could not load image file.");
        return;
    }

    // Optionally resize large images to reasonable size (e.g., 1920x1080 max)
    if (image.width() > 1920 || image.height() > 1080) {
        image = image.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Slide slide;
    slide.backgroundType = Slide::BackgroundType::Image;
    slide.backgroundImagePath = imagePath; // For reference

    // Encode image for IPC
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    slide.backgroundImageData = byteArray.toBase64();

    // Default text overlay
    slide.text = ""; // No text by default on image slides
    slide.textColor = QColor("#ffffff");
    slide.fontFamily = "Arial";
    slide.fontSize = 48;

    m_presentation.addSlide(slide);
    m_model->refreshModel();
    markDirty();
}
```

#### `src/Output/OutputDisplay.h` / `OutputDisplay.cpp`
**Changes:**
- Add Q_PROPERTY: `QString backgroundType` (read-only, exposed to QML)
- Add Q_PROPERTY: `QString backgroundImageSource` (read-only, data URI or temp file path)
- In `handleSlideData()`: Decode base64 image data and prepare for QML
- Add method: `prepareImageSource()` - Convert base64 to usable format

**Image Data Handling:**
Option A: Data URI (simple, may have size limits)
```cpp
// In handleSlideData when backgroundType is "image"
QByteArray imageData = QByteArray::fromBase64(slide.backgroundImageData);
QString dataUri = "data:image/png;base64," + QString(slide.backgroundImageData);
m_backgroundImageSource = dataUri;
emit backgroundImageSourceChanged();
```

Option B: Temporary file (more reliable for large images)
```cpp
QByteArray imageData = QByteArray::fromBase64(slide.backgroundImageData);
QString tempPath = QDir::temp().filePath("clarity_slide_bg.png");
QFile tempFile(tempPath);
if (tempFile.open(QIODevice::WriteOnly)) {
    tempFile.write(imageData);
    tempFile.close();
    m_backgroundImageSource = "file://" + tempPath;
    emit backgroundImageSourceChanged();
}
```

#### `src/Output/qml/OutputDisplay.qml`
**Changes:**
- Add Image element for background
- Show Image when backgroundType is "image"
- Show Rectangle when backgroundType is "solidColor"
- Use stacking order: Background → Text

**QML Structure:**
```qml
Window {
    id: root
    visible: false
    color: "black"

    // Background layer
    Loader {
        id: backgroundLoader
        anchors.fill: parent

        sourceComponent: {
            if (displayController.backgroundType === "image") {
                return imageBackground
            } else {
                return solidBackground
            }
        }
    }

    Component {
        id: solidBackground
        Rectangle {
            color: displayController.backgroundColor
        }
    }

    Component {
        id: imageBackground
        Image {
            source: displayController.backgroundImageSource
            fillMode: Image.PreserveAspectCrop // or Image.Stretch
            anchors.fill: parent
        }
    }

    // Text overlay layer
    Text {
        anchors.centerIn: parent
        text: displayController.slideText
        color: displayController.textColor
        font.family: displayController.fontFamily
        font.pixelSize: displayController.fontSize
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        width: parent.width * 0.9 // Leave margins
    }
}
```

### IPC Protocol Update

**Updated slideData message:**
```json
{
  "type": "slideData",
  "index": 0,
  "slide": {
    "text": "Welcome",
    "textColor": "#ffffff",
    "fontFamily": "Arial",
    "fontSize": 72,
    "backgroundType": "image",
    "backgroundImageData": "iVBORw0KGgoAAAANS..."
  }
}
```

### UI/UX Considerations

**Slide List Thumbnails:**
- Consider showing tiny image preview in control window's slide list
- Use QListView's `decoration` role to show thumbnails
- Keep thumbnails small (64x64px) to avoid performance issues

**Import Workflow:**
1. User clicks "Slide → Import Image Slide"
2. File dialog appears
3. Image loads and is added to end of presentation
4. User can add text overlay later via editor (Task 4)

### Performance Considerations

**Image Size Limits:**
- Recommend max 1920x1080 images
- Warn user if importing 4K+ images
- Optionally auto-resize on import

**IPC Efficiency:**
- Base64 encoding increases size by ~33%
- A 1920x1080 PNG might be 1-3MB encoded
- QLocalSocket should handle this fine (local IPC, not network)
- Consider compression if sizes become problematic

**Memory:**
- Don't keep decoded QImage in memory unless displayed
- Load, encode, discard original during import

### Testing Checklist

- [ ] Import a PNG image, verify it appears in slide list
- [ ] Navigate to image slide, verify it displays in output
- [ ] Test different image formats (PNG, JPG, BMP)
- [ ] Test very large images (4K), verify they don't crash
- [ ] Test images with transparency (PNG alpha channel)
- [ ] Save presentation with image slide, reload, verify image persists
- [ ] Test aspect ratios: landscape, portrait, square
- [ ] Test fillMode: Crop vs Stretch behavior
- [ ] Test text overlay visibility on dark and light images

### Dependencies
- Qt6 Core (QImage, QBuffer, QByteArray)
- Qt6 Widgets (QFileDialog)
- Qt6 Quick (Image QML type)

### Estimated Complexity
**Medium** - Image encoding/decoding is straightforward, but IPC size and QML integration require care

---

## Task 3: Gradient Backgrounds

### Overview
Add support for linear gradients as slide backgrounds (2-color gradients at configurable angles).

### Goals
- Support 2-color linear gradients
- Allow angle configuration (0°, 45°, 90°, 135°, 180°, etc.)
- Display gradients correctly in output
- Keep UI simple (no complex gradient editor)

### Files to Modify

#### `src/Core/Slide.h` / `Slide.cpp`
**Changes:**
- Add member: `QColor gradientColor1` (gradient start color)
- Add member: `QColor gradientColor2` (gradient end color)
- Add member: `int gradientAngle` (0-360 degrees, default 90)
- Update `toJson()`: Serialize gradient properties
- Update `fromJson()`: Deserialize gradient properties

**JSON Example:**
```json
{
  "text": "Amazing Grace",
  "textColor": "#ffffff",
  "fontFamily": "Arial",
  "fontSize": 48,
  "backgroundType": "gradient",
  "gradientColor1": "#1e3a8a",
  "gradientColor2": "#3b82f6",
  "gradientAngle": 90
}
```

#### `src/Control/ControlWindow.h` / `ControlWindow.cpp`
**Changes:**
- Add menu item: **Slide → New Gradient Slide**
- Add slot: `newGradientSlide()`
- Show simple dialog to pick:
  - Color 1 (QColorDialog)
  - Color 2 (QColorDialog)
  - Angle (QComboBox with presets: 0°, 45°, 90°, 135°, 180°, etc.)
- Create slide with gradient background

**Dialog Example:**
```cpp
void ControlWindow::newGradientSlide() {
    // Simple modal dialog with color pickers and angle selector
    QDialog dialog(this);
    dialog.setWindowTitle("New Gradient Slide");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Color 1 picker
    QPushButton* color1Btn = new QPushButton("Pick Color 1");
    QColor color1 = QColor("#1e3a8a");
    // Connect to QColorDialog...

    // Color 2 picker
    QPushButton* color2Btn = new QPushButton("Pick Color 2");
    QColor color2 = QColor("#3b82f6");
    // Connect to QColorDialog...

    // Angle selector
    QComboBox* angleCombo = new QComboBox();
    angleCombo->addItem("0° (Horizontal)", 0);
    angleCombo->addItem("45° (Diagonal)", 45);
    angleCombo->addItem("90° (Vertical)", 90);
    angleCombo->addItem("135° (Diagonal)", 135);
    angleCombo->addItem("180° (Horizontal Reverse)", 180);

    layout->addWidget(new QLabel("Start Color:"));
    layout->addWidget(color1Btn);
    layout->addWidget(new QLabel("End Color:"));
    layout->addWidget(color2Btn);
    layout->addWidget(new QLabel("Angle:"));
    layout->addWidget(angleCombo);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        Slide slide;
        slide.backgroundType = Slide::BackgroundType::Gradient;
        slide.gradientColor1 = color1;
        slide.gradientColor2 = color2;
        slide.gradientAngle = angleCombo->currentData().toInt();
        slide.text = "";
        slide.textColor = QColor("#ffffff");
        slide.fontFamily = "Arial";
        slide.fontSize = 48;

        m_presentation.addSlide(slide);
        m_model->refreshModel();
        markDirty();
    }
}
```

#### `src/Output/OutputDisplay.h` / `OutputDisplay.cpp`
**Changes:**
- Add Q_PROPERTY: `QString gradientColor1`
- Add Q_PROPERTY: `QString gradientColor2`
- Add Q_PROPERTY: `int gradientAngle`
- In `handleSlideData()`: Extract gradient properties and emit property changes

#### `src/Output/qml/OutputDisplay.qml`
**Changes:**
- Add gradient component to background loader
- Use Qt Quick's LinearGradient (requires `import QtQuick.Shapes`)

**Updated QML:**
```qml
import QtQuick
import QtQuick.Shapes // For LinearGradient

Window {
    id: root
    visible: false
    color: "black"

    Loader {
        id: backgroundLoader
        anchors.fill: parent

        sourceComponent: {
            if (displayController.backgroundType === "image") {
                return imageBackground
            } else if (displayController.backgroundType === "gradient") {
                return gradientBackground
            } else {
                return solidBackground
            }
        }
    }

    Component {
        id: solidBackground
        Rectangle {
            color: displayController.backgroundColor
        }
    }

    Component {
        id: gradientBackground
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                // Qt Quick gradients are vertical by default
                // Rotate the rectangle to achieve angle
                rotation: displayController.gradientAngle - 90

                GradientStop { position: 0.0; color: displayController.gradientColor1 }
                GradientStop { position: 1.0; color: displayController.gradientColor2 }
            }
        }
    }

    Component {
        id: imageBackground
        Image {
            source: displayController.backgroundImageSource
            fillMode: Image.PreserveAspectCrop
            anchors.fill: parent
        }
    }

    Text {
        anchors.centerIn: parent
        text: displayController.slideText
        color: displayController.textColor
        font.family: displayController.fontFamily
        font.pixelSize: displayController.fontSize
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        width: parent.width * 0.9
    }
}
```

**Alternative: Use LinearGradient from QtQuick.Shapes:**
```qml
Component {
    id: gradientBackground
    Shape {
        anchors.fill: parent

        ShapePath {
            strokeWidth: 0
            fillGradient: LinearGradient {
                x1: 0; y1: 0
                x2: Math.cos(displayController.gradientAngle * Math.PI / 180) * parent.width
                y2: Math.sin(displayController.gradientAngle * Math.PI / 180) * parent.height

                GradientStop { position: 0; color: displayController.gradientColor1 }
                GradientStop { position: 1; color: displayController.gradientColor2 }
            }

            startX: 0; startY: 0
            PathLine { x: parent.width; y: 0 }
            PathLine { x: parent.width; y: parent.height }
            PathLine { x: 0; y: parent.height }
            PathLine { x: 0; y: 0 }
        }
    }
}
```

### IPC Protocol Update

**Updated slideData message:**
```json
{
  "type": "slideData",
  "slide": {
    "backgroundType": "gradient",
    "gradientColor1": "#1e3a8a",
    "gradientColor2": "#3b82f6",
    "gradientAngle": 90,
    "text": "Amazing Grace",
    "textColor": "#ffffff"
  }
}
```

### UI/UX Considerations

**Preset Gradients:**
- Consider offering preset gradient combinations:
  - "Ocean Blue" (blue to cyan)
  - "Sunset" (orange to purple)
  - "Forest" (dark green to light green)
  - "Royal" (deep blue to gold)
- Save user from color theory decisions

**Preview in Dialog:**
- Show live preview of gradient in the creation dialog
- Update preview as user changes colors/angle

### Testing Checklist

- [ ] Create gradient slide with 0° angle, verify horizontal gradient
- [ ] Create gradient slide with 90° angle, verify vertical gradient
- [ ] Test diagonal angles (45°, 135°)
- [ ] Test reverse gradient (180°)
- [ ] Verify gradient renders correctly in output display
- [ ] Test contrasting colors (dark to light, complementary colors)
- [ ] Save and reload presentation with gradient slides
- [ ] Verify gradient data survives IPC transmission

### Dependencies
- Qt6 Quick (LinearGradient from QtQuick.Shapes or built-in Gradient)
- Qt6 Widgets (QColorDialog)

### Estimated Complexity
**Low** - Gradients are well-supported in Qt Quick, minimal code changes needed

---

## Task 4: Slide Editor (Inline Editing)

### Overview
Enable users to create and edit slides directly in Clarity. Provide UI for modifying text, colors, fonts, and background properties.

### Goals
- Add new blank slides
- Edit existing slide text
- Change text color, font, size
- Change background (solid color, gradient, or image)
- Delete slides
- Reorder slides (up/down buttons)
- Inline editing workflow (double-click to edit)

### Files to Modify/Create

#### `src/Control/SlideEditorDialog.h` / `SlideEditorDialog.cpp` (NEW)
**Purpose:** Modal dialog for editing a single slide

**UI Layout:**
```
+--------------------------------------------------+
| Edit Slide                                    [X] |
+--------------------------------------------------+
| Text Content:                                     |
| +----------------------------------------------+ |
| | Welcome to Our Church                        | |
| | [Multi-line text editor]                     | |
| |                                              | |
| +----------------------------------------------+ |
|                                                   |
| Text Formatting:                                  |
|   Font: [Arial ▼]  Size: [48 ▼]  Color: [🎨]    |
|                                                   |
| Background Type:                                  |
|   ○ Solid Color    ○ Gradient    ○ Image        |
|                                                   |
| [Background-specific controls shown here]        |
|                                                   |
|                         [Cancel]  [OK]            |
+--------------------------------------------------+
```

**Implementation:**
```cpp
class SlideEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SlideEditorDialog(const Slide& slide, QWidget* parent = nullptr);
    Slide getEditedSlide() const;

private slots:
    void onBackgroundTypeChanged(int index);
    void onSolidColorButtonClicked();
    void onGradientColor1ButtonClicked();
    void onGradientColor2ButtonClicked();
    void onImageSelectButtonClicked();

private:
    void setupUI();
    void loadSlideData(const Slide& slide);
    void updateBackgroundControls();

    // UI elements
    QTextEdit* m_textEdit;
    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpinBox;
    QPushButton* m_textColorButton;

    QComboBox* m_backgroundTypeCombo;
    QStackedWidget* m_backgroundControls; // Different controls per type

    // Background: Solid Color
    QPushButton* m_solidColorButton;
    QColor m_solidColor;

    // Background: Gradient
    QPushButton* m_gradientColor1Button;
    QPushButton* m_gradientColor2Button;
    QComboBox* m_gradientAngleCombo;
    QColor m_gradientColor1;
    QColor m_gradientColor2;

    // Background: Image
    QPushButton* m_imageSelectButton;
    QLabel* m_imagePreviewLabel;
    QString m_imagePath;
    QByteArray m_imageData;

    QColor m_textColor;
};
```

**Key Methods:**
```cpp
SlideEditorDialog::SlideEditorDialog(const Slide& slide, QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    loadSlideData(slide);
}

void SlideEditorDialog::setupUI() {
    setWindowTitle("Edit Slide");
    resize(600, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Text content section
    mainLayout->addWidget(new QLabel("Text Content:"));
    m_textEdit = new QTextEdit();
    m_textEdit->setAcceptRichText(false); // Plain text only
    mainLayout->addWidget(m_textEdit);

    // Text formatting section
    QHBoxLayout* textFormatLayout = new QHBoxLayout();
    textFormatLayout->addWidget(new QLabel("Font:"));
    m_fontCombo = new QFontComboBox();
    textFormatLayout->addWidget(m_fontCombo);

    textFormatLayout->addWidget(new QLabel("Size:"));
    m_fontSizeSpinBox = new QSpinBox();
    m_fontSizeSpinBox->setRange(12, 144);
    textFormatLayout->addWidget(m_fontSizeSpinBox);

    m_textColorButton = new QPushButton("Text Color");
    connect(m_textColorButton, &QPushButton::clicked, [this]() {
        QColor color = QColorDialog::getColor(m_textColor, this);
        if (color.isValid()) {
            m_textColor = color;
            updateButtonColor(m_textColorButton, color);
        }
    });
    textFormatLayout->addWidget(m_textColorButton);

    mainLayout->addLayout(textFormatLayout);

    // Background type selector
    mainLayout->addWidget(new QLabel("Background Type:"));
    m_backgroundTypeCombo = new QComboBox();
    m_backgroundTypeCombo->addItem("Solid Color", Slide::SolidColor);
    m_backgroundTypeCombo->addItem("Gradient", Slide::Gradient);
    m_backgroundTypeCombo->addItem("Image", Slide::Image);
    connect(m_backgroundTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorDialog::onBackgroundTypeChanged);
    mainLayout->addWidget(m_backgroundTypeCombo);

    // Stacked widget for background-specific controls
    m_backgroundControls = new QStackedWidget();

    // Page 0: Solid Color
    QWidget* solidPage = new QWidget();
    QHBoxLayout* solidLayout = new QHBoxLayout(solidPage);
    m_solidColorButton = new QPushButton("Choose Color");
    connect(m_solidColorButton, &QPushButton::clicked,
            this, &SlideEditorDialog::onSolidColorButtonClicked);
    solidLayout->addWidget(m_solidColorButton);
    solidLayout->addStretch();
    m_backgroundControls->addWidget(solidPage);

    // Page 1: Gradient
    QWidget* gradientPage = new QWidget();
    QVBoxLayout* gradientLayout = new QVBoxLayout(gradientPage);
    QHBoxLayout* gradientColorLayout = new QHBoxLayout();
    m_gradientColor1Button = new QPushButton("Color 1");
    m_gradientColor2Button = new QPushButton("Color 2");
    connect(m_gradientColor1Button, &QPushButton::clicked,
            this, &SlideEditorDialog::onGradientColor1ButtonClicked);
    connect(m_gradientColor2Button, &QPushButton::clicked,
            this, &SlideEditorDialog::onGradientColor2ButtonClicked);
    gradientColorLayout->addWidget(m_gradientColor1Button);
    gradientColorLayout->addWidget(m_gradientColor2Button);
    gradientLayout->addLayout(gradientColorLayout);

    QHBoxLayout* angleLayout = new QHBoxLayout();
    angleLayout->addWidget(new QLabel("Angle:"));
    m_gradientAngleCombo = new QComboBox();
    m_gradientAngleCombo->addItem("0° (Horizontal)", 0);
    m_gradientAngleCombo->addItem("45° (Diagonal)", 45);
    m_gradientAngleCombo->addItem("90° (Vertical)", 90);
    m_gradientAngleCombo->addItem("135° (Diagonal)", 135);
    m_gradientAngleCombo->addItem("180° (Reverse)", 180);
    angleLayout->addWidget(m_gradientAngleCombo);
    angleLayout->addStretch();
    gradientLayout->addLayout(angleLayout);
    m_backgroundControls->addWidget(gradientPage);

    // Page 2: Image
    QWidget* imagePage = new QWidget();
    QVBoxLayout* imageLayout = new QVBoxLayout(imagePage);
    m_imageSelectButton = new QPushButton("Select Image...");
    connect(m_imageSelectButton, &QPushButton::clicked,
            this, &SlideEditorDialog::onImageSelectButtonClicked);
    imageLayout->addWidget(m_imageSelectButton);
    m_imagePreviewLabel = new QLabel("No image selected");
    m_imagePreviewLabel->setMinimumHeight(100);
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->setStyleSheet("border: 1px solid #ccc; background: #f0f0f0;");
    imageLayout->addWidget(m_imagePreviewLabel);
    imageLayout->addStretch();
    m_backgroundControls->addWidget(imagePage);

    mainLayout->addWidget(m_backgroundControls);

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

Slide SlideEditorDialog::getEditedSlide() const {
    Slide slide;
    slide.text = m_textEdit->toPlainText();
    slide.fontFamily = m_fontCombo->currentFont().family();
    slide.fontSize = m_fontSizeSpinBox->value();
    slide.textColor = m_textColor;

    Slide::BackgroundType bgType = static_cast<Slide::BackgroundType>(
        m_backgroundTypeCombo->currentData().toInt()
    );
    slide.backgroundType = bgType;

    switch (bgType) {
        case Slide::SolidColor:
            slide.backgroundColor = m_solidColor;
            break;
        case Slide::Gradient:
            slide.gradientColor1 = m_gradientColor1;
            slide.gradientColor2 = m_gradientColor2;
            slide.gradientAngle = m_gradientAngleCombo->currentData().toInt();
            break;
        case Slide::Image:
            slide.backgroundImagePath = m_imagePath;
            slide.backgroundImageData = m_imageData;
            break;
    }

    return slide;
}
```

#### `src/Control/ControlWindow.h` / `ControlWindow.cpp`
**Changes:**
- Add menu items under **Slide** menu:
  - New Slide (blank)
  - Edit Slide (opens SlideEditorDialog)
  - Delete Slide
  - Move Slide Up
  - Move Slide Down
- Add context menu to slide list (right-click)
- Double-click on slide to edit
- Update presentation model after edits

**UI Changes:**
```cpp
// In constructor
QMenu* slideMenu = menuBar()->addMenu("&Slide");
slideMenu->addAction("&New Slide", this, &ControlWindow::newSlide, QKeySequence("Ctrl+Shift+N"));
slideMenu->addAction("&Edit Slide", this, &ControlWindow::editSlide, QKeySequence("Ctrl+E"));
slideMenu->addAction("&Delete Slide", this, &ControlWindow::deleteSlide, QKeySequence::Delete);
slideMenu->addSeparator();
slideMenu->addAction("Move Slide &Up", this, &ControlWindow::moveSlideUp, QKeySequence("Ctrl+Up"));
slideMenu->addAction("Move Slide &Down", this, &ControlWindow::moveSlideDown, QKeySequence("Ctrl+Down"));

// Connect double-click
connect(m_slideListView, &QListView::doubleClicked, this, &ControlWindow::onSlideDoubleClicked);

// Context menu
m_slideListView->setContextMenuPolicy(Qt::CustomContextMenu);
connect(m_slideListView, &QWidget::customContextMenuRequested,
        this, &ControlWindow::onSlideListContextMenu);
```

**Slot Implementations:**
```cpp
void ControlWindow::newSlide() {
    Slide blankSlide;
    blankSlide.text = "New Slide";
    blankSlide.backgroundColor = QColor("#1e3a8a");
    blankSlide.textColor = QColor("#ffffff");
    blankSlide.fontFamily = "Arial";
    blankSlide.fontSize = 48;
    blankSlide.backgroundType = Slide::SolidColor;

    m_presentation.addSlide(blankSlide);
    m_model->refreshModel();
    markDirty();
}

void ControlWindow::editSlide() {
    QModelIndex current = m_slideListView->currentIndex();
    if (!current.isValid()) return;

    int index = current.row();
    Slide slide = m_presentation.getSlide(index);

    SlideEditorDialog dialog(slide, this);
    if (dialog.exec() == QDialog::Accepted) {
        Slide edited = dialog.getEditedSlide();
        m_presentation.updateSlide(index, edited);
        m_model->refreshModel();
        markDirty();

        // Update output display if this is the current slide
        if (index == m_presentation.getCurrentSlideIndex()) {
            sendSlideUpdate();
        }
    }
}

void ControlWindow::deleteSlide() {
    QModelIndex current = m_slideListView->currentIndex();
    if (!current.isValid()) return;

    int index = current.row();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Slide",
        "Are you sure you want to delete this slide?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_presentation.removeSlide(index);
        m_model->refreshModel();
        markDirty();
    }
}

void ControlWindow::moveSlideUp() {
    QModelIndex current = m_slideListView->currentIndex();
    if (!current.isValid() || current.row() == 0) return;

    int index = current.row();
    m_presentation.moveSlide(index, index - 1);
    m_model->refreshModel();
    m_slideListView->setCurrentIndex(m_model->index(index - 1, 0));
    markDirty();
}

void ControlWindow::moveSlideDown() {
    QModelIndex current = m_slideListView->currentIndex();
    if (!current.isValid() || current.row() == m_model->rowCount() - 1) return;

    int index = current.row();
    m_presentation.moveSlide(index, index + 1);
    m_model->refreshModel();
    m_slideListView->setCurrentIndex(m_model->index(index + 1, 0));
    markDirty();
}

void ControlWindow::onSlideDoubleClicked(const QModelIndex& index) {
    m_slideListView->setCurrentIndex(index);
    editSlide();
}
```

#### `src/Core/Presentation.h` / `Presentation.cpp`
**Changes:**
- Add method: `void addSlide(const Slide& slide)`
- Add method: `void updateSlide(int index, const Slide& slide)`
- Add method: `void removeSlide(int index)`
- Add method: `void moveSlide(int fromIndex, int toIndex)`
- Add method: `Slide getSlide(int index) const`

```cpp
void Presentation::addSlide(const Slide& slide) {
    m_slides.append(slide);
}

void Presentation::updateSlide(int index, const Slide& slide) {
    if (index >= 0 && index < m_slides.size()) {
        m_slides[index] = slide;
    }
}

void Presentation::removeSlide(int index) {
    if (index >= 0 && index < m_slides.size()) {
        m_slides.removeAt(index);

        // Adjust current index if needed
        if (m_currentSlideIndex >= m_slides.size()) {
            m_currentSlideIndex = qMax(0, m_slides.size() - 1);
        }
    }
}

void Presentation::moveSlide(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= m_slides.size() ||
        toIndex < 0 || toIndex >= m_slides.size()) {
        return;
    }

    m_slides.move(fromIndex, toIndex);

    // Update current index if needed
    if (m_currentSlideIndex == fromIndex) {
        m_currentSlideIndex = toIndex;
    } else if (fromIndex < m_currentSlideIndex && toIndex >= m_currentSlideIndex) {
        m_currentSlideIndex--;
    } else if (fromIndex > m_currentSlideIndex && toIndex <= m_currentSlideIndex) {
        m_currentSlideIndex++;
    }
}

Slide Presentation::getSlide(int index) const {
    if (index >= 0 && index < m_slides.size()) {
        return m_slides[index];
    }
    return Slide(); // Return default slide if invalid
}
```

### UI/UX Considerations

**Slide List Display:**
- Show slide text preview (first 50 characters)
- Show background color or image thumbnail
- Highlight current slide being displayed
- Show slide number (1, 2, 3...)

**Keyboard Shortcuts:**
- Ctrl+N: New blank slide
- Ctrl+E: Edit selected slide
- Delete: Delete selected slide
- Ctrl+Up/Down: Reorder slides
- Enter: Navigate to slide in output

**Validation:**
- Warn if deleting last slide
- Prevent moving slide out of bounds
- Validate required fields (can have empty text, but need background)

### Testing Checklist

- [ ] Create new blank slide, verify it appears in list
- [ ] Double-click slide, edit text, verify changes save
- [ ] Edit slide background (solid, gradient, image), verify changes
- [ ] Delete slide, verify list updates and current slide adjusts
- [ ] Move slide up, verify order changes
- [ ] Move slide down, verify order changes
- [ ] Edit currently displayed slide, verify output updates immediately
- [ ] Create presentation with 20+ slides, verify performance is acceptable
- [ ] Test undo/redo if implemented (optional for Phase 2)

### Dependencies
- Qt6 Widgets (QTextEdit, QFontComboBox, QSpinBox, QColorDialog, QStackedWidget)
- All previous tasks (backgrounds must be implemented)

### Estimated Complexity
**High** - Most complex UI work in Phase 2, requires careful state management and validation

---

## Task 5: Confidence Monitor Implementation

### Overview
Implement the confidence monitor (presenter view) showing current slide, next slide, timer, and notes.

### Goals
- Show current slide in main area
- Show next slide preview
- Display elapsed/countdown timer
- Show slide notes (if added to Slide model)
- Clock display
- Connect to IPC for real-time updates

### Files to Modify

#### `src/Core/Slide.h` / `Slide.cpp`
**Changes (Optional):**
- Add member: `QString notes` (presenter notes, not shown on output)
- Update JSON serialization to include notes

#### `src/Confidence/ConfidenceMain.h` / `ConfidenceMain.cpp`
**Changes:**
- Replace stub implementation with real IPC client
- Create ConfidenceController (similar to OutputDisplay)
- Expose properties to QML: currentSlide, nextSlide, elapsedTime, notes

**Implementation:**
```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ConfidenceController.h"

int runConfidenceMode(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    ConfidenceController* controller = new ConfidenceController(&app);
    engine.rootContext()->setContextProperty("confidenceController", controller);

    const QUrl url(QStringLiteral("qrc:/qml/ConfidenceMonitor.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load ConfidenceMonitor.qml";
        return -1;
    }

    return app.exec();
}
```

#### `src/Confidence/ConfidenceController.h` / `ConfidenceController.cpp` (NEW)
**Purpose:** Bridge between IPC and confidence monitor QML

```cpp
#ifndef CONFIDENCECONTROLLER_H
#define CONFIDENCECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "../Core/IpcClient.h"
#include "../Core/Slide.h"

namespace Clarity {

class ConfidenceController : public QObject {
    Q_OBJECT

    // Current slide properties
    Q_PROPERTY(QString currentSlideText READ currentSlideText NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentBackgroundColor READ currentBackgroundColor NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentTextColor READ currentTextColor NOTIFY currentSlideChanged)
    Q_PROPERTY(int currentFontSize READ currentFontSize NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentFontFamily READ currentFontFamily NOTIFY currentSlideChanged)

    // Next slide properties
    Q_PROPERTY(QString nextSlideText READ nextSlideText NOTIFY nextSlideChanged)
    Q_PROPERTY(QString nextBackgroundColor READ nextBackgroundColor NOTIFY nextSlideChanged)

    // Timer and clock
    Q_PROPERTY(QString elapsedTime READ elapsedTime NOTIFY elapsedTimeChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)

    // Notes
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged)

public:
    explicit ConfidenceController(QObject* parent = nullptr);

    // Property getters
    QString currentSlideText() const { return m_currentSlide.text; }
    QString currentBackgroundColor() const { return m_currentSlide.backgroundColor.name(); }
    QString currentTextColor() const { return m_currentSlide.textColor.name(); }
    int currentFontSize() const { return m_currentSlide.fontSize; }
    QString currentFontFamily() const { return m_currentSlide.fontFamily; }

    QString nextSlideText() const { return m_nextSlide.text; }
    QString nextBackgroundColor() const { return m_nextSlide.backgroundColor.name(); }

    QString elapsedTime() const;
    QString currentTime() const;
    QString notes() const { return m_currentSlide.notes; }

public slots:
    void resetTimer();
    void startTimer();
    void stopTimer();

signals:
    void currentSlideChanged();
    void nextSlideChanged();
    void elapsedTimeChanged();
    void currentTimeChanged();
    void notesChanged();

private slots:
    void onConnected();
    void onMessageReceived(const QJsonObject& message);
    void updateClock();

private:
    IpcClient* m_ipcClient;
    Slide m_currentSlide;
    Slide m_nextSlide;

    QTimer* m_clockTimer;
    QDateTime m_presentationStartTime;
    QElapsedTimer m_elapsedTimer;
    bool m_timerRunning = false;
};

} // namespace Clarity

#endif // CONFIDENCECONTROLLER_H
```

**Implementation:**
```cpp
#include "ConfidenceController.h"
#include <QJsonObject>

namespace Clarity {

ConfidenceController::ConfidenceController(QObject* parent)
    : QObject(parent)
    , m_ipcClient(new IpcClient("confidence", this))
    , m_clockTimer(new QTimer(this))
{
    connect(m_ipcClient, &IpcClient::connected, this, &ConfidenceController::onConnected);
    connect(m_ipcClient, &IpcClient::messageReceived, this, &ConfidenceController::onMessageReceived);

    m_ipcClient->connectToServer("clarity-ipc");

    // Update clock every second
    connect(m_clockTimer, &QTimer::timeout, this, &ConfidenceController::updateClock);
    m_clockTimer->start(1000);
}

void ConfidenceController::onConnected() {
    qDebug() << "ConfidenceController: Connected to IPC server";
}

void ConfidenceController::onMessageReceived(const QJsonObject& message) {
    QString type = message["type"].toString();

    if (type == "slideData") {
        QJsonObject slideObj = message["slide"].toObject();
        m_currentSlide = Slide::fromJson(slideObj);
        emit currentSlideChanged();
        emit notesChanged();

        // Request next slide data from server (requires protocol extension)
        // For now, we'll handle this when server sends both current and next
    } else if (type == "nextSlideData") {
        QJsonObject slideObj = message["slide"].toObject();
        m_nextSlide = Slide::fromJson(slideObj);
        emit nextSlideChanged();
    }
}

QString ConfidenceController::elapsedTime() const {
    if (!m_timerRunning) {
        return "00:00:00";
    }

    qint64 elapsed = m_elapsedTimer.elapsed() / 1000; // Convert to seconds
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString ConfidenceController::currentTime() const {
    return QDateTime::currentDateTime().toString("hh:mm:ss AP");
}

void ConfidenceController::resetTimer() {
    m_elapsedTimer.invalidate();
    m_timerRunning = false;
    emit elapsedTimeChanged();
}

void ConfidenceController::startTimer() {
    if (!m_timerRunning) {
        m_elapsedTimer.start();
        m_timerRunning = true;
        m_presentationStartTime = QDateTime::currentDateTime();
    }
}

void ConfidenceController::stopTimer() {
    m_timerRunning = false;
}

void ConfidenceController::updateClock() {
    emit currentTimeChanged();
    if (m_timerRunning) {
        emit elapsedTimeChanged();
    }
}

} // namespace Clarity
```

#### `src/Confidence/qml/ConfidenceMonitor.qml`
**Changes:**
- Replace stub with full presenter view UI
- Three-panel layout: current slide (large), next slide (small), controls (bottom)

**QML Implementation:**
```qml
import QtQuick
import QtQuick.Layouts

Window {
    id: root
    visible: true
    width: 1024
    height: 768
    title: "Clarity - Confidence Monitor"
    color: "#2c2c2c"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Top row: Current and Next slides
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Current Slide (main area)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1a1a1a"
                border.color: "#4a4a4a"
                border.width: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5

                    Text {
                        text: "Current Slide"
                        color: "#aaaaaa"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    // Slide preview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: confidenceController.currentBackgroundColor

                        Text {
                            anchors.centerIn: parent
                            text: confidenceController.currentSlideText
                            color: confidenceController.currentTextColor
                            font.family: confidenceController.currentFontFamily
                            font.pixelSize: Math.min(parent.width, parent.height) / 10
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width * 0.9
                        }
                    }
                }
            }

            // Right sidebar: Next slide + info
            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                spacing: 10

                // Next Slide Preview
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    color: "#1a1a1a"
                    border.color: "#4a4a4a"
                    border.width: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5

                        Text {
                            text: "Next Slide"
                            color: "#aaaaaa"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: confidenceController.nextBackgroundColor

                            Text {
                                anchors.centerIn: parent
                                text: confidenceController.nextSlideText
                                color: "#ffffff"
                                font.pixelSize: 16
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width * 0.9
                            }
                        }
                    }
                }

                // Timer
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    color: "#1a1a1a"
                    border.color: "#4a4a4a"
                    border.width: 2

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 5

                        Text {
                            text: "Elapsed Time"
                            color: "#aaaaaa"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: confidenceController.elapsedTime
                            color: "#4caf50"
                            font.pixelSize: 36
                            font.bold: true
                            font.family: "monospace"
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Clock
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    color: "#1a1a1a"
                    border.color: "#4a4a4a"
                    border.width: 2

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 5

                        Text {
                            text: "Current Time"
                            color: "#aaaaaa"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: confidenceController.currentTime
                            color: "#ffffff"
                            font.pixelSize: 28
                            font.bold: true
                            font.family: "monospace"
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Notes section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1a1a1a"
                    border.color: "#4a4a4a"
                    border.width: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        Text {
                            text: "Notes"
                            color: "#aaaaaa"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: confidenceController.notes || "(No notes for this slide)"
                            color: "#cccccc"
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignTop
                        }
                    }
                }
            }
        }

        // Bottom row: Timer controls
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1a1a1a"
                border.color: "#4a4a4a"
                border.width: 2

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 10

                    Rectangle {
                        width: 80
                        height: 30
                        color: "#4caf50"
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: "Start"
                            color: "white"
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: confidenceController.startTimer()
                        }
                    }

                    Rectangle {
                        width: 80
                        height: 30
                        color: "#f44336"
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: "Reset"
                            color: "white"
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: confidenceController.resetTimer()
                        }
                    }
                }
            }
        }
    }
}
```

#### `src/Core/IpcServer.h` / `IpcServer.cpp`
**Changes:**
- Modify `sendSlideUpdate()` to also send next slide data to confidence monitor
- Add method: `sendNextSlideData(int nextIndex)`

```cpp
void IpcServer::sendSlideUpdate(const Slide& currentSlide, const Slide& nextSlide) {
    QJsonObject message;
    message["type"] = "slideData";
    message["slide"] = currentSlide.toJson();

    broadcastMessage(message);

    // Send next slide data specifically to confidence monitor
    QJsonObject nextMessage;
    nextMessage["type"] = "nextSlideData";
    nextMessage["slide"] = nextSlide.toJson();

    sendToClient("confidence", nextMessage);
}
```

#### `src/Control/ControlWindow.cpp`
**Changes:**
- Update slide navigation to send both current and next slide
- Add "Launch Confidence Monitor" button functionality

```cpp
void ControlWindow::sendSlideUpdate() {
    Slide current = m_presentation.getCurrentSlide();
    Slide next;

    int nextIndex = m_presentation.getCurrentSlideIndex() + 1;
    if (nextIndex < m_presentation.getSlideCount()) {
        next = m_presentation.getSlide(nextIndex);
    } else {
        // Last slide - send blank next slide
        next.text = "(End of presentation)";
        next.backgroundColor = QColor("#2c2c2c");
    }

    m_ipcServer->sendSlideUpdate(current, next);
}
```

### UI/UX Considerations

**Layout Options:**
- Classic presenter view (current large, next small on side)
- Alternative: Current/next side-by-side equal size
- User preference for layout (Phase 3+)

**Timer Features:**
- Elapsed time (count up)
- Optional: Countdown timer (set duration, warn when time is low)
- Optional: Time per slide tracking

**Notes Display:**
- Scrollable if notes are long
- Consider font size preference
- Optionally hide if no notes

**Color Scheme:**
- Dark theme (less distracting for presenter)
- High contrast for readability
- Avoid bright colors that strain eyes

### Testing Checklist

- [ ] Launch confidence monitor, verify it connects to IPC
- [ ] Navigate slides in control, verify confidence updates in real-time
- [ ] Verify current slide preview displays correctly
- [ ] Verify next slide preview displays correctly
- [ ] Start timer, verify it counts up every second
- [ ] Reset timer, verify it returns to 00:00:00
- [ ] Verify clock displays current time and updates
- [ ] Add notes to slides, verify they appear in confidence monitor
- [ ] Test on second monitor setup
- [ ] Verify confidence monitor survives control app restart

### Dependencies
- Qt6 Quick (QML, Layouts)
- Qt6 Core (QTimer, QDateTime, QElapsedTimer)
- IPC infrastructure from Phase 1

### Estimated Complexity
**Medium** - UI is straightforward, main challenge is extending IPC protocol for next slide data

---

## Implementation Order Summary

1. **Save/Load** (Low-Medium complexity, ~2-4 hours)
   - Enables persistence, foundational for all other work

2. **Image Backgrounds** (Medium complexity, ~3-5 hours)
   - Core rendering enhancement, unlocks import workflow

3. **Gradient Backgrounds** (Low complexity, ~1-2 hours)
   - Natural extension of background system

4. **Slide Editor** (High complexity, ~6-10 hours)
   - Most complex UI, benefits from stable data model

5. **Confidence Monitor** (Medium complexity, ~4-6 hours)
   - Independent feature, valuable for actual use

**Total Estimated Time:** 16-27 hours of development

---

## Build System Updates

### CMakeLists.txt Changes

Add new files to build:

```cmake
# In Control section
src/Control/SlideEditorDialog.h
src/Control/SlideEditorDialog.cpp

# In Confidence section
src/Confidence/ConfidenceController.h
src/Confidence/ConfidenceController.cpp
```

### Qt Dependencies

Ensure these Qt modules are linked:
- `Qt6::Core`
- `Qt6::Widgets`
- `Qt6::Quick`
- `Qt6::Network`

Phase 2 doesn't require new Qt modules beyond Phase 1.

---

## Risk Mitigation

### Potential Issues

1. **Image Size in IPC**: Large images may slow down IPC or cause memory issues
   - Mitigation: Resize on import, compress with PNG

2. **Slide Editor Complexity**: Complex UI may have bugs
   - Mitigation: Thorough manual testing, start simple and iterate

3. **Confidence Monitor Sync**: Timing issues with IPC updates
   - Mitigation: Use Qt's signal/slot mechanism, test extensively

4. **File Format Versioning**: Future schema changes may break old files
   - Mitigation: Include "version" field in JSON, implement migration logic in Phase 3+

### Testing Strategy

- Manual testing after each task
- Create sample presentations to stress-test features
- Test on different screen configurations
- Verify performance with large presentations (50+ slides)

---

## Phase 2 Complete Checklist

- [ ] Save presentations to .cly files
- [ ] Load presentations from .cly files
- [ ] Import images as slide backgrounds
- [ ] Create gradient backgrounds
- [ ] Add new blank slides
- [ ] Edit existing slides (text, colors, backgrounds)
- [ ] Delete slides
- [ ] Reorder slides
- [ ] Confidence monitor displays current slide
- [ ] Confidence monitor displays next slide
- [ ] Confidence monitor timer works
- [ ] Confidence monitor clock works
- [ ] Update DEVLOG.md with all changes
- [ ] Test complete workflow end-to-end

---

**End of Phase 2 Implementation Plan**

