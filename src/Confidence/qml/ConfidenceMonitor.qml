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
            anchors.bottomMargin: 80  // Leave room for timer bar
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

                        // Current slide text using settings
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.currentSlideText
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

                        // Next slide text or placeholder using settings
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.hasNextSlide ? confidenceDisplay.nextSlideText : "End of presentation"
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

        // Timer and clock bar at bottom - visible when content is showing
        Rectangle {
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

    Component.onCompleted: {
        console.log("ConfidenceMonitor QML loaded")
    }
}
