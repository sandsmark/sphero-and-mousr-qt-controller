#include "devicediscoverer.h"

#include "DeviceHandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QDebug>
#include <QQmlEngine>

DeviceDiscoverer::DeviceDiscoverer(QObject *parent) :
    QObject(parent)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    m_adapter = new QBluetoothLocalDevice(this);

    connect(m_adapter, &QBluetoothLocalDevice::error, this, &DeviceDiscoverer::onAdapterError);
    connect(m_adapter, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceDiscoverer::startScanning);

    // I hate these overload things..
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceDiscoverer::onAgentError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceDiscoverer::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this, [](const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields){
        qDebug() << "updated:" << info.name() << info.rssi() << updatedFields;
    });
    connect(this, &DeviceDiscoverer::deviceFound, this, &DeviceDiscoverer::stopScanning);

    QMetaObject::invokeMethod(this, &DeviceDiscoverer::startScanning);
}

DeviceDiscoverer::~DeviceDiscoverer()
{
    stopScanning();
}

QObject *DeviceDiscoverer::device()
{
    return m_device.data();
}

QString DeviceDiscoverer::statusString()
{
    if (QBluetoothLocalDevice::allDevices().isEmpty()) {
        return tr("No bluetooth adaptors available");
    }

    if (m_adapterError != QBluetoothLocalDevice::NoError) {
        switch (m_adapterError) {
        case QBluetoothLocalDevice::PairingError:
            return tr("Failed while pairing with device");
        case QBluetoothLocalDevice::UnknownError:
            return tr("Problem with bluetooth device");
        default:
            break;
        }
    }
    if (m_adapter->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        return tr("Bluetooth turned off");
    }

    if (m_discoveryAgent->error() != QBluetoothDeviceDiscoveryAgent::NoError) {
        return tr("Searching error: %1").arg(m_discoveryAgent->errorString());
    }

    if (m_discoveryAgent->isActive()) {
        return tr("Looking for device...");
    }

    return QString();
}

void DeviceDiscoverer::startScanning()
{
    qDebug() << "Starting scan requested";

    if (m_adapter->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        qDebug() << "Powering on adapter";
        m_adapter->powerOn();
    } else {
        qDebug() << "Starting scan";
        m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }

    emit statusStringChanged();
}

void DeviceDiscoverer::stopScanning()
{
    qDebug() << "Stopping scan";

    m_discoveryAgent->stop();
    emit statusStringChanged();
}

void DeviceDiscoverer::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.name() != "Mousr") {
        return;
    }

    qDebug() << "Found Mousr";
    m_device = new DeviceHandler(device, this);
    emit deviceFound();
    connect(m_device, &QObject::destroyed, this, &DeviceDiscoverer::startScanning);
    return;
}

void DeviceDiscoverer::onAgentError()
{
    qDebug() << "agent error" << m_discoveryAgent->errorString();

    if (m_discoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::PoweredOffError && m_adapterError != QBluetoothLocalDevice::NoError) {
        qDebug() << "Device powered off, trying to power on";
        m_adapter->powerOn();
    }

    emit statusStringChanged();
}

void DeviceDiscoverer::onAdapterError(const QBluetoothLocalDevice::Error error)
{
    qWarning() << "adapter error" << error;
    m_adapterError = error;
    emit statusStringChanged();
}
