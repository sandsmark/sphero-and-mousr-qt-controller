#include <QMetaEnum>
#include <QDebug>

#include "MousrHandler.h"
#include "utils.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>
#include <QQmlEngine>

namespace mousr {

bool MousrHandler::sendCommandPacket(const CommandPacket &packet)
{
    qDebug() << " + Sending packet" << packet.m_command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }
    QByteArray buffer(sizeof(CommandPacket), Qt::Uninitialized);
    qToLittleEndian<char>(&packet, sizeof(CommandPacket), buffer.data());

    qDebug() << "Writing" << buffer.toHex(':');
    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    return true;
}

bool MousrHandler::sendCommand(const CommandType command, const float arg1, const float arg2, const float arg3)
{
    qDebug() << " + Sending command with float args" << command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }
    CommandPacket packet(command);
    packet.vector3D.x = arg1;
    packet.vector3D.y = arg2;
    packet.vector3D.z = arg3;

    sendCommandPacket(packet);

    return true;
}

bool MousrHandler::sendCommand(const CommandType command, const uint32_t arg1, const uint32_t arg2)
{
    qDebug() << " + Sending command with int args" << command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    CommandPacket packet(command);
    packet.vector2D.x = arg1;
    packet.vector2D.y = arg2;
    sendCommandPacket(packet);

    return true;
}

bool MousrHandler::sendCommand(const CommandType command)
{
    qDebug() << " + Sending" << command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    sendCommandPacket(CommandPacket(command));

    return true;
}

void MousrHandler::sendInput()
{
    const bool angleChanged = !qFuzzyCompare(m_currentInput.angle, m_newInput.angle);
    const bool speedChanged = !qFuzzyCompare(m_currentInput.speed, m_newInput.speed);
    const bool heldChanged = !qFuzzyCompare(m_currentInput.held, m_newInput.held);
    if (!angleChanged && !speedChanged && !heldChanged) {
        qDebug() << " ! Nothing in the input changed";
        return;
    }
    qDebug() << " + Sending updated input";

    const bool isMoving = !qFuzzyIsNull(m_newInput.speed);

    CommandType command = CommandType::Stop;
    if (isMoving) {
        // should this be ReverseSignal if speed < 0?
        command = CommandType::Move;
    } else if (angleChanged) {
        command = CommandType::Spin;
    } else {
        m_currentInput.reset();
        sendCommand(CommandType::Stop);
        return;
    }

    CommandPacket packet(command);
    packet.input = m_newInput;
    m_currentInput = m_newInput;
    sendCommandPacket(packet);
}

void MousrHandler::sendAutoplay()
{
    CommandPacket packet(CommandType::ConfigAutoMode);
    packet.autoPlayConfig = m_newAutoConfig;
    sendCommandPacket(packet);
}

void MousrHandler::resetHeading()
{
    m_sendInputTimer.stop();
    m_currentInput.reset();
    m_newInput.reset();

    CommandPacket packet(CommandType::ResetHeading);
    packet.input = m_newInput;
    sendCommandPacket(packet);
}

