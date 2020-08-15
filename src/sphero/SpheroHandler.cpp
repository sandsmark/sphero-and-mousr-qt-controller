#include <QMetaEnum>
#include <QDebug>

#include "SpheroHandler.h"
#include "utils.h"
#include "Uuids.h"

#include "v1/ResponsePackets.h"
#include "v1/CommandPackets.h"

#include "v2/Packets.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>
#include <QCoreApplication>

namespace sphero {

RobotType typeFromName(const QString &name)
{
    if (name.length() < 4 || name[2] != '-') {
        return RobotType::Unknown;
    }

    static const QMap<QString, RobotType> prefixes = {
        {"BB", RobotType::BB8},
        {"FB", RobotType::ForceBand},
        {"LM", RobotType::LMQ},
        {"2B", RobotType::Ollie},
        {"SK", RobotType::SPRK},
        {"D2", RobotType::R2D2},
        {"Q5", RobotType::R2Q5},
        {"GB", RobotType::BB9E},
        {"SM", RobotType::SpheroMini},
        {"1C", RobotType::WeBall},
    };
    const QString prefix = name.left(2);
    if (!prefixes.contains(prefix)) {
        return RobotType::Unknown;
    }

    return prefixes[prefix];
}

QString displayName(const QString &id)
{
    const RobotType type = typeFromName(id);
    if (type == RobotType::Unknown) {
        return id;
    }

    const QString suffix = id.mid(3);

    QString prettyName;
    switch(type) {
    case RobotType::BB8:
        prettyName = "BB-8";
        break;
    case RobotType::ForceBand:
        prettyName = "Force Band";
        break;
    case RobotType::LMQ:
        prettyName = "Lightning McQueen";
        break;
    case RobotType::Ollie:
        prettyName = "Ollie";
        break;
    case RobotType::SPRK:
        prettyName = "SPRK";
        break;
    case RobotType::R2D2:
        prettyName = "R2-D2";
        break;
    case RobotType::R2Q5:
        prettyName = "R2-Q5";
        break;
    case RobotType::BB9E:
        prettyName = "BB-9E";
        break;
    case RobotType::SpheroMini:
        prettyName = "Sphero Mini";
        break;
    case RobotType::WeBall:
        prettyName = "We Ball";
        break;
    case RobotType::Unknown: // should never happen according to the check above, but idk lol
        prettyName = "[unknown]";
        break;
//    default:
//        return id;
    }

    return prettyName + ' ' + suffix;
}

bool isValidRobot(const QString &name, const QString &address)
{
    if (name.length() != 7) {
        return false;
    }
    if (address.length() != 17) {
        return false;
    }

    if (typeFromName(name) == RobotType::Unknown) {
        return false;
    }

    // The naming scheme is XX-YYZZ, where XX is the type prefix, YY is the second to last byte of the address and ZZ is the last byte
    const QString part1 = address.mid(12, 2);
    const QString part2 = address.mid(15, 2);
    if (!name.endsWith(part1 + part2, Qt::CaseInsensitive)) {
        return false;
    }

    return true;
}

SpheroHandler::SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_name(deviceInfo.name()),
    m_robot(typeFromName(deviceInfo.name()))

{
    m_robotType = typeFromName(m_name);
    qDebug() << "Connecting to" << deviceInfo.address().toString();

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
    connect(m_deviceController, &QLowEnergyController::disconnected, this, &SpheroHandler::disconnected);
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
    brake();
    goToSleep();

    // Disconnect from device invalidates
    disconnect(m_mainService, nullptr, this, nullptr);
    m_mainService->deleteLater();
    disconnect(m_radioService, nullptr, this, nullptr);
    m_radioService->deleteLater();

    m_deviceController->disconnectFromDevice();

    qDebug() << "Disconnected from robot";
    emit statusMessageChanged("Disconnected");
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
    const QString name = m_name.isEmpty() ? "device" : displayName(m_name);
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
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::SetColorsCommandPacket(r, g, b, v1::SetColorsCommandPacket::Temporary));
        break;
    case RobotDefinition::V2: {
        v2::ColorLED bodyLED = v2::InvalidLED;
        switch(m_robotType) {
        case RobotType::R2D2:
        case RobotType::R2Q5:
            bodyLED = v2::R2BodyLED;
            break;
        case RobotType::BB9E:
            bodyLED = v2::B9BodyLED;
            break;
        default:
            qWarning() << "Don't know ID of body led for" << m_robotType;
            break;
        }

        if (bodyLED != v2::InvalidLED) {
            // Set the body to green
            m_mainService->writeCharacteristic(m_commandsCharacteristic, v2::encode(v2::SetLED(bodyLED, 0, 255, 0)));
        }
        break;
    }
    default:
        qWarning() << "TODO setcolor";
        break;
    }

    const QColor color = QColor::fromRgb(r, g, b);
    if (color != m_color) {
        m_color = color;
        emit colorChanged();
    }
}

