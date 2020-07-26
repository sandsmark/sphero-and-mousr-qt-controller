#pragma once

#include <QBluetoothUuid>

namespace sphero {

namespace Services {
    namespace V1 {
        static const QBluetoothUuid main(QStringLiteral("{22bb746f-2ba0-7554-2d6f-726568705327}"));
        static const QBluetoothUuid radio(QStringLiteral("{22bb746f-2bb0-7554-2d6f-726568705327}"));
        static const QBluetoothUuid battery(QStringLiteral("{00001016-d102-11e1-9b23-00025b00a5a5}"));
    } // namespace V1

    namespace V2 {
        static const QBluetoothUuid main(QStringLiteral("{00010001-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid radio(QStringLiteral("{00020001-574f-4f20-5370-6865726f2121}"));
        static const QBluetoothUuid gatt = QBluetoothUuid::GenericAttribute; // 00001801-0000-1000-8000-00805f9b34fb

        // Not available while on the charger (I think)
        static const QBluetoothUuid battery = QBluetoothUuid::BatteryService; // 0000180f-0000-1000-8000-00805f9b34fb
    } // namespace V2
} // namespace Services

namespace Characteristics {
    namespace Radio {
        namespace V1 {
            static const QBluetoothUuid antiDos(QStringLiteral("{22bb746f-2bbd-7554-2d6f-726568705327}"));
            static const QBluetoothUuid antiDosTimeout(QStringLiteral("{22bb746f-2bbe-7554-2d6f-726568705327}"));

            static const QBluetoothUuid transmitPower(QStringLiteral("{22bb746f-2bb2-7554-2d6f-726568705327}"));
            static const QBluetoothUuid rssi(QStringLiteral("{22bb746f-2bb6-7554-2d6f-726568705327}"));

            static const QBluetoothUuid deepSleep(QStringLiteral("{22bb746f-2bb7-7554-2d6f-726568705327}"));
            static const QBluetoothUuid wake(QStringLiteral("{22bb746f-2bbf-7554-2d6f-726568705327}"));


            static const QBluetoothUuid unknownRadio1(QStringLiteral("{22bb746f-2bb1-7554-2d6f-726568705327}"));
            static const QBluetoothUuid unknownRadio2(QStringLiteral("{22bb746f-2bb8-7554-2d6f-726568705327}"));
            static const QBluetoothUuid unknownRadio3(QStringLiteral("{22bb746f-2bb9-7554-2d6f-726568705327}"));
            static const QBluetoothUuid unknownRadio4(QStringLiteral("{22bb746f-2bba-7554-2d6f-726568705327}"));
            static const QBluetoothUuid unknownRadio5(QStringLiteral("{22bb746f-3bba-7554-2d6f-726568705327}"));
        }

        namespace V2 {
            static const QBluetoothUuid write(QStringLiteral("{00020002-574f-4f20-5370-6865726f2121}"));
            static const QBluetoothUuid read(QStringLiteral("{00020004-574f-4f20-5370-6865726f2121}"));
            static const QBluetoothUuid antiDos(QStringLiteral("{00020005-574f-4f20-5370-6865726f2121}"));

            static const QBluetoothUuid unknown2(QStringLiteral("{00020003-574f-4f20-5370-6865726f2121}"));
        } // namespace V2
    } // namespace Radio

    namespace Main {
        namespace V1 {
            static const QBluetoothUuid commands(QStringLiteral("{22bb746f-2ba1-7554-2d6f-726568705327}"));
            static const QBluetoothUuid response(QStringLiteral("{22bb746f-2ba6-7554-2d6f-726568705327}"));

            // Just for completeness, these are the ones available
            static const QBluetoothUuid deviceInformation = QBluetoothUuid::DeviceInformation; // 0000180a-0000-1000-8000-00805f9b34fb
            static const QBluetoothUuid modelNumber = QBluetoothUuid::ModelNumberString; // 00002a24-0000-1000-8000-00805f9b34fb
            static const QBluetoothUuid serialNumber = QBluetoothUuid::SerialNumberString; // 00002a25-0000-1000-8000-00805f9b34fb
            static const QBluetoothUuid firmwareRevision = QBluetoothUuid::FirmwareRevisionString; // 00002a26-0000-1000-8000-00805f9b34fb
        } // namespace V1

        namespace V2 {
            static const QBluetoothUuid commands(QStringLiteral("{00010002-574f-4f20-5370-6865726f2121}"));
            static const QBluetoothUuid unknown1(QStringLiteral("{00010003-574f-4f20-5370-6865726f2121}")); // rfcomm protocol shit?
        } // namespace V2
    } // namespace Main

    namespace GATT {
        namespace V2 {
        // 00002a05-0000-1000-8000-00805f9b34fb
            static const QBluetoothUuid serviceChanged = QBluetoothUuid::ServiceChanged;
        } // namespace V2
    }
} // namespace Characteristics

} // namespace sphero
