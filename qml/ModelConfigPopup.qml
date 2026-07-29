/*
 * Copyright 2026 NXP
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
import QtQuick.Layouts

// ModelConfigPopup - editable LLM generation parameters popup.
Popup {
    id: root

    required property var theme
    required property var submitPrompt

    width: 430
    height: 360
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: (parent.width  - width)  / 2
    y: (parent.height - height) / 2

    property var pendingParams: ({})

    property bool allValid: fieldMaxTokens.acceptableInput &&
                            fieldTemperature.fieldValid &&
                            fieldTopK.acceptableInput &&
                            fieldTopP.fieldValid

    onOpened: {
        pendingParams = Object.assign({}, root.submitPrompt.llmParams)
        // Reset debounced valid flags so the popup opens without red borders
        fieldTemperature.fieldValid = true
        fieldTopP.fieldValid = true
    }

    // Refresh all TextField texts when pendingParams is replaced on Reset or on open
    onPendingParamsChanged: {
        if (pendingParams["generate_max_tokens"] !== undefined)
            fieldMaxTokens.text = pendingParams["generate_max_tokens"].toString()
        if (pendingParams["temperature"] !== undefined)
            fieldTemperature.text = pendingParams["temperature"].toString()
        if (pendingParams["top_k"] !== undefined)
            fieldTopK.text = pendingParams["top_k"].toString()
        if (pendingParams["top_p"] !== undefined)
            fieldTopP.text = pendingParams["top_p"].toString()
    }

    background: Rectangle {
        color: root.theme.bgSecondary
        border.color: "#48484A"
        border.width: root.theme.borderWidth
        radius: root.theme.standardRadius
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Text {
            text: "Model Configuration"
            color: root.theme.textPrimary
            font.pixelSize: root.theme.fontSmall
            font.family: "Poppins"
            font.bold: true
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: root.theme.borderInactive }

        // ── Max Tokens row ──────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "Max Tokens (1-4096):"
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                Layout.preferredWidth: 180
            }
            // Stepper: [<] [text input] [>] - steps by 1 per click (range 1-4096)
            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Button {
                    id: maxTokDecBtn
                    width: 26; height: 32
                    text: "<"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.max(1, (parseInt(fieldMaxTokens.text) || 1) - 1)
                        fieldMaxTokens.text = v.toString()
                        var m = Object.assign({}, root.pendingParams)
                        m["generate_max_tokens"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: maxTokDecBtn.pressed ? root.theme.accentPrimary
                               : maxTokDecBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: maxTokDecBtn.text; font: maxTokDecBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TextField {
                    id: fieldMaxTokens
                    Layout.fillWidth: true
                    height: 32
                    color: root.theme.textPrimary
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    horizontalAlignment: Text.AlignHCenter
                    text: root.pendingParams["generate_max_tokens"] !== undefined
                          ? root.pendingParams["generate_max_tokens"].toString() : ""
                    validator: IntValidator { bottom: 1; top: 4096 }
                    inputMethodHints: Qt.ImhDigitsOnly
                    background: Rectangle {
                        color: root.theme.bgTertiary
                        border.color: !fieldMaxTokens.acceptableInput ? "#FF453A"
                                      : fieldMaxTokens.activeFocus ? root.theme.accentPrimary
                                      : root.theme.borderInactive
                        border.width: root.theme.borderWidth
                        Behavior on border.color { ColorAnimation { duration: root.theme.animationFast } }
                    }
                    onTextEdited: {
                        if (!acceptableInput) return
                        var m = Object.assign({}, root.pendingParams)
                        m["generate_max_tokens"] = parseInt(text)
                        root.pendingParams = m
                    }
                    onEditingFinished: {
                        if (acceptableInput) {
                            var m = Object.assign({}, root.pendingParams)
                            m["generate_max_tokens"] = parseInt(text)
                            root.pendingParams = m
                        }
                    }
                }

                Button {
                    id: maxTokIncBtn
                    width: 26; height: 32
                    text: ">"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.min(4096, (parseInt(fieldMaxTokens.text) || 0) + 1)
                        fieldMaxTokens.text = v.toString()
                        var m = Object.assign({}, root.pendingParams)
                        m["generate_max_tokens"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: maxTokIncBtn.pressed ? root.theme.accentPrimary
                               : maxTokIncBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: maxTokIncBtn.text; font: maxTokIncBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // ── Temperature row ─────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "Temperature (0.0-1.0):"
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                Layout.preferredWidth: 180
            }
            // Stepper: [<] [text input] [>] - steps by 0.1 per click
            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Button {
                    id: tempDecBtn
                    width: 26; height: 32
                    text: "<"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.round((parseFloat(fieldTemperature.text || "0") - 0.1) * 10) / 10
                        v = Math.max(0.0, Math.min(1.0, v))
                        fieldTemperature.text = v.toFixed(1)
                        fieldTemperature.fieldValid = true
                        var m = Object.assign({}, root.pendingParams)
                        m["temperature"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: tempDecBtn.pressed ? root.theme.accentPrimary
                               : tempDecBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: tempDecBtn.text; font: tempDecBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TextField {
                    id: fieldTemperature
                    Layout.fillWidth: true
                    height: 32
                    color: root.theme.textPrimary
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    horizontalAlignment: Text.AlignHCenter
                    text: root.pendingParams["temperature"] !== undefined
                          ? root.pendingParams["temperature"].toString() : ""
                    // Accepts 0.9, .9, 0, 1, 1.0, etc. - no leading zero required
                    validator: RegularExpressionValidator {
                        regularExpression: /^(0(\.\d{0,2})?|\.\d{1,2}|1(\.0{0,2})?)$/
                    }
                    inputMethodHints: Qt.ImhFormattedNumbersOnly

                    // Red border is deferred until 600ms after the user stops typing
                    property bool fieldValid: true

                    Timer {
                        id: tempDebounce
                        interval: 600
                        repeat: false
                        onTriggered: {
                            fieldTemperature.fieldValid = fieldTemperature.acceptableInput
                            if (fieldTemperature.acceptableInput) {
                                var m = Object.assign({}, root.pendingParams)
                                m["temperature"] = parseFloat(fieldTemperature.text)
                                root.pendingParams = m
                            }
                        }
                    }

                    background: Rectangle {
                        color: root.theme.bgTertiary
                        border.color: !fieldTemperature.fieldValid ? "#FF453A"
                                      : fieldTemperature.activeFocus ? root.theme.accentPrimary
                                      : root.theme.borderInactive
                        border.width: root.theme.borderWidth
                        Behavior on border.color { ColorAnimation { duration: root.theme.animationFast } }
                    }
                    // Restart debounce on every keystroke - update only after pause
                    onTextEdited: tempDebounce.restart()
                    onEditingFinished: {
                        tempDebounce.stop()
                        fieldValid = acceptableInput
                        if (acceptableInput) {
                            var m = Object.assign({}, root.pendingParams)
                            m["temperature"] = parseFloat(text)
                            root.pendingParams = m
                        }
                    }
                }

                Button {
                    id: tempIncBtn
                    width: 26; height: 32
                    text: ">"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.round((parseFloat(fieldTemperature.text || "0") + 0.1) * 10) / 10
                        v = Math.max(0.0, Math.min(1.0, v))
                        fieldTemperature.text = v.toFixed(1)
                        fieldTemperature.fieldValid = true
                        var m = Object.assign({}, root.pendingParams)
                        m["temperature"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: tempIncBtn.pressed ? root.theme.accentPrimary
                               : tempIncBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: tempIncBtn.text; font: tempIncBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // ── Top-K row ────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "Top K (0-100):"
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                Layout.preferredWidth: 180
            }
            // Stepper: [<] [text input] [>] - steps by 1 per click (range 0-100)
            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Button {
                    id: topKDecBtn
                    width: 26; height: 32
                    text: "<"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.max(0, (parseInt(fieldTopK.text) || 0) - 1)
                        fieldTopK.text = v.toString()
                        var m = Object.assign({}, root.pendingParams)
                        m["top_k"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: topKDecBtn.pressed ? root.theme.accentPrimary
                               : topKDecBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: topKDecBtn.text; font: topKDecBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TextField {
                    id: fieldTopK
                    Layout.fillWidth: true
                    height: 32
                    color: root.theme.textPrimary
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    horizontalAlignment: Text.AlignHCenter
                    text: root.pendingParams["top_k"] !== undefined
                          ? root.pendingParams["top_k"].toString() : ""
                    validator: IntValidator { bottom: 0; top: 100 }
                    inputMethodHints: Qt.ImhDigitsOnly
                    background: Rectangle {
                        color: root.theme.bgTertiary
                        border.color: !fieldTopK.acceptableInput ? "#FF453A"
                                      : fieldTopK.activeFocus ? root.theme.accentPrimary
                                      : root.theme.borderInactive
                        border.width: root.theme.borderWidth
                        Behavior on border.color { ColorAnimation { duration: root.theme.animationFast } }
                    }
                    onTextEdited: {
                        if (!acceptableInput) return
                        var m = Object.assign({}, root.pendingParams)
                        m["top_k"] = parseInt(text)
                        root.pendingParams = m
                    }
                    onEditingFinished: {
                        if (acceptableInput) {
                            var m = Object.assign({}, root.pendingParams)
                            m["top_k"] = parseInt(text)
                            root.pendingParams = m
                        }
                    }
                }

                Button {
                    id: topKIncBtn
                    width: 26; height: 32
                    text: ">"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.min(100, (parseInt(fieldTopK.text) || 0) + 1)
                        fieldTopK.text = v.toString()
                        var m = Object.assign({}, root.pendingParams)
                        m["top_k"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: topKIncBtn.pressed ? root.theme.accentPrimary
                               : topKIncBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: topKIncBtn.text; font: topKIncBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // ── Top-P row ────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "Top P (0.0-1.0):"
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                Layout.preferredWidth: 180
            }
            // Stepper: [<] [text input] [>] - steps by 0.1 per click
            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Button {
                    id: topPDecBtn
                    width: 26; height: 32
                    text: "<"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.round((parseFloat(fieldTopP.text || "0") - 0.1) * 10) / 10
                        v = Math.max(0.0, Math.min(1.0, v))
                        fieldTopP.text = v.toFixed(1)
                        fieldTopP.fieldValid = true
                        var m = Object.assign({}, root.pendingParams)
                        m["top_p"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: topPDecBtn.pressed ? root.theme.accentPrimary
                               : topPDecBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: topPDecBtn.text; font: topPDecBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TextField {
                    id: fieldTopP
                    Layout.fillWidth: true
                    height: 32
                    color: root.theme.textPrimary
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    horizontalAlignment: Text.AlignHCenter
                    text: root.pendingParams["top_p"] !== undefined
                          ? root.pendingParams["top_p"].toString() : ""
                    // Accepts 0.9, .9, 0, 1, 1.0, etc. - no leading zero required
                    validator: RegularExpressionValidator {
                        regularExpression: /^(0(\.\d{0,4})?|\.\d{1,4}|1(\.0{0,4})?)$/
                    }
                    inputMethodHints: Qt.ImhFormattedNumbersOnly

                    // Red border is deferred until 600ms after the user stops typing
                    property bool fieldValid: true

                    Timer {
                        id: topPDebounce
                        interval: 600
                        repeat: false
                        onTriggered: {
                            fieldTopP.fieldValid = fieldTopP.acceptableInput
                            if (fieldTopP.acceptableInput) {
                                var m = Object.assign({}, root.pendingParams)
                                m["top_p"] = parseFloat(fieldTopP.text)
                                root.pendingParams = m
                            }
                        }
                    }

                    background: Rectangle {
                        color: root.theme.bgTertiary
                        border.color: !fieldTopP.fieldValid ? "#FF453A"
                                      : fieldTopP.activeFocus ? root.theme.accentPrimary
                                      : root.theme.borderInactive
                        border.width: root.theme.borderWidth
                        Behavior on border.color { ColorAnimation { duration: root.theme.animationFast } }
                    }
                    // Restart debounce on every keystroke - update only after pause
                    onTextEdited: topPDebounce.restart()
                    onEditingFinished: {
                        topPDebounce.stop()
                        fieldValid = acceptableInput
                        if (acceptableInput) {
                            var m = Object.assign({}, root.pendingParams)
                            m["top_p"] = parseFloat(text)
                            root.pendingParams = m
                        }
                    }
                }

                Button {
                    id: topPIncBtn
                    width: 26; height: 32
                    text: ">"
                    font.pixelSize: root.theme.fontMicro
                    font.family: "Poppins"
                    onClicked: {
                        var v = Math.round((parseFloat(fieldTopP.text || "0") + 0.1) * 10) / 10
                        v = Math.max(0.0, Math.min(1.0, v))
                        fieldTopP.text = v.toFixed(1)
                        fieldTopP.fieldValid = true
                        var m = Object.assign({}, root.pendingParams)
                        m["top_p"] = v
                        root.pendingParams = m
                    }
                    background: Rectangle {
                        color: topPIncBtn.pressed ? root.theme.accentPrimary
                               : topPIncBtn.hovered ? root.theme.bgSecondary : root.theme.bgTertiary
                        radius: root.theme.smallRadius
                        border.color: root.theme.borderInactive
                        border.width: root.theme.borderWidth
                    }
                    contentItem: Text {
                        text: topPIncBtn.text; font: topPIncBtn.font
                        color: root.theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: root.theme.borderInactive }

        // ── Reset / Apply row ────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Reset - restores server_config.json baseline + generation_config.json overlay
            // for the currently selected model
            Button {
                Layout.fillWidth: true
                height: 42
                text: "Reset"
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                font.bold: true
                onClicked: {
                    // Stop any running debounce timers before replacing pendingParams
                    tempDebounce.stop()
                    topPDebounce.stop()
                    // Restore the per-model default snapshot
                    root.pendingParams = Object.assign({}, root.submitPrompt.defaultLlmParams)
                    // Clear error state on debounced fields
                    fieldTemperature.fieldValid = true
                    fieldTopP.fieldValid = true
                }
                background: Rectangle {
                    color: parent.pressed ? "#C0392B"
                           : parent.hovered ? "#E74C3C" : root.theme.bgSecondary
                    radius: root.theme.smallRadius
                    border.color: "#E74C3C"
                    border.width: root.theme.borderWidth
                    Behavior on color { ColorAnimation { duration: root.theme.animationFast } }
                }
                contentItem: Text {
                    text: parent.text; font: parent.font
                    color: root.theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Apply - disabled until all 4 fields pass their validators
            Button {
                Layout.fillWidth: true
                height: 42
                text: "Apply"
                font.pixelSize: root.theme.fontMicro
                font.family: "Poppins"
                font.bold: true
                enabled: root.allValid
                opacity: enabled ? 1.0 : 0.35
                onClicked: {
                    // Write the complete map back in a single call - one signal emitted
                    root.submitPrompt.setLlmParams(root.pendingParams)
                    root.close()
                }
                background: Rectangle {
                    color: parent.enabled && parent.pressed ? "#27AE60"
                           : parent.enabled && parent.hovered ? "#2ECC71"
                           : parent.enabled ? root.theme.bgSecondary : root.theme.bgTertiary
                    radius: root.theme.smallRadius
                    border.color: parent.enabled ? root.theme.accentSecondary : root.theme.borderInactive
                    border.width: root.theme.borderWidth
                    Behavior on color      { ColorAnimation { duration: root.theme.animationFast } }
                    Behavior on border.color { ColorAnimation { duration: root.theme.animationFast } }
                }
                contentItem: Text {
                    text: parent.text; font: parent.font
                    color: parent.enabled ? root.theme.textPrimary : root.theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
