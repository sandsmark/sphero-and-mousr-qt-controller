#pragma once

#include <QBluetoothUuid>

namespace sphero {

namespace Services {
    static const QBluetoothUuid main(QStringLiteral("{22bb746f-2ba0-7554-2d6f-726568705327}"));
    static const QBluetoothUuid radio(QStringLiteral("{22bb746f-2bb0-7554-2d6f-726568705327}"));
    static const QBluetoothUuid batteryControl(QStringLiteral("{00001016-d102-11e1-9b23-00025b00a5a5}"));


    namespace R2D2 {
        static const QBluetoothUuid connect(QStringLiteral("{00020001-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid main(QStringLiteral("{00010001-574f-4f20-5370-6865726f2121}"));
    } // namespace R2D2

} // namespace Services

namespace Characteristics {
    static const QBluetoothUuid response(QStringLiteral("{22bb746f-2ba6-7554-2d6f-726568705327}"));
    static const QBluetoothUuid commands(QStringLiteral("{22bb746f-2ba1-7554-2d6f-726568705327}"));

    namespace Radio {
        static const QBluetoothUuid unknownRadio(QStringLiteral("{22bb746f-2bb1-7554-2d6f-726568705327}"));
        static const QBluetoothUuid transmitPower(QStringLiteral("{22bb746f-2bb2-7554-2d6f-726568705327}"));
        static const QBluetoothUuid rssi(QStringLiteral("{22bb746f-2bb6-7554-2d6f-726568705327}"));
        static const QBluetoothUuid deepSleep(QStringLiteral("{22bb746f-2bb7-7554-2d6f-726568705327}"));
        static const QBluetoothUuid unknownRadio2(QStringLiteral("{22bb746f-2bb8-7554-2d6f-726568705327}"));
        static const QBluetoothUuid unknownRadio3(QStringLiteral("{22bb746f-2bb9-7554-2d6f-726568705327}"));
        static const QBluetoothUuid unknownRadio4(QStringLiteral("{22bb746f-2bba-7554-2d6f-726568705327}"));
        static const QBluetoothUuid antiDos(QStringLiteral("{22bb746f-2bbd-7554-2d6f-726568705327}"));
        static const QBluetoothUuid antiDosTimeout(QStringLiteral("{22bb746f-2bbe-7554-2d6f-726568705327}"));
        static const QBluetoothUuid wake(QStringLiteral("{22bb746f-2bbf-7554-2d6f-726568705327}"));
        static const QBluetoothUuid unknownRadio5(QStringLiteral("{22bb746f-3bba-7554-2d6f-726568705327}"));
    } // namespace Radio

    namespace R2D2 {
        static const QBluetoothUuid connect(QStringLiteral("{00020005-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid handle(QStringLiteral("{00020002-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid main(QStringLiteral("{00010002-574f-4f20-5370-6865726f2121}"));

    } // namespace R2D2

    namespace Unknown {
        static const QBluetoothUuid unknown1(QStringLiteral("{00010003-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid unknown2(QStringLiteral("{00020003-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid dfu2(QStringLiteral("{00020004-574f-4f20-5370-6865726f2121}"));
    } // namespace Unknown

} // namespace Characteristics

} // namespace sphero
