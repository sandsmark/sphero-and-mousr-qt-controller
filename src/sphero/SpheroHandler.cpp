#include <QMetaEnum>
#include <QDebug>

#include "SpheroHandler.h"
#include "utils.h"
#include "Uuids.h"

#include "ResponsePackets.h"
#include "CommandPackets.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>

namespace sphero {

SpheroHandler::SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_name(deviceInfo.name())
{
    if (m_name.startsWith("BB-8")) {
        m_robotType = SpheroType::Bb8;
    }

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
        disconnectFromRobot();
    } else {
        qWarning() << "no controller";
    }
}

void SpheroHandler::disconnectFromRobot()
{
    if (!isConnected()) {
        qDebug() << "Can't disconnect when not connected";
        return;
    }

    setAutoStabilize(false);
    goToSleep();
    m_deviceController->disconnectFromDevice();
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

void SpheroHandler::setColor(const int r, const int g, const int b)
{
    sendCommand(SetColorsCommandPacket(r, g, b, SetColorsCommandPacket::Temporary));

    const QColor color = QColor::fromRgb(r, g, b);
    if (color != m_color) {
        m_color = color;
        emit colorChanged();
    }
}

void SpheroHandler::setSpeedAndAngle(int angle, int speed)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;
    speed = qBound(0, speed, 255);

    sendCommand(RollCommandPacket({uint8_t(speed), uint16_t(angle), RollCommandPacket::Roll}));

    if (m_speed != speed) {
        m_speed = speed;
        emit speedChanged();
    }
    if (m_angle != angle) {
        m_angle = angle;
        emit angleChanged();
    }
}

void SpheroHandler::setAngle(int angle)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;
    sendCommand(SetHeadingPacket({uint16_t(angle % 360)}));
    if (m_angle != angle) {
        m_angle = angle;
        emit angleChanged();
    }
}

void SpheroHandler::setAutoStabilize(const bool enabled)
{
    sendCommand(CommandPacketHeader::HardwareControl, CommandPacketHeader::SetStabilization, QByteArray(1, enabled ? 1 : 0));
    if (enabled != m_autoStabilize) {
        m_autoStabilize = enabled;
        emit autoStabilizeChanged();
    }
}

void SpheroHandler::setDetectCollisions(const bool enabled)
{
    sendCommand(EnableCollisionDetectionPacket(enabled));
}

void SpheroHandler::goToSleep()
{
    sendCommand(GoToSleepPacket());
}

void SpheroHandler::enablePowerNotifications()
{
    sendCommand(CommandPacketHeader::Internal, CommandPacketHeader::SetPwrNotify, QByteArray(1, 1));
}

void SpheroHandler::setEnableAsciiShell(const bool enabled)
{
    sendCommand(SetUserHackModePacket({enabled}));
}

void SpheroHandler::faceLeft()
{
    sendCommand(RollCommandPacket({uint8_t(0), uint16_t(270), RollCommandPacket::Brake}));
}

void SpheroHandler::faceRight()
{
    sendCommand(RollCommandPacket({uint8_t(0), uint16_t(90), RollCommandPacket::Brake}));
}

void SpheroHandler::faceForward()
{
    sendCommand(RollCommandPacket({uint8_t(0), uint16_t(0), RollCommandPacket::Brake}));
}

void SpheroHandler::boost(int angle, int duration)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;

    sendCommand(BoostCommandPacket({uint8_t(qBound(0, duration, 255)), uint16_t(angle)}));

    if (m_angle != angle) {
        m_angle = angle;
        emit angleChanged();
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
    connect(m_radioService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &SpheroHandler::onServiceError);

    connect(m_radioService, &QLowEnergyService::characteristicWritten, this, [](const QLowEnergyCharacteristic &c, const QByteArray &v) {
        qDebug() << " - " << c.uuid() << "radio written" << v;
    });


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
    connect(m_mainService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &SpheroHandler::onServiceError);
    connect(m_mainService, &QLowEnergyService::characteristicWritten, this, [](const QLowEnergyCharacteristic &info, const QByteArray &value) {
        qDebug() << " - main written" << info.uuid() << value.toHex(':');
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
        emit statusMessageChanged(tr("Sphero BLE service failed"));
        return;
    }

    if (newState == QLowEnergyService::DiscoveringServices) {
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

    const QLowEnergyCharacteristic responseCharacteristic = m_mainService->characteristic(Characteristics::response);
    if (!responseCharacteristic.isValid()) {
        qWarning() << "response characteristic invalid";
        return;
    }

    // Enable notifications
    m_mainService->writeDescriptor(responseCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

    qDebug() << " - Successfully connected";

    sendCommand(CommandPacketHeader::Internal, CommandPacketHeader::GetPwrState);
    sendCommand(CommandPacketHeader::HardwareControl, CommandPacketHeader::GetRGBLed);

    emit connectedChanged();
    emit statusMessageChanged(statusString());
}

void SpheroHandler::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::UnconnectedState) {
        qWarning() << " ! Disconnected";
        emit disconnected();
        emit statusMessageChanged(tr("Sphero lost connection"));
        return;
    }

    qDebug() << " - controller state changed" << state;
    emit connectedChanged();
    emit statusMessageChanged(statusString());
}

void SpheroHandler::onControllerError(QLowEnergyController::Error newError)
{
    qWarning() << " - controller error:" << newError << m_deviceController->errorString();
    if (newError == QLowEnergyController::UnknownError) {
        qWarning() << "Probably 'Operation already in progress' because qtbluetooth doesn't understand why it can't get answers over dbus when a connection attempt hangs";
        emit statusMessageChanged(tr("Sphero connection attempt hung, out of range?"));
    }
    emit disconnected();
}


void SpheroHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    qWarning() << "Service error:" << error;
    if (error == QLowEnergyService::NoError) {
        return;
    }

    emit statusMessageChanged(tr("Sphero service connection failed: %1").arg(error));
    emit disconnected();
}

void SpheroHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (data.isEmpty()) {
        qWarning() << " ! " << characteristic.uuid() << "got empty data";
        return;
    }

    if (characteristic.uuid() == Characteristics::Radio::rssi) {
        m_rssi = data[0];
        emit rssiChanged();
        return;
    }
    if (characteristic.uuid() == QBluetoothUuid::ServiceChanged) {
        qDebug() << " ? GATT service changed" << data.toHex(':');
        return;
    }

    if (characteristic.uuid() != Characteristics::response) {
        qWarning() << " ? Changed from unexpected characteristic" << characteristic.name() << characteristic.uuid() << data;
        return;
    }

    qDebug() << " ------------ Characteristic changed" << data.toHex(':');

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

    if (m_receiveBuffer.size() < int(sizeof(CommandPacketHeader))) {
        qDebug() << " - Not a full header" << m_receiveBuffer.size();
        return;
    }

    ResponsePacketHeader header;
    qFromBigEndian<uint8_t>(m_receiveBuffer.data(), sizeof(ResponsePacketHeader), &header);
    qDebug() << " - magic" << header.magic;
    if (header.magic != 0xFF) {
        qWarning() << " ! Invalid magic";
        return;
    }
    qDebug() << " - type" << header.type;
    switch(header.type) {
    case ResponsePacketHeader::Response:
        qDebug() << " - ack response" << ResponsePacketHeader::ResponseType(header.response);
        break;
    case ResponsePacketHeader::Notification:
        qDebug() << " - data response" << ResponsePacketHeader::NotificationType(header.response);
        break;
    default:
        qWarning() << " ! unhandled type" << header.type;
        m_receiveBuffer.clear();
    }

    qDebug() << " - sequence num" << header.sequenceNumber;
    qDebug() << " - data length" << header.dataLength;

    if (m_receiveBuffer.size() != sizeof(CommandPacketHeader) + header.dataLength - 1) {
        qWarning() << " ! Packet size wrong" << m_receiveBuffer.size();
        qDebug() << "  > Expected" << sizeof(CommandPacketHeader) << "+" << header.dataLength;
        return;
    }

    uint8_t checksum = 0;
    for (int i=2; i<m_receiveBuffer.size() - 1; i++) {
        checksum += uint8_t(m_receiveBuffer[i]);
    }
    checksum ^= 0xFF;
    if (!m_receiveBuffer.endsWith(checksum)) {
        qWarning() << " ! Invalid checksum" << checksum;
        qDebug() << "  > Expected" << uint8_t(m_receiveBuffer.back());
        return;
    }

    QByteArray contents = data.mid(sizeof(ResponsePacketHeader), header.dataLength);
    if (contents.isEmpty()) {
        qWarning() << "No contents";
    }
    qDebug() << " - received contents" << contents.size() << contents.toHex(':');

    qDebug() << " - response type:" << header.type;
    m_receiveBuffer.clear(); // TODO leave some

    switch(header.type) {
    case ResponsePacketHeader::Response: {
        qDebug() << " - ack response" << ResponsePacketHeader::ResponseType(header.response);
        qDebug() << "Content length" << contents.length() << "data length" << header.dataLength << "buffer length" << m_receiveBuffer.length() << "locator packet size" << sizeof(LocatorPacket) << "response packet size" << sizeof(ResponsePacketHeader);

        // TODO separate function
        AckResponsePacket::ResponseType responseType = AckResponsePacket::ResponseType (uint8_t(contents[0]));
        switch(responseType) {
        case AckResponsePacket::PowerNotification: {
            PowerStatePacket response;
            if (size_t(contents.size()) < sizeof(response)) {
                qWarning() << "Size of content too small";
                return;
            }
            qFromBigEndian<uint8_t>(contents.data(), sizeof(PowerStatePacket), &response);
            qDebug() << "  ========== power response ====== ";
            qDebug() << "  + version" << response.recordVersion;
            qDebug() << "  + state" << response.powerState;
            qDebug() << "  + battery voltage" << response.batteryVoltage;
            qDebug() << "  + number of charges" << response.numberOfCharges;
            qDebug() << "  + seconds since charge" << response.secondsSinceCharge;
            break;
        }
        case AckResponsePacket::Level1Diagnostic: {
            qDebug() << " ? Got level 1 diagnostic";
            break;
        }
        case AckResponsePacket::SensorStream: {
            qDebug() << " ? Got sensor data streaming";
            break;
        }
        case AckResponsePacket::ConfigBlock: {
            qDebug() << " ? Got config block";
            break;
        }
        case AckResponsePacket::SleepingIn10Sec: {
            qDebug() << " ? Got notification that it is going to sleep in 10 seconds";
            break;
        }
        case AckResponsePacket::MacroMarkers: {
            qDebug() << " ? Got macro markers";
            break;
        }
        case AckResponsePacket::Collision: {
            qDebug() << " ? Got collision notification";
            break;
        }
//        case LocatorResponse: {
//            if (size_t(contents.length()) < sizeof(LocatorPacket)) {
//                qWarning() << "Locator response too small" << contents.length();
//                return;
//            }
//            contents = contents.mid(4);
//            qDebug() << "Locator size" << sizeof(LocatorPacket) << "Locatorconf" << contents.size();
//            const LocatorPacket *location = reinterpret_cast<const LocatorPacket*>(contents.data());
//            qDebug() << "tilt" << location->tilt << "position" << location->position.x << location->position.y << (location->flags ? "calibrated" : "not calibrated");

//        }
        default:
            qWarning() << "Unhandled ack response" << AckResponsePacket::ResponseType(int(contents[0]));
            break;
        }

        break;
    }
    case ResponsePacketHeader::Notification:
        qDebug() << " - data notification" << header.response;
        break;
    default:
        qWarning() << " ! unhandled type" << header.type;
        m_receiveBuffer.clear();
    }

    sendCommand(CommandPacketHeader::HardwareControl, CommandPacketHeader::SetDataStreaming, DataStreamingCommandPacket::create(1));

    if (header.type != ResponsePacketHeader::Response) {
        qWarning() << " ! not a simple response" << header.type;
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
        return;
    }
    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << " ! unhandled radio service state changed:" << newState;
        return;
    }

    if (!sendRadioControlCommand(Characteristics::Radio::antiDos, "011i3") ||
        !sendRadioControlCommand(Characteristics::Radio::transmitPower, QByteArray(1, 7)) ||
        !sendRadioControlCommand(Characteristics::Radio::wake, QByteArray(1, 1))) {
        qWarning() << " ! Init sequence failed";
        emit disconnected();
        emit statusMessageChanged(tr("Sphero Init sequence failed"));
        return;
    }
    const QLowEnergyCharacteristic rssiCharacteristic = m_radioService->characteristic(Characteristics::Radio::rssi);
    m_radioService->writeDescriptor(rssiCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

    qDebug() << " - Init sequence done";
    m_mainService->discoverDetails();
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

void SpheroHandler::sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data)
{
    static int currentSequenceNumber = 0;
    CommandPacketHeader header;

    switch(deviceId) {
    case CommandPacketHeader::Internal:
        switch(commandID) {
        case CommandPacketHeader::GetPwrState:
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetPwrNotify:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        default:
            qWarning() << "Unhandled packet internal command" << commandID;
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        }

        break;
    case CommandPacketHeader::HardwareControl:
        switch(commandID) {
        case CommandPacketHeader::GetRGBLed:
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::GetLocatorData:
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetDataStreaming:
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::ConfigureCollisionDetection:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetRGBLed:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetBackLED:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::Roll:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetStabilization:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetHeading:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        case CommandPacketHeader::SetRotationRate:
            header.flags |= CommandPacketHeader::Asynchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        default:
            qWarning() << "Unhandled packet hardware command" << commandID;
            header.flags |= CommandPacketHeader::Synchronous;
            header.flags |= CommandPacketHeader::ResetTimeout;
            break;
        }

        break;
    default:
        qWarning() << "Unhandled device id" << deviceId;
        header.flags |= CommandPacketHeader::Synchronous;
        header.flags |= CommandPacketHeader::ResetTimeout;
        break;
    }

    header.dataLength = data.size() + 1; // + 1 for checksum

    header.sequenceNumber = currentSequenceNumber++;
    header.commandID = commandID;
    header.deviceID = deviceId;
    qDebug() << " + Packet:";
    qDebug() << "  ] Device id:" << header.deviceID;
    qDebug() << "  ] command id:" << header.commandID;
    qDebug() << "  ] seq number:" << header.sequenceNumber;

    QByteArray headerBuffer(sizeof(CommandPacketHeader), 0); // + 1 for checksum
    qToBigEndian<uint8_t>(&header, sizeof(CommandPacketHeader), headerBuffer.data());
    qDebug() << " - " << uchar(headerBuffer[0]);
    qDebug() << " - " << uchar(headerBuffer[1]);

    QByteArray toSend;
    toSend.append(headerBuffer);
    toSend.append(data);

    uint8_t checksum = 0;
    for (int i=2; i<toSend.size(); i++) {
        checksum += toSend[i];
    }
    toSend.append(checksum xor 0xFF);

    qDebug() << " - Writing command" << toSend.toHex();
    m_mainService->writeCharacteristic(m_commandsCharacteristic, toSend);
}

} // namespace sphero
