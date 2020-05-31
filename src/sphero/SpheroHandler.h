#pragma once

#include "BasicTypes.h"

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>

class QLowEnergyController;
class QBluetoothDeviceInfo;

namespace sphero {

class SpheroHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(QString deviceType READ deviceType CONSTANT)

    Q_PROPERTY(float signalStrength READ signalStrength NOTIFY rssiChanged)
    Q_PROPERTY(SpheroType robotType MEMBER m_robotType CONSTANT)

    enum StreamingType : uint8_t {
        NotStreaming = 0,
        StreamUnknown = 1,
        StreamUnknown2 = 2,
        Streaming = 3

    };

    enum ResponseType : uint16_t {
        StreamingResponse = 0x3,
        CollisionDetectedResponse = 0x7,
        PowerStateResponse = 0x9,
        LocatorResponse = 0xb,
    };

public:
    enum class SpheroType {
        Unknown,
        Bb8,
        ForceBand,
        Lmq,
        Ollie,
        Spkr,
        R2d2,
        R2Q5,
        Bb9e,
        SpheroMini,
    };
    Q_ENUM(SpheroType)

public:
    explicit SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);
    ~SpheroHandler();

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

    static QString deviceType() { return "Sphero"; }

    float signalStrength() const {
        return rssiToStrength(m_rssi);
    }

signals:
    void connectedChanged();
    void rssiChanged();
    void disconnected(); // TODO
    void statusMessageChanged(const QString &message);

public slots:

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerError(QLowEnergyController::Error newError);

    void onServiceDiscoveryFinished();
    void onMainServiceChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onRadioServiceChanged(QLowEnergyService::ServiceState newState);

private:
    bool sendRadioControlCommand(const QBluetoothUuid &characteristicUuid, const QByteArray &data);
    void sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data);


    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_commandsCharacteristic;

    QLowEnergyDescriptor m_readDescriptor;

    QPointer<QLowEnergyService> m_mainService;
    QPointer<QLowEnergyService> m_radioService;

    QByteArray m_receiveBuffer;

    QString m_name;
    int8_t m_rssi = 0;

    SpheroType m_robotType = SpheroType::Unknown;
};

} // namespace sphero
