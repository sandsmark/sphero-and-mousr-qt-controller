#include "DeviceHandler.h"

#include <QLowEnergyController>
#include <QTimer>
#include <QDateTime>

template<typename T>
static inline void setBytes(char **data, const T val)
{
    memcpy(reinterpret_cast<void*>(*data), reinterpret_cast<const char*>(&val), sizeof(T));
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

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    m_service->readDescriptor(m_readDescriptor);
    m_service->readCharacteristic(m_readCharacteristic);

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
    setBytes(&bytes, uint16_t(command));

    const size_t dataSize = bytes - buffer.data();
    const size_t expectedSize = 1 + sizeof(uint16_t) + 2 * sizeof(uint32_t);
    qDebug() << dataSize << expectedSize;
    if (dataSize != expectedSize) {
        qWarning() << "Invalid buffer length" << dataSize  << expectedSize;
        return false;
    }

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    m_service->readDescriptor(m_readDescriptor);
    m_service->readCharacteristic(m_readCharacteristic);

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
    qDebug() << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    for (const char &byte : data) {
        setBytes(&bytes, byte);
        qDebug() << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    }
    qDebug() << (uintptr_t(bytes) - uintptr_t(buffer.data()));
    setBytes(&bytes, uint16_t(command));
    qDebug() << (uintptr_t(bytes) - uintptr_t(buffer.data()));

    const uintptr_t dataSize = (uintptr_t(bytes) - uintptr_t(buffer.data()));
    const size_t expectedSize = 1 + data.size() + 2;
    if (dataSize != expectedSize) {
        qWarning() << "Invalid buffer length" << dataSize << uintptr_t(bytes) << uintptr_t(buffer.data()) << data.size() << expectedSize;
//        return false;
    }

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    m_service->readDescriptor(m_readDescriptor);
    m_service->readCharacteristic(m_readCharacteristic);

    return true;
}

DeviceHandler::DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent)
{
    qDebug() << deviceInfo.name() << deviceInfo.deviceUuid();
    m_deviceController = QLowEnergyController::createCentral(deviceInfo, this);

    m_deviceController->connectToDevice();

    connect(m_deviceController, &QLowEnergyController::serviceDiscovered, this, &DeviceHandler::onServiceDiscovered);
    connect(m_deviceController, &QLowEnergyController::connected, m_deviceController, &QLowEnergyController::discoverServices);

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

    qDebug() << "Found correct service";
    if (m_service) {
        m_service->deleteLater();
    }

    m_service = m_deviceController->createServiceObject(newService, this);
    qDebug()  << m_service->serviceName() << m_service->serviceUuid();

    connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &DeviceHandler::onServiceError);
    connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::onCharacteristicChanged);

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
        qDebug() << "characteristic:" << c.name() << c.uuid();
    }

    m_readCharacteristic = m_service->characteristic(readUuid);
    m_writeCharacteristic = m_service->characteristic(writeUuid);
    if (!m_readCharacteristic.descriptors().isEmpty()) {
        m_readDescriptor = m_readCharacteristic.descriptors().first();
    }

    qDebug() << m_readCharacteristic.descriptors().count();
    for (const QLowEnergyDescriptor &desc : m_readCharacteristic.descriptors()) {
        qDebug() << desc.name() << desc.uuid() << desc.value();
    }
    qDebug() << m_writeCharacteristic.descriptors().count();
    for (const QLowEnergyDescriptor &desc : m_writeCharacteristic.descriptors()) {
        qDebug() << desc.name() << desc.uuid() << desc.value();
    }

    if (!isConnected()) {
        qDebug() << "Finished scanning, but not valid";
        emit disconnected();
        return;
    }

    qDebug() << "Successfully connected";

    connect(m_service, &QLowEnergyService::characteristicRead, this, &DeviceHandler::onDataRead);
    connect(m_service, &QLowEnergyService::descriptorRead, this, &DeviceHandler::onDescriptorRead);

    if (!sendCommand(Command::InitializeDevice, mbApiVersion, QDateTime::currentSecsSinceEpoch())) {
        qWarning() << "Failed to send init command";
    }

    QTimer::singleShot(2000, [=]() {
        if (!sendCommand(Command::Chirp, {0, 6, 0 ,0})) {
            qWarning() << "Failed to send data bytes";
        }
        qDebug() << "Asked for chirp";
    });
    m_service->readDescriptor(m_readDescriptor);
//    m_service->readCharacteristic(m_readCharacteristic);

    emit connectedChanged();
}

void DeviceHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    if (error == QLowEnergyService::NoError) {
        return;
    }
    qWarning() << "Service error:" << error;

    emit disconnected();
}

void DeviceHandler::onDataRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    if (characteristic != m_readCharacteristic) {
        qWarning() << "Unexpected data from characteristic" << characteristic.uuid() << data;
//        return;
    }
    qDebug() << "data from characteristic" << characteristic.uuid() << data;
    static int attempt = 0;
//    if (++attempt < 3) {
//        m_service->readCharacteristic(m_readCharacteristic);
//    }

    emit dataRead(data);
}

void DeviceHandler::onDescriptorRead(const QLowEnergyDescriptor &characteristic, const QByteArray &data)
{

    if (characteristic != m_readDescriptor) {
        qWarning() << "Unexpected data from descriptor" << characteristic.uuid() << data;
//        return;
    }
    qDebug() << "data from descriptor" << characteristic.uuid() << data;
    static int attempt = 0;
//    if (++attempt < 3) {
//        m_service->readDescriptor(m_readDescriptor);
//    }

    emit dataRead(data);
}

void DeviceHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    qDebug() << "characteristic" << characteristic.uuid() << "changed" << newValue;

}
