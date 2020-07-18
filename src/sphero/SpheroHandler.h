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
#pragma pack(push,1)

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
        Bootloader = 0x01,
        HardwareControl = 0x02,
    };

    enum BootloaderCommand : uint8_t {
        BeginReflash = 0x02,
        HereIsPage = 0x03,
        JumpToMain = 0x04,
        IsPageBlank = 0x05,
        EraseUserConfig = 0x06
    };

    enum InternalCommand : uint8_t {
        Ping = 0x01,
        GetVersion = 0x02,
        SetBtName = 0x10,
        GetBtName = 0x11,
        SetAutoReconnect = 0x12,
        GetAutoReconnect = 0x13,
        GetPwrState = 0x20,
        SetPwrNotify = 0x21,
        Sleep = 0x22,
        GetVoltageTrip = 0x23,
        SetVoltageTrip = 0x24,
        SetInactiveTimeout = 0x22,
        GotoBl = 0x30,
        RunL1Diags = 0x40,
        RunL2Diags = 0x41,
        ClearCounters = 0x42,
        AssignCounter = 0x50,
        PollTimes = 0x51
    };

    enum HardwareCommand : uint8_t {
        SetHeading = 0x01,
        Stabilization = 0x02,
        RotationRate = 0x03,
        SetAppConfigBlk = 0x04,
        GetAppConfigBlk = 0x05,
        SelfLevel = 0x09,
        SetDataStreaming = 0x11,
        ConfigureCollisionDetection = 0x12,
        ConfigureLocator = 0x13,
        GetLocatorData = 0x15,
        RGBLEDOutput = 0x20,
        BackLEDOutput = 0x21,
        GetUserRGBLEDColor = 0x22,
        Roll = 0x30,
        Boost = 0x31,
        RawMotorValues = 0x33,
        SetMotionTimeout = 0x34,
        SetOptionFlags = 0x35,
        GetOptionFlags = 0x36,
        SetNonPersistentOptionFlags = 0x37,
        GetNonPersistentOptionFlags = 0x38,
        GetConfigurationBlock = 0x40,
        SetDeviceMode = 0x42,
        SetConfigurationBlock = 0x43,
        GetDeviceMode = 0x44,
        SetFactoryDeviceMode = 0x45,
        GetSSB = 0x46,
        SetSSB = 0x47,
        RefillBank = 0x48,
        BuyConsumable = 0x49,
        AddXp = 0x4C,
        LevelUpAttribute = 0x4D,
        RunMacro = 0x50,
        SaveTempMacro = 0x51,
        SaveMacro = 0x52,
        DelMacro = 0x53,
        GetMacroStatus = 0x56,
        SetMacroStatus = 0x57,
        SaveTempMacroChunk = 0x58,
        InitMacroExecutive = 0x54,
        AbortMacro = 0x55,
        GetConfigBlock = 0x40,
        OrbBasicEraseStorage = 0x60,
        OrbBasicAppendFragment = 0x61,
        OrbBasicExecute = 0x62,
        OrbBasicAbort = 0x63,
        OrbBasicCommitRamProgramToFlash = 0x65,
        RemoveCores = 0x71,
        SetSSBUnlockFlagsBlock = 0x72,
        ResetSoulBlock = 0x73,
        ReadOdometer = 0x75,
        WritePersistentPage = 0x90
    };
    enum SoulCommands : uint8_t {
        ReadSoulBlock = 0xf0,
        SoulAddXP = 0xf1
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
    enum AckType : uint8_t {
        Ok = 0x00,
        GeneralError = 0x01,
        ChecksumFailure = 0x02,
        FragmentReceived = 0x3,
        UnknownCommandId = 0x04,
        UnsupportedCommand = 0x05,
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
    Q_ENUM(AckType)

    enum DataType : uint8_t {
        PowerNotification = 0x01,
        Level1Diagnostic = 0x02,
        SensorStream = 0x03,
        ConfigBlock = 0x04,
        SleepingIn10Sec = 0x05,
        MacroMarkers = 0x06,
        Collision = 0x07
    };
    Q_ENUM(DataType)

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

struct AckResponsePacket
{
    uint16_t unk; /// eehidk
    uint16_t type;
};

struct LocatorPacket {
    // TODO
    enum Flags : uint16_t {
        Calibrated = 0x1, /// tilt is automatically corrected
    };

    uint8_t flags = Calibrated;

    // how is the cartesian (x, y) plane aligned with the heading
    Vector2D<int16_t> position{};

    // the tilt against the cartesian plane
    int16_t tilt = 0;
};


struct SleepCommandPacket
{
    PacketHeader header; // Just free-wheeling it here

    enum SleepType {
        SleepHighPower = 0x00,
        SleepDeep = 0x01,
        SleepLowPower = 0x02
    };

    uint16_t sleepTime;
    uint8_t wakeMacro;
    uint16_t basicLineNumber;
};

struct RotateCommandPacket
{
    PacketHeader header;
    float rate;
};

struct DataStreamingCommandPacket
{
    enum SourceMask : uint32_t {
        NoMask = 0x00000000,
        LeftMotorBackEMFFiltered = 0x00000060,
        RightMotorBackEMFFiltered = 0x00000060,

        MagnetometerZFiltered = 0x00000080,
        MagnetometerYFiltered = 0x00000100,
        MagnetometerXFiltered = 0x00000200,

        GyroZFiltered = 0x00000400,
        GyroYFiltered = 0x00000800,
        GyroXFiltered = 0x00001000,

        AccelerometerZFiltered = 0x00002000,
        AccelerometerYFiltered = 0x00004000,
        AccelerometerXFiltered = 0x00008000,

        IMUYawAngleFiltered = 0x00010000,
        IMURollAngleFiltered = 0x00020000,
        IMUPitchAngleFiltered = 0x00040000,

        LeftMotorBackEMFRaw =  0x00600000,
        RightMotorBackEMFRaw = 0x00600000,

        GyroFilteredAll = 0x00001C00,
        IMUAnglesFilteredAll = 0x00070000,
        AccelerometerFilteredAll = 0x0000E000,

        MotorPWM =  0x00100000 | 0x00080000, // -2048 -> 2047

        Magnetometer = 0x02000000,

        GyroZRaw = 0x04000000,
        GyroYRaw = 0x08000000,
        GyroXRaw = 0x10000000,

        AccelerometerZRaw = 0x20000000,
        AccelerometerYRaw = 0x40000000,
        AccelerometerXRaw = 0x80000000,
        AccelerometerRaw =  0xE0000000,

        AllSources = 0xFFFFFFFF,
    };

    uint16_t maxRateDivisor = 10;
    uint16_t framesPerPacket = 1;
    uint32_t sourceMask = AllSources;
    uint8_t packetCount = 0; // 0 == forever
};

struct DataStreamingCommandPacket1_17 : DataStreamingCommandPacket
{
    enum SourceMask : uint64_t {
        Quaternion0 = 0x80000000,
        Quaternion1 = 0x40000000,
        Quaternion2 = 0x20000000,
        Quaternion3 = 0x10000000,
        LocatorX = 0x0800000,
        LocatorY = 0x0400000,

        // TODO:
        //MaskAccelOne                  = 0x0200000000000000,

        VelocityX = 0x0100000,
        VelocityY = 0x0080000,

        LocatorAll = 0x0D80000,
        QuaternionAll = 0xF0000000,

        AllSourcesHigh = 0xFFFFFFFF,

    };
    uint32_t sourceMaskHighBits = AllSourcesHigh; // firmware >= 1.17
};

struct SensorStreamPacket
{
    struct Motor {
        int16_t left;
        int16_t right;
    };

    ResponsePacketHeader header;

    Vector3D<int16_t> accelerometerRaw; // -2048 to 2047
    Vector3D<int16_t> gyroRaw; // -2048 to 2047

    Vector3D<int16_t> unknown; // not used?

    Motor motorBackRaw; // motor back EMF, raw   -32768 to 32767 22.5 cm
    Motor motorRaw; // motor, PWM raw    -2048 to 2047   duty cycle

    Orientation<int16_t> filteredOrientation; // IMU pitch angle, yaw and angle filtered   -179 to 180 degrees

    Vector3D<int16_t> accelerometer; // accelerometer axis, filtered  -32768 to 32767 1/4096 G
    Vector3D<int16_t> gyro; // filtered   -20000 to 20000 0.1 dps

    Vector3D<int16_t> unknown2; // unused?

    Motor motorBack; // motor back EMF, filtered  -32768 to 32767 22.5 cm

    uint16_t unknown3[5]; // unused?

    Quaternion<int16_t> quaternion; // -10000 to 10000 1/10000 Q

    Vector2D<int16_t> odometer; // // 0800 0000h   Odometer X  -32768

    int16_t acceleration; // 0 to 8000   1 mG

    Vector2D<int16_t> velocity; // -32768 to 32767 mm/s
};

static_assert(sizeof(LocatorPacket) == 7);
static_assert(sizeof(ResponsePacketHeader) == 5);
static_assert(sizeof(PacketHeader) == 6);
static_assert(sizeof(SensorStreamPacket) == 87); // should be 90, I think?

#pragma pack(pop)

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
    void sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data, const PacketHeader::SynchronousType synchronous, const PacketHeader::TimeoutHandling keepTimeout);


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
