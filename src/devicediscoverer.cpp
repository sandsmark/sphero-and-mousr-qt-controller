#include "devicediscoverer.h"

#include "mousr/MousrHandler.h"
#include "sphero/SpheroHandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QDebug>
#include <QQmlEngine>

DeviceDiscoverer::DeviceDiscoverer(QObject *parent) :
    QObject(parent),
    m_scanning(false)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    m_adapter = new QBluetoothLocalDevice(this);

    connect(m_adapter, &QBluetoothLocalDevice::error, this, &DeviceDiscoverer::onAdapterError);
    connect(this, &DeviceDiscoverer::availableDevicesChanged, this, &DeviceDiscoverer::statusStringChanged);

    // I hate these overload things..
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceDiscoverer::onAgentError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceDiscoverer::onDeviceDiscovered, Qt::QueuedConnection);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this, &DeviceDiscoverer::onDeviceUpdated);

    m_discoveryAgent->setLowEnergyDiscoveryTimeout(0);

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
    if (m_device) {
        m_device->deleteLater();
    }
}

QObject *DeviceDiscoverer::device()
{
    return m_device.data();
}

QString DeviceDiscoverer::statusString()
{
    if (m_lastDeviceStatusTimer.isValid() && m_lastDeviceStatusTimer.elapsed() < 5000) {
        if (!m_lastDeviceStatus.isEmpty()) {
            return m_lastDeviceStatus;
        }
    }
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
            return tr("Unknown device error %1").arg(m_adapterError);
            break;
        }
    }
    if (m_adapter->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        return tr("Bluetooth turned off");
    }

    if (m_discoveryAgent->error() != QBluetoothDeviceDiscoveryAgent::NoError) {
        return tr("Searching error: %1").arg(m_discoveryAgent->errorString());
    }

    if (!m_availableDevices.isEmpty()) {
        return tr("Found devices");
    }

    if (m_scanning) {
        return tr("Scanning for devices...");
    }

    return QString();
}

QStringList DeviceDiscoverer::availableDevices() const
{
    return m_availableDevices.keys();
}

void DeviceDiscoverer::connectDevice(const QString &name)
{
    if (m_device) {
        qWarning() << "already have device, skipping" << name;
        return;
    }

    if (!m_availableDevices.contains(name)) {
        qWarning() << "We don't know" << name;
        return;
    }

    stopScanning();

    const QBluetoothDeviceInfo &device = m_availableDevices[name];

    const RobotType type = robotType(device);
    if (type == Mousr) {
        mousr::MousrHandler *handler = new mousr::MousrHandler(device, this);
        connect(handler, &mousr::MousrHandler::disconnected, this, &DeviceDiscoverer::onDeviceDisconnected);
//        connect(handler, &mousr::MousrHandler::connectedChanged, this, &DeviceDiscoverer::onRobotStatusChanged); todo
        m_device = handler;
    } else if (type == Sphero) {
        qDebug() << "Found BB8";

        sphero::SpheroHandler *handler = new sphero::SpheroHandler(device, this);
        connect(handler, &sphero::SpheroHandler::disconnected, this, &DeviceDiscoverer::onDeviceDisconnected);
        connect(handler, &sphero::SpheroHandler::statusMessageChanged, this, &DeviceDiscoverer::onRobotStatusChanged);
        m_device = handler;
    } else {
        qWarning() << "unknown device!" << device.name();
        Q_ASSERT(false);
        return;
    }

    QQmlEngine::setObjectOwnership(m_device, QQmlEngine::CppOwnership);
    emit deviceChanged();

    m_availableDevices.clear();
    emit availableDevicesChanged();

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
    m_discoveryAgent->stop();
    emit statusStringChanged();
}

inline void debugVisibleDevices(const QBluetoothDeviceInfo &device)
{
    if (device.manufacturerIds().isEmpty()) {
        return;
    }
    if (DeviceDiscoverer::robotType(device) != DeviceDiscoverer::Unknown) {
        return;
    }

    static const QSet<int> knownIds({
        223, // Misfit Wearables
        224, // Google
        76, // Apple
        117, // Samsung
        1447, // Sonos
        6, // Microsoft
        89, //Nordic Semiconductor ASA
        272, // Nippon Seiki Co., Ltd.
        196, // ​​LG Electronics​
        761, // IMAGINATION TECHNOLOGIES LTD
        135, // Garmin International, Inc.
        474, // Logitech International SA

        // Unregistered/illegal
        9474, // 'Expert'
        0x3638, // UVC G3 camera?
        2561, // LE-Bose Free SoundSport
        61374, // Anki (Vector Cube)
    });

    for (const quint16 mfgid : device.manufacturerIds()) {
        if (knownIds.contains(mfgid)) {
            return;
        }
        qDebug() << "discovered" << device.name() << device.address().toString() << device.manufacturerIds() << device.minorDeviceClass() << device.majorDeviceClass() << device.manufacturerData() << device.serviceClasses() << device.rssi();
    }
}

