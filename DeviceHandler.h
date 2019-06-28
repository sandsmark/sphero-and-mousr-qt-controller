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
    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    static constexpr QUuid serviceUuid = {0x6e400001, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};
    static constexpr QUuid writeUuid   = {0x6e400002, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};
    static constexpr QUuid readUuid    = {0x6e400003, 0xb5a3, 0xf393, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e};

public:
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

    enum class ResultType : uint16_t {
        FirmwareVersionReport = 28,
        DeviceOrientationReport = 48,
        CrashlogParseFinished = 95,
        CrashlogAddDebugString = 96,
        CrashlogAddDebugMem = 97,
        BatteryInfo = 98,
        DeviceStopped = 99,
        DeviceStuck = 100,
        AutoAckReport = 49,
        AutoAckSuccess = 48,
        AutoAckFailed = 255,
        InitEndReport = 30,
        ResetTailReport = 50,
        DebugNumber = 80,
        DebugCharacter = 81,
        DebugCharacterAlt = 82,
        DebugChecksum = 83,
        TofStuck = 83,
        SensorDirty = 64,
    };

    enum class Response : uint16_t {
        AutoAck = 15,
        Pose = 48,
        ResetTailFailInfo = 50,
        StuckTofInfo = 64,
        RecordsSummary = 80,
        RecordsStart,
        RecordsContinue,
        RecordsFinished,
        CrashLogFinished = 95,
        CrashLogString,
        DebugInfo,
        BatteryVoltage,
        RobotStopped,
        RcStuck,
        FirmwareVersion = 28,
        HardwareVersion,
        InitDone,
        Nack = 255
    };
    const uint32_t mbApiVersion = 3u;

    bool sendCommand(const Command command, float arg1, float arg2, float arg3);
    bool sendCommand(const Command command, uint32_t arg1, uint32_t arg2);
//    bool sendCommand(const Command command, const char arg1, const char arg2, const char arg3, const char arg4);
    bool sendCommand(const DeviceHandler::Command command, std::vector<char> data);
//    bool sendCommand(const Command command, QByteArray data);

//    struct CommandPacket {
//        CommandPacket(Command command, )
//    };

    explicit DeviceHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

signals:
    void connectedChanged();
    void dataRead(const QByteArray &data);
    void disconnected(); // TODO

public slots:

private slots:
    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onDataRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &data);
    void onDescriptorRead(const QLowEnergyDescriptor &characteristic, const QByteArray &data);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_readCharacteristic;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QLowEnergyDescriptor m_readDescriptor;

    QPointer<QLowEnergyService> m_service;
};

#endif // DEVICEHANDLER_H
