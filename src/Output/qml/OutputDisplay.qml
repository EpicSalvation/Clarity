import QtQuick
import QtQuick.Window
import QtMultimedia

/**
 * Output display window - fullscreen presentation view with transitions
 *
 * Transition Implementation:
 * - Uses two slide containers that swap roles (current/next)
 * - Each container has its own cached copy of slide data
 * - When transitioning, outgoing keeps old data while incoming shows new data
 * - Supports: cut, fade, slideLeft, slideRight, slideUp, slideDown
 */
Window {
    id: root

    title: "Clarity - Output Display"
    visible: false
    color: "black"

    // Track which container is currently showing (A or B)
    property bool showingA: true

    // Track if we're currently in a transition
    property bool transitioning: false

    // Cached slide data for container A
    property string slideTextA: ""
    property color backgroundColorA: "#000000"
    property color textColorA: "#ffffff"
    property string fontFamilyA: "Arial"
    property int fontSizeA: 48
    property string backgroundTypeA: "solidColor"
    property string backgroundImageDataBase64A: ""
    property color gradientStartColorA: "#1e3a8a"
    property color gradientEndColorA: "#60a5fa"
    property int gradientAngleA: 135
    property string backgroundVideoSourceA: ""
    property bool videoLoopA: true
    // Text legibility properties for container A
    property bool dropShadowEnabledA: true
    property color dropShadowColorA: "#000000"
    property int dropShadowOffsetXA: 2
    property int dropShadowOffsetYA: 2
    property int dropShadowBlurA: 4
    property bool overlayEnabledA: false
    property color overlayColorA: "#80000000"
    property int overlayBlurA: 0
    property bool textContainerEnabledA: false
    property color textContainerColorA: "#80000000"
    property int textContainerPaddingA: 20
    property int textContainerRadiusA: 8
    property int textContainerBlurA: 0
    property bool textBandEnabledA: false
    property color textBandColorA: "#80000000"
    property int textBandBlurA: 0

    // Cached slide data for container B
    property string slideTextB: ""
    property color backgroundColorB: "#000000"
    property color textColorB: "#ffffff"
    property string fontFamilyB: "Arial"
    property int fontSizeB: 48
    property string backgroundTypeB: "solidColor"
    property string backgroundImageDataBase64B: ""
    property color gradientStartColorB: "#1e3a8a"
    property color gradientEndColorB: "#60a5fa"
    property int gradientAngleB: 135
    property string backgroundVideoSourceB: ""
    property bool videoLoopB: true
    // Text legibility properties for container B
    property bool dropShadowEnabledB: true
    property color dropShadowColorB: "#000000"
    property int dropShadowOffsetXB: 2
    property int dropShadowOffsetYB: 2
    property int dropShadowBlurB: 4
    property bool overlayEnabledB: false
    property color overlayColorB: "#80000000"
    property int overlayBlurB: 0
    property bool textContainerEnabledB: false
    property color textContainerColorB: "#80000000"
    property int textContainerPaddingB: 20
    property int textContainerRadiusB: 8
    property int textContainerBlurB: 0
    property bool textBandEnabledB: false
    property color textBandColorB: "#80000000"
    property int textBandBlurB: 0

    // Copy current displayController values to the specified container
    function copyToContainerA() {
        slideTextA = displayController.slideText
        backgroundColorA = displayController.backgroundColor
        textColorA = displayController.textColor
        fontFamilyA = displayController.fontFamily
        fontSizeA = displayController.fontSize
        backgroundTypeA = displayController.backgroundType
        backgroundImageDataBase64A = displayController.backgroundImageDataBase64
        gradientStartColorA = displayController.gradientStartColor
        gradientEndColorA = displayController.gradientEndColor
        gradientAngleA = displayController.gradientAngle
        backgroundVideoSourceA = displayController.backgroundVideoSource
        videoLoopA = displayController.videoLoop
        // Text legibility
        dropShadowEnabledA = displayController.dropShadowEnabled
        dropShadowColorA = displayController.dropShadowColor
        dropShadowOffsetXA = displayController.dropShadowOffsetX
        dropShadowOffsetYA = displayController.dropShadowOffsetY
        dropShadowBlurA = displayController.dropShadowBlur
        overlayEnabledA = displayController.overlayEnabled
        overlayColorA = displayController.overlayColor
        overlayBlurA = displayController.overlayBlur
        textContainerEnabledA = displayController.textContainerEnabled
        textContainerColorA = displayController.textContainerColor
        textContainerPaddingA = displayController.textContainerPadding
        textContainerRadiusA = displayController.textContainerRadius
        textContainerBlurA = displayController.textContainerBlur
        textBandEnabledA = displayController.textBandEnabled
        textBandColorA = displayController.textBandColor
        textBandBlurA = displayController.textBandBlur
    }

    function copyToContainerB() {
        slideTextB = displayController.slideText
        backgroundColorB = displayController.backgroundColor
        textColorB = displayController.textColor
        fontFamilyB = displayController.fontFamily
        fontSizeB = displayController.fontSize
        backgroundTypeB = displayController.backgroundType
        backgroundImageDataBase64B = displayController.backgroundImageDataBase64
        gradientStartColorB = displayController.gradientStartColor
        gradientEndColorB = displayController.gradientEndColor
        gradientAngleB = displayController.gradientAngle
        backgroundVideoSourceB = displayController.backgroundVideoSource
        videoLoopB = displayController.videoLoop
        // Text legibility
        dropShadowEnabledB = displayController.dropShadowEnabled
        dropShadowColorB = displayController.dropShadowColor
        dropShadowOffsetXB = displayController.dropShadowOffsetX
        dropShadowOffsetYB = displayController.dropShadowOffsetY
        dropShadowBlurB = displayController.dropShadowBlur
        overlayEnabledB = displayController.overlayEnabled
        overlayColorB = displayController.overlayColor
        overlayBlurB = displayController.overlayBlur
        textContainerEnabledB = displayController.textContainerEnabled
        textContainerColorB = displayController.textContainerColor
        textContainerPaddingB = displayController.textContainerPadding
        textContainerRadiusB = displayController.textContainerRadius
        textContainerBlurB = displayController.textContainerBlur
        textBandEnabledB = displayController.textBandEnabled
        textBandColorB = displayController.textBandColor
        textBandBlurB = displayController.textBandBlur
    }

    // Initialize container A with first slide
    Component.onCompleted: {
        copyToContainerA()
        console.log("OutputDisplay QML loaded with transition support")
    }

    // Listen for signals from C++
    Connections {
        target: displayController

        function onStartTransition() {
            if (!root.transitioning) {
                root.beginTransition()
            }
        }

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

        // Handle cut transitions - immediate update without animation
        function onCutTransition() {
            if (root.showingA) {
                root.copyToContainerA()
            } else {
                root.copyToContainerB()
            }
        }

        // When display is cleared, immediately update the current container
        function onIsClearedChanged() {
            if (displayController.isCleared) {
                // Force immediate update when clearing (no transition)
                if (root.showingA) {
                    root.copyToContainerA()
                } else {
                    root.copyToContainerB()
                }
            }
        }
    }

    // Begin a transition to the new slide
    function beginTransition() {
        // For cut transition, just update the current container immediately
        if (displayController.transitionType === "cut") {
            if (showingA) {
                copyToContainerA()
            } else {
                copyToContainerB()
            }
            displayController.transitionComplete()
            return
        }

        root.transitioning = true

        // Copy new slide data to the incoming container and prepare positions
        // Set animation destination values based on transition type
        outX.to = getOutgoingX()
        outY.to = getOutgoingY()

        // For crossfade, we need the incoming slide to render ON TOP of the outgoing slide
        // So we need to manage z-order
        var isFade = (displayController.transitionType === "fade")

        if (root.showingA) {
            // A is current (outgoing), B is next (incoming)
            copyToContainerB()
            slideB.prepareForTransitionIn()

            // For crossfade: incoming (B) must be on top, outgoing (A) stays fully visible
            if (isFade) {
                slideB.z = 1
                slideA.z = 0
            }

            // Set animation targets explicitly
            outOpacity.target = slideA
            outX.target = slideA
            outY.target = slideA
            inOpacity.target = slideB
            inX.target = slideB
            inY.target = slideB

            // For crossfade: outgoing stays at full opacity
            outOpacity.to = isFade ? 1 : 0
        } else {
            // B is current (outgoing), A is next (incoming)
            copyToContainerA()
            slideA.prepareForTransitionIn()

            // For crossfade: incoming (A) must be on top, outgoing (B) stays fully visible
            if (isFade) {
                slideA.z = 1
                slideB.z = 0
            }

            // Set animation targets explicitly
            outOpacity.target = slideB
            outX.target = slideB
            outY.target = slideB
            inOpacity.target = slideA
            inX.target = slideA
            inY.target = slideA

            // For crossfade: outgoing stays at full opacity
            outOpacity.to = isFade ? 1 : 0
        }

        // Start the animations
        transitionOutAnimation.start()
        transitionInAnimation.start()
    }

    // Called when transition animations complete
    function onTransitionFinished() {
        root.showingA = !root.showingA
        root.transitioning = false

        // Reset positions for next transition
        slideA.x = 0
        slideA.y = 0
        slideB.x = 0
        slideB.y = 0

        // Reset z-order (both at same level)
        slideA.z = 0
        slideB.z = 0

        // Update opacities - the new current slide is fully visible, the other is hidden
        slideA.opacity = root.showingA ? 1 : 0
        slideB.opacity = root.showingA ? 0 : 1

        // Tell C++ transition is done
        displayController.transitionComplete()
    }

    // Slide container A
    Item {
        id: slideA
        width: parent.width
        height: parent.height
        opacity: 1
        visible: opacity > 0

        function prepareForTransitionIn() {
            x = root.getIncomingStartX()
            y = root.getIncomingStartY()
            opacity = (displayController.transitionType === "fade") ? 0 : 1
        }

        // Solid color background
        Rectangle {
            anchors.fill: parent
            color: root.backgroundTypeA === "solidColor" ? root.backgroundColorA : "black"
            visible: root.backgroundTypeA === "solidColor"
        }

        // Gradient background
        Rectangle {
            anchors.centerIn: parent
            property real angleRad: root.gradientAngleA * Math.PI / 180.0
            property real absSin: Math.abs(Math.sin(angleRad))
            property real absCos: Math.abs(Math.cos(angleRad))
            width: parent.width * absCos + parent.height * absSin
            height: parent.width * absSin + parent.height * absCos
            visible: root.backgroundTypeA === "gradient"
            rotation: root.gradientAngleA
            gradient: Gradient {
                GradientStop { position: 0.0; color: root.gradientStartColorA }
                GradientStop { position: 1.0; color: root.gradientEndColorA }
            }
        }

        // Background image
        Image {
            anchors.fill: parent
            visible: root.backgroundTypeA === "image"
            source: root.backgroundTypeA === "image" && root.backgroundImageDataBase64A !== ""
                    ? "data:image/png;base64," + root.backgroundImageDataBase64A : ""
            fillMode: Image.PreserveAspectCrop
            smooth: true
        }

        // Background video
        Video {
            id: videoA
            anchors.fill: parent
            visible: root.backgroundTypeA === "video"
            source: root.backgroundTypeA === "video" ? root.backgroundVideoSourceA : ""
            loops: root.videoLoopA ? MediaPlayer.Infinite : 1
            muted: true  // Always muted for background videos
            fillMode: VideoOutput.PreserveAspectCrop
            // Auto-play when source changes and video is visible
            onSourceChanged: {
                if (source !== "" && root.backgroundTypeA === "video") {
                    play()
                }
            }
            // Also play when becoming visible during transitions
            onVisibleChanged: {
                if (visible && source !== "") {
                    play()
                } else if (!visible) {
                    stop()
                }
            }
        }

        // Background overlay - darkens the entire background for better text visibility
        Rectangle {
            id: overlayA
            anchors.fill: parent
            color: root.overlayColorA
            visible: root.overlayEnabledA
        }

        // Text band - horizontal strip behind text area
        Rectangle {
            id: textBandA
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: textContentA.height + root.textContainerPaddingA * 2
            color: root.textBandColorA
            visible: root.textBandEnabledA
        }

        // Text container - box behind text
        Rectangle {
            id: textContainerA
            anchors.centerIn: parent
            width: textContentA.width + root.textContainerPaddingA * 2
            height: textContentA.height + root.textContainerPaddingA * 2
            radius: root.textContainerRadiusA
            color: root.textContainerColorA
            visible: root.textContainerEnabledA
        }

        // Text shadow (rendered behind main text for drop shadow effect)
        Text {
            id: textShadowA
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.dropShadowOffsetXA
            anchors.verticalCenterOffset: root.dropShadowOffsetYA
            text: root.slideTextA
            color: root.dropShadowColorA
            font.family: root.fontFamilyA
            font.pixelSize: root.fontSizeA
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            width: parent.width * 0.8
            visible: root.dropShadowEnabledA
        }

        // Main text content
        Text {
            id: textContentA
            anchors.centerIn: parent
            text: root.slideTextA
            color: root.textColorA
            font.family: root.fontFamilyA
            font.pixelSize: root.fontSizeA
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            width: parent.width * 0.8
            style: Text.Outline
            styleColor: "black"
        }
    }

    // Slide container B
    Item {
        id: slideB
        width: parent.width
        height: parent.height
        opacity: 0
        visible: opacity > 0

        function prepareForTransitionIn() {
            x = root.getIncomingStartX()
            y = root.getIncomingStartY()
            opacity = (displayController.transitionType === "fade") ? 0 : 1
        }

        // Solid color background
        Rectangle {
            anchors.fill: parent
            color: root.backgroundTypeB === "solidColor" ? root.backgroundColorB : "black"
            visible: root.backgroundTypeB === "solidColor"
        }

        // Gradient background
        Rectangle {
            anchors.centerIn: parent
            property real angleRad: root.gradientAngleB * Math.PI / 180.0
            property real absSin: Math.abs(Math.sin(angleRad))
            property real absCos: Math.abs(Math.cos(angleRad))
            width: parent.width * absCos + parent.height * absSin
            height: parent.width * absSin + parent.height * absCos
            visible: root.backgroundTypeB === "gradient"
            rotation: root.gradientAngleB
            gradient: Gradient {
                GradientStop { position: 0.0; color: root.gradientStartColorB }
                GradientStop { position: 1.0; color: root.gradientEndColorB }
            }
        }

        // Background image
        Image {
            anchors.fill: parent
            visible: root.backgroundTypeB === "image"
            source: root.backgroundTypeB === "image" && root.backgroundImageDataBase64B !== ""
                    ? "data:image/png;base64," + root.backgroundImageDataBase64B : ""
            fillMode: Image.PreserveAspectCrop
            smooth: true
        }

        // Background video
        Video {
            id: videoB
            anchors.fill: parent
            visible: root.backgroundTypeB === "video"
            source: root.backgroundTypeB === "video" ? root.backgroundVideoSourceB : ""
            loops: root.videoLoopB ? MediaPlayer.Infinite : 1
            muted: true  // Always muted for background videos
            fillMode: VideoOutput.PreserveAspectCrop
            // Auto-play when source changes and video is visible
            onSourceChanged: {
                if (source !== "" && root.backgroundTypeB === "video") {
                    play()
                }
            }
            // Also play when becoming visible during transitions
            onVisibleChanged: {
                if (visible && source !== "") {
                    play()
                } else if (!visible) {
                    stop()
                }
            }
        }

        // Background overlay - darkens the entire background for better text visibility
        Rectangle {
            id: overlayB
            anchors.fill: parent
            color: root.overlayColorB
            visible: root.overlayEnabledB
        }

        // Text band - horizontal strip behind text area
        Rectangle {
            id: textBandB
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: textContentB.height + root.textContainerPaddingB * 2
            color: root.textBandColorB
            visible: root.textBandEnabledB
        }

        // Text container - box behind text
        Rectangle {
            id: textContainerB
            anchors.centerIn: parent
            width: textContentB.width + root.textContainerPaddingB * 2
            height: textContentB.height + root.textContainerPaddingB * 2
            radius: root.textContainerRadiusB
            color: root.textContainerColorB
            visible: root.textContainerEnabledB
        }

        // Text shadow (rendered behind main text for drop shadow effect)
        Text {
            id: textShadowB
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.dropShadowOffsetXB
            anchors.verticalCenterOffset: root.dropShadowOffsetYB
            text: root.slideTextB
            color: root.dropShadowColorB
            font.family: root.fontFamilyB
            font.pixelSize: root.fontSizeB
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            width: parent.width * 0.8
            visible: root.dropShadowEnabledB
        }

        // Main text content
        Text {
            id: textContentB
            anchors.centerIn: parent
            text: root.slideTextB
            color: root.textColorB
            font.family: root.fontFamilyB
            font.pixelSize: root.fontSizeB
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            width: parent.width * 0.8
            style: Text.Outline
            styleColor: "black"
        }
    }

    // Animation for outgoing slide
    ParallelAnimation {
        id: transitionOutAnimation

        NumberAnimation {
            id: outOpacity
            property: "opacity"
            to: 0
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            id: outX
            property: "x"
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            id: outY
            property: "y"
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }
    }

    // Animation for incoming slide
    ParallelAnimation {
        id: transitionInAnimation

        NumberAnimation {
            id: inOpacity
            property: "opacity"
            to: 1
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            id: inX
            property: "x"
            to: 0
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            id: inY
            property: "y"
            to: 0
            duration: displayController.transitionDuration
            easing.type: Easing.InOutQuad
        }

        onFinished: {
            root.onTransitionFinished()
        }
    }

    // Calculate outgoing position based on transition type
    function getOutgoingX() {
        switch (displayController.transitionType) {
            case "slideLeft": return -root.width
            case "slideRight": return root.width
            default: return 0
        }
    }

    function getOutgoingY() {
        switch (displayController.transitionType) {
            case "slideUp": return -root.height
            case "slideDown": return root.height
            default: return 0
        }
    }

    // Calculate incoming start position based on transition type
    function getIncomingStartX() {
        switch (displayController.transitionType) {
            case "slideLeft": return root.width
            case "slideRight": return -root.width
            default: return 0
        }
    }

    function getIncomingStartY() {
        switch (displayController.transitionType) {
            case "slideUp": return root.height
            case "slideDown": return -root.height
            default: return 0
        }
    }
}
