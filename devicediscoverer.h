#ifndef DEVICEDISCOVERER_H
#define DEVICEDISCOVERER_H

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QPointer>

class DeviceHandler;

class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;


// Awkward name, but naming is hard
class DeviceDiscoverer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusString READ statusString NOTIFY statusStringChanged)
    Q_PROPERTY(QObject* device READ device NOTIFY deviceFound)

public:
    explicit DeviceDiscoverer(QObject *parent = nullptr);
    ~DeviceDiscoverer();

    QObject *device();

    QString statusString();

public slots:
    void startScanning();
    void stopScanning();

signals:
    void statusStringChanged();
    void deviceFound();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onDeviceUpdated(const QBluetoothDeviceInfo &device);

    void onAgentError();
    void onAdapterError(const QBluetoothLocalDevice::Error error);
    void onAdapterStateChanged(const QBluetoothLocalDevice::HostMode mode);

private:
    QPointer<DeviceHandler> m_device;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QBluetoothLocalDevice *m_adapter;
    QBluetoothLocalDevice::Error m_adaperError = QBluetoothLocalDevice::NoError;
    bool m_adapterPoweredOn = false;
    bool m_attemptingScan = false;
};

#endif // DEVICEDISCOVERER_H
