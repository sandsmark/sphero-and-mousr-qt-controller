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
    enum TimeoutHandling : uint8_t {
        KeepTimeout = 0,
        ResetTimeout = 1 << 0
    };

    enum SynchronousType : uint8_t {
        Asynchronous = 0,
        Synchronous = 1 << 1
    };

    enum CommandTarget : uint8_t {
        Internal = 0x00,
        HardwareControl = 0x02,
    };

    enum InternalCommand : uint8_t {
        Ping = 0x01,
        Version = 0x02,
        SetBtName = 0x10,
        GetBtName = 0x11,
        SetAutoReconnect = 0x12,
        GetAutoReconnect = 0x13,
        GetPwrState = 0x20,
        SetPwrNotify = 0x21,
        Sleep = 0x22,
        GotoBl = 0x30,
        RunL1Diags = 0x40,
        RunL2Diags = 0x41,
        ClearCounters = 0x42,
        AssignCounter = 0x50,
        PollTimes = 0x51
    };

    enum HardwareCommand : uint8_t {
        SetHeading = 0x01,
        SetStabiliz = 0x02,
        SetRotationRate = 0x03,
        SetAppConfigBlk = 0x04,
        GetAppConfigBlk = 0x05,
        SetDataStream = 0x11,
        CfgColDet = 0x12,
        ConfigLocator = 0x13,
        GetLocatorData = 0x15,
        SetRgbLed = 0x20,
        SetBackLed = 0x21,
        GetRgbLed = 0x22,
        Roll = 0x30,
        Boost = 0x31,
        SetRawMotors = 0x33,
        SetMotionTo = 0x34,
        GetConfigBlk = 0x40,
        SetDeviceMode = 0x42,
        SetCfgBlk = 0x43,
        GetDeviceMode = 0x44,
        RunMacro = 0x50,
        SaveTempMacro = 0x51,
        SaveMacro = 0x52,
        DelMacro = 0x53,
        InitMacroExecutive = 0x54,
        AbortMacro = 0x55,
        GetMacroStatus = 0x56,
        SetMacroStatus = 0x57,
    };

    const uint8_t magic = 0xFF;
    uint8_t flags = 0xFC;

    uint8_t deviceID = 0;
    uint8_t commandID = 0;
    uint8_t sequenceNumber = 0;
    uint8_t dataLength = 0;
};

struct ResponsePacketHeader {
    Q_GADGET
public:
    enum ResponseCodes : uint8_t {
        Ok = 0x00,
        GeneralError = 0x01,
        ChecksumFailure = 0x02,
        FragmentReceived = 0x3,
        UnknownCommandId = 0x04,
        UnsupporedCommand = 0x05,
        BadMessageFormat = 0x06,
        InvalidParameter = 0x07,
        ExecutionFailed = 0x08,
        UnknownDeviceId = 0x09,
        VoltageTooLow = 0x31,
        IllegalPage = 0x32,
        FlashFailed = 0x33,
        MainApplicationCorrupt = 0x34,
        Timeout = 0x35
    };
    Q_ENUM(ResponseCodes)

    enum AsyncID : uint8_t {
        PowerNotification = 0x01,
        Level1Diagnostic = 0x02,
        SensorStream = 0x03,
        ConfigBlock = 0x04,
        SleepingIn10Sec = 0x05,
        MacroMarkers = 0x06,
        Collision = 0x07
    };

    enum PacketType {
        Ack = 0xFF,
        Data = 0xFE
    };

    const uint8_t magic = 0xFF;
    uint8_t type = 0xFF;

    uint8_t response = 0;
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

    void onServiceDiscovered();
    void onMainServiceChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data);
    void onDescriptorRead(const QLowEnergyDescriptor &characteristic, const QByteArray &data);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onBleServiceChanged(QLowEnergyService::ServiceState newState);
    void onBleCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    void sendRadioControlCommand(const QBluetoothUuid &characteristicUuid, const QByteArray &data);
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
