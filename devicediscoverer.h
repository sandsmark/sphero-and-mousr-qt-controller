#ifndef DEVICEDISCOVERER_H
#define DEVICEDISCOVERER_H

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QPointer>

class MousrHandler;

class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;


// Awkward name, but naming is hard
class DeviceDiscoverer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusString READ statusString NOTIFY statusStringChanged)
    Q_PROPERTY(QObject* device READ device NOTIFY deviceFound)
    Q_PROPERTY(bool isError READ isError NOTIFY statusStringChanged) // yeye


public:
    explicit DeviceDiscoverer(QObject *parent = nullptr);
    ~DeviceDiscoverer();

    QObject *device();

    QString statusString();

    bool isError() const { return m_adapterError != QBluetoothLocalDevice::NoError || QBluetoothLocalDevice::allDevices().isEmpty(); }

public slots:
    void startScanning();
    void stopScanning();

signals:
    void statusStringChanged();
    void deviceFound();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);

    void onAgentError();
    void onAdapterError(const QBluetoothLocalDevice::Error error);

private:
    QPointer<MousrHandler> m_device;

    QPointer<QBluetoothDeviceDiscoveryAgent> m_discoveryAgent;
    QPointer<QBluetoothLocalDevice> m_adapter;
    QBluetoothLocalDevice::Error m_adapterError = QBluetoothLocalDevice::NoError;
    bool m_adapterPoweredOn = false;
    bool m_attemptingScan = false;
};

#endif // DEVICEDISCOVERER_H
