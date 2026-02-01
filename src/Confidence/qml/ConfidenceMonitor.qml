import QtQuick
import QtQuick.Window

/**
 * Confidence monitor window - Stage display for presenter
 *
 * Shows the presenter:
 * - Current slide (what the audience sees) - large display on left
 * - Next slide preview (what's coming up) - smaller preview on right
 * - Slide position indicator (current / total)
 *
 * Future enhancements: timer, notes, clock
 */
Window {
    id: root

    // Don't set visibility here - let C++ code control when to show
    // This allows screen selection to happen before the window appears
    visible: false
    title: "Clarity - Confidence Monitor"

    // Background
    color: "#1a1a1a"

    // Main layout container
    Rectangle {
        anchors.fill: parent
        color: "#1a1a1a"

        // Show message when cleared or no slides
        Rectangle {
            anchors.fill: parent
            color: "#1a1a1a"
            visible: confidenceDisplay.isCleared

            Text {
                anchors.centerIn: parent
                text: "No slide displayed"
                color: "#666666"
                font.pixelSize: 32
            }

            // Timer and clock always visible, even when cleared
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 60
                color: "#2a2a2a"

                // Elapsed time display (left side)
                Text {
                    id: elapsedTimeText
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: confidenceDisplay.elapsedTime
                    color: confidenceDisplay.timerRunning ? "#00ff00" : "#ffffff"
                    font.pixelSize: 36
                    font.family: "Consolas"
                }

                // Current time clock (right side)
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: confidenceDisplay.currentTime
                    color: "#ffffff"
                    font.pixelSize: 36
                    font.family: "Consolas"
                }
            }
        }

        // Main content - visible when not cleared
        Row {
            anchors.fill: parent
            anchors.margins: 20
            anchors.bottomMargin: confidenceDisplay.currentNotes ? 140 : 80  // Extra room for notes if present
            spacing: 20
            visible: !confidenceDisplay.isCleared

            // Left side: Current slide (65% of width)
            Rectangle {
                width: parent.width * 0.65
                height: parent.height
                color: "#2a2a2a"
                border.color: "#00ff00"
                border.width: 3
                radius: 5

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // Label
                    Rectangle {
                        width: parent.width
                        height: 40
                        color: "#00ff00"

                        Text {
                            anchors.centerIn: parent
                            text: "CURRENT SLIDE (" + (confidenceDisplay.currentSlideIndex + 1) + " / " + confidenceDisplay.totalSlides + ")"
                            color: "#000000"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    // Current slide display - uses settings colors, not slide backgrounds
                    Rectangle {
                        width: parent.width
                        height: parent.height - 40
                        color: confidenceDisplay.settingsBackgroundColor
                        clip: true

                        // Current slide text using settings - supports rich text for red letters
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.currentUseRichText
                                ? "<style>.jesus{color:" + confidenceDisplay.redLetterColor + "}</style>" + confidenceDisplay.currentSlideRichText
                                : confidenceDisplay.currentSlideText
                            textFormat: confidenceDisplay.currentUseRichText ? Text.RichText : Text.PlainText
                            color: confidenceDisplay.settingsTextColor
                            font.family: confidenceDisplay.settingsFontFamily
                            font.pixelSize: confidenceDisplay.settingsFontSize
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            width: parent.width * 0.9
                        }
                    }
                }
            }

            // Right side: Next slide preview (35% of width)
            Rectangle {
                width: parent.width * 0.35 - 20
                height: parent.height
                color: "#2a2a2a"
                border.color: confidenceDisplay.hasNextSlide ? "#ffaa00" : "#666666"
                border.width: 2
                radius: 5

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // Label
                    Rectangle {
                        width: parent.width
                        height: 35
                        color: confidenceDisplay.hasNextSlide ? "#ffaa00" : "#666666"

                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.hasNextSlide ? "NEXT SLIDE" : "NO NEXT SLIDE"
                            color: "#000000"
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    // Next slide preview - uses settings colors, not slide backgrounds
                    Rectangle {
                        width: parent.width
                        height: parent.height - 35
                        color: confidenceDisplay.settingsBackgroundColor
                        clip: true

                        // Next slide text or placeholder using settings - supports rich text for red letters
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.hasNextSlide
                                ? (confidenceDisplay.nextUseRichText
                                    ? "<style>.jesus{color:" + confidenceDisplay.redLetterColor + "}</style>" + confidenceDisplay.nextSlideRichText
                                    : confidenceDisplay.nextSlideText)
                                : "End of presentation"
                            textFormat: (confidenceDisplay.hasNextSlide && confidenceDisplay.nextUseRichText) ? Text.RichText : Text.PlainText
                            color: confidenceDisplay.hasNextSlide ? confidenceDisplay.settingsTextColor : "#666666"
                            font.family: confidenceDisplay.settingsFontFamily
                            font.pixelSize: confidenceDisplay.hasNextSlide ? confidenceDisplay.settingsFontSize * 0.6 : 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            width: parent.width * 0.9
                            font.italic: !confidenceDisplay.hasNextSlide
                        }
                    }
                }
            }
        }

        // Presenter notes panel - visible when notes exist
        Rectangle {
            anchors.bottom: timerBar.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 20
            anchors.bottomMargin: 10
            height: 50
            color: "#2a2a2a"
            border.color: "#4a90d9"
            border.width: 1
            radius: 5
            visible: !confidenceDisplay.isCleared && confidenceDisplay.currentNotes

            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                // Notes icon/label
                Text {
                    text: "NOTES:"
                    color: "#4a90d9"
                    font.pixelSize: 14
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Notes content
                Text {
                    width: parent.width - 80
                    text: confidenceDisplay.currentNotes
                    color: "#ffffff"
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Timer and clock bar at bottom - visible when content is showing
        Rectangle {
            id: timerBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 60
            color: "#2a2a2a"
            visible: !confidenceDisplay.isCleared

            // Elapsed time display (left side)
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: confidenceDisplay.elapsedTime
                color: confidenceDisplay.timerRunning ? "#00ff00" : "#ffffff"
                font.pixelSize: 36
                font.family: "Consolas"
            }

            // Current time clock (right side)
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: confidenceDisplay.currentTime
                color: "#ffffff"
                font.pixelSize: 36
                font.family: "Consolas"
            }
        }
    }

    // Listen for signals from C++
    Connections {
        target: confidenceDisplay

        // Toggle visibility when C key is pressed
        function onToggleVisibility() {
            if (root.visible) {
                root.visible = false
                console.log("ConfidenceMonitor: Hidden")
            } else {
                root.visible = true
                console.log("ConfidenceMonitor: Shown")
            }
        }
    }

    Component.onCompleted: {
        console.log("ConfidenceMonitor QML loaded")
    }
}
