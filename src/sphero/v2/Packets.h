#pragma once

#include "BasicTypes.h"

#include "utils.h"

#include <QDebug>
#include <QObject>
#include <QtEndian>
#include <cstdint>

// TODO: the audio upload stuff, don't know the command IDs

namespace sphero {
namespace v2 {

static constexpr char Escape = 0xAB;
static constexpr char EscapedEscape = 0x23;
static constexpr char StartOfPacket = 0x8D;
static constexpr char EscapedStartOfPacket = 0x03;
static constexpr char EndOfPacket = 0xD8;
static constexpr char EscapedEndOfPacket = 0x50;

template <typename PACKET>
QByteArray encode(const PACKET &packet)
{
    QByteArray raw = packetToByteArray(packet);
    uint8_t checksum = 0;
    for (const char c : raw) {
        checksum += c;
    }
    raw.append(checksum xor 0xFF);

    QByteArray encoded(1, StartOfPacket);
    for (const char c : raw) {
        switch(c) {
        case Escape:
            encoded.append(Escape);
            encoded.append(EscapedEscape);
            break;
        case StartOfPacket:
            encoded.append(Escape);
            encoded.append(EscapedStartOfPacket);
            break;
        case EndOfPacket:
            encoded.append(Escape);
            encoded.append(EscapedEndOfPacket);
            break;
        default:
            encoded.append(c);
            break;
        }
    }



    encoded.append(EndOfPacket);

    qDebug() << " + Packet:";
    qDebug() << "  ] Flags:" << packet.m_flags;
    qDebug() << "  ] Device ID:" << packet.m_deviceID;
    qDebug() << "  ] Command ID:" << packet.m_commandID;
//    qDebug() << "  ] Sequence number:" << packet.m_sequenceNumber;
//    qDebug() << "  ] Source:" << packet.sourceID;
//    qDebug() << "  ] Target:" << packet.targetID;
//    qDebug() << "  ] Error code:" << packet.errorCode;

    return encoded;
}
template <typename PACKET>
PACKET decode(const QByteArray &input, bool *ok)
{
    if (!input.startsWith(StartOfPacket) || !input.endsWith(EndOfPacket)) {
        qWarning() << "invalid start or end";
        *ok = false;
        return {};
    }

    QByteArray decoded;
    for (int i=1; i<input.size()-1; i++){
        const char c = input[i];
        if (c != Escape) {
            decoded.append(c);
            continue;
        }
        i++;
        switch(input[i]) {
        case EscapedEscape:
            decoded.append(Escape);
            break;
        case EscapedStartOfPacket:
            decoded.append(StartOfPacket);
            break;
        case EscapedEndOfPacket:
            decoded.append(EndOfPacket);
            break;
        default:
            *ok = false;
            qWarning() << "Invalid escape sequence" << c << input[i];
            qDebug() << input.toHex(':');
            return {};
        }
    }
    qDebug() << "decoded" << decoded.toHex(':');

    uint8_t checksum = 0;
    for (int i=0; i<decoded.size() - 1; i++) {
        checksum += decoded[i];
    }

    if (!decoded.endsWith(checksum xor 0xff)) {
        qWarning() <<  "invalid checksum" << decoded.back() << "expected" << checksum;
        *ok = false;
        return {};
    }

    decoded.chop(1); // remove the checksum at the end

    return byteArrayToPacket<PACKET>(decoded, ok);
}

#pragma pack(push,1)

struct Packet {
    // TODO: share with v1 (mostly compatible, ish)
    enum Flags {
        HasErrorCode = 1 << 0,
        Synchronous = 1 << 1,
        ReportError = 1 << 2,
        ResetTimeout = 1 << 3,
        HasTargetAddress = 1 << 4,
        HasSourceAddress = 1 << 5,
        Reserved = 1 << 6,
        TwoByteFlags = 1 << 7,
    };

    uint8_t m_flags = Synchronous | ResetTimeout;

    // These are only used on the big robot I don't remember the name of
    // The Mini etc. don't have several systems so they don't need this
    // They are unset/unused unless the appropriate flags are set
//    uint8_t sourceID = 0;
//    uint8_t targetID = 0;

    uint8_t m_deviceID;
    uint8_t m_commandID;

    uint8_t m_sequenceNumber = 0;

    // Only if flag is set
//    uint8_t errorCode = 0;

    Q_GADGET
public:
    Packet() = default;

    enum CommandTarget : uint8_t {
        Internal = 0x00,

