#include <QMetaEnum>
#include <QDebug>

#include "SpheroHandler.h"
#include "utils.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>

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

namespace Descriptors {
    static const QBluetoothUuid read(QStringLiteral("{00002902-0000-1000-8000-00805f9b34fb}"));
} // namespace Descriptors

SpheroHandler::SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_name(deviceInfo.name())
{
    qDebug() << sizeof(SensorStreamPacket);
    m_deviceController = QLowEnergyController::createCentral(deviceInfo, this);

    connect(m_deviceController, &QLowEnergyController::connected, m_deviceController, &QLowEnergyController::discoverServices);
    connect(m_deviceController, &QLowEnergyController::discoveryFinished, this, &SpheroHandler::onServiceDiscoveryFinished);

    connect(m_deviceController, &QLowEnergyController::connectionUpdated, this, [](const QLowEnergyConnectionParameters &parms) {
            qDebug() << " - controller connection updated, latency" << parms.latency() << "maxinterval:" << parms.maximumInterval() << "mininterval:" << parms.minimumInterval() << "supervision timeout" << parms.supervisionTimeout();
            });
    connect(m_deviceController, &QLowEnergyController::connected, this, []() {
            qDebug() << " - controller connected";
            });
    connect(m_deviceController, &QLowEnergyController::disconnected, this, []() {
            qDebug() << " ! controller disconnected";
            });
    connect(m_deviceController, &QLowEnergyController::discoveryFinished, this, []() {
            qDebug() << " - controller discovery finished";
            });
    connect(m_deviceController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &SpheroHandler::onControllerError);

    connect(m_deviceController, &QLowEnergyController::stateChanged, this, &SpheroHandler::onControllerStateChanged);

    m_deviceController->connectToDevice();

    if (m_deviceController->error() != QLowEnergyController::NoError) {
        qWarning() << " ! controller error when starting:" << m_deviceController->error() << m_deviceController->errorString();
    }
    qDebug() << " - Created handler";
}

SpheroHandler::~SpheroHandler()
{
    qDebug() << " - sphero handler dead";
    if (m_deviceController) {
        m_deviceController->disconnectFromDevice();
    } else {
        qWarning() << "no controller";
    }
}

bool SpheroHandler::isConnected()
{
    return m_deviceController && m_deviceController->state() != QLowEnergyController::UnconnectedState &&
            m_mainService && m_mainService->state() == QLowEnergyService::ServiceDiscovered &&
            m_radioService && m_radioService->state() == QLowEnergyService::ServiceDiscovered &&
            m_commandsCharacteristic.isValid();
}

QString SpheroHandler::statusString()
{
    const QString name = m_name.isEmpty() ? "device" : m_name;
    if (isConnected()) {
        return tr("Connected to %1").arg(name);
    } else if (m_deviceController && m_deviceController->state() == QLowEnergyController::UnconnectedState) {
//        QTimer::singleShot(1000, this, &QObject::deleteLater);
        return tr("Failed to connect to %1").arg(name);
    } else {
        return tr("Found %1, trying to establish connection...").arg(name);
    }
}

void SpheroHandler::onServiceDiscoveryFinished()
{
    qDebug() << " - Discovered services";


    m_radioService = m_deviceController->createServiceObject(Services::radio, this);
    if (!m_radioService) {
        qWarning() << " ! Failed to get ble service";
        return;
    }
    qDebug() << " - Got ble service";

    connect(m_radioService, &QLowEnergyService::characteristicChanged, this, &SpheroHandler::onCharacteristicChanged);
    connect(m_radioService, &QLowEnergyService::stateChanged, this, &SpheroHandler::onRadioServiceChanged);
    connect(m_radioService, &QLowEnergyService::characteristicWritten, this, &SpheroHandler::onRadioCharacteristicWritten);
    connect(m_radioService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &SpheroHandler::onServiceError);

    if (m_mainService) {
        qWarning() << " ! main service already exists!";
        return;
    }

    m_mainService = m_deviceController->createServiceObject(Services::main, this);
    if (!m_mainService) {
        qWarning() << " ! no main service";
        return;
    }
    connect(m_mainService, &QLowEnergyService::characteristicChanged, this, &SpheroHandler::onCharacteristicChanged);
    connect(m_mainService, &QLowEnergyService::characteristicRead, this, &SpheroHandler::onCharacteristicRead);
    connect(m_mainService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &SpheroHandler::onServiceError);
    connect(m_mainService, &QLowEnergyService::characteristicWritten, this, [](const QLowEnergyCharacteristic &info, const QByteArray &value) {
        qDebug() << " - main written" << info.uuid() << value;
    });
    connect(m_mainService, &QLowEnergyService::stateChanged, this, &SpheroHandler::onMainServiceChanged);

    m_radioService->discoverDetails();
}

