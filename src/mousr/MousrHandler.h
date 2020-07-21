#pragma once

#include "AutoplayConfig.h"

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>

class QLowEnergyController;
class QBluetoothDeviceInfo;

namespace mousr {

static constexpr int manufacturerID = 1500;

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

class MousrHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceType READ deviceType CONSTANT)
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(int memory READ memory NOTIFY powerChanged)

    Q_PROPERTY(int voltage READ voltage NOTIFY powerChanged)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY powerChanged)
    Q_PROPERTY(bool isBatteryLow READ isBatteryLow NOTIFY powerChanged)
    Q_PROPERTY(bool isFullyCharged READ isFullyCharged NOTIFY powerChanged)

    Q_PROPERTY(bool isAutoRunning READ isAutoRunning NOTIFY autoRunningChanged)

    Q_PROPERTY(mousr::AutoplayConfig::Surface autoplaySurface READ autoplaySurface WRITE setAutoplaySurface NOTIFY autoPlayChanged)
    Q_PROPERTY(mousr::AutoplayConfig::TailType autoplayTailType READ autoplayTailType WRITE setAutoplayTailType NOTIFY autoPlayChanged)
    Q_PROPERTY(mousr::AutoplayConfig::DrivingMode autoplayDrivingMode READ autoplayDrivingMode NOTIFY autoPlayChanged)
    Q_PROPERTY(mousr::AutoplayConfig::GameMode autoplayGameMode READ autoplayGameMode WRITE setAutoplayGameMode NOTIFY autoPlayChanged)
    Q_PROPERTY(int autoplayPauseTime READ autoplayPauseTime WRITE setAutoplayPauseTime NOTIFY autoPlayChanged)

    Q_PROPERTY(float xRotation READ xRotation NOTIFY orientationChanged)
    Q_PROPERTY(float yRotation READ yRotation NOTIFY orientationChanged)
    Q_PROPERTY(float zRotation READ zRotation NOTIFY orientationChanged)
    Q_PROPERTY(bool isFlipped READ isFlipped NOTIFY orientationChanged)

    Q_PROPERTY(bool sensorDirty READ sensorDirty NOTIFY sensorDirtyChanged)

    Q_PROPERTY(bool soundVolume READ soundVolume NOTIFY soundVolumeChanged)

public: // enums
    AutoplayConfig::Surface autoplaySurface() const { return m_autoplay.surface(); }
    AutoplayConfig::TailType autoplayTailType() const { return m_autoplay.tailType(); }
    int autoplayPauseTime() const { return m_autoplay.pauseTime(); }

    AutoplayConfig::GameMode autoplayGameMode() const { return m_autoplay.gameMode(); }
    void setAutoplayGameMode(const AutoplayConfig::GameMode mode) { m_autoplay.setGameMode(mode); emit autoPlayChanged(); }
    Q_INVOKABLE QStringList autoplayGameModeNames() const { return AutoplayConfig::gameModeNames(); }

    AutoplayConfig::DrivingMode autoplayDrivingMode() const { return m_autoplay.drivingMode(); }
    void setAutoplayDrivingMode(const AutoplayConfig::DrivingMode mode) { m_autoplay.setDrivingMode(mode); emit autoPlayChanged(); }
    Q_INVOKABLE QStringList autoplayDrivingModeNames() const { return AutoplayConfig::drivingModeNames(); }

    void setAutoplaySurface(const AutoplayConfig::Surface surface) { m_autoplay.setSurface(surface); emit autoPlayChanged(); }
    void setAutoplayTailType(const AutoplayConfig::TailType tailType) { m_autoplay.setTailType(tailType); emit autoPlayChanged(); }
    void setAutoplayPauseTime(const int time) { m_autoplay.setPauseTime(time); emit autoPlayChanged(); }

//    AutoplayConfig::Surface autoplayPreset() const { return m_autoplay.preset(); }

    enum class CommandType : uint16_t {
        Stop = 0,
        Spin = 1,
        Move = 2,
        ResetHeading = 3,
        GetDebugLog = 4,
        SpinPlan = 5,

        EnterDfuMode = 8,
        TurnOff = 9,
        Sleep = 10,

        ConfigAutoMode = 15,

        Chirp = 18,
        SoundVolume = 19,

        FlickSignal = 23,
        ReverseSignal = 24,
        TailCalibSignal = 25,
        SetTailSignal = 26,

        InitializeDevice = 28,

        FlipRobot = 31,

        RequestAnalyticsRecords = 33,
        EraseAnalyticsRecords = 34,

        ConfigDriverAssist = 41,
        TutorialStep = 45,
        SetTime = 46,
        SchedulePlay = 47,

        Invalid = 100
    };
    Q_ENUM(CommandType)

    enum ResponseType : uint8_t {
        AutoModeChanged = 15,

        FirmwareVersion = 28,
        HardwareVersion = 29,
        InitDone = 30,

//        AutoAckSuccess = 48,
        DeviceOrientation = 48,
//        AutoAckReport = 49,
//        ResetTailReport = 50,
        ResetTailFailInfo = 50,

        SensorDirty = 64, // sensor dirty

        AnalyticsBegin = 80,
        AnalyticsEntry = 81,
        AnalyticsData = 82,

//        TofStuck = 83,
        AnalyticsEnd = 83,

        CrashLogFinished = 95,
        CrashLogString = 96,
        DebugInfo = 97, //CrashlogAddDebugMem = 97,

        BatteryVoltage = 98,
        RobotStopped = 99,
        RcStuck = 100,

        Nack = 255
    };
    Q_ENUM(ResponseType)

    enum FirmwareType : uint8_t {
        DebugFirmware = 0,
        BetaFirmware = 1,
        StableFirmware = 2
    };
    Q_ENUM(FirmwareType)

    enum AnalyticsEvent {
        TailHeld,
        StuckOnCable,
        StationaryModeNoSpace, // stationary mode, no free locations to move to
        WallHuggerNoSpace,
        OpenWanderNoSpace,
        StruggleModeTailCaughtTimeout, // tail stuck for more than 30 seconds
        TailFailedToReset, // tries 5 times
        FailedToFlipBack, // failed to flip back after trying for one minutte
        FailedToGetAwayFromCable,
        FailedToFindFreeSpace,
        SensorPermanentlyDirty,
        WallHuggerMultipleBadSensorReadings,
        InitState,
        FullJuiceInPlaySession,
        HighAlertInPlaySession,
        LowEnergyInPlaySession,
        HighAlertInWaitForSession,
        LowEnergyInWaitForSession,
        EnticeAfterWaitForSession,
        HighAlertAfterEnticeBeforeNewPlaySession
    };
    Q_ENUM(AnalyticsEvent)