MousrHandler::MousrHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_name(deviceInfo.name())
{
    // In case the UI asks us to update more than 100 times a second
    m_sendInputTimer.setInterval(10);
    m_sendInputTimer.setSingleShot(true);
    connect(this, &MousrHandler::inputChanged, &m_sendInputTimer, [this]() {
        if (!m_sendInputTimer.isActive()) {
            m_sendInputTimer.start();
        }
    });

    m_deviceController = QLowEnergyController::createCentral(deviceInfo, this);

    connect(m_deviceController, &QLowEnergyController::connected, m_deviceController, &QLowEnergyController::discoverServices);
    connect(m_deviceController, &QLowEnergyController::serviceDiscovered, this, &MousrHandler::onServiceDiscovered);

    connect(m_deviceController, &QLowEnergyController::connectionUpdated, this, [](const QLowEnergyConnectionParameters &parms) {
            qDebug() << " - controller connection updated, latency" << parms.latency() << "maxinterval:" << parms.maximumInterval() << "mininterval:" << parms.minimumInterval() << "supervision timeout" << parms.supervisionTimeout();
            });
    connect(m_deviceController, &QLowEnergyController::connected, this, []() {
            qDebug() << " - controller connected";
            });
    connect(m_deviceController, &QLowEnergyController::disconnected, this, []() {
            qDebug() << " - controller disconnected";
            });
    connect(m_deviceController, &QLowEnergyController::discoveryFinished, this, []() {
            qDebug() << " - controller discovery finished";
            });
    connect(m_deviceController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &MousrHandler::onControllerError);

    connect(m_deviceController, &QLowEnergyController::stateChanged, this, &MousrHandler::onControllerStateChanged);
    connect(this, &MousrHandler::initComplete, this, &MousrHandler::resetHeading);

    m_deviceController->connectToDevice();

    if (m_deviceController->error() != QLowEnergyController::NoError) {
        qDebug() << "controller error when starting:" << m_deviceController->error() << m_deviceController->errorString();
    }
}

MousrHandler::~MousrHandler()
{
    qDebug() << "mousr handler dead";
    if (!m_isAutoActive) {
        sendCommand(CommandType::Stop);
    }

    if (m_deviceController) {
        m_deviceController->disconnectFromDevice();
    } else {
        qWarning() << "no controller";
    }
}

bool MousrHandler::isConnected()
{
    return m_deviceController && m_deviceController->state() != QLowEnergyController::UnconnectedState &&
            m_service && m_writeCharacteristic.isValid() && m_readCharacteristic.isValid();

}

QString MousrHandler::statusString()
{
    const QString name = m_name.isEmpty() ? "device" : m_name;
    if (isConnected()) {
        return tr("Connected to %1").arg(name);
    } else if (m_deviceController && m_deviceController->state() == QLowEnergyController::UnconnectedState) {
//        QTimer::singleShot(1000, this, &QObject::deleteLater);
        return tr("Failed to connect to %1").arg(name);
    } else {
        return tr("Connecting to %1...").arg(name);
    }
}

void MousrHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    // 00001801-0000-1000-8000-00805f9b34fb
    static const QBluetoothUuid genericServiceUuid = QUuid(0x00001801, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb);
    if (newService == genericServiceUuid) {
        qDebug() << "Got generic service uuid, not sure what this is for" << newService;
        return;
    }

    // 0000fe59-0000-1000-8000-00805f9b34fb
    static const QBluetoothUuid dfuServiceUuid = QUuid(0x0000fe59, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb);
    if (newService == dfuServiceUuid) {
        // Read:         8ec90001-f315-4f60-9fb8-838830daea50
        // Write:        8ec90002-f315-4f60-9fb8-838830daea50
        qDebug() << "TODO: firmware update mode";
        return;
    }

    // Service UUID: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
    static const QBluetoothUuid serviceUuid    = QUuid(0x6e400001, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e);
    if (newService != serviceUuid) {
        qWarning() << "discovered unhandled service" << newService << "expected" << serviceUuid;
        return;
    }

    //qDebug() << "Found correct service";
    if (m_service) {
        m_service->deleteLater();
    }

    m_service = m_deviceController->createServiceObject(newService, this);
    //qDebug() << "got service:"  << m_service->serviceName() << m_service->serviceUuid();

    connect(m_service, &QLowEnergyService::characteristicChanged, this, &MousrHandler::onCharacteristicChanged);

    connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &MousrHandler::onServiceError);
    connect(m_service, &QLowEnergyService::stateChanged, this, &MousrHandler::onServiceStateChanged);

    m_service->discoverDetails();
}

void MousrHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    static constexpr QUuid writeUuid      = {0x6e400002, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};
    static constexpr QUuid readUuid       = {0x6e400003, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};

    if (newState == QLowEnergyService::InvalidService) {
        qWarning() << "Got invalid service";
        emit disconnected();
        return;
    }

    if (newState == QLowEnergyService::DiscoveringServices) {
        return;
    }


    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << "unhandled service state changed:" << newState;
        return;
    }