void SpheroHandler::setSpeedAndAngle(int speed, int angle)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;
    speed = qBound(0, speed, 255);

    if (speed == m_speed && angle == m_angle) {
        return;
    }

    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(speed), qbswap<quint16>(uint16_t(angle)), v1::RollCommandPacket::Roll}));
        break;
    default:
        qWarning() << "TODO setspeedandangle";
        break;
    }

    if (m_speed != speed) {
        m_speed = speed;
        emit speedChanged();
    }
    if (m_angle != angle) {
        m_angle = angle;
        emit angleChanged();
    }
}

void SpheroHandler::setSpeed(int speed)
{
    if (speed == m_speed) {
        return;
    }

    setSpeedAndAngle(speed, m_angle);

    if (m_speed != speed) {
        m_speed = speed;
        emit speedChanged();
    }
}

void SpheroHandler::setAngle(int angle)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;
    qDebug() << "Setting angle to" << angle;

    if (angle == m_angle) {
        return;
    }

    // why the fuck do I need to swap the angle bytes?
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(m_speed), qbswap<quint16>(uint16_t(angle)), v1::RollCommandPacket::Brake}));
        break;
    default:
        qWarning() << "TODO setangle";
        break;
    }

    m_angle = angle;
    emit angleChanged();
}

void SpheroHandler::brake()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(0), uint16_t(0), v1::RollCommandPacket::Brake}));
        break;
    default:
        qWarning() << "TODO brake";
        break;
    }
}

void SpheroHandler::faceLeft()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(0), uint16_t(270), v1::RollCommandPacket::Brake}));
        break;
    default:
        qWarning() << "TODO faceleft";
        break;
    }
}

void SpheroHandler::faceRight()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(0), uint16_t(90), v1::RollCommandPacket::Brake}));
        break;
    default:
        qWarning() << "TODO faceleft";
        break;
    }
}

void SpheroHandler::faceForward()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::RollCommandPacket({uint8_t(0), uint16_t(0), v1::RollCommandPacket::Brake}));
        break;
    default:
        qWarning() << "TODO faceforward";
        break;
    }
}


void SpheroHandler::setAutoStabilize(const bool enabled)
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::CommandPacketHeader::HardwareControl, v1::CommandPacketHeader::SetStabilization, QByteArray(enabled ? "\x1" : "\x0"));
        if (enabled != m_autoStabilize) {
            m_autoStabilize = enabled;
            emit autoStabilizeChanged();
        }
        break;
    default:
        qWarning() << "TODO setautostabilize";
        break;
    }
}

void SpheroHandler::setDetectCollisions(const bool enabled)
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::EnableCollisionDetectionPacket(enabled));
        break;
    default:
        qWarning() << "TODO set detect collisions";
        break;
    }
}

void SpheroHandler::goToSleep()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::GoToSleepPacket());
        break;
    case RobotDefinition::V2:
        m_mainService->writeCharacteristic(m_commandsCharacteristic, v2::encode(v2::GoToLightSleep()));
        break;
    default:
        qWarning() << "TODO gotosleep";
        break;
    }
}

void SpheroHandler::goToDeepSleep()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
    // Not sure if this is necessary first
