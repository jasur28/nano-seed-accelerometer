import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 1000
    height: 800
    title: "NanoSeed Monitor"

    property var dataX: []
    property var dataY: []
    property var dataZ: []

    property int maxPoints: 500

    function updateBuffer(buffer, value) {
        buffer.push(value)
        if (buffer.length > maxPoints)
            buffer.shift()
    }

    Column {
        anchors.fill: parent
        spacing: 10
        padding: 10

        Text {
            text: "Samples/sec: " + serialMonitor.samplesPerSecond
            font.pixelSize: 20
        }

        // ===================== X =====================

        Text { text: "X Axis"; font.pixelSize: 18 }

        Canvas {
            id: graphX
            width: parent.width
            height: 200

            onPaint: {
                var ctx = getContext("2d")
                ctx.fillStyle = "black"
                ctx.fillRect(0, 0, width, height)

                ctx.strokeStyle = "red"
                ctx.lineWidth = 1
                ctx.beginPath()

                if (dataX.length > 1) {
                    for (var i = 0; i < dataX.length; i++) {
                        var x = i * width / maxPoints
                        var y = height/2 - (dataX[i] / 20000) * height/2

                        if (i === 0)
                            ctx.moveTo(x, y)
                        else
                            ctx.lineTo(x, y)
                    }
                }

                ctx.stroke()
            }
        }

        // ===================== Y =====================

        Text { text: "Y Axis"; font.pixelSize: 18 }

        Canvas {
            id: graphY
            width: parent.width
            height: 200

            onPaint: {
                var ctx = getContext("2d")
                ctx.fillStyle = "black"
                ctx.fillRect(0, 0, width, height)

                ctx.strokeStyle = "lime"
                ctx.lineWidth = 1
                ctx.beginPath()

                if (dataY.length > 1) {
                    for (var i = 0; i < dataY.length; i++) {
                        var x = i * width / maxPoints
                        var y = height/2 - (dataY[i] / 20000) * height/2

                        if (i === 0)
                            ctx.moveTo(x, y)
                        else
                            ctx.lineTo(x, y)
                    }
                }

                ctx.stroke()
            }
        }

        // ===================== Z =====================

        Text { text: "Z Axis"; font.pixelSize: 18 }

        Canvas {
            id: graphZ
            width: parent.width
            height: 200

            onPaint: {
                var ctx = getContext("2d")
                ctx.fillStyle = "black"
                ctx.fillRect(0, 0, width, height)

                ctx.strokeStyle = "cyan"
                ctx.lineWidth = 1
                ctx.beginPath()

                if (dataZ.length > 1) {
                    for (var i = 0; i < dataZ.length; i++) {
                        var x = i * width / maxPoints
                        var y = height/2 - (dataZ[i] / 20000) * height/2

                        if (i === 0)
                            ctx.moveTo(x, y)
                        else
                            ctx.lineTo(x, y)
                    }
                }

                ctx.stroke()
            }
        }

        Row {
            spacing: 10

            Button {
                text: "Open COM5"
                onClicked: serialMonitor.openPort("COM5")
            }

            Button {
                text: "Close"
                onClicked: serialMonitor.closePort()
            }
        }
    }

    Connections {
        target: serialMonitor
        function onNewSample(ax, ay, az) {

            updateBuffer(dataX, ax)
            updateBuffer(dataY, ay)
            updateBuffer(dataZ, az)

            graphX.requestPaint()
            graphY.requestPaint()
            graphZ.requestPaint()
        }
    }
}
