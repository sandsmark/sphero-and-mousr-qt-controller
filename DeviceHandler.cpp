#include <QMetaEnum>
#include <QDebug>

namespace EnumHelper {

template <typename T> static const char *toKey(const T val) {
    const QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(val);
}

template <typename T> static QString toString(const T val) {
    return QString::fromUtf8(EnumHelper::toKey(val));
}

}

#include "DeviceHandler.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>

// Fuck endiannes

template<typename T>
static inline T parseBytes(const char **data)
{
    T ret;
    memcpy(reinterpret_cast<char*>(&ret), reinterpret_cast<const void*>(*data), sizeof(T));
    *data += sizeof(T);

    return ret;
}

template<typename T>
static inline void setBytes(char **data, const T val)
{
    memcpy(reinterpret_cast<void*>(*data), reinterpret_cast<const char*>(&val), sizeof(T));
    *data += sizeof(T);
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, float arg1, float arg2, float arg3)
{
    qDebug() << " + Sending" << command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    QByteArray buffer(15, 0);
    char *bytes = buffer.data();

    bytes[0] = 48;
    bytes++;
    setBytes(&bytes, arg1);
    setBytes(&bytes, arg2);
    setBytes(&bytes, arg3);
    setBytes(&bytes, uint16_t(command));

    const size_t dataSize = bytes - buffer.data();
    const size_t expectedSize = 1 + sizeof(uint16_t) + 3 * sizeof(float);
    if (dataSize != expectedSize) {
        qWarning() << "Invalid buffer length" << dataSize  << expectedSize;
        return false;
    }
    qDebug() << "sending with 3 floats" << buffer.toHex() << buffer.size();

    //m_service->writeCharacteristic(m_writeCharacteristic, buffer, QLowEnergyService::WriteWithoutResponse);
    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    //m_service->readDescriptor(m_readDescriptor);
    //m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, uint32_t arg1, uint32_t arg2)
{
    qDebug() << " + Sending" << command;
    if (!isConnected()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    QByteArray buffer(15, 0);
    char *bytes = buffer.data();

    bytes[0] = 48;
    bytes++;
    setBytes(&bytes, arg1);
    setBytes(&bytes, arg2);

    size_t dataSize = bytes - buffer.data();
    bytes += 4;
    dataSize = bytes - buffer.data();
    setBytes(&bytes, uint16_t(command));

    dataSize = bytes - buffer.data();
    const size_t expectedSize = 15;//1 + sizeof(uint16_t) + 2 * sizeof(uint32_t);
    //qDebug() << "data size:" << dataSize << "expected:" << expectedSize;
    if (dataSize != expectedSize) {
        qWarning() << "Invalid buffer length" << dataSize  << expectedSize;
        return false;
    }
    //qDebug() << "writing" << buffer.toHex();
    //qDebug() << "sending with two ints" << buffer.toHex() << buffer.size();
    //qDebug() << uint8_t(command) << arg1 << arg2;
    //qDebug() << uint8_t(buffer[0]) << buffer.mid(0,1).toHex() << buffer.mid(1,4).toHex() << buffer.mid(5,4).toHex();
    //qDebug() << "command:" << buffer.mid(13, 2).toHex();

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);
    //m_service->writeCharacteristic(m_writeCharacteristic, buffer, QLowEnergyService::WriteWithoutResponse);

    //m_service->readDescriptor(m_readDescriptor);
    //m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, std::vector<char> data)
{
    qDebug() << " + Sending" << command;
    if (data.empty()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    if (data.size() <= 4) {
        data.resize(4, 0);
    } else if (data.size() != 12) {
        qWarning() << "invalid data length" << data.size();
        return false;
    }

    QByteArray buffer(15, 0);
    char *bytes = buffer.data();

    bytes[0] = 48;
    bytes++;
    //qDebug() << "data size:" << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    for (const char &byte : data) {
        setBytes(&bytes, byte);
        //qDebug() << "after adding byte:" << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    }
    bytes = buffer.data();
    bytes += 13;
    //qDebug() << "size before" << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    setBytes(&bytes, uint16_t(command));
    //qDebug() << "size after" << (uintptr_t(bytes) - uintptr_t(buffer.data()));

    //const uintptr_t dataSize = (uintptr_t(bytes) - uintptr_t(buffer.data()));
    //const size_t expectedSize = 1 + data.size() + 2;
    //if (dataSize != expectedSize) {
        //qWarning() << "Invalid buffer length" << dataSize << uintptr_t(bytes) << uintptr_t(buffer.data()) << data.size() << expectedSize;
//        return false;
    //}

    //qDebug() << "sending bytes" << buffer.toHex() << buffer.size();
    //qDebug() << uint8_t(command);
    //qDebug() << uint8_t(buffer[0]) << buffer.mid(0,1).toHex() << buffer.mid(1,4).toHex() << buffer.mid(5,4).toHex();
    //qDebug() << "command:" << buffer.mid(13, 2).toHex();

    m_service->writeCharacteristic(m_writeCharacteristic, buffer, QLowEnergyService::WriteWithoutResponse);
    //m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    //m_service->readDescriptor(m_readDescriptor);
    //m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

DeviceHandler::DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_name(deviceInfo.name())
{
    static_assert(sizeof(AutoplayConfig) == 15);
    static_assert(sizeof(Version) == 19);

    m_deviceController = QLowEnergyController::createCentral(deviceInfo, this);

    connect(m_deviceController, &QLowEnergyController::connected, m_deviceController, &QLowEnergyController::discoverServices);
    connect(m_deviceController, &QLowEnergyController::serviceDiscovered, this, &DeviceHandler::onServiceDiscovered);

    connect(m_deviceController, &QLowEnergyController::connectionUpdated, this, [](const QLowEnergyConnectionParameters &parms) {
            qDebug() << "controller connection updated, latency" << parms.latency() << "maxinterval:" << parms.maximumInterval() << "mininterval:" << parms.minimumInterval() << "supervision timeout" << parms.supervisionTimeout();
            });
    connect(m_deviceController, &QLowEnergyController::connected, this, []() {
            qDebug() << "controller connected";
            });
    connect(m_deviceController, &QLowEnergyController::disconnected, this, []() {
            qDebug() << "controller disconnected";
            });
    connect(m_deviceController, &QLowEnergyController::discoveryFinished, this, []() {
            qDebug() << "controller discovery finished";
            });
    connect(m_deviceController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this,
      [=](QLowEnergyController::Error newError){ qWarning() << "controller error:" << newError; });

    connect(m_deviceController, &QLowEnergyController::stateChanged, this, &DeviceHandler::onControllerStateChanged);

    m_deviceController->connectToDevice();

    if (m_deviceController->error() != QLowEnergyController::NoError) {
        qDebug() << "controller error:" << m_deviceController->error() << m_deviceController->errorString();
    }
}

DeviceHandler::~DeviceHandler()
{
    if (m_deviceController) {
        m_deviceController->disconnectFromDevice();
    } else {
        qWarning() << "no controller";
    }
}

bool DeviceHandler::isConnected()
{
    return m_deviceController && m_deviceController->state() != QLowEnergyController::UnconnectedState &&
            m_service && m_writeCharacteristic.isValid() && m_readCharacteristic.isValid();

}

QString DeviceHandler::statusString()
{
    const QString name = m_name.isEmpty() ? "device" : m_name;
    if (isConnected()) {
        return tr("Connected to %1").arg(name);
    } else if (m_deviceController && m_deviceController->state() == QLowEnergyController::UnconnectedState) {
        QTimer::singleShot(1000, this, &QObject::deleteLater);
        return tr("Failed to connect to %1").arg(name);
    } else {
        return tr("Connecting to %1...").arg(name);
    }
}

void DeviceHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    // 00001801-0000-1000-8000-00805f9b34fb
    static constexpr QUuid genericServiceUuid = {0x00001801, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
    if (newService == genericServiceUuid) {
        qDebug() << "Got generic service uuid, not sure what this is for" << newService;
        return;
    }

    // 0000fe59-0000-1000-8000-00805f9b34fb
    static constexpr QUuid dfuServiceUuid = {0x0000fe59, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
    if (newService == dfuServiceUuid) {
        // Read:         8ec90001-f315-4f60-9fb8-838830daea50
        // Write:        8ec90002-f315-4f60-9fb8-838830daea50
        qDebug() << "TODO: firmware update mode";
        return;
    }

    // Service UUID: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
    static constexpr QUuid serviceUuid    = {0x6e400001, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};
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

    connect(m_service, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::characteristicRead, this, &DeviceHandler::onCharacteristicRead);
    //connect(m_service, &QLowEnergyService::characteristicWritten, this, [](const QLowEnergyCharacteristic &descriptor, const QByteArray &newValue) {
    //        qDebug() << "Characteristic" << descriptor.uuid() << "wrote" << newValue;
    //        });;

    connect(m_service, &QLowEnergyService::descriptorRead, this, &DeviceHandler::onDescriptorRead);
    //connect(m_service, &QLowEnergyService::descriptorWritten, this, [](const QLowEnergyDescriptor &descriptor, const QByteArray &newValue) {
    //        qDebug() << "Descriptor" << descriptor.uuid() << "wrote" << newValue;
    //        });;

    connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &DeviceHandler::onServiceError);
    connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::onServiceStateChanged);

    m_service->discoverDetails();
}

void DeviceHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
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
//    qDebug() << "read characteristic" << m_readCharacteristic.name() << m_readCharacteristic.uuid() << m_readCharacteristic.properties();
    m_writeCharacteristic = m_service->characteristic(writeUuid);
//    qDebug() << "write characteristic" << m_writeCharacteristic.name() << m_writeCharacteristic.uuid() << m_writeCharacteristic.properties();
    if (!m_readCharacteristic.descriptors().isEmpty()) {
        m_readDescriptor = m_readCharacteristic.descriptors().first();
    }

//    qDebug() << "read descriptors descriptor count:" << m_readCharacteristic.descriptors().count();
//    for (const QLowEnergyDescriptor &desc : m_readCharacteristic.descriptors()) {
//        qDebug() << "read descriptor name:" << desc.name() << desc.uuid() << desc.value();
//    }
//    qDebug() << "write characteristic descriptor count:" << m_writeCharacteristic.descriptors().count();
//    for (const QLowEnergyDescriptor &desc : m_writeCharacteristic.descriptors()) {
//        qDebug() << "write descripto namer:" << desc.name() << desc.uuid() << desc.value();
//    }

    if (!isConnected()) {
        qDebug() << "Finished scanning, but not valid";
        emit disconnected();
        return;
    }

    qDebug() << "Successfully connected";

    // Who the _fuck_ designed this API, requiring me to write magic bytes to a
    // fucking read descriptor to get characteristicChanged to work?
    m_service->writeDescriptor(m_readDescriptor, QByteArray::fromHex("0100"));

//    qDebug() << "Requested notifications on values changes";

    //if (!sendCommand(Command::InitializeDevice, QDateTime::currentSecsSinceEpoch(), mbApiVersion)) {
    if (!sendCommand(Command::InitializeDevice, mbApiVersion, QDateTime::currentSecsSinceEpoch())) {
        qWarning() << "Failed to send init command";
    }

    QTimer::singleShot(2000, this, &DeviceHandler::chirp);

    emit connectedChanged();
}

void DeviceHandler::chirp()
{
    if (!sendCommand(Command::Chirp, {0, 6, 0 ,0})) {
        qWarning() << "Failed to request chirp";
    }
}

void DeviceHandler::pause()
{
    if (!sendCommand(Command::Stop, {0, 0, 0 ,0})) {
        qWarning() << "Failed to request stop";
    }
}

void DeviceHandler::resume()
{
    sendCommand(Command::Stop, m_speed, m_held, m_angle);
}

void DeviceHandler::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::UnconnectedState) {
        qWarning() << "Disconnected";
    }

    emit connectedChanged();
}


void DeviceHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    qWarning() << "Service error:" << error;
    if (error == QLowEnergyService::NoError) {
        return;
    }

    emit disconnected();
}

void DeviceHandler::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (characteristic != m_readCharacteristic) {
        qWarning() << "data from unexpected characteristic" << characteristic.uuid() << data;
        return;
    }
    qDebug() << "data from read characteristic" << data.toHex();
    qDebug() << "data type:" << data[0];

    emit dataRead(data);
}

void DeviceHandler::onDescriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &data)
{

    if (descriptor != m_readDescriptor) {
        qWarning() << "data from unexpected descriptor" << descriptor.uuid() << data;
        return;
    }
    if (data.isEmpty()) {
        qWarning() << "got empty data";
        return;
    }
    qDebug() << "data from read descriptor" << data;
    qDebug() << "data type:" << int(data[0]);

    emit dataRead(data);
}

void DeviceHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (characteristic != m_readCharacteristic) {
        qWarning() << "changed from unexpected characteristic" << characteristic.uuid() << data;
        return;
    }
    if (data.isEmpty()) {
        qWarning() << "Empty characteristic?";
    }

    if (data.size() != 20) {
        qWarning() << "invalid packet size" << data.size() << "expected 20";
        return;
    }
    uint8_t command = uint8_t(data[0]);
    QString resp = EnumHelper::toString(Response(command));

    if (resp.isEmpty()) {
        qDebug() << "Unknown command";
        qDebug() << command << data;
        return;
    }

    const char *bytes = data.constData();
    Response type = Response(*bytes++);
    //qDebug() << " - " << type;

    switch(type){
    case DeviceOrientation: {
        // I just assume the order here, haven't bothered testing
        m_rotX = parseBytes<float>(&bytes);
        m_rotY = parseBytes<float>(&bytes);
        m_rotZ = parseBytes<float>(&bytes);
        m_isFlipped = parseBytes<bool>(&bytes);

        emit orientationChanged();

        break;
    }
    case BatteryVoltage:{
        // Voltage seems to be percent? wtf
        m_voltage = parseBytes<uint8_t>(&bytes);
        m_batteryLow = parseBytes<bool>(&bytes);

        m_charging = parseBytes<bool>(&bytes);
        m_fullyCharged = parseBytes<bool>(&bytes);

        m_autoRunning = parseBytes<bool>(&bytes);

        m_memory = parseBytes<uint16_t>(&bytes);

        emit powerChanged();
        emit autoRunningChanged();

        break;
    }
    case CrashLogString: {
        const QString crashLog = QString::fromUtf8(data.right(data.length() - 1));
        if (crashLog != "No crash log.") {
            qDebug() << " Crash log string:" << crashLog;
        }
        break;
    }
    case CrashLogFinished:
        qDebug() << type;
        break;
    case AnalyticsBegin: {
        int numberOfEntries = parseBytes<uint8_t>(&bytes);
        qDebug() << "Number of analytics entries:" << numberOfEntries;
        break;
    }
    case SensorDirty: {
        m_sensorDirty = parseBytes<bool>(&bytes);
        emit sensorDirtyChanged();
        break;
    }
    case AutoModeChanged: {
        qDebug() << "auto mode changed";
        static const int requiredSize = 1 + sizeof(AutoplayConfig);
        if (data.size() < requiredSize) {
            qWarning() << "Not enough data in packet, require" << requiredSize << "got" << data.size();
            break;
        }
        m_autoplay = parseBytes<AutoplayConfig>(&bytes);
        //qDebug() << "autoplay, enabled:" << m_autoplay.enabled << "surface" << m_autoplay.surface << "tail:" << m_autoplay.tail << "gamemode:" << m_autoplay.gameMode << "playmode" << m_autoplay.playMode;
        qDebug() << "enabled" << m_autoplay.enabled;
        qDebug() << "surface" << m_autoplay.surface;
        qDebug() << "tail" << m_autoplay.tail;
        qDebug() << "speed" << m_autoplay.speed;
        qDebug() << "gameMode" << m_autoplay.gameMode;
        qDebug() << "playMode" << m_autoplay.playMode;
        qDebug() << "pauseFrequency" << m_autoplay.pauseFrequency;
        qDebug() << "confinedOrPauseTime" << m_autoplay.confinedOrPauseTime;
        qDebug() << "pauseLengthOrBackup" << m_autoplay.pauseLengthOrBackUp;
        qDebug() << "allDay" << m_autoplay.allDay;

        qDebug() << "unknown1" << m_autoplay.unknown1;
        qDebug() << "unknown2" << m_autoplay.unknown2;
        qDebug() << "unknown3" << m_autoplay.unknown3;
        qDebug() << "response type" << m_autoplay.response;
        qDebug() << "unknown4" << m_autoplay.unknown4;
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
        static const int requiredSize = 1 + sizeof(Version);
        if (data.size() < requiredSize) {
            qWarning() << "Not enough data in packet for version, require" << requiredSize << "got" << data.size();
            break;
        }

        m_version = parseBytes<Version>(&bytes);

        qDebug() << "Firmware mode:" << m_version.firmwareType;
        qDebug().noquote() << "Version" << (QByteArray::number(m_version.major) + "." + QByteArray::number(m_version.minor) + "." + QByteArray::number(m_version.commitNumber) + "-" + QByteArray(m_version.commitHash, 4).toHex());
        qDebug() << "Mousr version" << m_version.mousrVersion << "hardware version" << m_version.hardwareVersion << "bootloader version" << m_version.bootloaderVersion;
        break;
    }
    case Nack: {
        Command command = parseBytes<Command>(&bytes);
        uint32_t unknown = parseBytes<uint8_t>(&bytes);
        uint32_t currentApiVer = parseBytes<uint32_t>(&bytes);
        uint32_t minApiVer = parseBytes<uint32_t>(&bytes);
        uint32_t maxApiVer = parseBytes<uint32_t>(&bytes);
        qWarning() << "!! Got NACK for command" << command;
        qDebug() << "unknown num:" << unknown;
        qDebug() << "Api version current:" << currentApiVer << "min:" << minApiVer << "max:" << maxApiVer;
        break;
    }
    default:
        qWarning() << "Unhandled response" << type << data;
    }
}
