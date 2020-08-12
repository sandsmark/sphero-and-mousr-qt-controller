#pragma once

#include "AutoplayConfig.h"

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>
#include <QTimer>
#include <QElapsedTimer>

class QLowEnergyController;
class QBluetoothDeviceInfo;

namespace mousr {

static constexpr int manufacturerID = 1500;

template<typename T>
struct Vector3D {
    T x;
    T y;
    T z;

    // No const reference because of alignment issues
    template<typename OTHER>
    Vector3D &operator=(const Vector3D<OTHER> other) {
        x = qRound(other.x);
        y = qRound(other.y);
        z = qRound(other.z);
        return *this;
    }

    // This fucks up because alignment
    template<typename OTHER>
    bool operator==(const OTHER &other) = delete;
};

// Can't use a member function and can't use references because of alignment issues (the vectors in the packets aren't properly aligned)
inline bool fuzzyVectorsEqual(Vector3D<float> floatVector, Vector3D<int> intVector)
{
        return qRound(floatVector.x) == intVector.x
            && qRound(floatVector.y) == intVector.y
            && qRound(floatVector.z) == intVector.z;
}

inline bool fuzzyVectorsEqual(Vector3D<int> intVector, Vector3D<float> floatVector)
{
        return qRound(floatVector.x) == intVector.x
            && qRound(floatVector.y) == intVector.y
            && qRound(floatVector.z) == intVector.z;
}

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

    Q_PROPERTY(float angle READ angle WRITE setAngle NOTIFY inputChanged)
    Q_PROPERTY(float speed READ speed WRITE setSpeed NOTIFY inputChanged)
    Q_PROPERTY(bool controlsPressed READ isControlsPressed WRITE setControlsPressed NOTIFY inputChanged)
    Q_PROPERTY(bool driverAssistEnabled READ isDriverAssistEnabled WRITE setDriverAssistEnabled NOTIFY driverAssistChanged)

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
    Q_PROPERTY(bool stuck READ isStuck NOTIFY stuckChanged)

    Q_PROPERTY(bool soundVolume READ soundVolume NOTIFY soundVolumeChanged)

public:
    AutoplayConfig::Surface autoplaySurface() const { return m_currentAutoConfig.surface(); }
    AutoplayConfig::TailType autoplayTailType() const { return m_currentAutoConfig.tailType(); }
    int autoplayPauseTime() const { return m_currentAutoConfig.pauseTime(); }

    AutoplayConfig::GameMode autoplayGameMode() const { return m_currentAutoConfig.gameMode(); }
    void setAutoplayGameMode(const AutoplayConfig::GameMode mode) { m_newAutoConfig.setGameMode(mode); emit autoPlayChanged(); sendAutoplay(); }
    Q_INVOKABLE QStringList autoplayGameModeNames() const { return AutoplayConfig::gameModeNames(); }

    AutoplayConfig::DrivingMode autoplayDrivingMode() const { return m_currentAutoConfig.drivingMode(); }
    void setAutoplayDrivingMode(const AutoplayConfig::DrivingMode mode) { m_newAutoConfig.setDrivingMode(mode); emit autoPlayChanged(); sendAutoplay(); }
    Q_INVOKABLE QStringList autoplayDrivingModeNames() const { return AutoplayConfig::drivingModeNames(); }

    void setAutoplaySurface(const AutoplayConfig::Surface surface) { m_newAutoConfig.setSurface(surface); emit autoPlayChanged(); sendAutoplay(); }
    void setAutoplayTailType(const AutoplayConfig::TailType tailType) { m_newAutoConfig.setTailType(tailType); emit autoPlayChanged(); sendAutoplay(); }
    void setAutoplayPauseTime(const int time) { m_newAutoConfig.setPauseTime(time); emit autoPlayChanged(); sendAutoplay(); }

//    AutoplayConfig::Surface autoplayPreset() const { return m_currentAutoConfig.preset(); }

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

        DeviceOrientation = 48,
        AutoAckReport = 49,
        TailStateUpdated = 50,

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

        CommandCompleted = 255
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

    enum LeftOrRight {
        Left,
        Right
    };
    Q_ENUM(LeftOrRight)

public:
    static QString deviceType() { return "Mousr"; }


    float angle() const { return m_newInput.angle; }
    float speed() const { return m_newInput.speed; }
    bool isControlsPressed() const { return !qFuzzyIsNull(m_newInput.held); }
    void setAngle(const float angle) { m_newInput.angle = angle; emit inputChanged(); }
    void setSpeed(const float speed) { m_newInput.speed = qMin(speed, 1.f); emit inputChanged(); }
    void setControlsPressed(const bool held) { if (held == isControlsPressed()) return;  m_newInput.held = held ? 1.f : 0.f; emit inputChanged(); m_lastRotationTimer.invalidate(); }

    void setDriverAssistEnabled(const bool enabled) { m_driverAssistMode.enabled = enabled ? 1 : 0; emit driverAssistChanged(); }
    bool isDriverAssistEnabled() const { return m_driverAssistMode.enabled != 0; }

    // System stuff
    int memory() const { return m_memory; }
    int voltage() const { return m_voltage; }
    bool isCharging() const { return m_charging; }
    bool isBatteryLow() const { return m_batteryLow; }
    bool isFullyCharged() const { return m_fullyCharged; }

    bool isAutoRunning() const { return m_isAutoActive; }
    void setAutoPlay(const bool enabled);