        PingPong = 0x10,
        Info = 0x11,
        DrivingSystem = 0x12,
        MainSystem = 0x13,
        CarControl = 0x16,
        AnimationControl = 0x17,
        Sensors = 0x18,
        AVControl = 0x1a,
        AVFlashControl = 0x1b,
        QASystem = 0x1f,
        InvalidTarget = 0xFF
    };
    Q_ENUM(CommandTarget)

    enum class Error {
        Success = 0x00,
        BadDeviceID = 0x01,
        BadCommandID = 0x02,
        NotYetImplemented = 0x03,
        CommandisRestricted = 0x04,
        BadDataLength = 0x05,
        CommandFailed = 0x06,
        BadParameterValue = 0x07,
        Busy = 0x08,
        BadTargetID = 0x09,
        TargetUnavailable = 0x0A,
    };
    Q_ENUM(Error)

    enum SoulCommands : uint8_t {
        ReadSoulBlock = 0xf0,
        SoulAddXP = 0xf1
    };
    Q_ENUM(SoulCommands)

    bool isValid() const {
        return m_flags != 0;
    }

    bool isSynchronous() const {
        return m_flags & Synchronous;
    }

    //void setSequenceNumber(const uint8_t number) {
//        m_sequenceNumber = number;
    //}

protected:
    Packet(const uint8_t deviceID, const uint8_t commandID) :
        m_deviceID(deviceID),
        m_commandID(commandID)
    {}
};

namespace UserIO {
enum Command {
    EnableGestureEventNotification = 0,
    GestureEvent = 1,
    ButtonEventMaybe = 2,
    ButtonEvent = 3,
    SetLED = 4,
    ReleaseLED = 5,
    PlayHapticPattern = 6,
    PlayAudioFile = 7,
    SetVolume = 8,
    GetVolume = 9,
    StopAudio = 0xa,
    EnableCapacitiveTouch = 0xb,
    EnableALS = 0xc,
    SetLEDs = 0xe,
    Backlight = 0xf,
    CapacitiveTouchASync = 0x10,
    EnableDebug = 0x11,
    HeadLights = 0x13,
    TailLights = 0x14,
    PlayTestTone = 0x18,
    StartIdleLED = 0x19,
    ToyCommands = 0x20,

    V2Event = 0x21, // ??

    SetUserProfile = 0x22,
    GetUserProfile = 0x23,
};
} // namespace UserIO

namespace Animatronics {
enum Command {
    AnimateSuspension = 3,
    AnimateMouth = 4,
    ExecuteAnimationProfile = 5,
    UploadAnimationBundle = 6,
    UploadAnimationBundleAsyncResult = 9,
    EnableUserDrive = 0xb,
    DoShoulder = 0xd,
    SetHeadPosition = 0xf,
    ExecuteAnimationBundleCompleteAsync = 0x11,
    GetHeadPosition = 0x14,
    SetShoulderCamPosition = 0x15,
    GetShoulderPosition = 0x16,
    GetShoulderAction = 0x25,
    GetShoulderActionAsync = 0x26,
    SetHeadBehavior = 0x29,
    EnableShoulderAsync = 0x2a,
    StopAnimation = 0x2b,
    GetTrophyMode = 0x2e,
    GetDroidToDroidMode = 0x30,


