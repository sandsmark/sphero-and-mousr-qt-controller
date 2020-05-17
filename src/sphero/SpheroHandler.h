#pragma once

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>

class QLowEnergyController;
class QBluetoothDeviceInfo;

namespace sphero {
#pragma pack(push,1)

template<typename T>
struct Vector3D {
    T x;
    T y;
    T z;
};

template<typename T>
struct Vector2D {
    T x;
    T y;
};

struct PacketHeader {
    enum SynchronousType : bool {
        Asynchronous,
        Synchronous
    };
    enum TimeoutHandling : bool {
        KeepTimeout = 0,
        ResetTimeout = 1
    };

    const uint16_t magic:14 { 0b11111111111111 };

    enum TimeoutHandling resetTimeout:1 = KeepTimeout;

    enum SynchronousType synchronous:1 = Synchronous;

    uint8_t deviceID = 0;
    uint8_t commandID = 0;
    uint8_t sequenceNumber = 0;
    uint8_t dataLength = 0;
};

struct PowerStatePacket {
    PacketHeader header;
    uint8_t recordVersion = 0;
    uint8_t powerState = 0;
    uint16_t batteryVoltage = 0;
    uint16_t numberOfCharges = 0;
    uint16_t secondsSinceCharge = 0;

    uint8_t checksum = 0; // i think this is right
};

struct LocatorPacket {
    PacketHeader header;

    // TODO
    enum Flags : uint16_t {
        Calibrated = 0x1, /// tilt is automatically corrected
    };


    uint16_t flags = 0;

    // how is the cartesian (x, y) plane aligned with the heading
    Vector2D<int16_t> position;

    // the tilt against the cartesian plane
    int16_t tilt = 0;

    uint8_t checksum = 0; // i think this is right
};

#pragma pack(pop)

class SpheroHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(QString deviceType READ deviceType CONSTANT)


    enum class DeviceType {
        Bb8,
        ForceBand,
        Lmq,
        Ollie,
        Spkr,
        R2d2,
        R2Q5,
        Bb9e,
        SpheroMini
    };

    enum ResponseType : uint8_t {
        StreamingResponse = 0x3,
        CollisionDetectedResponse = 0x7,
        PowerStateResponse = 0x9,
        LocatorResponse = 0xb,

    };

public:
    explicit SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);
    ~SpheroHandler();

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

    static QString deviceType() { return "Sphero"; }

signals:
    void connectedChanged();
    void disconnected(); // TODO

public slots:

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerError(QLowEnergyController::Error newError);

    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onMainServiceChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data);
    void onDescriptorRead(const QLowEnergyDescriptor &characteristic, const QByteArray &data);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onBleServiceChanged(QLowEnergyService::ServiceState newState);
    void onBleCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    void setTxPower(uint8_t power);
    void sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data, const PacketHeader::SynchronousType synchronous, const PacketHeader::TimeoutHandling keepTimeout);


    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_wakeCharacteristic;
    QLowEnergyCharacteristic m_txPowerCharacteristic;
    QLowEnergyCharacteristic m_commandsCharacteristic;
    QLowEnergyCharacteristic m_responseCharacteristic;

    QLowEnergyCharacteristic m_rssiCharacteristic;

    QLowEnergyDescriptor m_readDescriptor;

//    QPointer<QLowEnergyService> m_commandService;
    QPointer<QLowEnergyService> m_mainService;
    QPointer<QLowEnergyService> m_bleService;

    QByteArray m_receiveBuffer;

    uint8_t deviceId = 0;
    QString m_name;
    uint8_t m_rssi = 0;
};

} // namespace sphero
