#include "DeviceHandler.h"

#include <QLowEnergyController>

DeviceHandler::DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_serviceUuid(QLatin1String("6e400001-b5a3-f393-e0a9-e50e24dcca9e")),
    m_writeUuid(QLatin1String("6e400002-b5a3-f393-e0a9-e50e24dcca9e")),
    m_readUuid(QLatin1String("6e400003-b5a3-f393-e0a9-e50e24dcca9e"))

{
    m_deviceController = QLowEnergyController::createCentral(deviceInfo, this);

    m_deviceController->connectToDevice();
//    m_deviceController->discoverServices();
//    qDebug() << m_deviceController->services().count() << m_deviceController->createServiceObject(m_serviceUuid);

    connect(m_deviceController, &QLowEnergyController::serviceDiscovered, this, &DeviceHandler::onServiceDiscovered);
    connect(m_deviceController, &QLowEnergyController::connected, m_deviceController, &QLowEnergyController::discoverServices);

}

bool DeviceHandler::isValid()
{
    return m_service && m_writeCharacteristic.isValid() && m_readCharacteristic.isValid();

}

QString DeviceHandler::statusString()
{
    if (isValid()) {
        return tr("Found device");
    } else {
        return tr("Connecting to device...");
    }
}

void DeviceHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    if (newService != m_serviceUuid) {
        // TODO: device firmware upgrade
        // Service UUID: 0000fe59-0000-1000-8000-00805f9b34fb
        // Read: 8ec90001-f315-4f60-9fb8-838830daea50
        // 8ec90002-f315-4f60-9fb8-838830daea50

        qDebug() << "discovered unhandled service" << newService << m_serviceUuid;
        return;
    }

    qDebug() << "Was correct service";
    if (m_service) {
        m_service->deleteLater();
    }

    m_service = m_deviceController->createServiceObject(newService, this);
    connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
            [=](QLowEnergyService::ServiceError newError){ qDebug() << newError; });

    connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::onServiceStateChanged);

    m_service->discoverDetails();
}

void DeviceHandler::onCharacteristicsChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{

    if (characteristic.uuid() == m_readUuid) {
        qDebug() << "Got read characteristic";
        m_readCharacteristic = characteristic;
    } else if (characteristic.uuid() == m_writeUuid) {
        m_writeCharacteristic = characteristic;
        qDebug() << "Got read characteristic";
    } else {
        qDebug() << "unexpected characteristic change" << characteristic.name() << characteristic.uuid() << newValue;
    }
}

void DeviceHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::ServiceDiscovered) {
        qDebug() << "service state changed:" << newState;
        qDebug() << m_service->characteristics().count();
        return;
    }

    for (const QLowEnergyCharacteristic &characteristic : m_service->characteristics()) {
        onCharacteristicsChanged(characteristic, "Finished scanning");
    }

    if (!isValid()) {
        qDebug() << "Finished scanning, but not valid";
        this->deleteLater();
    }

    emit validChanged();
}