void SpheroHandler::onMainServiceChanged(QLowEnergyService::ServiceState newState)
{
    qDebug() << " ! mainservice change" << newState;

    if (newState == QLowEnergyService::InvalidService) {
        qWarning() << "Got invalid service";
        emit disconnected();
        return;
    }

    if (newState == QLowEnergyService::DiscoveringServices) {
        qDebug() << " ! Discovering services";
        return;
    }


    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << " ! unhandled service state changed:" << newState;
        return;
    }

    m_commandsCharacteristic = m_mainService->characteristic(Characteristics::commands);
    if (!m_commandsCharacteristic.isValid()) {
        qWarning() << "Commands characteristic invalid";
        return;
    }

    for (const QLowEnergyCharacteristic &c : m_mainService->characteristics()) {
        qDebug() << " - characteristic available:" << c.name() << c.uuid() << c.properties();
        if (c.properties() & QLowEnergyCharacteristic::Read) {
            qDebug() << "  > Readable";
        }
        if (c.properties() & QLowEnergyCharacteristic::Write) {
            qDebug() << "  > Writable";
        }
        if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse) {
            qDebug() << "  > Async write";
        }
        if (c.properties() & QLowEnergyCharacteristic::Notify) {
            qDebug() << "  > Notify";
            for (const QLowEnergyDescriptor &d : c.descriptors()) {
                m_mainService->writeDescriptor(d, QByteArray::fromHex("0100"));
            }
        }
    }

    QLowEnergyCharacteristic responseCharacteristic = m_mainService->characteristic(Characteristics::response);
    if (!responseCharacteristic.isValid()) {
        qWarning() << "response characteristic invalid";
        return;
    }
    QLowEnergyDescriptor readDescriptor = responseCharacteristic.descriptor(Descriptors::read);
    if (!readDescriptor.isValid()) {
        qWarning() << "Read descriptor on response descriptor is invalid";
        return;
    }
    // Enable notifications
    m_mainService->writeDescriptor(readDescriptor, QByteArray::fromHex("0100"));
    qDebug() << " - Requested notifications on values changes";

    qDebug() << " - Successfully connected";


    sendCommand(PacketHeader::HardwareControl, PacketHeader::GetLocatorData, "", PacketHeader::Synchronous, PacketHeader::ResetTimeout);

    emit connectedChanged();
}

void SpheroHandler::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::UnconnectedState) {
        qWarning() << " ! Disconnected";
        emit disconnected();
    }

    qDebug() << " - controller state changed" << state;
    emit connectedChanged();
}

void SpheroHandler::onControllerError(QLowEnergyController::Error newError)
{
    qWarning() << " - controller error:" << newError << m_deviceController->errorString();
    if (newError == QLowEnergyController::UnknownError) {
        qWarning() << "Probably 'Operation already in progress' because qtbluetooth doesn't understand why it can't get answers over dbus when a connection attempt hangs";
    }
    connect(m_deviceController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &SpheroHandler::onControllerError);
}


void SpheroHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    qWarning() << "Service error:" << error;
    if (error == QLowEnergyService::NoError) {
        return;
    }

    emit disconnected();
}

void SpheroHandler::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (characteristic.uuid() != Characteristics::response) {
        qWarning() << "data from unexpected characteristic" << characteristic.uuid() << data;
        return;
    }
    qDebug() << " - data from read characteristic" << data.toHex();
    qDebug() << " - data type:" << data[0];
}

