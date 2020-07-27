#pragma once

#include "BasicTypes.h"

#include "utils.h"

#include <QDebug>
#include <QObject>
#include <QtEndian>
#include <cstdint>

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
        Unknown = 0x1f,
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

    void setSequenceNumber(const uint8_t number) {
//        m_sequenceNumber = number;
    }

protected:
    Packet(const uint8_t deviceID, const uint8_t commandID) :
        m_deviceID(deviceID),
        m_commandID(commandID)
    {}
};

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

//    const uint8_t unknown = 0x17;

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

struct SetMainLEDColor : public Packet {
    static constexpr uint8_t id = 14;
    SetMainLEDColor(const uint8_t red, const uint8_t green, const uint8_t blue) :
        Packet(Packet::AVControl, id),
        m_red(red),
        m_green(green),
        m_blue(blue)
    {}
    const uint8_t mask = 0x70;

    enum LED : uint8_t {
        FrontRed = 1 << 0,
        FrontGreen = 1 << 1,
        FrontBlue = 1 << 2,

        FrontLED = FrontRed | FrontGreen | FrontBlue,

        BackRed = 1 << 3,
        BackGreen = 1 << 4,
        BackBlue = 1 << 5,

        BackLED = BackRed | BackGreen | BackBlue,
    };

    uint8_t m_red = 0;
    uint8_t m_green = 0;
    uint8_t m_blue = 0;
};

struct SetLEDIntensity : public Packet {
    static constexpr uint8_t id = 0xe;

    enum LED : uint8_t {
        BackLED = 0x1,
        MainRedLED = 0x2,
        MainGreenLED = 0x4,
        MainBlueLED = 0x8,

    };

    SetLEDIntensity(const uint8_t led, uint8_t intensity) : Packet(Packet::AVControl, id),
//        m_led(led),
        m_intensity(intensity)
    {}
//    const uint8_t unknown1 = 0x1c;
    const uint8_t unknown2 = 0x0;
    const uint8_t unknown3 = 0x80;
    uint8_t m_intensity = 0;

//    const uint8_t unknown = 0;
//    uint16_t m_led = 0;
//    uint8_t m_intensity = 0;
};

#pragma pack(pop)

} // namespace v2
} // namespace sphero
