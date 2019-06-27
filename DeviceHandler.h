#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>

class QLowEnergyController;
class QBluetoothDeviceInfo;

class DeviceHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusString READ statusString NOTIFY validChanged)

public:
    explicit DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);

    bool isValid();

    QString statusString();

signals:
    void validChanged();

public slots:

private slots:
    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onCharacteristicsChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);


private:
    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_readCharacteristic;
    QLowEnergyCharacteristic m_writeCharacteristic;

    QPointer<QLowEnergyService> m_service;
    const QBluetoothUuid m_serviceUuid;
    const QBluetoothUuid m_writeUuid;
    const QBluetoothUuid m_readUuid;
};

#endif // DEVICEHANDLER_H