void SpheroHandler::onDescriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &data)
{
    if (descriptor.uuid() != Descriptors::read) {
        qWarning() << "data from unexpected descriptor" << descriptor.uuid() << data;
        return;
    }
    if (data.isEmpty()) {
        qWarning() << "got empty data";
        return;
    }
    qDebug() << " - data from read descriptor" << data;
    qDebug() << " - data type:" << int(data[0]);

//    emit dataRead(data);
}

void SpheroHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (data.isEmpty()) {
        qWarning() << " ! " << characteristic.uuid() << "got empty data";
        return;
    }

    if (characteristic.uuid() == Characteristics::Radio::rssi) {
        m_rssi = data[0];
        qDebug() << " + RSsi" << m_rssi;
        return;
    }
    if (characteristic.uuid() == QBluetoothUuid::ServiceChanged) {
        qDebug() << " ? GATT service changed" << data.toHex();
        return;
    }

    if (characteristic.uuid() != Characteristics::response) {
        qWarning() << " ? Changed from unexpected characteristic" << characteristic.name() << characteristic.uuid() << data;
        return;
    }

    static_assert(sizeof(PacketHeader) == 6);
    static_assert(sizeof(LocatorPacket) == 7);
    static_assert(sizeof(ResponsePacketHeader) == 5);
    static_assert(sizeof(SensorStreamPacket) == 87); // should be 90, I think?

    qDebug() << " ------------ Characteristic changed" << data.toHex();

    if (data.isEmpty()) {
        qWarning() << " ! No data received";
        return;
    }

    // New messages start with 0xFF, so reset buffer in that case
    if (data.startsWith(0xFF)) {
        m_receiveBuffer = data;
    } else if (data.startsWith("u>\xff\xff")) {
        // We don't always get this
         // I _think_ the 'u>' is a separate packet, so just strip it
        qDebug() << " - Got unknown something that looks like a prompt (u>), is an ack of some sorts?";
        m_receiveBuffer = data.mid(2);
    } else if (!m_receiveBuffer.isEmpty()) {
        m_receiveBuffer.append(data);
    } else {
        qWarning() << " ! Got data but without correct start";
        qDebug() << data;
        const int startOfData = data.indexOf(0xFF);
        if (startOfData < 0) {
            qWarning() << " ! Contains nothing useful";
            m_receiveBuffer.clear();
            return;
        }
        m_receiveBuffer = data.mid(startOfData);
    }

    if (m_receiveBuffer.size() > 10000) {
        qWarning() << " ! Receive buffer too large, nuking" << m_receiveBuffer.size();
        m_receiveBuffer.clear();
        return;
    }

    if (m_receiveBuffer.size() < int(sizeof(PacketHeader))) {
        qDebug() << " - Not a full header" << m_receiveBuffer.size();
        return;
    }

    ResponsePacketHeader header;
    if (header.response != ResponsePacketHeader::Ok) {

    }
    qFromBigEndian<uint8_t>(m_receiveBuffer.data(), sizeof(PacketHeader), &header);
    qDebug() << " - magic" << header.magic;
    if (header.magic != 0xFF) {
        qWarning() << " ! Invalid magic";
        return;
    }
    qDebug() << " - type" << header.type;
    switch(header.type) {
    case ResponsePacketHeader::Ack:
        qDebug() << " - ack response" << ResponsePacketHeader::AckType(header.response);
        break;
    case ResponsePacketHeader::Data:
        qDebug() << " - data response" << ResponsePacketHeader::DataType(header.response);
        break;
    default:
        qWarning() << " ! unhandled type" << header.type;
        m_receiveBuffer.clear();
    }

    qDebug() << " - sequence num" << header.sequenceNumber;
    qDebug() << " - data length" << header.dataLength;

    if (m_receiveBuffer.size() != sizeof(PacketHeader) + header.dataLength - 1) {
        qWarning() << " ! Packet size wrong" << m_receiveBuffer.size();
        qDebug() << "  > Expected" << sizeof(PacketHeader) << "+" << header.dataLength;
        return;
    }

    QByteArray contents;

    uint8_t checksum = 0;
    for (int i=2; i<m_receiveBuffer.size() - 1; i++) {
        checksum += uint8_t(m_receiveBuffer[i]);
        contents.append(m_receiveBuffer[i]);
    }
    checksum ^= 0xFF;
    if (!m_receiveBuffer.endsWith(checksum)) {
        qWarning() << " ! Invalid checksum" << checksum;
        qDebug() << "  > Expected" << uint8_t(m_receiveBuffer.back());
        return;
    }
    qDebug() << " - received contents" << contents.size() << contents.toHex();
    contents = contents.left(header.dataLength);

    switch(header.type) {
    case ResponsePacketHeader::Ack: {
        qDebug() << " - ack response" << ResponsePacketHeader::AckType(header.response);
        qDebug() << contents.length() << header.dataLength << m_receiveBuffer.length() << sizeof(LocatorPacket) << sizeof(ResponsePacketHeader);

        // TODO separate function
        if (size_t(contents.size()) < sizeof(AckResponsePacket)) {
            qWarning() << "Impossibly short data response packet, size" << contents.size() << "we require at least" << sizeof(AckResponsePacket);
            qDebug() << contents;
            return;
        }
        const AckResponsePacket *response = reinterpret_cast<const AckResponsePacket*>(contents.data());
        qDebug() << "Response type" << response->type << "unknown" << response->unk;
        switch(response->type) {
        case LocatorResponse: {
            if (size_t(contents.length()) < sizeof(LocatorPacket)) {
                qWarning() << "Locator response too small" << contents.length();
                return;
            }
            contents = contents.mid(4);
            qDebug() << "Locator size" << sizeof(LocatorPacket) << "Locatorconf" << contents.size();
            const LocatorPacket *location = reinterpret_cast<const LocatorPacket*>(contents.data());
            qDebug() << "tilt" << location->tilt << "position" << location->position.x << location->position.y << (location->flags ? "calibrated" : "not calibrated");

            DataStreamingCommandPacket def;
            def.packetCount = 1;
            sendCommand(PacketHeader::HardwareControl, PacketHeader::SetDataStreaming, QByteArray((char*)&def, sizeof(def)), PacketHeader::Synchronous, PacketHeader::ResetTimeout);
            break;
        }
        default:
            qWarning() << "Unhandled ack response" << ResponseType(response->type);
            break;
        }

        break;
    }
    case ResponsePacketHeader::Data:
        qDebug() << " - data response" << ResponsePacketHeader::DataType(header.response);
        break;
    default:
        qWarning() << " ! unhandled type" << header.type;
        m_receiveBuffer.clear();
    }

    if (header.type != ResponsePacketHeader::Ack) {
        qWarning() << " ! not a simple response";
        return;
    }