void DeviceDiscoverer::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (m_device) {
        qWarning() << "already have device, skipping" << device.name();
        return;
    }

#ifndef NDEBUG
    debugVisibleDevices(device);
#endif

    if (DeviceDiscoverer::robotType(device) == DeviceDiscoverer::Unknown) {
        return;
    }

    m_availableDevices[device.name()] = device;
    emit availableDevicesChanged();

    updateRssi(device);
}

void DeviceDiscoverer::onDeviceUpdated(const QBluetoothDeviceInfo &device, QBluetoothDeviceInfo::Fields fields)
{
#ifndef NDEBUG
    if (!(fields & QBluetoothDeviceInfo::Field::RSSI)) {
        debugVisibleDevices(device);
    }
#endif

    if (DeviceDiscoverer::robotType(device) == DeviceDiscoverer::Unknown) {
        return;
    }

    if (fields & QBluetoothDeviceInfo::Field::RSSI) {
        updateRssi(device);
    }
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

    m_availableDevices.clear();
    emit availableDevicesChanged();

    if (m_lastDeviceStatus.isEmpty() || !m_lastDeviceStatusTimer.isValid() || m_lastDeviceStatusTimer.elapsed() > 20000) {
        m_lastDeviceStatus = tr("Unexpected disconnect from device");
        m_lastDeviceStatusTimer.restart();
    }
    emit statusStringChanged();

    QMetaObject::invokeMethod(this, &DeviceDiscoverer::startScanning); // otherwise we might loop, because qbluetooth-crap caches
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

void DeviceDiscoverer::onRobotStatusChanged(const QString &message)
{
    m_lastDeviceStatus = message;
    m_lastDeviceStatusTimer.restart();
}

float DeviceDiscoverer::signalStrength(const QString &name)
{
    if (!m_availableDevices.contains(name)) {
        qWarning() << "Unknown device" << name;
        return 0;
    }
    return rssiToStrength(m_availableDevices[name].rssi());
}

void DeviceDiscoverer::updateRssi(const QBluetoothDeviceInfo &device)
{
    emit signalStrengthChanged(device.name(), rssiToStrength(device.rssi()));
}

DeviceDiscoverer::RobotType DeviceDiscoverer::robotType(const QBluetoothDeviceInfo &device)
{
    const QVector<quint16> manufacturerIds = device.manufacturerIds();
    if (manufacturerIds.count() > 1) {
        qDebug() << "Unexpected amount of manufacturer IDs" << device.name() << manufacturerIds;
    }

    if (manufacturerIds.contains(mousr::manufacturerID)) {
        // It _seems_ like the manufacturer data is the reversed of most of the address, except the last part which is 0xFC in the address and 0x3C in the manufacturer data
//        QByteArray deviceAddress = QByteArray::fromHex(device.address().toString().toLatin1());
//        if (!deviceAddress.isEmpty()) {
//            std::reverse(deviceAddress.begin(), deviceAddress.end());
//            qDebug() << deviceAddress.toHex(':') << device.manufacturerData(mousr::petronicsManufacturerID).toHex(':').toUpper();
//        }
        return Mousr;
    }

    if (manufacturerIds.contains(sphero::manufacturerID)) {
        return Sphero;
    }

    const QString name = device.name();
    if (name == QLatin1String("Mousr")) {
        if (!manufacturerIds.isEmpty()) {
            qDebug() << "unexpected manufacturer ID for mousr:" << manufacturerIds;
        }
        return Mousr;
    } else if (name.startsWith(QLatin1String("BB-"))) {
        if (!manufacturerIds.isEmpty()) {
            qDebug() << "unexpeced manufacturer ID for Sphero:" << manufacturerIds;
        }
        return Sphero;
    }

    return Unknown;
}
