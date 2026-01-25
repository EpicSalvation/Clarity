import QtQuick
import QtQuick.Window

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
    }

    // Initialize container A with first slide
    Component.onCompleted: {
        copyToContainerA()
        console.log("OutputDisplay QML loaded with transition support")
    }

    // Listen for transition start signal from C++
    Connections {
        target: displayController

        function onStartTransition() {
            if (!root.transitioning) {
                root.beginTransition()
            }
        }

        // When slide data changes and we're not transitioning, update immediately
        function onSlideTextChanged() {
            if (!root.transitioning && displayController.transitionType === "cut") {
                if (root.showingA) {
                    root.copyToContainerA()
                } else {
                    root.copyToContainerB()
                }
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

        if (root.showingA) {
            // A is current (outgoing), B is next (incoming)
            copyToContainerB()
            slideB.prepareForTransitionIn()
            // Set animation targets explicitly
            outOpacity.target = slideA
            outX.target = slideA
            outY.target = slideA
            inOpacity.target = slideB
            inX.target = slideB
            inY.target = slideB
        } else {
            // B is current (outgoing), A is next (incoming)
            copyToContainerA()
            slideA.prepareForTransitionIn()
            // Set animation targets explicitly
            outOpacity.target = slideB
            outX.target = slideB
            outY.target = slideB
            inOpacity.target = slideA
            inX.target = slideA
            inY.target = slideA
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

        // Update opacities
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

        // Text content
        Text {
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

        // Text content
        Text {
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
