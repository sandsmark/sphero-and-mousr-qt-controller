#include "devicediscoverer.h"

#include "DeviceHandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QDebug>

DeviceDiscoverer::DeviceDiscoverer(QObject *parent) :
    QObject(parent)
{
    m_adapter = new QBluetoothLocalDevice(this);
    qDebug() << "Creating discovery agent";
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);


    m_adapterPoweredOn = m_adapter->hostMode() != QBluetoothLocalDevice::HostPoweredOff;
    qDebug() << "Power on?" << m_adaperError;

    // I hate these overload things..
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceDiscoverer::onAgentError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceDiscoverer::onDeviceDiscovered);
    connect(m_adapter, &QBluetoothLocalDevice::error, this, &DeviceDiscoverer::onAdapterError);
    connect(m_adapter, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceDiscoverer::onAdapterStateChanged);

    connect(this, &DeviceDiscoverer::deviceFound, this, &DeviceDiscoverer::stopScanning);
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

    if (m_adaperError != QBluetoothLocalDevice::NoError) {
        switch (m_adaperError) {
        case QBluetoothLocalDevice::PairingError:
            return tr("Failed while pairing with device");
        case QBluetoothLocalDevice::UnknownError:
            return tr("Problem with bluetooth device");
        default:
            break;
        }
    }
    if (!m_adapterPoweredOn) {
        return tr("Bluetooth turned off");
    }

    if (m_discoveryAgent->error() != QBluetoothDeviceDiscoveryAgent::NoError) {
        return tr("Searching error: %1").arg(m_discoveryAgent->errorString());
    }

    if (m_attemptingScan) {
        return tr("Looking for device...");
    }

    return QString();
}

void DeviceDiscoverer::startScanning()
{
    qDebug() << "Starting scan requested" << m_attemptingScan;

    m_attemptingScan = true;

    if (m_adapterPoweredOn) {
        qDebug() << "Starting scan";
        m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    } else {
        qDebug() << "Powering on adapter";
        m_adapter->powerOn();
    }

    emit statusStringChanged();
}

void DeviceDiscoverer::stopScanning()
{
    qDebug() << "Stopping scan";

    m_attemptingScan = false;

    m_discoveryAgent->stop();

    emit statusStringChanged();
}

void DeviceDiscoverer::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.name() == "Mousr") {
        qDebug() << "Found Mousr";
        m_device = new DeviceHandler(device, this);
        stopScanning();
        emit deviceFound();
        return;
    }
}

void DeviceDiscoverer::onDeviceUpdated(const QBluetoothDeviceInfo &device)
{
    if (device.name() == "Mousr") {
        qDebug() << "Found Mousr";
        emit deviceFound();
        return;
    }

    qDebug() << "Updated" << device.name();
}

void DeviceDiscoverer::onAgentError()
{
    qDebug() << "agent error" << m_discoveryAgent->errorString();

    if (m_discoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::PoweredOffError && m_adaperError != QBluetoothLocalDevice::NoError) {
        if (m_attemptingScan) {
            qDebug() << "Device powered off, trying to power on";
            m_adapter->powerOn();
        }
        return;
    }

    emit statusStringChanged();
}

void DeviceDiscoverer::onAdapterError(const QBluetoothLocalDevice::Error error)
{
    qDebug() << "Adapter error:" << error;

    m_adaperError = error;
    emit statusStringChanged();
}

void DeviceDiscoverer::onAdapterStateChanged(const QBluetoothLocalDevice::HostMode mode)
{
    qDebug() << "Adapter state changed:" << mode;

    if (mode == QBluetoothLocalDevice::HostPoweredOff) {
        m_adapterPoweredOn = false;
        m_discoveryAgent->stop();
    } else {
        m_adapterPoweredOn = true;
    }

    emit statusStringChanged();

    if (m_attemptingScan) {
        qDebug() << "Attempting scan";
        startScanning();
    }
}