//    sendCommand(GoToSleepPacket());
        sendRadioControlCommand(Characteristics::Radio::V1::deepSleep, "011i3");
        break;
    default:
        qWarning() << "TODO gotodeepsleep";
        break;
    }
}

void SpheroHandler::enablePowerNotifications()
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::CommandPacketHeader::Internal, v1::CommandPacketHeader::SetPwrNotify, QByteArray("\x1", 1));
        break;
    default:
        qWarning() << "TODO enable power notifications";
        break;
    }
}

void SpheroHandler::setEnableAsciiShell(const bool enabled)
{
    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::SetUserHackModePacket({enabled}));
        break;
    default:
        qWarning() << "TODO enable ascii shell";
        break;
    }
}

void SpheroHandler::boost(int angle, int duration)
{
    while (angle < 0) {
        angle += 360;
    }
    angle %= 360;

    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::BoostCommandPacket({uint8_t(qBound(0, duration, 255)), uint16_t(angle)}));
        break;
    default:
        qWarning() << "TODO boost";
        break;
    }

    if (m_angle != angle) {
        m_angle = angle;
        emit angleChanged();
    }
}

void SpheroHandler::onServiceDiscoveryFinished()
{
    qDebug() << " - Discovered services";

#if 0 // for dumping all services and all their characteristics
    for (const QBluetoothUuid &service : m_deviceController->services()) {
        qDebug() << service;
        QLowEnergyService *leService = m_deviceController->createServiceObject(service);
        connect(leService, &QLowEnergyService::stateChanged, this, [=](QLowEnergyService::ServiceState newState) {
            if (newState == QLowEnergyService::InvalidService) {
                qWarning() << "invalided" << service;
                leService->deleteLater();
                return;
            }

            if (newState == QLowEnergyService::DiscoveringServices) {
                return;
            }

            if (newState != QLowEnergyService::ServiceDiscovered) {
                qDebug() << " ! unhandled service state changed:" << newState;
                leService->deleteLater();
                return;
            }
            for (const QLowEnergyCharacteristic &characteristic : leService->characteristics()) {
                qDebug() << "service" << service << "has char" << characteristic.uuid() << characteristic.name();
            }
            leService->deleteLater();
        });
        leService->discoverDetails();
    }
#endif

    m_radioService = m_deviceController->createServiceObject(m_robot.radioService, this);
    if (!m_radioService) {
        qWarning() << " ! Failed to get radio service";
        return;
    }
    qDebug() << " - Got radio service";

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

    m_mainService = m_deviceController->createServiceObject(m_robot.mainService, this);
    if (!m_mainService) {
        qWarning() << " ! no main service";
        return;
    }

    connect(m_mainService, &QLowEnergyService::descriptorWritten, this, [](const QLowEnergyDescriptor  &info, const QByteArray &value) {
        qDebug() << "main descriptor write" << value;
    });
    connect(m_mainService, &QLowEnergyService::descriptorRead, this, [](const QLowEnergyDescriptor  &info, const QByteArray &value) {
        qDebug() << "main descriptor read" << value;
    });

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

    for (const QLowEnergyCharacteristic &characteristic : m_mainService->characteristics()) {
        qDebug() << "service has char" << characteristic.uuid() << characteristic.name();
    }

    m_commandsCharacteristic = m_mainService->characteristic(m_robot.commandsCharacteristic);
    if (!m_commandsCharacteristic.isValid()) {
        qWarning() << " ! Commands characteristic invalid";
        return;
    }

    QLowEnergyCharacteristic responseCharacteristic;
    switch(m_robot.api) {
    case RobotDefinition::V1:
        responseCharacteristic = m_mainService->characteristic(Characteristics::Main::V1::response);
        break;
    case RobotDefinition::V2:
        responseCharacteristic = m_commandsCharacteristic;
        break;
    default:
        qWarning() << "Unhandled API version";
        return;
    }
    if (!responseCharacteristic.isValid()) {
        qWarning() << " ! response characteristic invalid";
        return;
    }


    // Enable notifications
    m_mainService->writeDescriptor(responseCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));
    m_mainService->writeDescriptor(m_mainService->characteristic(Characteristics::Main::V2::unknown1).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

    qDebug() << " - Successfully connected";

    switch(m_robot.api) {
    case RobotDefinition::V1:
        sendCommandV1(v1::SetPowerNotifyCommandPacket{0});
        sendCommandV1(v1::CommandPacketHeader::Internal, v1::CommandPacketHeader::Ping, {});
        sendCommandV1(v1::SetNonPersistentOptionsPacket{v1::SetNonPersistentOptionsPacket::StopOnDisconnect});
        setAutoStabilize(true);
        setDetectCollisions(true);
        sendCommandV1(v1::CommandPacketHeader::HardwareControl, v1::CommandPacketHeader::SetDataStreaming, v1::DataStreamingCommandPacket::create(1) );
        break;
    case RobotDefinition::V2:
        m_mainService->writeCharacteristic(m_commandsCharacteristic, v2::encode(v2::WakePacket()));
        break;
    default:
        qWarning() << "Unhandled API version";
        return;
    }

    setColor(Qt::green);

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

    if (characteristic.uuid() == QBluetoothUuid::ServiceChanged) {
        // TODO: I think maybe this is when it is removed from the charger, and the battery service becomes available
        qDebug() << " ? GATT service changed" << data.toHex(':');
        return;
    }

    switch(m_robot.api) {
    case RobotDefinition::V1:
        qDebug() << "main characteristic changed";
        if (characteristic.uuid() == Characteristics::Radio::V1::rssi) {
            m_rssi = data[0];
            emit rssiChanged();
            break;
        }

        if (characteristic.uuid() == Characteristics::Main::V1::response) {
            parsePacketV1(data);
            break;
        }

        qWarning() << " ? Changed from unexpected characteristic" << characteristic.name() << characteristic.uuid() << data;
        break;

    case RobotDefinition::V2:
        parsePacketV2(data);
        break;
    default:
        qWarning() << " !!!!! Unhandled API version";
        qDebug() << "characteristic" << characteristic.uuid();
        qDebug() << "DatA:" << data.toHex(':');
        break;

    }
}

