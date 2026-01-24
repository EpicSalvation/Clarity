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
        }

        // Main content - visible when not cleared
        Row {
            anchors.fill: parent
            anchors.margins: 20
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

                    // Current slide display
                    Rectangle {
                        width: parent.width
                        height: parent.height - 40
                        color: confidenceDisplay.currentBackgroundType === "solidColor" ? confidenceDisplay.currentBackgroundColor : "transparent"
                        clip: true

                        // Gradient background for current slide
                        Rectangle {
                            anchors.centerIn: parent
                            visible: confidenceDisplay.currentBackgroundType === "gradient"

                            property real angleRad: confidenceDisplay.currentGradientAngle * Math.PI / 180.0
                            property real absSin: Math.abs(Math.sin(angleRad))
                            property real absCos: Math.abs(Math.cos(angleRad))

                            width: parent.width * absCos + parent.height * absSin
                            height: parent.width * absSin + parent.height * absCos
                            rotation: confidenceDisplay.currentGradientAngle

                            gradient: Gradient {
                                GradientStop { position: 0.0; color: confidenceDisplay.currentGradientStartColor }
                                GradientStop { position: 1.0; color: confidenceDisplay.currentGradientEndColor }
                            }
                        }

                        // Image background for current slide
                        Image {
                            anchors.fill: parent
                            visible: confidenceDisplay.currentBackgroundType === "image"
                            source: visible ? "data:image/png;base64," + confidenceDisplay.currentBackgroundImageDataBase64 : ""
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                        }

                        // Current slide text
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.currentSlideText
                            color: confidenceDisplay.currentTextColor
                            font.family: confidenceDisplay.currentFontFamily
                            font.pixelSize: confidenceDisplay.currentFontSize * 0.6 // Scale down for confidence monitor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            width: parent.width * 0.9
                            style: Text.Outline
                            styleColor: "black"
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

                    // Next slide preview
                    Rectangle {
                        width: parent.width
                        height: parent.height - 35
                        color: confidenceDisplay.hasNextSlide && confidenceDisplay.nextBackgroundType === "solidColor" ? confidenceDisplay.nextBackgroundColor : "#3a3a3a"
                        clip: true

                        // Gradient background for next slide
                        Rectangle {
                            anchors.centerIn: parent
                            visible: confidenceDisplay.hasNextSlide && confidenceDisplay.nextBackgroundType === "gradient"

                            property real angleRad: confidenceDisplay.nextGradientAngle * Math.PI / 180.0
                            property real absSin: Math.abs(Math.sin(angleRad))
                            property real absCos: Math.abs(Math.cos(angleRad))

                            width: parent.width * absCos + parent.height * absSin
                            height: parent.width * absSin + parent.height * absCos
                            rotation: confidenceDisplay.nextGradientAngle

                            gradient: Gradient {
                                GradientStop { position: 0.0; color: confidenceDisplay.nextGradientStartColor }
                                GradientStop { position: 1.0; color: confidenceDisplay.nextGradientEndColor }
                            }
                        }

                        // Image background for next slide
                        Image {
                            anchors.fill: parent
                            visible: confidenceDisplay.hasNextSlide && confidenceDisplay.nextBackgroundType === "image"
                            source: visible ? "data:image/png;base64," + confidenceDisplay.nextBackgroundImageDataBase64 : ""
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                        }

                        // Next slide text or placeholder
                        Text {
                            anchors.centerIn: parent
                            text: confidenceDisplay.hasNextSlide ? confidenceDisplay.nextSlideText : "End of presentation"
                            color: confidenceDisplay.hasNextSlide ? confidenceDisplay.nextTextColor : "#666666"
                            font.family: confidenceDisplay.hasNextSlide ? confidenceDisplay.nextFontFamily : "Arial"
                            font.pixelSize: confidenceDisplay.hasNextSlide ? confidenceDisplay.nextFontSize * 0.3 : 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            width: parent.width * 0.9
                            style: confidenceDisplay.hasNextSlide ? Text.Outline : Text.Normal
                            styleColor: "black"
                            font.italic: !confidenceDisplay.hasNextSlide
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("ConfidenceMonitor QML loaded")
    }
}
