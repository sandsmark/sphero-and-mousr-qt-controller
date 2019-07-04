#include "DeviceHandler.h"

#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QDateTime>
#include <QtEndian>

template<typename T>
static inline void setBytes(char **data, const T val)
{
    //qToBigEndian<T>(reinterpret_cast<const T*>(&val), 1, reinterpret_cast<T*>(*data));
    //qToBigEndian<T>(reinterpret_cast<void*>(*data), 1, reinterpret_cast<const char*>(&val));
    memcpy(reinterpret_cast<void*>(*data), reinterpret_cast<const char*>(&val), sizeof(T));
    //qToBigEndian(data,
    *data += sizeof(T);
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, float arg1, float arg2, float arg3)
{
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
    qDebug() << "sending with two ints" << buffer.toHex() << buffer.size();
    qDebug() << uint8_t(command) << arg1 << arg2;
    qDebug() << uint8_t(buffer[0]) << buffer.mid(0,1).toHex() << buffer.mid(1,4).toHex() << buffer.mid(5,4).toHex();
    qDebug() << "command:" << buffer.mid(13, 2).toHex();

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);
    //m_service->writeCharacteristic(m_writeCharacteristic, buffer, QLowEnergyService::WriteWithoutResponse);

    //m_service->readDescriptor(m_readDescriptor);
    //m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, std::vector<char> data)
{
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
        qDebug() << "after adding byte:" << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    }
    bytes = buffer.data();
    bytes += 13;
    qDebug() << "size before" << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    setBytes(&bytes, uint16_t(command));
    //qDebug() << "size after" << (uintptr_t(bytes) - uintptr_t(buffer.data()));

    const uintptr_t dataSize = (uintptr_t(bytes) - uintptr_t(buffer.data()));
    const size_t expectedSize = 1 + data.size() + 2;
    //if (dataSize != expectedSize) {
        //qWarning() << "Invalid buffer length" << dataSize << uintptr_t(bytes) << uintptr_t(buffer.data()) << data.size() << expectedSize;
//        return false;
    //}

    qDebug() << "sending bytes" << buffer.toHex() << buffer.size();
    qDebug() << uint8_t(command);
    qDebug() << uint8_t(buffer[0]) << buffer.mid(0,1).toHex() << buffer.mid(1,4).toHex() << buffer.mid(5,4).toHex();
    qDebug() << "command:" << buffer.mid(13, 2).toHex();

    m_service->writeCharacteristic(m_writeCharacteristic, buffer, QLowEnergyService::WriteWithoutResponse);
    //m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    //m_service->readDescriptor(m_readDescriptor);
    //m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

DeviceHandler::DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent)
{
    qDebug() << "device name:" << deviceInfo.name() << "device uuid" << deviceInfo.deviceUuid();
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

    connect(m_deviceController, &QLowEnergyController::stateChanged, this, [](QLowEnergyController::ControllerState state) {
            qDebug() << "controller state changed" << state;
            });

    m_deviceController->connectToDevice();

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
    return m_service && m_writeCharacteristic.isValid() && m_readCharacteristic.isValid();

}

QString DeviceHandler::statusString()
{
    if (isConnected()) {
        return tr("Connected to device");
    } else {
        return tr("Connecting to device...");
    }
}

void DeviceHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    if (newService != serviceUuid) {
        // TODO: device firmware upgrade
        // Service UUID: 0000fe59-0000-1000-8000-00805f9b34fb
        // Read:         8ec90001-f315-4f60-9fb8-838830daea50
        // Write:        8ec90002-f315-4f60-9fb8-838830daea50

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
    connect(m_service, &QLowEnergyService::characteristicWritten, this, [](const QLowEnergyCharacteristic &descriptor, const QByteArray &newValue) {
            qDebug() << "Characteristic" << descriptor.uuid() << "wrote" << newValue;
            });;

    connect(m_service, &QLowEnergyService::descriptorRead, this, &DeviceHandler::onDescriptorRead);
    connect(m_service, &QLowEnergyService::descriptorWritten, this, [](const QLowEnergyDescriptor &descriptor, const QByteArray &newValue) {
            qDebug() << "Descriptor" << descriptor.uuid() << "wrote" << newValue;
            });;

    connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &DeviceHandler::onServiceError);
    connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::onServiceStateChanged);

    m_service->discoverDetails();
}

void DeviceHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::InvalidService) {
        qWarning() << "Got invalid service";
        emit disconnected();
        return;
    }

    if (newState == QLowEnergyService::DiscoveringServices) {
        qDebug() << "Requesting services";
    }


    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << "unhandled service state changed:" << newState;
        return;
    }

    for (const QLowEnergyCharacteristic &c : m_service->characteristics()) {
        qDebug() << "characteristic available:" << c.name() << c.uuid() << c.properties();
    }

    m_readCharacteristic = m_service->characteristic(readUuid);
    m_writeCharacteristic = m_service->characteristic(writeUuid);
    if (!m_readCharacteristic.descriptors().isEmpty()) {
        m_readDescriptor = m_readCharacteristic.descriptors().first();
    }

    qDebug() << "read descriptors descriptor count:" << m_readCharacteristic.descriptors().count();
    for (const QLowEnergyDescriptor &desc : m_readCharacteristic.descriptors()) {
        qDebug() << "read descriptor name:" << desc.name() << desc.uuid() << desc.value();
    }
    qDebug() << "write characteristic descriptor count:" << m_writeCharacteristic.descriptors().count();
    for (const QLowEnergyDescriptor &desc : m_writeCharacteristic.descriptors()) {
        qDebug() << "write descripto namer:" << desc.name() << desc.uuid() << desc.value();
    }

    if (!isConnected()) {
        qDebug() << "Finished scanning, but not valid";
        emit disconnected();
        return;
    }

    qDebug() << "Successfully connected";

    //if (!sendCommand(Command::InitializeDevice, mbApiVersion, QDateTime::currentSecsSinceEpoch())) {
    if (!sendCommand(Command::InitializeDevice, mbApiVersion, QDateTime::currentSecsSinceEpoch())) {
        qWarning() << "Failed to send init command";
    }

    QTimer::singleShot(2000, [=]() {
        if (!sendCommand(Command::Chirp, {0, 6, 0 ,0})) {
            qWarning() << "Failed to send data bytes";
        }
        qDebug() << "Asked for chirp";
    });
    //m_service->readDescriptor(m_readDescriptor);
//    m_service->readCharacteristic(m_readCharacteristic);

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

void DeviceHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    if (characteristic != m_readCharacteristic) {
        qWarning() << "changed from unexpected characteristic" << characteristic.uuid() << newValue;
        return;
    }
    qDebug() << "read characteristic changed" << newValue;
}