void SpheroHandler::parsePacketV2(const QByteArray &data)
{
    if (data.startsWith(v2::StartOfPacket)) {
        m_receiveBuffer = data;
    } else if (!m_receiveBuffer.isEmpty()) {
        m_receiveBuffer.append(data);
    } else {
        qWarning() << " ! Got data but without correct start";
        qDebug() << data.toHex(':');
        const int startOfData = data.indexOf(v2::StartOfPacket);
        if (startOfData < 0) {
            qWarning() << " ! Contains nothing useful";
            m_receiveBuffer.clear();
            return;
        }
        m_receiveBuffer = data.mid(startOfData);
    }

    int end = m_receiveBuffer.indexOf(v2::EndOfPacket);
    while (end != -1) {
        const QByteArray packetData = m_receiveBuffer.mid(0, end + 1);
        m_receiveBuffer = m_receiveBuffer.mid(end + 1);
        end = m_receiveBuffer.indexOf(v2::EndOfPacket);

        qDebug() << "Decoding" << packetData.toHex(':');
        if (size_t(packetData.size()) < sizeof(v2::Packet)) {
            qWarning() << "not enough data" << packetData.size();
            qDebug() << packetData.toHex(':');
            continue;
        }
        bool ok;
        const v2::Packet base = v2::decode<v2::Packet>(packetData, &ok);
        if (!ok) {
            qWarning() << "Failed to decode" << packetData.toHex(':');
        }
        if (base.m_flags & v2::Packet::HasErrorCode) {
            const v2::ResponsePacket response = v2::decode<v2::ResponsePacket>(packetData, &ok);
            qWarning() << "Got error code" << v2::Packet::Error(response.errorCode);
            qDebug() << "for" << v2::Packet::CommandTarget(base.m_deviceID) << base.m_commandID;
            continue;
        }

//        qDebug() << "Got data for" << v2::Packet::CommandTarget(base.m_deviceID) << base.;
    }
}

