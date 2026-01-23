import QtQuick
import QtQuick.Window

/**
 * Output display window - fullscreen presentation view
 *
 * QML Concepts used here:
 * - Window: Top-level QML type for creating windows
 * - Rectangle: Basic visual element with color and border properties
 * - Text: Displays text with styling options
 * - Property bindings: text, color properties automatically update when C++ properties change
 * - Anchors: Positioning system (centerIn aligns item to parent center)
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
    color: displayController.backgroundColor

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
    }

    // Debug output when window loads
    Component.onCompleted: {
        console.log("OutputDisplay QML loaded")
    }
}
