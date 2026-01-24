import QtQuick
import QtQuick.Window

/**
 * Output display window - fullscreen presentation view
 *
 * QML Concepts used here:
 * - Window: Top-level QML type for creating windows
 * - Rectangle: Basic visual element with color and border properties
 * - Image: Displays images from various sources (files, base64 data URLs)
 * - Text: Displays text with styling options
 * - Property bindings: text, color properties automatically update when C++ properties change
 * - Anchors: Positioning system (centerIn aligns item to parent center, fill makes element fill parent)
 * - Conditional visibility: visible property can be bound to expressions
 */
Window {
    id: root

    // Window title (not visible in fullscreen but useful for debugging)
    title: "Clarity - Output Display"

    // Don't set visibility here - let C++ code control when to show
    // This allows screen selection to happen before the window appears
    visible: false

    // Background color binds to C++ displayController.backgroundColor property
    // This creates a reactive binding - when backgroundColor changes in C++, QML updates automatically
    // Only used for solid color backgrounds
    color: displayController.backgroundType === "solidColor" ? displayController.backgroundColor : "transparent"

    // Gradient background (only visible when backgroundType is "gradient")
    Rectangle {
        id: gradientBackground

        // Fill the entire window
        anchors.fill: parent

        // Only show when background type is gradient
        visible: displayController.backgroundType === "gradient"

        // Apply gradient
        // QML Gradient works from top to bottom by default
        // We rotate the rectangle to achieve the desired angle
        rotation: displayController.gradientAngle

        gradient: Gradient {
            GradientStop { position: 0.0; color: displayController.gradientStartColor }
            GradientStop { position: 1.0; color: displayController.gradientEndColor }
        }
    }

    // Background image (only visible when backgroundType is "image")
    // QML Image can load from data URLs: "data:image/png;base64,iVBORw0KG..."
    Image {
        id: backgroundImage

        // Fill the entire window
        anchors.fill: parent

        // Only show when background type is image
        visible: displayController.backgroundType === "image"

        // Convert QByteArray to base64 data URL for QML Image
        // The Image source expects a URL string, so we create a data URL from the base64 data
        source: visible ? "data:image/png;base64," + displayController.backgroundImageData.toBase64() : ""

        // Fill mode - preserveAspectCrop fills the window while maintaining aspect ratio
        fillMode: Image.PreserveAspectCrop

        // Smooth scaling for better image quality
        smooth: true
    }

    // Text element for slide content
    Text {
        id: slideText

        // Center the text in the window
        anchors.centerIn: parent

        // Bind text content to C++ property
        text: displayController.slideText

        // Styling properties bound to C++ controller
        color: displayController.textColor
        font.family: displayController.fontFamily
        font.pixelSize: displayController.fontSize

        // Center-align multi-line text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        // Allow text to wrap if needed
        wrapMode: Text.WordWrap

        // Limit width to 80% of screen for readability
        width: parent.width * 0.8

        // Add text shadow for better readability over images
        style: Text.Outline
        styleColor: "black"
    }

    // Debug output when window loads
    Component.onCompleted: {
        console.log("OutputDisplay QML loaded")
    }
}