void SpheroHandler::parsePacketV1(const QByteArray &data)
{

    qDebug() << " ------------ Characteristic changed" << data.toHex(':') << " ----------";

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

    if (m_receiveBuffer.size() < int(sizeof(v1::CommandPacketHeader))) {
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
    case ResponsePacketHeader::Notification:
        break;
    default:
        qWarning() << " !!!!!!!!!!!!!!!!!!!!!  unhandled response type!: " << header.type;
        m_receiveBuffer.clear();
    }

    qDebug() << " - sequence num" << header.sequenceNumber;
    qDebug() << " - data length" << header.dataLength;

    if (m_receiveBuffer.size() != sizeof(v1::CommandPacketHeader) + header.dataLength - 1) {
        qWarning() << " ! Packet size wrong" << m_receiveBuffer.size();
        qDebug() << "  > Expected" << sizeof(v1::CommandPacketHeader) << "+" << header.dataLength;
        return;
    }

    uint8_t checksum = 0;
    for (int i=2; i<m_receiveBuffer.size() - 1; i++) {
        checksum += uint8_t(m_receiveBuffer[i]);
    }
    checksum ^= 0xFF;
    if (!m_receiveBuffer.endsWith(checksum)) {
        qWarning() << " !!!! Invalid checksum !!!!" << checksum;
        qDebug() << "  > Expected" << uint8_t(m_receiveBuffer.back());
        return;
    }

    qDebug() << " - Checksum:" << QByteArray::number(checksum, 16);

    QByteArray contents = data.mid(sizeof(ResponsePacketHeader), header.dataLength - 1); // checksum is last byte
    if (contents.isEmpty()) {
        qWarning() << " ! No contents";
    }
    qDebug() << " - received contents" << contents.size() << contents.toHex(':');
    qDebug() << " - response type:" << header.type;

    m_receiveBuffer.clear(); // TODO leave some

    switch(header.type) {
    case ResponsePacketHeader::Response: {
        qDebug() << " - ack response" << ResponsePacketHeader::PacketType(header.packetType);
//        qDebug() << "Content length" << contents.length() << "data length" << header.dataLength << "buffer length" << m_receiveBuffer.length() << "locator packet size" << sizeof(LocatorPacket) << "response packet size" << sizeof(ResponsePacketHeader);

        if (header.packetType == ResponsePacketHeader::InvalidParameter) {
            qWarning() << " !!!!! We sent an invalid parameter!";
            if (m_pendingSyncRequests.contains(header.sequenceNumber)) {
                const QPair<uint8_t, uint8_t> responseToCommand = m_pendingSyncRequests.take(header.sequenceNumber);
                if (responseToCommand.first == v1::CommandPacketHeader::HardwareControl) {
                    qDebug() << " ! hardware command" << v1::CommandPacketHeader::HardwareCommand(responseToCommand.second);

                    if (responseToCommand.second != v1::CommandPacketHeader::GetLocatorData) {
                        sendCommandV1(v1::CommandPacketHeader::HardwareControl, v1::CommandPacketHeader::GetLocatorData, {});
                    }
                } else if (responseToCommand.first == v1::CommandPacketHeader::Internal) {
                    qDebug() << " ! internal command" << v1::CommandPacketHeader::InternalCommand(responseToCommand.second);
                    sendCommandV1(v1::CommandPacketHeader::HardwareControl, v1::CommandPacketHeader::GetLocatorData, {});
                } else {
                    qDebug() << " ! invalid command target" << responseToCommand;
                }
            } else {
                qDebug() << "unknown request";
            }
            break;
        }

        if (!m_pendingSyncRequests.contains(header.sequenceNumber)) {
            qWarning() << " ! this was not an expected response";
            break;
        }

        const QPair<uint8_t, uint8_t> responseToCommand = m_pendingSyncRequests.take(header.sequenceNumber);
        // TODO separate function
        switch(responseToCommand.first) {
        case v1::CommandPacketHeader::Internal:
            switch(responseToCommand.second) {
            case v1::CommandPacketHeader::Ping: {
                qDebug() << "Got pong";
                break;
            }
            case v1::CommandPacketHeader::GetPwrState: {
                bool ok;
                PowerStatePacket response = byteArrayToPacket<PowerStatePacket>(contents, &ok);
                if (!ok) {
                    return;
                }
                qDebug() << "  ========== power response ====== ";
                qDebug() << "  + version" << response.recordVersion;
                qDebug() << "  + state" << response.powerState;
                qDebug() << "  + battery voltage" << response.batteryVoltage;
                qDebug() << "  + number of charges" << response.numberOfCharges;
                qDebug() << "  + seconds since charge" << response.secondsSinceCharge;
                break;
            }
            default:
                qWarning() << " !!!!! unhandled internal response" << v1::CommandPacketHeader::InternalCommand(responseToCommand.second) << "!!!!!!!!!!!";
                break;
            }
            break;
        case v1::CommandPacketHeader::HardwareControl: {
            switch(responseToCommand.second) {
            case v1::CommandPacketHeader::GetLocatorData: {
                bool ok;
                LocatorPacket resp = byteArrayToPacket<LocatorPacket>(contents, &ok);
                if (!ok) {
                    qDebug() << " ! Locator state packet invalid";
                    break;
                }
                qDebug() << "  ========== locator response ====== ";
                qDebug() << "  + calibrated?" << bool(resp.flags & LocatorPacket::Calibrated);
                qDebug() << "  + position x:" << resp.position.x;
                qDebug() << "  + position y:" << resp.position.y;
                qDebug() << "  + tilt:" << resp.tilt;
                break;
            }
            case v1::CommandPacketHeader::GetRGBLed: {
                bool ok;
                RgbPacket resp = byteArrayToPacket<RgbPacket>(contents, &ok);
                if (!ok) {
                    qDebug() << " ! RGB packet invalid";
                    break;
                }

                m_color = QColor (resp.red, resp.green, resp.blue);
                emit colorChanged();
                break;
            }
            case v1::CommandPacketHeader::SetNonPersistentOptionFlags: {
                qDebug() << " + temporary options set";
                break;
            }
            case v1::CommandPacketHeader::SetStabilization: {
                qDebug() << " + Stabilization set";
                break;
            }
            case v1::CommandPacketHeader::SetHeading: {
                qDebug() << " + Heading set";
                break;
            }
            case v1::CommandPacketHeader::Roll: {
                qDebug() << " + Roll set";
                break;
            }
            case v1::CommandPacketHeader::SetDataStreaming: {
                qDebug() << " + Data streaming enabled";
//                sendCommand();
//                faceLeft();
//                setAngle(180);
//                setSpeedAndAngle(0, 180);
                sendCommandV1(v1::RollCommandPacket({uint8_t(0), uint16_t(0), v1::RollCommandPacket::Calibrate}));
                sendCommandV1(v1::CommandPacketHeader::HardwareControl, v1::CommandPacketHeader::GetLocatorData, {});
//                faceRight();
                break;
            }
            default:
                qWarning() << " !!!! unhandled hardware response" << v1::CommandPacketHeader::HardwareCommand(responseToCommand.second) << "!!!!!!!!";
                break;
            }
            break;
        }
        default:
            qWarning() << " ! unhandled command target" << responseToCommand.first;
        }
        break;
    }
    case ResponsePacketHeader::Notification:
        qDebug() << " - data notification" << header.packetType;
        switch(header.packetType) {
        case ResponsePacketHeader::PowerNotification: {
            if (contents.size() != 1) {
                qWarning() << " ! Invalid size of power notification";
                break;
            }
            const uint8_t state = contents[0];
            if (state > 4) {
                qWarning() << " ! Invalid reported state";
                break;
            }
            if (state != m_powerState) {
                m_powerState = PowerState(state);
                qDebug() << "new power state" << m_powerState;
                emit powerChanged();
            }

            break;
        }
        default:
            qWarning() << " ! unhandled notification type" << header.packetType;

        }

        break;
    default:
        qWarning() << " ! unhandled type" << header.type;
        m_receiveBuffer.clear();
    }
    qDebug() << " ************************* ";

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

    if (!sendRadioControlCommand(m_robot.passwordCharacteristic, m_robot.radioPassword)) {
        qWarning() << "Failed to send unlock password";
        return;
    }

    switch(m_robot.api) {
    case RobotDefinition::V1: {
        if (!sendRadioControlCommand(Characteristics::Radio::V1::transmitPower, "\x7") ||
            !sendRadioControlCommand(Characteristics::Radio::V1::wake, "\x1")) {
            qWarning() << " ! Init sequence failed";
            emit disconnected();
            emit statusMessageChanged(tr("Sphero Init sequence failed"));
            return;
        }
        const QLowEnergyCharacteristic rssiCharacteristic = m_radioService->characteristic(Characteristics::Radio::V1::rssi);
        m_radioService->writeDescriptor(rssiCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));
        break;
    }
    case RobotDefinition::V2: {
        // DONE (I think): R2D2:
        // 1. Antidos
        // 2. write dfu handle request notification
        // 2. read DFU:handle, receive: 0000   0b 00 01 00 04 00 02 02                           ........
        // 3. write main:main notification
        // 4. write main service (wake from sleep packet): 0000   8d 0a 13 0d 00 d5 d8                              .......

        const QLowEnergyCharacteristic rssiCharacteristic = m_radioService->characteristic(Characteristics::Radio::V2::write); // idk??
        m_radioService->writeDescriptor(rssiCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

        break;
    }
    default:
        qWarning() << "unhandled robot api";
        return;
    }

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

void SpheroHandler::sendCommandV1(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data)
{
    v1::CommandPacketHeader packet(deviceId, commandID);
    if (!packet.isValid()) {
        return;
    }
    qDebug() << " >>>>>>>>>>> sending command <<<<<<<<<<";
    qDebug() << " - data" << data;

    if (packet.isSynchronous()) {
        if (m_pendingSyncRequests.contains(m_nextSequenceNumber)) {
            qWarning() << " !!!!!! We have outstanding requests, overflow?";
            qWarning() << " !!!!!! Next request:" << m_nextSequenceNumber;
            qWarning() << " !!!!!! Outstanding requests:" << m_pendingSyncRequests;
            return;
        }

        packet.setSequenceNumber(m_nextSequenceNumber);
        m_pendingSyncRequests.insert(m_nextSequenceNumber, {deviceId, commandID});

        m_nextSequenceNumber++;
    }

    const QByteArray toSend = packet.encode(data);
    if (toSend.isEmpty()) {
        qDebug() << " ! Encoding packet failed!";
        return;
    }
    qDebug() << " ++++++++++++++++++++++++++++++++++++++";

    m_mainService->writeCharacteristic(m_commandsCharacteristic, toSend);
}

SpheroHandler::RobotDefinition::RobotDefinition(const RobotType type)
{
    switch (type) {
    // I think so from random googling, but I don't have one so not tested
    case RobotType::Ollie:
    case RobotType::WeBall:

        // Tested
    case RobotType::BB8:
        mainService = Services::V1::main;
        batteryService = Services::V1::battery;
        radioService = Services::V1::radio;

        commandsCharacteristic = Characteristics::Main::V1::commands;

        radioPassword = "011i3";
        passwordCharacteristic = Characteristics::Radio::V1::antiDos;

        api = V1;
        break;

        // Not tested
    case RobotType::SpheroMini:
    case RobotType::LMQ:

        // These I have
    case RobotType::BB9E:
    case RobotType::R2D2:
    case RobotType::R2Q5:
        mainService = Services::V2::main;
        batteryService = Services::V2::battery;
        radioService = Services::V2::radio;

        commandsCharacteristic = Characteristics::Main::V2::commands;

        radioPassword = "usetheforce...band";
        passwordCharacteristic = Characteristics::Radio::V2::antiDos;

        api = V2;
        break;

    default:
        qWarning() << "unhandled type" << int(type);
        return;
    }
}

} // namespace sphero
