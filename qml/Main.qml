

/*
 * Copyright 2024-2026 NXP
 *
 * NXP Proprietary. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and that
 * you agree to comply with and are bound by, such license terms.  If you do
 * not agree to be bound by the applicable license terms, then you may not
 * retain, install, activate or otherwise use the software.
 *
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Window {
    id: root
    width: 1920
    height: 1080
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    visibility: "FullScreen"
    color: theme.bgPrimary

    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════
    readonly property bool multiEndpoint: false
    readonly property bool cancelSupport: false

    // ═══════════════════════════════════════════════════════════════
    // COLOR SCHEME - Minimal Dark Pro
    // ═══════════════════════════════════════════════════════════════
    QtObject {
        id: theme
        readonly property color bgPrimary: "#000000"
        readonly property color bgSecondary: "#1C1C1E"
        readonly property color bgTertiary: "#2C2C2E"
        readonly property color accentPrimary: "#0A84FF"
        readonly property color accentSecondary: "#30D158"
        readonly property color accentWarning: "#FF9F0A"
        readonly property color textPrimary: "#FFFFFF"
        readonly property color textSecondary: "#98989D"
        readonly property color borderActive: "#0A84FF"
        readonly property color borderInactive: "#38383A"

        // UI Dimensions
        readonly property int brandingHeight: 10
        readonly property int titleHeight: 125
        readonly property int inputHeight: 100
        readonly property int submitWidth: 200
        readonly property int statsHeight: 40
        readonly property int selectorHeight: 50
        readonly property int standardMargin: 15
        readonly property int bottomMargin: 25
        readonly property int standardRadius: 15
        readonly property int smallRadius: 5
        readonly property int borderWidth: 2

        // Font sizes
        readonly property int fontLarge: 26
        readonly property int fontMedium: 22
        readonly property int fontSmall: 18
        readonly property int fontTiny: 16
        readonly property int fontMicro: 14
        readonly property int fontProgress: 12

        // Animation durations
        readonly property int animationFast: 200
        readonly property int animationMedium: 500
        readonly property int animationSlow: 800
        readonly property int animationPulse: 1000
        readonly property int animationShimmer: 1500
    }

    // ═══════════════════════════════════════════════════════════════
    // TIMERS
    // ═══════════════════════════════════════════════════════════════
    Timer {
        id: progressTimerTTFT
        interval: 100
        repeat: true
        running: mySubmitPrompt.progressTTFT
        onTriggered: {
            if (progressBarTTFT.value < progressBarTTFT.to) {
                progressBarTTFT.value += 100
            }
        }
    }

    Timer {
        id: progressTimerModelLoad
        interval: 100
        repeat: true
        running: mySubmitPrompt.loadModel
        onTriggered: {
            if (progressBarModel.value < (progressBarModel.to * 0.95)) {
                progressBarModel.value += 100
            }
        }
        onRunningChanged: {
            if (!running) {
                progressBarModel.value = progressBarModel.to
            }
        }
    }
    // ═══════════════════════════════════════════════════════════════
    // DIALOGS
    // ═══════════════════════════════════════════════════════════════
    AboutDialog {
        id: aboutDialog
    }

    // ═══════════════════════════════════════════════════════════════
    // MAIN LAYOUT
    // ═══════════════════════════════════════════════════════════════
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 15

        // NXP Branding Bar
        Rectangle {
            id: brandingBar
            Layout.maximumHeight: 15
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredHeight: theme.brandingHeight
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.1
                    color: "#F9B500"
                }
                GradientStop {
                    position: 0.45
                    color: "#69CA00"
                }
                GradientStop {
                    position: 0.55
                    color: "#69CA00"
                }
                GradientStop {
                    position: 0.9
                    color: "#0EAFE0"
                }
            }
        }

        // Title Bar
        Rectangle {
            id: titleBar
            height: 120
            Layout.fillWidth: true
            Layout.preferredHeight: theme.titleHeight
            color: "transparent"
            Layout.maximumWidth: 1920
            Layout.maximumHeight: 120
            Layout.fillHeight: true

            Image {
                id: titleImage
                horizontalAlignment: Image.AlignLeft
                anchors {
                    left: parent.left
                    right: rightButtonsRow.left
                    top: parent.top
                    bottom: parent.bottom
                    leftMargin: 5
                    rightMargin: theme.standardMargin
                }
                source: "../images/title.png"
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 3096
            }

            Row {
                id: rightButtonsRow
                anchors.right: parent.right
                anchors.rightMargin: 15
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                spacing: theme.standardMargin

                Button {
                    id: aboutButton
                    width: 120
                    height: 50
                    text: "About"

                    font.pixelSize: theme.fontMedium
                    font.family: "Poppins"
                    font.bold: true

                    onClicked: aboutDialog.visible = true

                    background: Rectangle {
                        color: aboutButton.hovered ? theme.accentPrimary : theme.bgSecondary
                        radius: theme.smallRadius
                        border.color: theme.textSecondary
                        border.width: theme.borderWidth

                        Behavior on color {
                            ColorAnimation {
                                duration: theme.animationFast
                            }
                        }
                    }

                    contentItem: Text {
                        text: aboutButton.text
                        font: aboutButton.font
                        color: theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: closeButton
                    width: 120
                    height: 50
                    text: "Close"

                    font.pixelSize: theme.fontMedium
                    font.family: "Poppins"
                    font.bold: true

                    onClicked: {
                        mySubmitPrompt.killConnectorProcess()
                        Qt.quit()
                    }

                    background: Rectangle {
                        color: closeButton.hovered ? theme.accentWarning : theme.bgSecondary
                        radius: theme.smallRadius
                        border.color: theme.textSecondary
                        border.width: theme.borderWidth

                        Behavior on color {
                            ColorAnimation {
                                duration: theme.animationFast
                            }
                        }
                    }

                    contentItem: Text {
                        text: closeButton.text
                        font: closeButton.font
                        color: theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // Output Area
        Rectangle {
            id: outputArea
            width: 0
            height: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: theme.standardMargin
            Layout.rightMargin: theme.standardMargin
            color: theme.bgSecondary
            radius: theme.standardRadius
            border.color: theme.borderInactive
            border.width: theme.borderWidth

            ColumnLayout {
                id: outputLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 0
                anchors.topMargin: 0
                spacing: 0
                // Scrollable Output Text
                ScrollView {
                    id: scrollView
                    padding: 5
                    rightPadding: 5
                    leftPadding: 5
                    bottomPadding: 5
                    topPadding: 5
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    Layout.bottomMargin: theme.standardMargin
                    implicitWidth: 0
                    implicitHeight: 0
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    Flickable {
                        id: flickable
                        width: scrollView.width
                        contentWidth: scrollView.width
                        contentHeight: outputText.contentHeight + theme.standardMargin * 2
                        boundsBehavior: Flickable.StopAtBounds

                        Text {
                            id: outputText
                            width: parent.width - 30
                            x: theme.standardMargin
                            y: theme.standardMargin
                            color: theme.textPrimary
                            text: mySubmitPrompt.outputLlm
                            font.pixelSize: theme.fontLarge
                            font.family: "Poppins"
                            wrapMode: Text.WordWrap
                            textFormat: Text.MarkdownText

                            onTextChanged: {
                                scrollToBottom.restart()
                            }
                        }
                    }

                    // Timer to ensure scroll happens after layout update
                    Timer {
                        id: scrollToBottom
                        interval: 50
                        onTriggered: {
                            if (flickable.contentHeight > flickable.height) {
                                flickable.contentY = flickable.contentHeight - flickable.height
                            }
                        }
                    }
                }
                // Stats Bar with Cancel Button
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: theme.statsHeight
                    Layout.leftMargin: theme.standardMargin
                    Layout.rightMargin: theme.standardMargin

                    Rectangle {
                        id: statsBar
                        anchors.fill: parent
                        color: theme.bgTertiary
                        radius: theme.smallRadius

                        Text {
                            id: statsText
                            anchors {
                                fill: parent
                                leftMargin: theme.standardMargin
                                rightMargin: 50
                            }
                            color: "#ebe7dd"
                            text: mySubmitPrompt.outputStats
                            font.pixelSize: theme.fontMedium
                            font.family: "Poppins"
                            verticalAlignment: Text.AlignVCenter
                        }

                        BusyIndicator {
                            id: processingSpinner
                            width: theme.statsHeight
                            height: theme.statsHeight
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                                rightMargin: 5
                            }
                            running: mySubmitPrompt.processingLLM
                        }
                    }

                    // Cancel Button (overlays stats bar)
                    /*IconButton {
                        id: cancelButton
                        width: 100
                        height: 40
                        anchors {
                            right: parent.right
                            bottom: parent.top
                            rightMargin: 0
                            bottomMargin: 5
                        }
                        iconSource: "../images/cancel"
                        iconWidth: 90
                        iconHeight: 30
                        enabled: mySubmitPrompt.processingLLM
                        visible: root.cancelSupport
                                 && mySubmitPrompt.processingLLM
                        opacity: enabled ? 1.0 : 0.3
                        onClicked: mySubmitPrompt.stopLlm()
                    }*/
                }

                // Model Selector Bar
                Rectangle {
                    id: modelSelectorBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: theme.selectorHeight
                    Layout.leftMargin: theme.standardMargin
                    Layout.rightMargin: theme.standardMargin
                    Layout.topMargin: 5
                    Layout.bottomMargin: theme.standardMargin
                    color: theme.bgTertiary
                    radius: theme.smallRadius

                    Row {
                        id: selectorRow
                        anchors {
                            left: parent.left
                            verticalCenter: parent.verticalCenter
                            leftMargin: theme.standardMargin
                        }
                        spacing: 10

                        Text {
                            text: "Model:"
                            color: "#ebe7dd"
                            font.pixelSize: theme.fontSmall
                            font.family: "Poppins"
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        StyledComboBox {
                            id: modelSelector
                            width: 200
                            height: 32
                            model: mySubmitPrompt.getAvailableModels()
                            currentIndex: mySubmitPrompt.currentModelIndex
                            enabled: !mySubmitPrompt.modelLoaded && !mySubmitPrompt.processingLLM && !mySubmitPrompt.loadModel

                            Component.onCompleted: {
                                console.log("[QML] ModelSelector initialized")
                                console.log("[QML] Available models:",
                                            mySubmitPrompt.getAvailableModels())
                                console.log("[QML] Current index:",
                                            mySubmitPrompt.currentModelIndex)
                            }

                            onActivated: function (index) {
                                console.log("[QML] Model selector activated with index:",
                                            index)
                                if (index !== mySubmitPrompt.currentModelIndex) {
                                    mySubmitPrompt.setCurrentModelIndex(index)
                                }
                            }

                            Connections {
                                target: mySubmitPrompt

                                function onAvailableModelsChanged() {
                                    console.log("[QML] availableModelsChanged signal received")
                                    console.log("[QML] New models list:",
                                                mySubmitPrompt.getAvailableModels(
                                                    ))
                                    modelSelector.model = mySubmitPrompt.getAvailableModels()
                                }

                                function onCurrentModelIndexChanged() {
                                    console.log("[QML] currentModelIndexChanged signal received")
                                    console.log("[QML] New index:",
                                                mySubmitPrompt.currentModelIndex)
                                    if (modelSelector.currentIndex
                                            !== mySubmitPrompt.currentModelIndex) {
                                        modelSelector.currentIndex
                                                = mySubmitPrompt.currentModelIndex
                                    }
                                }
                            }
                        }

                        Text {
                            text: "Endpoint:"
                            color: "#ebe7dd"
                            font.pixelSize: theme.fontSmall
                            font.family: "Poppins"
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        StyledComboBox {
                            id: endpointSelector
                            width: 120
                            height: 32
                            model: root.multiEndpoint ? ["USB", "PCIe0", "PCIe1", "All"] : ["PCIe0"]
                            currentIndex: root.multiEndpoint ? 1 : 0
                            enabled: !mySubmitPrompt.modelLoaded && !mySubmitPrompt.loadModel && !mySubmitPrompt.processingLLM
                            onActivated: function (index) {
                                console.log("[QML] Endpoint changed to:",
                                            currentText)
                                mySubmitPrompt.setCurrentEndpoint(currentText)
                            }
                        }
                    }

                    Row {
                        id: buttonRow
                        anchors {
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            rightMargin: theme.standardMargin
                        }
                        spacing: 5

                        ActionButton {
                            id: loadButton
                            width: 80
                            height: 36
                            text: "Load"
                            accentColor: theme.accentSecondary
                            enabled: !mySubmitPrompt.modelLoaded
                                     && !mySubmitPrompt.loadModel
                            onClicked: {
                                progressBarModel.value = 0
                                mySubmitPrompt.setLoadModel(true)
                            }
                        }

                        ActionButton {
                            id: ejectButton
                            width: 80
                            height: 36
                            text: "Eject"
                            accentColor: theme.accentWarning
                            enabled: mySubmitPrompt.modelLoaded
                                     && !mySubmitPrompt.processingLLM
                            onClicked: mySubmitPrompt.ejectModel()
                        }
                    }
                }

                // Progress Bar Container (overlays selector when loading)
                Item {
                    id: progressContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: 15
                    Layout.leftMargin: theme.standardMargin
                    Layout.rightMargin: theme.standardMargin
                    Layout.bottomMargin: theme.standardMargin
                    visible: mySubmitPrompt.loadModel
                    z: 10

                    AnimatedProgressBar {
                        id: progressBarModel
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                        }
                        height: 15
                        from: 0
                        to: mySubmitPrompt.modelLoadTime
                        value: 0
                        property bool allowAnimation: true

                        onVisibleChanged: {
                            if (visible) {
                                // When becoming visible, disable animation and reset to 0
                                allowAnimation = false
                                value = 0
                                allowAnimation = true
                            } else if (!mySubmitPrompt.loadModel) {
                                allowAnimation = false
                                value = 0
                                allowAnimation = true
                            }
                        }

                        Text {
                            id: progressText
                            anchors.centerIn: parent
                            text: Math.round(
                                      parent.value / parent.to * 100.0) + "%"
                            font.pixelSize: theme.fontProgress
                            font.family: "Poppins"
                            font.weight: Font.Medium
                            color: theme.textPrimary

                            SequentialAnimation on opacity {
                                running: progressContainer.visible
                                loops: Animation.Infinite
                                NumberAnimation {
                                    from: 0.7
                                    to: 1.0
                                    duration: theme.animationPulse
                                }
                                NumberAnimation {
                                    from: 1.0
                                    to: 0.7
                                    duration: theme.animationPulse
                                }
                            }
                        }
                    }
                }
            }
        }

        // Input Area
        RowLayout {
            id: inputRow
            Layout.maximumHeight: 100
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredHeight: theme.inputHeight
            Layout.leftMargin: theme.standardMargin
            Layout.rightMargin: theme.bottomMargin
            Layout.bottomMargin: theme.bottomMargin
            spacing: 25

            TextField {
                id: inputPrompt
                Layout.fillWidth: true
                Layout.preferredHeight: theme.inputHeight

                placeholderText: "Enter your prompt here..."
                placeholderTextColor: theme.textSecondary
                color: theme.textPrimary
                selectedTextColor: "#EBE7DD"
                selectionColor: "#0068DF"
                text: mySubmitPrompt.promptText

                font.pixelSize: theme.fontLarge
                font.family: "Poppins"
                wrapMode: TextInput.WordWrap
                padding: 5
                rightPadding: 5
                leftPadding: 5
                bottomPadding: 5
                topPadding: 5
                verticalAlignment: TextInput.AlignVCenter

                enabled: !mySubmitPrompt.processingLLM
                         && mySubmitPrompt.modelLoaded
                opacity: enabled ? 1.0 : 0.5

                background: Rectangle {
                    color: theme.bgSecondary
                    radius: theme.standardRadius
                    border.color: inputPrompt.activeFocus ? theme.accentPrimary : theme.borderInactive
                    border.width: theme.borderWidth

                    Behavior on border.color {
                        ColorAnimation {
                            duration: theme.animationFast
                        }
                    }
                }

                Keys.onReturnPressed: handleSubmit()

                function handleSubmit() {
                    if (text.trim().length === 0 || !enabled)
                        return

                    progressBarTTFT.value = 0
                    mySubmitPrompt.promptText = text
                    mySubmitPrompt.submitPrompt()
                }
            }

            Button {
                id: submitButton
                Layout.preferredWidth: theme.submitWidth
                Layout.preferredHeight: theme.inputHeight

                enabled: !mySubmitPrompt.processingLLM
                         && mySubmitPrompt.modelLoaded && inputPrompt.text.trim(
                             ).length > 0

                hoverEnabled: true

                onClicked: inputPrompt.handleSubmit()

                background: Rectangle {
                    color: theme.bgSecondary
                    radius: theme.standardRadius
                    border.color: submitButton.enabled ? (submitButton.hovered ? theme.accentSecondary : theme.borderInactive) : theme.borderInactive
                    border.width: theme.borderWidth
                    opacity: submitButton.enabled ? 1.0 : 0.3

                    Behavior on border.color {
                        ColorAnimation {
                            duration: theme.animationFast
                        }
                    }

                    Behavior on opacity {
                        NumberAnimation {
                            duration: theme.animationFast
                        }
                    }
                }

                contentItem: Item {
                    Image {
                        id: submitIcon
                        source: "../images/submit.svg"
                        sourceSize.width: 200
                        sourceSize.height: theme.inputHeight - 40
                        fillMode: Image.Stretch
                        smooth: true
                        antialiasing: true

                        opacity: submitButton.enabled ? (submitButton.hovered ? 0.7 : 1.0) : 0.3

                        Behavior on opacity {
                            NumberAnimation {
                                duration: theme.animationFast
                            }
                        }
                        anchors.fill: parent
                    }
                }

                // Subtle scale animation on hover
                scale: submitButton.enabled && submitButton.hovered ? 1.05 : 1.0

                Behavior on scale {
                    NumberAnimation {
                        duration: theme.animationFast
                        easing.type: Easing.OutQuad
                    }
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // REUSABLE COMPONENTS
    // ═══════════════════════════════════════════════════════════════
    component IconButton: Button {
        id: iconBtn
        property string iconSource: ""
        property int iconWidth: 40
        property int iconHeight: 40

        icon.source: down ? iconSource + "_pressed.png" : iconSource + ".png"
        icon.width: iconWidth
        icon.height: iconHeight
        display: AbstractButton.IconOnly

        background: Rectangle {
            color: "transparent"
        }
    }

    component ActionButton: Button {
        id: actionBtn
        property color accentColor: theme.accentPrimary

        font.pixelSize: theme.fontMicro
        font.family: "Poppins"
        font.bold: true
        opacity: enabled ? 1.0 : 0.3

        background: Rectangle {
            color: actionBtn.enabled
                   && actionBtn.hovered ? actionBtn.accentColor : theme.bgSecondary
            radius: theme.smallRadius
            border.color: actionBtn.enabled ? actionBtn.accentColor : theme.borderInactive
            border.width: theme.borderWidth

            Behavior on color {
                ColorAnimation {
                    duration: theme.animationFast
                }
            }
        }

        contentItem: Text {
            text: actionBtn.text
            font: actionBtn.font
            color: actionBtn.enabled ? theme.textPrimary : theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    component StyledComboBox: ComboBox {
        id: combo

        font.pixelSize: theme.fontTiny
        font.family: "Poppins"
        opacity: enabled ? 1.0 : 0.5

        background: Rectangle {
            color: combo.enabled ? theme.bgSecondary : theme.bgTertiary
            border.color: combo.down ? theme.accentPrimary : theme.borderInactive
            border.width: 1
            radius: 4
        }

        contentItem: Text {
            leftPadding: 10
            text: combo.displayText
            font: combo.font
            color: combo.enabled ? theme.textPrimary : theme.textSecondary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        popup: Popup {
            y: combo.height
            width: combo.width
            implicitHeight: contentItem.implicitHeight
            padding: 1

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
                ScrollIndicator.vertical: ScrollIndicator {}
            }

            background: Rectangle {
                color: "#262626"
                border.color: "#69ca00"
                border.width: 1
                radius: 4
            }
        }

        delegate: ItemDelegate {
            width: combo.width
            contentItem: Text {
                text: modelData
                color: "#ffffff"
                font: combo.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
            }
            highlighted: combo.highlightedIndex === index

            background: Rectangle {
                color: highlighted ? "#0eafe0" : "transparent"
                opacity: highlighted ? 0.3 : 1.0
            }
        }
    }

    component AnimatedProgressBar: ProgressBar {
        id: progBar
        indeterminate: false

        background: Rectangle {
            implicitWidth: parent.width
            implicitHeight: parent.height
            color: theme.bgTertiary
            radius: 10

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: 9
                color: "transparent"
                border.width: 1
                border.color: "#20FFFFFF"
            }
        }

        contentItem: Item {
            Rectangle {
                id: mainProgress
                height: parent.height
                width: progBar.visualPosition * parent.width
                radius: 10
                transformOrigin: Item.Left
                clip: true

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0.0
                        color: "#00D9FF"
                    }
                    GradientStop {
                        position: 1.0
                        color: "#0A84FF"
                    }
                }

                SequentialAnimation on scale {
                    running: progBar.visible
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 1.0
                        to: 1.02
                        duration: theme.animationSlow
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        from: 1.02
                        to: 1.0
                        duration: theme.animationSlow
                        easing.type: Easing.InOutQuad
                    }
                }

                Rectangle {
                    id: shimmer
                    width: 60
                    height: parent.height
                    radius: 10

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop {
                            position: 0.0
                            color: "#00FFFFFF"
                        }
                        GradientStop {
                            position: 0.3
                            color: "#80FFFFFF"
                        }
                        GradientStop {
                            position: 0.7
                            color: "#80FFFFFF"
                        }
                        GradientStop {
                            position: 1.0
                            color: "#00FFFFFF"
                        }
                    }

                    SequentialAnimation on x {
                        running: progBar.visible
                                 && mainProgress.width > shimmer.width
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 0
                            to: mainProgress.width - shimmer.width
                            duration: theme.animationShimmer
                            easing.type: Easing.InOutQuad
                        }
                        PauseAnimation {
                            duration: theme.animationFast
                        }
                    }
                }
            }
        }

        Behavior on value {
            enabled: progressBarModel.allowAnimation
            NumberAnimation {
                duration: theme.animationMedium
                easing.type: Easing.OutQuad
            }
        }
    }

    // TTFT Progress Bar (if needed elsewhere)
    ProgressBar {
        id: progressBarTTFT
        visible: false
        from: 0
        to: 10000
        value: 0
    }
}