public:
    static QString deviceType() { return "Mousr"; }

    int memory() const { return m_memory; }

    // Power stuff
    int voltage() const { return m_voltage; }
    bool isCharging() const { return m_charging; }
    bool isBatteryLow() const { return m_batteryLow; }
    bool isFullyCharged() const { return m_fullyCharged; }

    bool isAutoRunning() const { return m_autoRunning; }

    float xRotation() const { return m_rotX; }
    float yRotation() const { return m_rotY; }
    float zRotation() const { return m_rotZ; }
    bool isFlipped() const { return m_isFlipped; }

    bool sensorDirty() const { return m_sensorDirty; }

    const uint32_t mbApiVersion = 3u;

    explicit MousrHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);
    ~MousrHandler();

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

    int soundVolume() { return m_volume; }
    void setSoundVolume(const int volumePercent);

signals:
    void connectedChanged();
    void disconnected(); // TODO
    void powerChanged();
    void autoRunningChanged();
    void orientationChanged();
    void sensorDirtyChanged();
    void soundVolumeChanged();
    void autoPlayChanged();

public slots:
    void chirp();
    void pause();
    void resume();

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerError(QLowEnergyController::Error newError);

    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    bool sendCommand(const CommandType command, float arg1, const float arg2, const float arg3);
    bool sendCommand(const CommandType command, const uint32_t arg1, const uint32_t arg2 = 0);
    bool sendCommand(const CommandType command);

    #pragma pack(push,1)
    static_assert(sizeof(bool) == 1);

    struct Version {
        FirmwareType firmwareType;
        uint8_t major;
        uint16_t minor;
        uint16_t commitNumber;
        char commitHash[4];

        uint8_t mousrVersion;
        uint32_t hardwareVersion;
        uint32_t bootloaderVersion;
    };

    Version m_version;
    struct BatteryVoltageResponse {
        uint8_t voltage;
        bool isBatteryLow;
        bool isCharging;
        bool isFullyCharged;
        bool isAutoMode;
        uint16_t memory;

        uint8_t padding[12];
    };

    struct DeviceOrientationResponse {
        static_assert(sizeof(float) == 4);
        Vector3D<float> rotation;

        bool isFlipped;

        uint8_t padding[6];
    };

    struct CrashLogStringResponse {
        QString message() const { return QString::fromUtf8(m_string, sizeof(m_string)); }
    private:
        char m_string [19];
    };

    struct AnalyticsBeginResponse {
        uint8_t numberOfEntries;
        char unknown[18];
    };
    struct AutoPlayConfigResponse {
        AutoplayConfig config;
        uint32_t unknown;
//        char unknown[4];
    };
    struct IsSensorDirtyResponse {
        bool isDirty;

        char unknown[18];
    };
    struct FirmwareVersionResponse {
        Version version;
    };
    struct NackResponse {
        CommandType command;
        uint8_t unknown1;
        uint32_t currentApiVersion;
        uint32_t minimumApiVersion;
        uint32_t maximumApiVersion;

        uint8_t unknown2[4];
    };

    struct ResponsePacket {
        ResponseType type;
        union {
            BatteryVoltageResponse battery;
            DeviceOrientationResponse orientation;
            CrashLogStringResponse crashString;
            AnalyticsBeginResponse analyticsBegin;
            AutoPlayConfigResponse autoPlay{};
            IsSensorDirtyResponse sensorDirty;
            FirmwareVersionResponse firmwareVersion;
            NackResponse nack;
        };
    };

    struct CommandPacket {
        CommandPacket(const CommandType command) : vector3D({}), // we have to initialize ourselves, so idk initialize the 3d vector
            m_command(command) {}

        const uint8_t magic = 48;
        union {
            Vector3D<float> vector3D;
            Vector3D<uint32_t> vector2D;
        };
        CommandType m_command = CommandType::Invalid;
    };
    #pragma pack(pop)

    bool sendCommandPacket(const CommandPacket &packet);

    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_readCharacteristic;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QLowEnergyDescriptor m_readDescriptor;

    QPointer<QLowEnergyService> m_service;

    int m_voltage = 0, m_memory = 0;
    int m_volume = 0; // todo: read this from device
    bool m_batteryLow = false, m_charging = false, m_fullyCharged = false;
    bool m_autoRunning = false;

    float m_speed = 0.f, m_held = 0.f, m_angle = 0.f;

    float m_rotX = 0.f, m_rotY = 0.f, m_rotZ = 0.f;
    bool m_isFlipped = false;

    bool m_sensorDirty = false;

    QString m_name;

    AutoplayConfig m_autoplay;
};

QDebug operator<<(QDebug debug, const AutoplayConfig &c);

} // namespace mousr
