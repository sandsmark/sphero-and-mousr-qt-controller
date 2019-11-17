#include "devicediscoverer.h"

#include "mousr/MousrHandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QDebug>
#include <QQmlEngine>

DeviceDiscoverer::DeviceDiscoverer(QObject *parent) :
    QObject(parent),
    m_scanning(false)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    m_adapter = new QBluetoothLocalDevice(this);

    connect(m_adapter, &QBluetoothLocalDevice::error, this, &DeviceDiscoverer::onAdapterError);
    connect(m_adapter, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceDiscoverer::startScanning);

    // I hate these overload things..
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceDiscoverer::onAgentError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceDiscoverer::onDeviceDiscovered);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this, [=](const QBluetoothDeviceInfo &, QBluetoothDeviceInfo::Fields ){
        if (m_restartScanTimer.isActive()) {
            qDebug() << " - Restarting scan timer";
            m_restartScanTimer.start();
        }
    });
#endif
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, [=]() {
        qDebug() << "discovery finihsed";
        if (m_scanning) {
            qDebug() << " - Starting timer";
            m_restartScanTimer.start();
        }
    });
    m_restartScanTimer.setInterval(5000);
    m_restartScanTimer.setSingleShot(true);
    connect(&m_restartScanTimer, &QTimer::timeout, this, [=]() {
        if (m_scanning) {
            qDebug() << " ! Restarting scan";
            m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        }

    });

    connect(this, &DeviceDiscoverer::deviceFound, this, &DeviceDiscoverer::stopScanning);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(100);

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
    if (!m_adapter->isValid()) {
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
    if (m_adapter->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        qDebug() << "Powering on adapter";
        m_adapter->powerOn();
    } else if (m_scanning) {
        qDebug() << "Already scanning";
        return;
    }

    qDebug() << "Starting scan";
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    m_scanning = true;

    emit statusStringChanged();
}

void DeviceDiscoverer::stopScanning()
{
    qDebug() << "Stopping scan";

    m_scanning = false;
    m_restartScanTimer.stop();
    m_discoveryAgent->stop();
    emit statusStringChanged();
}

void DeviceDiscoverer::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.name() != "Mousr") {
        return;
    }

    qDebug() << "Found Mousr";
    m_device = new mousr::MousrHandler(device, this);
    emit deviceFound();
    connect(m_device, &mousr::MousrHandler::disconnected, this, &DeviceDiscoverer::startScanning);
    return;
}

void DeviceDiscoverer::onDeviceDisconnected()
{
    if (m_device) {
        m_device->deleteLater();
        disconnect(m_device, nullptr, this, nullptr);
        m_device = nullptr;
    } else {
        qWarning() << "device disconnected, but is not set?";
    }
    startScanning();
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