    float xRotation() const { return m_rotation.x; }
    float yRotation() const { return m_rotation.y; }
    float zRotation() const { return m_rotation.z; }
    bool isFlipped() const { return m_isFlipped; }

    bool sensorDirty() const { return m_sensorDirty; }
    bool isStuck() const { return m_isStuck; }

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
    void stuckChanged();
    void soundVolumeChanged();
    void autoPlayChanged();
    void inputChanged();
    void driverAssistChanged();
    void initComplete();
    void tailFailed();

public slots:
    void chirp();
    void pause();
    void resetHeading();
    void stop();
    void resetTail();
    void rotate(const LeftOrRight direction);
    void flickTail();

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerError(QLowEnergyController::Error newError);

    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

    void sendInput();
    void sendDriverAssistConfig();

private:
    bool sendCommand(const CommandType command, float arg1, const float arg2, const float arg3);
    bool sendCommand(const CommandType command, const uint32_t arg1, const uint32_t arg2 = 0);
    bool sendCommand(const CommandType command);
    void sendAutoplay();

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
    static_assert(sizeof(Version) == 19);

    struct BatteryVoltageResponse {
        uint8_t voltage;
        bool isBatteryLow;
        bool isCharging;
        bool isFullyCharged;
        bool isAutoMode;
        uint16_t memory;

        uint8_t padding[12];
    };
    static_assert(sizeof(BatteryVoltageResponse) == 19);

    struct DeviceOrientationResponse {
        static_assert(sizeof(float) == 4);
        Vector3D<float> rotation;

        bool isFlipped;

        uint8_t padding[6];
    };
    static_assert(sizeof(DeviceOrientationResponse) == 19);

    struct CrashLogStringResponse {
        QString message() const {
            if (memchr(m_string, '\0', sizeof(m_string))) {
                return QString::fromUtf8(m_string);
            } else {
                return QString::fromUtf8(m_string, sizeof(m_string));
            }
        }
    private:
        char m_string [19];
    };
    static_assert(sizeof(CrashLogStringResponse) == 19);

    struct AnalyticsBeginResponse {
        uint8_t numberOfEntries;
        char padding[18];
    };
    static_assert(sizeof(AnalyticsBeginResponse) == 19);

    struct AutoPlayConfigResponse {
        AutoplayConfig config;
        char unknown[3];
        uint8_t responseTo; // ?
        char padding[5];
    };
    static_assert(sizeof(AutoPlayConfigResponse) == 19);

    struct IsSensorDirtyResponse {
        bool isDirty;

        char padding[18];
    };
    static_assert(sizeof(IsSensorDirtyResponse) == 19);

    struct RcStuckResponse {
        uint8_t stuckType;

        char padding[18];
    };
    static_assert(sizeof(IsSensorDirtyResponse) == 19);

    struct CommandResult {
        CommandType commandType;
        int8_t resultCode;
        uint32_t currentApiVersion;
        uint32_t minimumApiVersion;
        uint32_t maximumApiVersion;

        uint8_t padding[4];
    };
    static_assert(sizeof(CommandResult) == 19);

    struct TailStateResponse {
        bool failState;
        uint8_t padding[18];
    };
    static_assert(sizeof(TailStateResponse) == 19);

    struct ResponsePacket {
        ResponseType type;
        union {
            BatteryVoltageResponse battery;
            DeviceOrientationResponse orientation;
            CrashLogStringResponse crashString;
            AnalyticsBeginResponse analyticsBegin;
            AutoPlayConfigResponse autoPlay{};
            IsSensorDirtyResponse sensorDirty;
            Version firmwareVersion;
            CommandResult commandResult;
            RcStuckResponse stuck;
            TailStateResponse tail;
        };
    };
    static_assert(sizeof(ResponsePacket) == 20);

    struct InputState {
        float speed = 0.f;
        float held = 0.f;
        float angle = 0.f;

        void reset() {
            speed = 0.f;
            angle = 0.f;
            held = 0.f;
        }
    };
    struct DriverAssistMode {
        uint8_t enabled = 0;

        // TODO: move out of autoplayconfig
        AutoplayConfig::Surface surface = AutoplayConfig::Carpet;
    };

    struct FlickTailRequest {
        AutoplayConfig::TailType tail;
    };

    struct CommandPacket {
        CommandPacket(const CommandType command) : vector3D({}), // we have to initialize ourselves, so idk initialize the 3d vector
            m_command(command) {}

        const uint8_t magic = 48;
        union {
            Vector3D<float> vector3D;
            Vector3D<uint32_t> vector2D;
            InputState input;
            AutoplayConfig autoPlayConfig;
            DriverAssistMode driverAssistMode;
            //FlickTailRequest flick;
            AutoplayConfig::TailType flick;
        };
        const CommandType m_command = CommandType::Invalid;
    };
    static_assert(sizeof(CommandPacket) == 15);

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

    InputState m_currentInput;
    InputState m_newInput;

    Vector3D<int> m_rotation{};
    bool m_isFlipped = false;

    bool m_sensorDirty = false;
    bool m_isStuck = false;

    QString m_name;

    AutoplayConfig m_currentAutoConfig;
    AutoplayConfig m_newAutoConfig;
    bool m_isAutoActive = false;
    Version m_version;
    QTimer m_sendInputTimer; // so we can batch up input updates
    DriverAssistMode m_driverAssistMode;
    QElapsedTimer m_lastRotationTimer;
    bool m_waitingForOrientationChange = true;
};

QDebug operator<<(QDebug debug, const AutoplayConfig &c);

} // namespace mousr
