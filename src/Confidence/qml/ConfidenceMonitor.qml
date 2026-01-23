import QtQuick
import QtQuick.Window

/**
 * Confidence monitor window - Stage display (Phase 1: Stub)
 *
 * This is a placeholder for Phase 1. Future phases will show:
 * - Current slide
 * - Next slide preview
 * - Timer
 * - Notes
 */
Window {
    id: root

    width: 800
    height: 600
    visible: true
    title: "Clarity - Confidence Monitor (Stub)"

    // Background
    Rectangle {
        anchors.fill: parent
        color: "#1a1a1a"

        // Placeholder text
        Text {
            anchors.centerIn: parent
            text: "Confidence Monitor\n\n(Phase 1 Stub - Not Implemented Yet)"
            color: "#888888"
            font.pixelSize: 32
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Component.onCompleted: {
        console.log("ConfidenceMonitor QML loaded (stub mode)")
    }
}