//    // async
//    switch(header.response) {
//    case StreamingResponse:
//        if (m_receiveBuffer.size() != 88) {
//            qWarning() << "Invalid streaming data size" << m_receiveBuffer.size();
//            break;
//        }
//        qDebug() << "Got streaming command";
//        break;
//    case PowerStateResponse: {
//        if (m_receiveBuffer.size() != sizeof(PowerStatePacket)) {
//            qWarning() << "Invalid size of powerstate packet" << m_receiveBuffer.size();
//            qDebug() << "Expected" << sizeof(PowerStatePacket);
//            break;
//        }

//        qDebug() << "Got powerstate response";
//        PowerStatePacket powerState;
//        qFromBigEndian<uint8_t>(m_receiveBuffer.data(), sizeof(PowerStatePacket), &powerState);
//        qDebug() << "record version" << powerState.recordVersion;
//        qDebug() << "power state" << powerState.powerState;
//        qDebug() << "battery voltage" << powerState.batteryVoltage;
//        qDebug() << "number of charges" << powerState.numberOfCharges;
//        qDebug() << "seconds since last charge" << powerState.secondsSinceCharge;

//        break;
//    }
//    case LocatorResponse: {
//        if (m_receiveBuffer.size() != 16) {
//            qWarning() << "Invalid size of locator response" << m_receiveBuffer.size();
//            break;
//        }
//        qDebug() << "Got locator response";
//        LocatorPacket locator;
//        qFromBigEndian<uint8_t>(m_receiveBuffer.data(), sizeof(PowerStatePacket), &locator);
//        qDebug() << "flags" << locator.flags;
//        qDebug() << "X:" << locator.position.x;
//        qDebug() << "Y:" << locator.position.y;
//        qDebug() << "tilt:" << locator.tilt;