    HeadResetToZeroAsyncEnable = 0x39,
    HeadResetToZeroAsync = 0x3a,
};
enum ShoulderCamPosition {
    ThreeLegs = 1,
    TwoLegs = 2,
    Waddle = 3, // I think? Not 100%, damn std::string and it's short string optimization
    Transitioning = 4,
};
} // namespace Animatronics

namespace APIProcessor {
enum Command {
    Data,
    Echo
};
} // namespace APIProcessor

namespace DrivingSystem {
enum Response {
    Ok = 1 // IDK
};
} // namespace DrivingSystem

namespace PeerConnection {
enum Command {
    EnablePeerConnectionEventNotification = 0,
    PeerConnectionEvent = 1,
    GetPeerConnectionState = 2,
    SetBluetoothName = 3,
    GetBluetoothName = 4
};
} // namespace PeerConnection

namespace Power {
enum Command {
    EnterDeepSleep = 0,
    EnterSoftSleep = 1,
    GetUSBState = 2,
    GetBatteryVoltage = 3,
    GetBatteryState = 4,
    EnableBatteryStateNotify = 5,
    BatteryStateChanged = 6,
    Wake = 0xd,
    GetBatteryPercent = 0x10,
    SetPowerOptions = 0x12,
    GetPowerOptions = 0x13,
    GetBatteryVoltageState = 0x17,
    WillSleep = 0x19,
    Sleep = 0x1a,
    EnableBatteryVoltageStateChangedAsync = 0x1b,
    GetBatteryVoltageAlt = 0x1c, // async maybe? complementary bits of the other getbattery voltage at least
};
} // namespace Power

namespace Proto {
enum Command {
    SetMACTable = 0,
    GetMACTable = 1,
    EnableTableRSSI = 2,
    RSSIReply = 3,
};
} // namespace Proto

namespace QASystem {
enum Command {
    RunQATest = 0x1F // has one int argument, id
};
} // namespace QASystem

namespace AVFlashControl {
enum Command {
    WriteData = 2,
    ReadData = 4,
    CrcRegion = 6,
    EraseSectors = 0xa,
    SpotcheckCrcRegion = 0xf
};
} // namespace QASystem

namespace Sensors {
enum Command {
    SetSensorAppMask = 0,
    Sensor = 1,
    GetSensorAppMask = 2,
    SetSensorAppMaskExtended = 0xc,
    GetSensorAppMaskExtended = 0xd,
    SetGyroMaxNotification = 0xf,
    GyroMaxNotificationReply = 0x10,
    ConfigCollisionDetection = 0x11,
    Collision = 0x12,
    ResetLocator = 0x13,
    EnableCollisionDetection = 0x14,
    SubsscribeManeuverNotification = 0x15,
    ManeuverDetectionAsync = 0x16,
    SetLocatorFlags = 0x17,
    SetAccelerometerActivityThreshold = 0x18,
    EnableAccelerometerActivityAsync = 0x19,
    AccelerometerActivityAsync = 0x1a,
    SetGyroActivityThreshold = 0x1b,
    EnableGyroActivityAsync = 0x1c,
    GyroActivityAsync = 0x1d
};
} // namespace Sensors

namespace SystemInfo {
enum Command {
    GetVersion = 0,
    GetVersionAlt = 1,
    GetFirmwareBuildInfoMainApp = 2,
    GetFirmwareBoardRev = 3,
    FirmwareConfigWrite = 4,
    ConfigWriteComplete = 5,
    GetMACAddress = 6,
    GetUserConfigBlock = 7,
    SetUserConfigBlock = 8,
    SetUserConfigBlockComplete = 9,
    DisableFactoryMode = 10,
    GetLogStatus = 0xb,
    GetLogChunk = 0xc,
    StClear = 0xd, // statistics?
    GetTemperature = 0xe,
    GetModelNumber = 0x12,
    GetStatisticssID = 0x13,
    GetSecondaryMCUVersion = 0x17,
    GetSecondaryMCUVersionAsync = 0x18,
    GetAnimationVersion = 0x1a,
    GetAnimationVersionAsync = 0x1a,
    SetAudioCRC = 0x22,
    GetAudioCRC = 0x23,
    Level1Diagnostics = 0x26,
    Level1DiagnosticsReply = 0x27,
    GetSKU = 0x28,
    GetSecondaryMCUStatus = 0x29,
    GetSecondaryMCUStatusAsync = 0x2a
};
} // namespace SystemInfo

namespace SystemMode {
enum Command {
    SetPlayMode = 0,
    GetSystemMode = 1,
    EnablePlayModeUpdateNotification = 2,
    SystemModeChanged = 3,
    SetMaxSpeed = 4,
    GetMaxSpeed = 5,
    SetCurrentWeapon = 6,
    GetCurrentWeapon = 7,
    SetAimMode = 8,
    GetPlayModeMask = 9,
    SetPlayUnlocks = 10,
    EnablePlayModeUpdateNotificationAlt = 0xb,
    MenuItemChanged = 0xc,
    GetWeaponMask = 0xd,
    SetEnabledWeapons = 0xe,
    ForcebandGetHolocronCount = 0x10,
    DecrementHolocronCount = 0x13,
    ClearHolocronCount = 0x14,
    FindHolochronNow = 0x15,
    EnableForceAware = 0x16,
    ForcebandForceAwarenessEvent = 0x17,
    SetHolocronCategoryRarity = 0x18, // Guessing
    ForcebangGetAngleToHolocron = 0x19,
    DeviceForceAwareTimeout = 0x1a,
    AUseTHeaderComplete = 0x1b, // ???
    SetAudioHederEvent = 0x1c,
    EnableRobotBadApp = 0x20,
    RobotBadApp = 0x21,
    GetPendingActions = 0x22,
    AudioInfo = 0x23

};
} // namespace SystemMode

namespace WiFi {
enum Command {
    PowerOn = 0,
    PowerOnComplete = 1,
    GetMAC = 2
};
} // namespace WiFi {
struct ResponsePacket: public Packet {
    uint8_t errorCode = 0;

    ResponsePacket() = default;

