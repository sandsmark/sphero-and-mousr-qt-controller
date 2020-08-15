/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15

Rectangle {
    id: root

    property real hue: 1
    property real lightness: 1
    property real saturation: 1

    signal colorSelected(color selected)

    onColorChanged: {
        hue = color.hslHue
        lightness = color.hslLightness
        saturation = color.hslSaturation
    }

    function updateColor() {
        colorSelected(Qt.hsla(hue, saturation, lightness, 1))
    }

    Rectangle {
        id: hueSlider
        width: paletteMap.width
        height: 20
        x: (parent.width - width) / 2
        y: 10
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {position: 0.000; color: Qt.rgba(1, 0, 0, 1)}
            GradientStop {position: 0.167; color: Qt.rgba(1, 1, 0, 1)}
            GradientStop {position: 0.333; color: Qt.rgba(0, 1, 0, 1)}
            GradientStop {position: 0.500; color: Qt.rgba(0, 1, 1, 1)}
            GradientStop {position: 0.667; color: Qt.rgba(0, 0, 1, 1)}
            GradientStop {position: 0.833; color: Qt.rgba(1, 0, 1, 1)}
            GradientStop {position: 1.000; color: Qt.rgba(1, 0, 0, 1)}
        }

        Rectangle {
            x: hue * parent.width - width/2
            y: parent.height / 2 - height/2
            height: parent.height + border.width * 2
            width: 20

            border.width: 2
            color: "transparent"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.hue = mouse.x / width;
                root.updateColor();
            }
        }
    }

    Item {
        id: paletteMap
        x: (parent.width - width) / 2
        y: hueSlider.height + hueSlider.y + 10
        height: Math.min(parent.height, parent.width) - y
        width: height

        // note we smoothscale the shader from a smaller version to improve performance
        ShaderEffect {
            id: map
            width: 64
            height: 64
            scale: paletteMap.width / width;
            layer.enabled: true
            layer.smooth: true
            anchors.centerIn: parent
            property real hue: root.hue

            fragmentShader: OpenGLInfo.profile === OpenGLInfo.CoreProfile ? "#version 150
            in vec2 qt_TexCoord0;
            uniform float qt_Opacity;
            uniform float hue;
            out vec4 fragColor;

            float hueToIntensity(float v1, float v2, float h) {
                h = fract(h);
                if (h < 1.0 / 6.0)
                    return v1 + (v2 - v1) * 6.0 * h;
                else if (h < 1.0 / 2.0)
                    return v2;
                else if (h < 2.0 / 3.0)
                    return v1 + (v2 - v1) * 6.0 * (2.0 / 3.0 - h);

                return v1;
            }

            vec3 HSLtoRGB(vec3 color) {
                float h = color.x;
                float l = color.z;
                float s = color.y;

                if (s < 1.0 / 256.0)
                    return vec3(l, l, l);

                float v1;
                float v2;
                if (l < 0.5)
                    v2 = l * (1.0 + s);
                else
                    v2 = (l + s) - (s * l);

                v1 = 2.0 * l - v2;

                float d = 1.0 / 3.0;
                float r = hueToIntensity(v1, v2, h + d);
                float g = hueToIntensity(v1, v2, h);
                float b = hueToIntensity(v1, v2, h - d);
                return vec3(r, g, b);
            }

            void main() {
                vec4 c = vec4(1.0);
                c.rgb = HSLtoRGB(vec3(hue, 1.0 - qt_TexCoord0.t, qt_TexCoord0.s));
                fragColor = c * qt_Opacity;
            }
            " : "
            varying mediump vec2 qt_TexCoord0;
            uniform highp float qt_Opacity;
            uniform highp float hue;

            highp float hueToIntensity(highp float v1, highp float v2, highp float h) {
                h = fract(h);
                if (h < 1.0 / 6.0)
                    return v1 + (v2 - v1) * 6.0 * h;
                else if (h < 1.0 / 2.0)
                    return v2;
                else if (h < 2.0 / 3.0)
                    return v1 + (v2 - v1) * 6.0 * (2.0 / 3.0 - h);

                return v1;
            }

            highp vec3 HSLtoRGB(highp vec3 color) {
                highp float h = color.x;
                highp float l = color.z;
                highp float s = color.y;

                if (s < 1.0 / 256.0)
                    return vec3(l, l, l);

                highp float v1;
                highp float v2;
                if (l < 0.5)
                    v2 = l * (1.0 + s);
                else
                    v2 = (l + s) - (s * l);

                v1 = 2.0 * l - v2;

                highp float d = 1.0 / 3.0;
                highp float r = hueToIntensity(v1, v2, h + d);
                highp float g = hueToIntensity(v1, v2, h);
                highp float b = hueToIntensity(v1, v2, h - d);
                return vec3(r, g, b);
            }

            void main() {
                lowp vec4 c = vec4(1.0);
                c.rgb = HSLtoRGB(vec3(hue, 1.0 - qt_TexCoord0.t, qt_TexCoord0.s));
                gl_FragColor = c * qt_Opacity;
            }
            "
        }

        MouseArea {
            id: mapMouseArea
            anchors.fill: parent
            onClicked: {
                var xx = Math.max(0, Math.min(mouse.x, parent.width))
                var yy = Math.max(0, Math.min(mouse.y, parent.height))
                root.saturation = 1.0 - yy / parent.height
                root.lightness = xx / parent.width
                root.updateColor();
            }
        }

        Rectangle {
            height: parent.height
            width: 1
            x: crosshairs.x + crosshairs.radius
        }
        Rectangle {
            height: 1
            width: parent.width
            y: crosshairs.y + crosshairs.radius
        }
        Rectangle {
            id: crosshairs
            radius: 4
            width: radius * 2
            height: radius * 2
            border.width: 2
            y: (1.0 - root.saturation) * paletteMap.height - radius
            x: root.lightness * paletteMap.width - radius
            color: "transparent"
        }
    }
}