//    for (const QLowEnergyCharacteristic &c : m_service->characteristics()) {
//        qDebug() << "characteristic available:" << c.name() << c.uuid() << c.properties();
//    }

    m_readCharacteristic = m_service->characteristic(readUuid);
    m_writeCharacteristic = m_service->characteristic(writeUuid);
    if (!m_readCharacteristic.descriptors().isEmpty()) {
        m_readDescriptor = m_readCharacteristic.descriptors().first();
    } else {
        qWarning() << "No read characteristic";
    }

    if (!isConnected()) {
        qDebug() << "Finished scanning, but not valid";
        emit disconnected();
        return;
    }

    qDebug() << "Successfully connected";

    // Who the _fuck_ designed this API, requiring me to write magic bytes to a
    // fucking read descriptor to get characteristicChanged to work?
    m_service->writeDescriptor(m_readDescriptor, QByteArray::fromHex("0100"));

    if (!sendCommand(CommandType::InitializeDevice, mbApiVersion, quint32(QDateTime::currentSecsSinceEpoch()))) {
        qWarning() << "Failed to send init command";
    }

    emit connectedChanged();
}

void MousrHandler::chirp()
{
    const int soundClip = 6; // todo: discover if there are more clips stored
    if (!sendCommand(CommandType::Chirp, 0, soundClip)) {
        qWarning() << "Failed to request chirp";
    }
}

void MousrHandler::pause()
{
    if (!sendCommandPacket(CommandPacket(CommandType::Stop))) {
        qWarning() << "Failed to request stop";
    }
}

void MousrHandler::setSoundVolume(const int volumePercent)
{
    if (volumePercent < 0 || volumePercent > 100) {
        qWarning() << "Invalid volume";
        return;
    }

    if (sendCommand(CommandType::SoundVolume, volumePercent)) {
        m_volume = volumePercent;
    }
    emit soundVolumeChanged();
}

void MousrHandler::setAutoPlay(const bool enabled)
{
    if (enabled == m_isAutoActive) {
        qWarning() << "already same state!" << enabled << m_currentAutoConfig << m_isAutoActive;
    }
    m_newAutoConfig.enabled = enabled ? 1 : 0;
    sendAutoplay();
}

void MousrHandler::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::UnconnectedState) {
        qWarning() << "Disconnected";
        emit disconnected();
    }

    qWarning() << " ! controller state changed" << state;
    emit connectedChanged();
}

void MousrHandler::onControllerError(QLowEnergyController::Error newError)
{
    qWarning() << " - controller error:" << newError << m_deviceController->errorString();
    if (newError == QLowEnergyController::UnknownError) {
        qWarning() << "Probably 'Operation already in progress' because qtbluetooth doesn't understand why it can't get answers over dbus when a connection attempt hangs";
    }
    connect(m_deviceController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &MousrHandler::onControllerError);
}


void MousrHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    qWarning() << "Service error:" << error;
    if (error == QLowEnergyService::NoError) {
        return;
    }

    emit disconnected();
}

void MousrHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (characteristic != m_readCharacteristic) {
        qWarning() << "changed from unexpected characteristic" << characteristic.uuid() << data;
        return;
    }

    if (data.size() != 20) {
        qWarning() << "invalid packet size" << data.size() << "expected 20";
        return;
    }


    ResponsePacket response;

    static_assert(sizeof(response) == 20);
    qFromLittleEndian<ResponsePacket>(data.data(), 1, &response);

    const QString responseName = EnumHelper::toString(ResponseType(response.type));

    if (responseName.isEmpty()) {
        qDebug() << "Unknown command";
        qDebug() << response.type << data;
        return;
    }
    qDebug() << "Got response" << ResponseType(response.type);


    switch(response.type){
    case DeviceOrientation: {
        if (!fuzzyVectorsEqual(response.orientation.rotation, m_rotation)) {
            m_rotation = response.orientation.rotation;
            emit orientationChanged();
        }
        if (response.orientation.isFlipped != m_isFlipped) {
            m_isFlipped = response.orientation.isFlipped;
            emit orientationChanged();
        }

        break;
    }
    case BatteryVoltage:{
        if (response.battery.isAutoMode != m_isAutoActive) {
            m_isAutoActive = response.battery.isAutoMode;
            emit autoRunningChanged();
        }

        const bool differentValues =
                response.battery.voltage != m_voltage ||
                response.battery.isBatteryLow != m_batteryLow ||
                response.battery.isCharging != m_charging ||
                response.battery.isFullyCharged != m_fullyCharged ||
                response.battery.memory != m_memory;

        if (differentValues) {
            // Voltage seems to be percent? wtf
            m_voltage = response.battery.voltage;
            m_batteryLow = response.battery.isBatteryLow;
            m_charging = response.battery.isCharging;
            m_fullyCharged = response.battery.isFullyCharged;
            m_memory = response.battery.memory;
            emit powerChanged();
        }

        break;
    }
    case CrashLogString: {
        const QString crashLog = response.crashString.message();
        if (crashLog != "No crash log.") {
            qDebug() << " Crash log string:" << crashLog;
        }
        break;
    }
    case CrashLogFinished:
        qDebug() << response.type;
        break;
    case AnalyticsBegin: {
        int numberOfEntries = response.analyticsBegin.numberOfEntries;
        qDebug() << "Number of analytics entries:" << numberOfEntries;
        break;
    }
    case SensorDirty: {
        m_sensorDirty = response.sensorDirty.isDirty;
        emit sensorDirtyChanged();
        break;
    }
    case AutoModeChanged: {
        qDebug() << "auto mode changed";
        m_currentAutoConfig = std::move(response.autoPlay.config);
        qDebug() << m_currentAutoConfig;
        emit autoPlayChanged();
        break;
    }

        ///////////////// ANALYTICS: Who Gives A Shit™ ///////////////////////
        // Analytics: fragmented packages, single byte header in each, and CRC at the end of all I think
        // That's how it looks at least, and a readable ascii string for what it is
    case AnalyticsData:
    case AnalyticsEntry: // TODO: debug log text I think
    case AnalyticsEnd:
        break;

    case InitDone:
        qDebug() << "Init complete";
        break;

    case FirmwareVersion: {
        m_version = response.firmwareVersion;

        qDebug() << "Firmware mode:" << m_version.firmwareType;
        qDebug().noquote() << "Version" << (QByteArray::number(m_version.major) + "." + QByteArray::number(m_version.minor) + "." + QByteArray::number(m_version.commitNumber) + "-" + QByteArray(m_version.commitHash, 4).toHex());
        qDebug() << "Mousr version" << m_version.mousrVersion << "hardware version" << m_version.hardwareVersion << "bootloader version" << m_version.bootloaderVersion;
        break;
    }
    case CommandCompleted: {
        qDebug() << sizeof(CommandResult) << data.size();
        const CommandType command = response.commandResult.commandType;
        const uint32_t currentApiVer = response.commandResult.currentApiVersion;
        const uint32_t minApiVer = response.commandResult.minimumApiVersion;
        const uint32_t maxApiVer = response.commandResult.maximumApiVersion;
        switch(response.commandResult.commandType) {
        case CommandType::InitializeDevice:
            emit initComplete();
            break;
        case CommandType::EraseAnalyticsRecords:
            switch(response.commandResult.resultCode) {
            case 0:
                qDebug() << "Analytics erase succeeded";
                break;
            case -1:
                qWarning() << "Analytics erase failed";
                break;
            default:
                qWarning() << "unknown result code for erasing analytics" << response.commandResult.resultCode;
            }

            break;
        default:
            qWarning() << "!! Got NACK for command" << command;
            qDebug() << "unknown num:" << response.commandResult.resultCode;
            qDebug() << "Api version current:" << currentApiVer << "min:" << minApiVer << "max:" << maxApiVer;
            break;
        }

        break;
    }
    default:
        qWarning() << "Unhandled response" << response.type << data;
    }
}

} // namespace mousr
