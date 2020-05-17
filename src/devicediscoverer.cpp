#include "devicediscoverer.h"

#include "mousr/MousrHandler.h"
#include "sphero/SpheroHandler.h"

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

    // I hate these overload things..
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceDiscoverer::onAgentError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceDiscoverer::onDeviceDiscovered, Qt::QueuedConnection);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this, [this](const QBluetoothDeviceInfo &, QBluetoothDeviceInfo::Fields ){
        if (m_restartScanTimer.isActive()) {
            qDebug() << " - Restarting scan timer";
            m_restartScanTimer.start();
        }
    });
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, [this]() {
        qDebug() << "discovery finihsed";
        if (m_scanning) {
            qDebug() << " - Starting timer";
            m_restartScanTimer.start();
        }
    });
    m_restartScanTimer.setInterval(5000);
    m_restartScanTimer.setSingleShot(true);
    connect(&m_restartScanTimer, &QTimer::timeout, this, [this]() {
        if (m_scanning) {
            qDebug() << " ! Restarting scan";
            m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        }

    });

    m_discoveryAgent->setLowEnergyDiscoveryTimeout(100);

    if (m_adapter->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        connect(m_adapter, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceDiscoverer::startScanning);
        m_adapter->powerOn();
    } else {
        QMetaObject::invokeMethod(this, &DeviceDiscoverer::startScanning);
    }
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

    if (m_scanning) {
        return tr("Looking for device...");
    }

    return QString();
}

void DeviceDiscoverer::startScanning()
{
    if (m_scanning) {
        qDebug() << "Already scanning";
        return;
    }

    m_scanning = true;
    disconnect(m_adapter, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceDiscoverer::startScanning);

    qDebug() << "Starting scan";
    // This might immediately lead to the other things getting called, just fyi
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

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
    if (m_device) {
        qWarning() << "already have device, skipping" << device.name();
        return;
    }

    if (device.name() == "Mousr") {
        qDebug() << "Found Mousr";

        mousr::MousrHandler *handler = new mousr::MousrHandler(device, this);
        connect(handler, &mousr::MousrHandler::disconnected, this, &DeviceDiscoverer::onDeviceDisconnected);
        m_device = handler;
    } else if (device.name().startsWith("BB-")) {
        qDebug() << "Found BB8";

        sphero::SpheroHandler *handler = new sphero::SpheroHandler(device, this);
        connect(handler, &sphero::SpheroHandler::disconnected, this, &DeviceDiscoverer::onDeviceDisconnected);
        m_device = handler;
    } else {
        return;
    }

    QQmlEngine::setObjectOwnership(m_device, QQmlEngine::CppOwnership);
    emit deviceChanged();
    stopScanning();

    return;
}

void DeviceDiscoverer::onDeviceDisconnected()
{
    qDebug() << "device disconnected";

    if (m_device) {
        m_device->deleteLater();
        disconnect(m_device, nullptr, this, nullptr);
        m_device = nullptr;
        emit deviceChanged();
    } else {
        qWarning() << "device disconnected, but is not set?";
    }

    m_scanning = true;
    m_restartScanTimer.start(); // otherwise we might loop, because qbluetooth-crap caches
//    startScanning();
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