//        break;
//    }
//    default:
//        qDebug() << "Unknown command" << header.commandID;
//        break;
//    }

//    switch(m_receiveBuffer[2]) {

    //    }
}

void SpheroHandler::onRadioServiceChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::DiscoveringServices) {
        qDebug() << "unhandled radio service state changed:" << newState;
        return;
    }
    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << "unhandled radio service state changed:" << newState;
        return;
    }
    if (!sendRadioControlCommand(Characteristics::Radio::antiDos, "011i3") ||
        !sendRadioControlCommand(Characteristics::Radio::transmitPower, QByteArray(1, 7)) ||
        !sendRadioControlCommand(Characteristics::Radio::wake, QByteArray(1, 1))) {
        qWarning() << " ! Init sequence failed";
        emit disconnected();
        return;
    }
    qDebug() << " - Init sequence done";
    m_mainService->discoverDetails();
}

void SpheroHandler::onRadioCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    qDebug() << " - " << characteristic.uuid() << "radio written" << newValue;
}

bool SpheroHandler::sendRadioControlCommand(const QBluetoothUuid &characteristicUuid, const QByteArray &data)
{
    if (!m_radioService || m_radioService->state() != QLowEnergyService::ServiceDiscovered) {
        qWarning() << "Radio service not connected";
        return false;
    }
    QLowEnergyCharacteristic characteristic = m_radioService->characteristic(characteristicUuid);
    if (!characteristic.isValid()) {
        qWarning() << "Radio characteristic" << characteristicUuid << "not available";
        return false;
    }
    m_radioService->writeCharacteristic(characteristic, data);
    return true;
}

void SpheroHandler::sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data, const PacketHeader::SynchronousType synchronous, const PacketHeader::TimeoutHandling keepTimeout)
{
    static int currentSequenceNumber = 0;
    PacketHeader header;
    header.dataLength = data.size() + 1; // + 1 for checksum
    header.flags |= synchronous;
    header.flags |= keepTimeout;
//    header.synchronous = synchronous;
//    header.resetTimeout = keepTimeout;
    header.sequenceNumber = currentSequenceNumber++;
    header.commandID = commandID;
    header.deviceID = deviceId;
    qDebug() << " - Device id:" << header.deviceID;
    qDebug() << " - command id:" << header.commandID;
    qDebug() << " - seq number:" << header.sequenceNumber;

    if (header.dataLength != data.size() + 1) {
        qWarning() << "Invalid data length in header";
    }
    QByteArray headerBuffer(sizeof(PacketHeader), 0); // + 1 for checksum
//    q<uint8_t>(&header, sizeof(PacketHeader), headerBuffer.data());
    qToBigEndian<uint8_t>(&header, sizeof(PacketHeader), headerBuffer.data());
    qDebug() << " - " << uchar(headerBuffer[0]);
    qDebug() << " - " << uchar(headerBuffer[1]);

    QByteArray toSend;
    toSend.append(headerBuffer);
    toSend.append(data);

//    for (const char &b : data) {
//        checksum += uint8_t(b) & 0xFF;
//    }
    uint8_t checksum = 0;
    for (int i=2; i<toSend.size(); i++) {
        checksum += toSend[i];
    }
    toSend.append(checksum xor 0xFF);

    qDebug() << " - Writing command" << toSend.toHex();
    m_mainService->writeCharacteristic(m_commandsCharacteristic, toSend);
}

} // namespace sphero