    ResponsePacket(const Packet::CommandTarget target, const uint8_t commandID) :
        Packet(target, commandID) {}
};

struct RequestBatteryVoltagePacket : public Packet {
    static constexpr uint8_t id = 0x3;

    RequestBatteryVoltagePacket() : Packet(Packet::MainSystem, id) {}
};

struct GoToLightSleep : public Packet {
    static constexpr uint8_t id = 0x1;

    GoToLightSleep() : Packet(Packet::MainSystem, id) {}
};

struct WakePacket : public Packet {
    static constexpr uint8_t id = 0xd;

    WakePacket() : Packet(Packet::MainSystem, id) {}
};

struct PingPacket : public Packet {
    static constexpr uint8_t id = 0;
    PingPacket() : Packet(Packet::PingPong, id) {}
};

struct GetTemperaturePacket : public Packet {
    static constexpr uint8_t id = 0xe;
    GetTemperaturePacket() : Packet(Packet::Info, id) {}
};

struct DrivePacket : public Packet {
    static constexpr uint8_t id = 0x07;

    DrivePacket(uint8_t speed, uint16_t heading, uint8_t flags = 0) : Packet(Packet::DrivingSystem, id),
        m_speed(speed),
        m_heading(heading),
        m_driveFlags(flags)
    {}

    enum Flag : uint8_t {
        Reverse = 1 << 0,
        Boost = 1 << 1,
        FastTurn = 1 << 2,
        ReverseLeftMotor = 1 << 3,
        ReverseRightMotor = 1 << 4,
    };

    uint8_t m_speed = 0;
    uint16_t m_heading = 0;
    uint8_t m_driveFlags = 0;
};

struct RCDrivePacket : public Packet {
    static constexpr uint8_t id = 0x02;

    RCDrivePacket(const float heading, const float speed) : Packet(Packet::CarControl, id),
        m_heading(heading),
        m_speed(speed)
    {}

    float m_heading = 0.f;
    float m_speed = 0.f;
};

struct SetStancePacket : public Packet {
    static constexpr uint8_t id = 0x0d;

    enum Stance : uint8_t {
        Tripod,
        Bipod
    };

    SetStancePacket(const Stance stance) : Packet(Packet::AVControl, id),
        m_stance(stance)
    {}

    Stance m_stance;
};

struct PlayAnimationPacket : public Packet {
    static constexpr uint8_t id = 0x05;
    PlayAnimationPacket(const uint8_t animation) : Packet(Packet::AVControl, id),
        m_animation(animation)
    {}

    enum Animation : uint16_t {
        Yes = 0x41,
        No = 0x3f,

        Alarm = 0x17,
        Angry = 0x18,
        Annoyed = 0x19,
//        IonBlast = 0x1a,
//        Sad = 0x1c,
        Scared = 0x1d,
        Chatty = 0x17,
        Confident = 0x18,
        Excited = 0x19,

        Happy = 0x1a,
        Laugh = 0x1B,
        Surprise = 0x1C,
    };

    uint16_t m_animation;
};

enum ColorLED : uint16_t {
    InvalidLED = 0,

    BackLED = 0b111 << 12, // 0x1800

    R2BodyLED = 0b111 << 8,

    // Bit 8, 9 10, and 12
    // Not sure what the extra bit is for (12)
    B9BodyLED = 0b10111 << 8, // 0x1700

    BB9HeadLED = 1 << 12,
};

enum MonoLED : uint16_t {
    R2LogicDisplayLED = 1 << 11, // 0x800
    R2HoloProjectorLED = 1 << 15, // 0x8000

    B9BackLED = 0b11 << 11, // 0x1800, bit 11 and 12
};

struct SetLED : public Packet {
    static constexpr uint8_t id = 0xe;

    SetLED(const ColorLED led, const uint8_t r, const uint8_t g, const uint8_t b) :
        Packet(Packet::AVControl, id),
        m_led(led),
        m_red(r),
        m_green(g),
        m_blue(b)
    {}

    const uint16_t m_led;

    // I think it is actually sent as an uint32_t, but easier this way with the padding
    const uint8_t m_red;
    const uint8_t m_green;
    const uint8_t m_blue;

    const uint8_t padding = 0xFF;

protected:
    SetLED(const uint16_t led, const uint8_t r, const uint8_t g, const uint8_t b) :
        m_led(led),
        m_red(r),
        m_green(g),
        m_blue(b)
    {}
};

/// LEDs that only can set brightness
struct SetLEDIntensity : public SetLED {
    SetLEDIntensity(const MonoLED led, const uint8_t strength) :
        SetLED(uint16_t(led), strength, strength, strength)
    {}
};

#pragma pack(pop)

} // namespace v2
} // namespace sphero
