#ifndef DEVICEDISCOVERER_H
#define DEVICEDISCOVERER_H

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>

namespace mousr {
class MousrHandler;
}

class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;


// Awkward name, but naming is hard
class DeviceDiscoverer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusString READ statusString NOTIFY statusStringChanged)
    Q_PROPERTY(QObject* device READ device NOTIFY deviceChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusStringChanged) // yeye
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY statusStringChanged) // yeye
    Q_PROPERTY(QStringList availableDevices READ availableDevices NOTIFY availableDevicesChanged)


public:
    explicit DeviceDiscoverer(QObject *parent = nullptr);
    ~DeviceDiscoverer();

    QObject *device();

    QString statusString();

    bool isError() const { return m_adapterError != QBluetoothLocalDevice::NoError || QBluetoothLocalDevice::allDevices().isEmpty(); }

    QStringList availableDevices() const;

    bool isScanning() const { return m_scanning; }

    static bool isSupportedDevice(const QBluetoothDeviceInfo &device);

public slots:
    void connectDevice(const QString &name);

signals:
    void statusStringChanged();
    void deviceChanged();
    void availableDevicesChanged();
    void signalStrengthChanged(const QString &deviceName, float strength);

private slots:
    void startScanning();
    void stopScanning();

    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onDeviceUpdated(const QBluetoothDeviceInfo &device, QBluetoothDeviceInfo::Fields fields);
    void onDeviceDisconnected();

    void onAgentError();
    void onAdapterError(const QBluetoothLocalDevice::Error error);

    void onRobotStatusChanged(const QString &message);

private:
    void updateRssi(const QBluetoothDeviceInfo &device);


    QPointer<QObject> m_device;

    QPointer<QBluetoothDeviceDiscoveryAgent> m_discoveryAgent;
    QPointer<QBluetoothLocalDevice> m_adapter;
    QBluetoothLocalDevice::Error m_adapterError = QBluetoothLocalDevice::NoError;
    bool m_adapterPoweredOn = false;
    bool m_attemptingScan = false;
    bool m_hasDevices = false;
    bool m_scanning = false;
    QHash<QString, QBluetoothDeviceInfo> m_availableDevices;

    QString m_lastDeviceStatus;
    QElapsedTimer m_lastDeviceStatusTimer;
};

#endif // DEVICEDISCOVERER_H
