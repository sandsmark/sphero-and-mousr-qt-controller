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

class MousrHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(int memory READ memory NOTIFY powerChanged)

    Q_PROPERTY(int voltage READ voltage NOTIFY powerChanged)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY powerChanged)
    Q_PROPERTY(bool isBatteryLow READ isBatteryLow NOTIFY powerChanged)
    Q_PROPERTY(bool isFullyCharged READ isFullyCharged NOTIFY powerChanged)

    Q_PROPERTY(bool isAutoRunning READ isAutoRunning NOTIFY autoRunningChanged)

    Q_PROPERTY(float xRotation READ xRotation NOTIFY orientationChanged)
    Q_PROPERTY(float yRotation READ yRotation NOTIFY orientationChanged)
    Q_PROPERTY(float zRotation READ zRotation NOTIFY orientationChanged)
    Q_PROPERTY(bool isFlipped READ isFlipped NOTIFY orientationChanged)

    Q_PROPERTY(bool sensorDirty READ sensorDirty NOTIFY sensorDirtyChanged)


public:

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

    enum class Command : uint16_t {
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
    Q_ENUM(Command)

    enum class ResultType : uint16_t {
        //FirmwareVersionReport = 28,
        //InitEndReport = 30,

        //DeviceOrientationReport = 48,
        AutoAckSuccess = 48,
        AutoAckReport = 49,

        //ResetTailReport = 50,
        //SensorDirty = 64,

        DebugNumber = 80,
        DebugCharacter = 81,
        DebugCharacterAlt = 82,
        DebugChecksum = 83,

        TofStuck = 83,

        //CrashlogParseFinished = 95,
        //CrashlogAddDebugString = 96,
        //CrashlogAddDebugMem = 97,
        //BatteryInfo = 98,
        //DeviceStopped = 99,
        //DeviceStuck = 100,

        //AutoAckFailed = 255
    };
    enum Response : uint8_t {
        AutoModeChanged = 15,

        FirmwareVersion = 28,
        HardwareVersion = 29,
        InitDone = 30,

        DeviceOrientation = 48,
        ResetTailFailInfo = 50,

        SensorDirty = 64, // sensor dirty

        AnalyticsBegin = 80,
        AnalyticsEntry = 81,
        AnalyticsData = 82,
        AnalyticsEnd = 83,

        CrashLogFinished = 95,
        CrashLogString = 96,
        DebugInfo = 97, //CrashlogAddDebugMem = 97,

        BatteryVoltage = 98,
        RobotStopped = 99,
        RcStuck = 100,

        Nack = 255
    };
    Q_ENUM(Response)

    enum FirmwareType : uint8_t {
        DebugFirmware = 0,
        BetaFirmware = 1,
        StableFirmware = 2
    };
    Q_ENUM(FirmwareType)

    const uint32_t mbApiVersion = 3u;

    bool sendCommand(const Command command, float arg1, float arg2, float arg3);
    bool sendCommand(const Command command, uint32_t arg1, uint32_t arg2);
//    bool sendCommand(const Command command, const char arg1, const char arg2, const char arg3, const char arg4);
    bool sendCommand(const MousrHandler::Command command, std::vector<char> data);
//    bool sendCommand(const Command command, QByteArray data);

//    struct CommandPacket {
//        CommandPacket(Command command, )
//    };

    explicit MousrHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);
    ~MousrHandler();

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

signals:
    void connectedChanged();
    void disconnected(); // TODO
    void powerChanged();
    void autoRunningChanged();
    void orientationChanged();
    void sensorDirtyChanged();

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

    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data);
    void onDescriptorRead(const QLowEnergyDescriptor &characteristic, const QByteArray &data);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:

    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_readCharacteristic;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QLowEnergyDescriptor m_readDescriptor;

    QPointer<QLowEnergyService> m_service;

    int m_voltage = 0, m_memory = 0;
    bool m_batteryLow = false, m_charging = false, m_fullyCharged = false;
    bool m_autoRunning = false;

    float m_speed = 0.f, m_held = 0.f, m_angle = 0.f;

    float m_rotX = 0.f, m_rotY = 0.f, m_rotZ = 0.f;
    bool m_isFlipped = false;

    bool m_sensorDirty = false;

    QString m_name;

    Autoplay::Config m_autoplay;

    struct Version {
        FirmwareType firmwareType = StableFirmware;
        uint8_t major = 0;
        uint16_t minor = 0;
        uint16_t commitNumber = 0;
        char commitHash[4] {"   "};

        uint8_t mousrVersion = 0;
        uint32_t hardwareVersion = 0;
        uint32_t bootloaderVersion;
    } __attribute__((packed));
    Version m_version;
};

QDebug operator<<(QDebug debug, const Autoplay::Config &c);

} // namespace mousr
