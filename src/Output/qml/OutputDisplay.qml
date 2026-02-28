// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

import QtQuick
import QtQuick.Window

/**
 * Output display window - thin Window wrapper around OutputDisplayContent
 *
 * All visual rendering is in OutputDisplayContent.qml (Item-rooted),
 * shared between this fullscreen window mode and NDI offscreen mode.
 * This wrapper only handles window-specific behavior (fullscreen, visibility).
 */
Window {
    id: root

    title: "Clarity - Output Display"
    visible: false
    color: "black"

    // The shared visual content (Item-rooted, same scene used by NDI mode)
    OutputDisplayContent {
        id: content
        anchors.fill: parent
    }

    // Window-specific signal handlers
    Connections {
        target: displayController

        // Toggle fullscreen mode when F key is pressed
        function onToggleFullscreen() {
            if (root.visibility === Window.FullScreen) {
                root.visibility = Window.Windowed
                console.log("OutputDisplay: Switched to windowed mode")
            } else {
                root.visibility = Window.FullScreen
                console.log("OutputDisplay: Switched to fullscreen mode")
            }
        }

        // Toggle visibility when O key is pressed
        function onToggleVisibility() {
            if (root.visible) {
                root.visible = false
                console.log("OutputDisplay: Hidden")
            } else {
                root.visible = true
                console.log("OutputDisplay: Shown")
            }
        }
    }

    Component.onCompleted: {
        console.log("OutputDisplay Window wrapper loaded")
    }
}
