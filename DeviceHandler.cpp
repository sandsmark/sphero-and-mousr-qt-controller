#include "DeviceHandler.h"

#include <QLowEnergyController>

template<typename T>
static inline void setBytes(char **data, const T val)
{
    memcpy(reinterpret_cast<void*>(*data), reinterpret_cast<const char*>(&val), sizeof(T));
    data += sizeof(T);
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

    return true;
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, const char arg1, const char arg2, const char arg3, const char arg4)
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
    setBytes(&bytes, arg4);
    setBytes(&bytes, uint16_t(command));

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);

    return true;
}

bool DeviceHandler::sendCommand(const DeviceHandler::Command command, QByteArray data)
{
    if (data.isEmpty()) {
        qWarning() << "trying to send when unconnected";
        return false;
    }

    if (data.length() != 2 && data.length() < 4) {
        data = data.leftJustified(4, 0);
    } else if (data.length() != 2) {
        qWarning() << "invalid data length" << data.length();
        return false;
    }

    QByteArray buffer(15, 0);
    char *bytes = buffer.data();

    bytes[0] = 48;
    bytes++;
    for (const char &byte : data) {
        setBytes(&bytes, byte);
    }
    setBytes(&bytes, uint16_t(command));

    const size_t dataSize = bytes - buffer.data();
    const size_t expectedSize = 1 + data.size() + 2;
    if (dataSize != expectedSize) {
        qWarning() << "Invalid buffer length" << dataSize << expectedSize;
        return false;
    }

    m_service->writeCharacteristic(m_writeCharacteristic, buffer);
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

    m_readCharacteristic = m_service->characteristic(readUuid);
    m_writeCharacteristic = m_service->characteristic(writeUuid);

    if (!isConnected()) {
        qDebug() << "Finished scanning, but not valid";
        emit disconnected();
        return;
    }

    qDebug() << "Successfully connected";

    connect(m_service, &QLowEnergyService::characteristicRead, this, &DeviceHandler::onDataRead);
    sendCommand(Command::Chirp, 0, 6, 0 ,0);

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
        return;
    }
    qDebug() << "data from characteristic" << characteristic.uuid() << data;

    emit dataRead(data);
}
