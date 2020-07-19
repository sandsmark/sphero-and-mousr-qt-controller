#pragma once

#include "BasicTypes.h"

#include <QObject>

namespace sphero {

#pragma pack(push,1)

struct ResponsePacketHeader {
    Q_GADGET
public:
    enum PacketType : uint8_t {
        Ack = 0x00,
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
        Timeout = 0x35,
        ErrorTimeout = 0xFE, // SDK introduced??
        TimeoutErr = 0xFF
    };
    Q_ENUM(PacketType)

    enum NotificationType : uint8_t {
        PowerNotification = 0x01,
        Level1Diagnostic = 0x02,
        SensorStream = 0x03,
        ConfigBlock = 0x04,
        SleepingIn10Sec = 0x05,
        MacroMarkers = 0x06,
        Collision = 0x07,
        OrbBasicErrorASCII = 0x09,
        OrbBasicErrorBinary = 0x0A,
        SelfLevelComplete = 0x0B,
        GyroRangeExceeded = 0x0C,
        SoulDataResponse = 0x0D,
        SoulLevelUpNotification = 0x0E,
        SoulShieldNotification = 0x0F,
        BoostNotification = 0x11,
        SoulBlockData = 0x20,
        XPUpdateEvent = 0x21
    };
    Q_ENUM(NotificationType)

    enum Type {
        Response = 0xFF,
        Notification = 0xFE
    };

    const uint8_t magic = 0xFF;
    uint8_t type = 0xFF;

    uint8_t packetType = 0;
    uint8_t sequenceNumber = 0;
    uint8_t dataLength = 0;
};

struct NotificationPacket
{
    const uint8_t magic = 0xFF;
    const uint8_t notificationIndicator = 0xFE;

    uint8_t type;

    uint16_t dataLength;

};

struct AckResponsePacket
{
    Q_GADGET
public:
    // I don't think all these are valid here?
    enum ResponseType : uint8_t {
        PowerNotification = 0x01,
        Level1Diagnostic = 0x02,
        SensorStream = 0x03,
        ConfigBlock = 0x04,
        SleepingIn10Sec = 0x05,
        MacroMarkers = 0x06,
        Collision = 0x07,
        OrbBasicErrorASCII = 0x09,
        OrbBasicErrorBinary = 0x0A,
        SelfLevelComplete = 0x0B,
        GyroRangeExceeded = 0x0C,
        SoulDataResponse = 0x0D,
        SoulLevelUpNotification = 0x0E,
        SoulShieldNotification = 0x0F,
        BoostNotification = 0x11,
        SoulBlockData = 0x20,
        XPUpdateEvent = 0x21
    };
    Q_ENUM(ResponseType)

    uint8_t type;
};

struct PowerStatePacket : public AckResponsePacket {
    enum PowerState : uint8_t {
        BatteryCharging = 0x1,
        BatteryOK = 0x2,
        BatteryLow = 0x3,
        BatteryCritical = 0x4,
    };

    uint8_t recordVersion = 0;
    uint8_t powerState = 0;
    uint16_t batteryVoltage = 0;
    uint16_t numberOfCharges = 0;
    uint16_t secondsSinceCharge = 0;
};
static_assert(sizeof(PowerStatePacket) == 9);

// TODO: size/contents
/// BB-8 and later
struct ChargerStatusPacket : public AckResponsePacket
{
    enum ChargerState : uint32_t {
        UnknownState = 0x00,
        NotCharging = 0x01,
        Charging = 0x02,
    };

    uint32_t state;
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
static_assert(sizeof(SensorStreamPacket) == 87); // should be 90, I think?

#pragma pack(pop)

} // namespace sphero
